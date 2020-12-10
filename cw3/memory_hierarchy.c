/*************************************************************************************|
|   1. YOU ARE NOT ALLOWED TO SHARE/PUBLISH YOUR CODE (e.g., post on piazza or
online)| |   2. Fill memory_hierarchy.c | |   3. Do not use any other .c files
neither alter mipssim.h or parser.h              | |   4. Do not include any
other library files                                         |
|*************************************************************************************/

#include "mipssim.h"

#define BLOCK_BITS ((uint32_t)4)
#define BLOCK_SIZE ((uint32_t)1 << BLOCK_BITS)
#define OFFSET_MASK (~((-1) << BLOCK_BITS))
#define NUM_BLOCKS (cache_size >> BLOCK_BITS)
#define TAG_MASK ((-1) << (32 - len_tag))

uint32_t cache_type = 0;

static int len_tag = -1;
static int len_index = -1;
static void *cache = NULL;
static int t = 0;

static inline uint32_t bits_needed(int n) {
  for (int i = 0; i <= 32; i++) {
    if (1 << i >= n) {
      return i;
    }
  }
  assert(0);
}

static inline void print_sw_stats() {
  float r = (float)arch_state.mem_stats.sw_cache_hits /
            (float)arch_state.mem_stats.sw_total;
  printf("  SW hit rate: %.2f percent\n", r * 100.0);
}

static inline void print_lw_stats() {
  float r = (float)arch_state.mem_stats.lw_cache_hits /
            (float)arch_state.mem_stats.lw_total;
  printf("  LW hit rate: %.5f percent\n", r * 100.0);
}

//-------------------------------------------------------------------
// Direct Mapped
//-------------------------------------------------------------------

static inline uint32_t dmtag(uint32_t address) {
  return address >> (31 - len_tag);
}

static inline uint32_t dmindex(uint32_t address) {
  // printf("Tag mask: 0x%x, len tag: %u, len index: %u\n", TAG_MASK, len_tag,
  //      len_index);
  return (address << len_tag) >> (31 - len_index);
}

static inline uint32_t dmoffset(uint32_t address) {
  return address & OFFSET_MASK;
}

static void print_data(int *data) {
  for (int i = 0; i < (BLOCK_SIZE >> 2); i++) {
    printf("0x%08x ", data[i]);
  }
  printf("\n");
}

struct dmentry {
  char valid;
  int tag;
  int data[BLOCK_SIZE >> 2];
};

static inline struct dmentry *dmaccess(uint32_t index) {
  return (struct dmentry *)cache + index;
}

static void dminit() {
  printf("Initialising DM cache (size: %u, block size: %u, num blocks: %u)\n",
         cache_size, BLOCK_SIZE, NUM_BLOCKS);
  len_index = bits_needed(NUM_BLOCKS);
  len_tag = 32 - BLOCK_BITS - len_index;
  cache = malloc(NUM_BLOCKS * sizeof(struct dmentry));
  struct dmentry *e = cache;
  for (int i = 0; i < NUM_BLOCKS; i++) {
    (e++)->valid = 0; // whatever is in the entry now is definetly not valid
  }
}

static struct dmentry *dmalloc(uint32_t address) {
  uint32_t tag = dmtag(address);
  uint32_t index = dmindex(address);
  printf("MISS\n  Allocating new block (tag: 0x%x, index: 0x%x)\n", tag, index);
  struct dmentry *e = dmaccess(index);
  uint32_t block_addr = (address >> BLOCK_BITS) << (BLOCK_BITS - 2);
  printf("  Copying %u bytes from 0x%x to 0x%lx\n", BLOCK_SIZE, block_addr,
         (uint64_t)e->data);
  memcpy(e->data, arch_state.memory + block_addr, BLOCK_SIZE);
  e->tag = tag;
  e->valid = 1;
  return e;
}

static int dmread(int address) {
  uint32_t tag = dmtag(address);
  uint32_t index = dmindex(address);
  uint32_t offset = dmoffset(address);
  printf("Reading 0x%x (tag: 0x%x, index: 0x%x, offset: 0x%x)...", address, tag,
         index, offset);
  struct dmentry *e = dmaccess(index);
  if (e->valid && (e->tag == tag)) {
    printf("HIT\n");
    ++arch_state.mem_stats.lw_cache_hits;
  } else {
    e = dmalloc(address);
  }
  printf("  ");
  print_data(e->data);
  printf("  Returning 0x%x\n", (e->data)[offset >> 2]);
  print_lw_stats();
  return (e->data)[offset >> 2];
}

static void dmwrite(int address, int write_data) {
  uint32_t tag = dmtag(address);
  uint32_t index = dmindex(address);
  uint32_t offset = dmoffset(address);
  printf("Writing 0x%x (tag: 0x%x, index: 0x%x, offset: 0x%x)\n", address, tag,
         index, offset);
  struct dmentry *e = dmaccess(index);
  if (e->valid && (e->tag == tag)) {
    (e->data)[offset >> 2] = write_data;
    ++arch_state.mem_stats.sw_cache_hits;
  }
  arch_state.memory[address >> 2] = write_data;
  print_sw_stats();
}

//-------------------------------------------------------------------
// Fully Associative
//-------------------------------------------------------------------

static inline uint32_t fatag(uint32_t address) {
  return address >> (31 - len_tag);
}

static inline uint32_t faoffset(uint32_t address) {
  return address & OFFSET_MASK;
}

struct faentry {
  char valid;
  int dt; // time stamp relative to most recent allocation
  int tag;
  int data[BLOCK_SIZE >> 2];
};

static void fainit() {
  printf("Initialising FA cache\n");
  len_index = bits_needed(NUM_BLOCKS);
  len_tag = 32 - BLOCK_BITS;
  cache = malloc(NUM_BLOCKS * sizeof(struct faentry));
  struct faentry *e = cache;
  for (int i = 0; i < NUM_BLOCKS; i++) {
    e->valid = 0;
    e->dt = 0;
    ++e;
  }
}

static struct faentry *faalloc(uint32_t address) {
  uint32_t tag = fatag(address);
  printf("MISS\n  Allocating new block (tag: 0x%x)\n", tag);
  int min_i = 0;
  struct faentry *e, *min_e;
  e = min_e = cache;
  // finding lru block
  for (int i = 1; i < NUM_BLOCKS; ++i) {
    e++;
    // if we find an invalid block we don't need to evict any
    if (!e->valid) {
      min_i = i;
      min_e = e;
      printf("  Found invalid block: 0x%x\n", i);
      break;
    }
    if (min_e->dt > e->dt) {
      min_i = i;
      min_e = e;
    }
  }
  printf("  New location: 0x%x\n", min_i);
  // re-centering time stamps
  e = cache;
  for (int i = 0; i < NUM_BLOCKS; ++i) {
    if (e->valid) {
      e->dt -= t;
    }
    e++;
  }
  t = 0;
  // updating entry
  min_e->tag = tag;
  min_e->valid = 1;
  uint32_t block_addr = (address >> BLOCK_BITS) << (BLOCK_BITS - 2);
  printf("  Copyig %u bytes from 0x%x to 0x%lx\n", BLOCK_SIZE, block_addr,
         (uint64_t)min_e->data);
  memcpy(min_e->data, arch_state.memory + block_addr, BLOCK_SIZE);
  min_e->dt = t;
  return min_e;
}

static struct faentry *fafind(int tag) {
  struct faentry *e = cache;
  for (int i = 0; i < NUM_BLOCKS; i++) {
    if (e->valid && e->tag == tag) {
      return e;
    }
    e++;
  }
  return NULL;
}

static int faread(int address) {
  ++t;
  uint32_t tag = fatag(address);
  uint32_t offset = faoffset(address);
  printf("Reading 0x%x (tag: 0x%x, offset: 0x%x)... ", address, tag, offset);
  struct faentry *e = fafind(tag);
  if (e) {
    printf("HIT\n");
    ++arch_state.mem_stats.lw_cache_hits;
    e->dt = t;
  } else {
    e = faalloc(address);
  }
  printf("  Returning 0x%x\n", (e->data)[offset >> 2]);
  print_lw_stats();
  return (e->data)[offset >> 2];
}

static void fawrite(int address, int write_data) {
  ++t;
  uint32_t tag = fatag(address);
  uint32_t offset = faoffset(address);
  struct faentry *e = fafind(tag);
  if (e) {
    ++arch_state.mem_stats.sw_cache_hits;
    e->data[offset >> 2] = write_data;
    e->dt = t;
  }
  printf("Writing 0x%x to 0x%x\n", address, write_data);
  arch_state.memory[address >> 2] = write_data;
  print_sw_stats();
}

//-------------------------------------------------------------------
// Set associative
//-------------------------------------------------------------------

static uint32_t SET_BITS;
#define NUM_SETS (NUM_BLOCKS >> SET_BITS)
#define NUM_BLOCKS_SET (1 << SET_BITS)

static inline uint32_t satag(uint32_t address) {
  return address >> (31 - len_tag);
}

static inline uint32_t saindex(uint32_t address) {
  return (address << len_tag) >> (31 - len_index);
}

static inline uint32_t saoffset(uint32_t address) {
  return address & OFFSET_MASK;
}

struct saentry {
  char valid;
  int dt;
  int tag;
  int data[BLOCK_SIZE >> 2];
};

static void sainit() {
  len_index = bits_needed(NUM_SETS);
  len_tag = 32 - len_index - BLOCK_BITS;
  printf("Initialising %d-way SA cache (tag: %d, index: %d, sets: %d, blocks "
         "in set: %d)\n",
         1 << SET_BITS, len_tag, len_index, NUM_SETS, NUM_BLOCKS_SET);
  cache = malloc(NUM_BLOCKS * sizeof(struct saentry));
  struct saentry *e = cache;
  for (int i = 0; i < NUM_BLOCKS; i++) {
    e->valid = 0;
    e->dt = 0;
    ++e;
  }
}

static struct saentry *saalloc(uint32_t address) {
  uint32_t tag = satag(address);
  uint32_t index = saindex(address);
  printf("MISS\n  Allocationg new block (tag: 0x%x, set: 0x%x)\n", tag, index);
  // Finding line to evict/populate
  struct saentry *e, *min_e;
  e = cache;
  min_e = e += (index << SET_BITS);
  int min_i = 0;
  for (int i = 0; i < NUM_BLOCKS_SET; i++, e++) {
    if (!e->valid) {
      min_i = i;
      min_e = e;
      printf("  Found invalid block (set: 0x%x, index: 0x%x)\n", index, i);
      break;
    }
    if (min_e->dt > e->dt) {
      min_i = i;
      min_e = e;
    }
  }
  printf("  New location: set: 0x%x, index: 0x%x\n", index, min_i);
  // re-centering time stamps
  e = cache;
  // We're resetting all the time stamps, not just in the current set,
  // for simplicity. Otherwise we would have to manage separate t's for
  // every set.
  for (int i = 0; i < NUM_BLOCKS; i++) {
    if (e->valid) {
      e->dt -= t;
    }
    e++;
  }
  t = 0;
  // updating entry
  min_e->tag = tag;
  min_e->valid = 1;
  uint32_t block_addr = (address >> BLOCK_BITS) << (BLOCK_BITS - 2);
  printf("  Copying %u bytes from 0x%x to 0x%lx\n", BLOCK_SIZE, block_addr,
         (uint64_t)min_e->data);
  memcpy(min_e->data, arch_state.memory + block_addr, BLOCK_SIZE);
  min_e->dt = t;
  return min_e;
}

static struct saentry *safind(uint32_t address) {
  uint32_t index = saindex(address);
  uint32_t x = address;
  uint32_t tag = satag(address);
  struct saentry *e = cache;
  e += (index << SET_BITS);
  for (int i = 0; i < NUM_BLOCKS_SET; i++, e++) {
    if (e->valid && e->tag == tag) {
      printf("HIT\n  Found matching block (set: 0x%x, index: 0x%x)\n", index,
             i);
      return e;
    }
  }
  return NULL;
}

static int saread(int address) {
  ++t;
  uint32_t tag = satag(address);
  uint32_t index = saindex(address);
  uint32_t offset = saoffset(address);
  printf("Reading 0x%x (tag: 0x%x, set: 0x%x, offset: 0x%x)...", address, tag,
         index, offset);
  struct saentry *e = safind(address);
  if (e) {
    ++arch_state.mem_stats.lw_cache_hits;
    e->dt = t;
  } else {
    e = saalloc(address);
  }
  printf("  ");
  print_data(e->data);
  printf("  Returning 0x%x\n", (e->data)[offset >> 2]);
  print_lw_stats();
  return (e->data)[offset >> 2];
}

static void sawrite(int address, int write_data) {
  ++t;
  uint32_t offset = saoffset(address);
  struct saentry *e = safind(address);
  if (e) {
    ++arch_state.mem_stats.sw_cache_hits;
    e->data[offset >> 2] = write_data;
    e->dt = t;
  }
  printf("Writing 0x%x to 0x%x\n", address, write_data);
  arch_state.memory[address >> 2] = write_data;
  print_sw_stats();
}

//-------------------------------------------------------------------
// Public Functions
//-------------------------------------------------------------------

void memory_state_init(struct architectural_state *arch_state_ptr) {
  arch_state_ptr->memory = malloc(sizeof(uint32_t) * MEMORY_WORD_NUM);
  memset(arch_state_ptr->memory, 0, sizeof(uint32_t) * MEMORY_WORD_NUM);
  if (cache_size == 0) {
    // CACHE DISABLED
    memory_stats_init(arch_state_ptr,
                      0); // WARNING: we initialize for no cache 0
  } else {
    // CACHE ENABLED

    /// @students: memory_stats_init(arch_state_ptr, X); <-- fill # of tag bits
    /// for cache 'X' correctly

    switch (cache_type) {
    case CACHE_TYPE_DIRECT: // direct mapped
      dminit();
      break;
    case CACHE_TYPE_FULLY_ASSOC: // fully associative
      fainit();
      break;
    case CACHE_TYPE_2_WAY: // 2-way associative
      SET_BITS = 1;
      sainit();
      break;
    }
    memory_stats_init(arch_state_ptr, len_tag);
  }
}

// returns data on memory[address / 4]
int memory_read(int address) {
  arch_state.mem_stats.lw_total++;
  check_address_is_word_aligned(address);

  if (cache_size == 0) {
    // CACHE DISABLED
    uint32_t base_addr = (address >> BLOCK_BITS) << BLOCK_BITS;
    printf("Reading 0x%x (block: 0x%x, offset: 0x%x)\n  ", address,
           base_addr >> BLOCK_BITS, address & OFFSET_MASK);
    int v = arch_state.memory[address / 4];
    print_data(arch_state.memory + (base_addr >> 2));
    printf("  Returning 0x%08x\n", v);
    return v;
  } else {
    // CACHE ENABLED

    /// @students: your implementation must properly increment:
    /// arch_state_ptr->mem_stats.lw_cache_hits
    if (len_tag <= 0) {
      assert(0);
    }
    switch (cache_type) {
    case CACHE_TYPE_DIRECT: // direct mapped
      return dmread(address);
    case CACHE_TYPE_FULLY_ASSOC: // fully associative
      return faread(address);
    case CACHE_TYPE_2_WAY: // 2-way associative
      return saread(address);
    }
  }
  return 0;
}

// writes data on memory[address / 4]
void memory_write(int address, int write_data) {
  arch_state.mem_stats.sw_total++;
  check_address_is_word_aligned(address);

  if (cache_size == 0) {
    // CACHE DISABLED
    arch_state.memory[address / 4] = (uint32_t)write_data;
    printf("Writing 0x%x (block: 0x%x, offset: 0x%x)\n", address,
           address >> BLOCK_BITS, address & OFFSET_MASK);
  } else {
    // CACHE ENABLED

    /// @students: your implementation must properly increment:
    /// arch_state_ptr->mem_stats.sw_cache_hits

    switch (cache_type) {
    case CACHE_TYPE_DIRECT: // direct mapped
      dmwrite(address, write_data);
      break;
    case CACHE_TYPE_FULLY_ASSOC: // fully associative
      fawrite(address, write_data);
      break;
    case CACHE_TYPE_2_WAY: // 2-way associative
      sawrite(address, write_data);
      break;
    }
  }
}
