/*************************************************************************************|
|   1. YOU ARE NOT ALLOWED TO SHARE/PUBLISH YOUR CODE (e.g., post on piazza or
online)| |   2. Fill mipssim.c | |   3. Do not use any other .c files neither
alter mipssim.h or parser.h              | |   4. Do not include any other
library files                                         |
|*************************************************************************************/

#include "mipssim.h"

#define BREAK_POINT 500  // exit after so many cycles -- useful for debugging

// Custom instructions types
#define I_TYPE 2
#define J_TYPE 3

// Custom op/function codes
#define ADDU 33

// Custom FSM states
#define ADDI_EXEC 13
#define ADDI_WB   14

// Global variables
char mem_init_path[1000];
char reg_init_path[1000];

uint32_t cache_size = 0;
struct architectural_state arch_state;

static inline uint8_t get_instruction_type(int opcode) {
  switch (opcode) {
      /// opcodes are defined in mipssim.h

    case SPECIAL: return R_TYPE;
    case LW: return I_TYPE;
    case SW: return I_TYPE;
    case ADDI: return I_TYPE;
    case J: return J_TYPE;
    case BEQ: return I_TYPE;
    case SLT: return R_TYPE;
    case EOP:
      return EOP_TYPE;

      ///@students: fill in the rest

    default: assert(false);
  }
  assert(false);
}

void FSM() {
  struct ctrl_signals *control = &arch_state.control;
  struct instr_meta *IR_meta   = &arch_state.IR_meta;

  // reset control signals
  memset(control, 0, (sizeof(struct ctrl_signals)));

  int opcode = IR_meta->opcode;
  int state  = arch_state.state;
  switch (state) {
    case INSTR_FETCH:
      control->MemRead  = 1;
      control->ALUSrcA  = 0;
      control->IorD     = 0;
      control->IRWrite  = 1;
      control->ALUSrcB  = 1;
      control->ALUOp    = 0;
      control->PCWrite  = 1;
      control->PCSource = 0;
      state             = DECODE;
      break;
    case DECODE:
      control->ALUSrcA = 0;
      control->ALUSrcB = 3;
      control->ALUOp   = 0;
      if (IR_meta->type == R_TYPE) {
        state = EXEC;
      } else if (opcode == LW || opcode == SW) {
        state = MEM_ADDR_COMP;
      } else if (opcode == BEQ) {
        state = BRANCH_COMPL;
      } else if (opcode == J) {
        state = JUMP_COMPL;
      } else if (opcode == ADDI) {
        state = ADDI_EXEC;
      } else if (opcode == EOP) {
        state = EXIT_STATE;
      } else {
        assert(false);
      }
      break;
    case MEM_ADDR_COMP:
      control->ALUSrcA = 1;
      control->ALUSrcB = 2;
      control->ALUOp   = 0;
      if (opcode == LW) {
        state = MEM_ACCESS_LD;
      } else if (opcode == SW) {
        state = MEM_ACCESS_ST;
      } else {
        assert(false);
      }
      break;
    case MEM_ACCESS_LD:
      control->MemRead = 1;
      control->IorD    = 1;
      state            = WB_STEP;
      break;
    case WB_STEP:
      control->RegDst   = 0;
      control->RegWrite = 1;
      control->MemtoReg = 1;
      state             = INSTR_FETCH;
      break;
    case MEM_ACCESS_ST:
      control->MemWrite = 1;
      control->IorD     = 1;
      state             = INSTR_FETCH;
      break;
    case EXEC:
      control->ALUSrcA = 1;
      control->ALUSrcB = 0;
      control->ALUOp   = 2;
      state            = R_TYPE_COMPL;
      break;
    case R_TYPE_COMPL:
      control->RegDst   = 1;
      control->RegWrite = 1;
      control->MemtoReg = 0;
      state             = INSTR_FETCH;
      break;
    case BRANCH_COMPL:
      control->ALUSrcA     = 1;
      control->ALUSrcB     = 0;
      control->ALUOp       = 1;
      control->PCWriteCond = 1;
      control->PCSource    = 1;
      state                = INSTR_FETCH;
      break;
    case JUMP_COMPL:
      control->PCWrite  = 1;
      control->PCSource = 2;
      state             = INSTR_FETCH;
      break;
    case ADDI_EXEC:
      control->ALUSrcA = 1;
      control->ALUSrcB = 2;
      control->ALUOp   = 0;
      state            = ADDI_WB;
      break;
    case ADDI_WB:
      control->RegDst   = 0;
      control->MemtoReg = 0;
      control->RegWrite = 1;
      state             = INSTR_FETCH;
      break;
    default: assert(false);
  }
  arch_state.state = state;
}

void instruction_fetch() {
  if (arch_state.control.MemRead) {
    int address                  = arch_state.curr_pipe_regs.pc;
    arch_state.next_pipe_regs.IR = memory_read(address);
  }
}

void decode_and_read_RF() {
  int read_register_1 = arch_state.IR_meta.reg_21_25;
  int read_register_2 = arch_state.IR_meta.reg_16_20;
  check_is_valid_reg_id(read_register_1);
  check_is_valid_reg_id(read_register_2);
  arch_state.next_pipe_regs.A = arch_state.registers[read_register_1];
  arch_state.next_pipe_regs.B = arch_state.registers[read_register_2];
}

void execute() {
  struct ctrl_signals *control     = &arch_state.control;
  struct instr_meta *IR_meta       = &arch_state.IR_meta;
  struct pipe_regs *curr_pipe_regs = &arch_state.curr_pipe_regs;
  struct pipe_regs *next_pipe_regs = &arch_state.next_pipe_regs;

  int alu_opA   = control->ALUSrcA ? curr_pipe_regs->A : curr_pipe_regs->pc;
  int alu_opB   = 0;
  int immediate = IR_meta->immediate;
  int shifted_immediate = (immediate) << 2;
  switch (control->ALUSrcB) {
    case 0: alu_opB = curr_pipe_regs->B; break;
    case 1: alu_opB = WORD_SIZE; break;
    case 2: alu_opB = immediate; break;
    case 3: alu_opB = shifted_immediate; break;
    default: assert(false);
  }

  switch (control->ALUOp) {
    case 0: next_pipe_regs->ALUOut = alu_opA + alu_opB; break;
    case 1: next_pipe_regs->ALUOut = alu_opA - alu_opB; break;
    case 2:
      switch (IR_meta->function) {
        case ADD: next_pipe_regs->ALUOut = alu_opA + alu_opB; break;
        case ADDU:
          // I'm not sure this is actually making a difference, but that's the
          // way it *should* be.
          next_pipe_regs->ALUOut =
              (unsigned int)alu_opA + (unsigned int)alu_opB;
          break;
        default: assert(false);
      };
      break;
    default: assert(false);
  }

  // PC calculation
  switch (control->PCSource) {
    case 0: next_pipe_regs->pc = next_pipe_regs->ALUOut; break;
    case 1: next_pipe_regs->pc = curr_pipe_regs->ALUOut; break;
    case 2:
      next_pipe_regs->pc =
          curr_pipe_regs->pc & 0xf000 | (IR_meta->jmp_offset << 2);
      break;
    default: assert(false);
  }
}

void memory_access() {
  ///@students: appropriate calls to functions defined in memory_hierarchy.c
  /// must be added
  struct ctrl_signals *control     = &arch_state.control;
  struct instr_meta *IR_meta       = &arch_state.IR_meta;
  struct pipe_regs *curr_pipe_regs = &arch_state.curr_pipe_regs;
  struct pipe_regs *next_pipe_regs = &arch_state.next_pipe_regs;

  if (!control->MemRead && !control->MemWrite) {
    // We don't need to access memory if the control doesn't say so.
    return;
  }
  if (control->RegDst && control->MemtoReg) {  // 4a: Result write
    memory_write(IR_meta->reg_11_15, curr_pipe_regs->ALUOut);
  } else if (control->IorD && control->MemRead) {  // 4b: Load from mem
    next_pipe_regs->MDR = memory_read(curr_pipe_regs->ALUOut);
  } else if (control->IorD && control->MemWrite) {
    memory_write(
        curr_pipe_regs->ALUOut,
        curr_pipe_regs->B);  // 4c: Store to mem
  }
}

void write_back() {
  struct ctrl_signals *control = &arch_state.control;
  if (control->RegWrite) {
    int write_reg_id = control->RegDst ? arch_state.IR_meta.reg_11_15
                                       : arch_state.IR_meta.reg_16_20;
    check_is_valid_reg_id(write_reg_id);
    int write_data = arch_state.curr_pipe_regs.ALUOut;
    if (write_reg_id > 0) {
      arch_state.registers[write_reg_id] = write_data;
      // printf("Reg $%u = %d \n", write_reg_id, write_data);
    } else
      printf("Attempting to write reg_0. That is likely a mistake \n");
  }
}

void set_up_IR_meta(int IR, struct instr_meta *IR_meta) {
  IR_meta->opcode     = get_piece_of_a_word(IR, OPCODE_OFFSET, OPCODE_SIZE);
  IR_meta->immediate  = get_sign_extended_imm_id(IR, IMMEDIATE_OFFSET);
  IR_meta->function   = get_piece_of_a_word(IR, 0, 6);
  IR_meta->jmp_offset = get_piece_of_a_word(IR, 0, 26);
  IR_meta->reg_11_15  = (uint8_t)get_piece_of_a_word(IR, 11, REGISTER_ID_SIZE);
  IR_meta->reg_16_20  = (uint8_t)get_piece_of_a_word(IR, 16, REGISTER_ID_SIZE);
  IR_meta->reg_21_25  = (uint8_t)get_piece_of_a_word(IR, 21, REGISTER_ID_SIZE);
  IR_meta->type       = get_instruction_type(IR_meta->opcode);

  switch (IR_meta->opcode) {
    case SPECIAL:
      if (IR_meta->function == ADD)
        printf(
            "Executing ADD(%d), $%u = $%u + $%u (function: %u) \n",
            IR_meta->opcode,
            IR_meta->reg_11_15,
            IR_meta->reg_21_25,
            IR_meta->reg_16_20,
            IR_meta->function);
      else if (IR_meta->function == SLT)
        printf(
            "Executing SLT(%d), $%u = $%u < $%u (function: %u) \n",
            IR_meta->opcode,
            IR_meta->reg_11_15,
            IR_meta->reg_21_25,
            IR_meta->reg_16_20,
            IR_meta->function);
      else
        assert(false);
      break;
    case LW:
      printf(
          "Executing LW(%d), $%u<-%d($%u) \n",
          IR_meta->opcode,
          IR_meta->reg_16_20,
          IR_meta->immediate,
          IR_meta->reg_21_25);
      break;
    case SW:
      printf(
          "Executing SW(%d), $%u->%d($%u) \n",
          IR_meta->opcode,
          IR_meta->reg_16_20,
          IR_meta->immediate,
          IR_meta->reg_21_25);
      break;
    case BEQ:
      printf(
          "Executing BEQ(%d), $PC += $%u == $%u ? 4 : %d (%d << 2) \n",
          IR_meta->opcode,
          IR_meta->reg_21_25,
          IR_meta->reg_16_20,
          IR_meta->immediate << 2,
          IR_meta->immediate);
      break;
    case ADDI:
      printf(
          "Executing ADDI(%d), $%u = $%u + %d \n",
          IR_meta->opcode,
          IR_meta->reg_16_20,
          IR_meta->reg_21_25,
          IR_meta->immediate);
      break;
    case J:
      printf(
          "Executing J(%d), $PC = $PC & 0xf000 | (%d << 2)\n",
          IR_meta->opcode,
          IR_meta->jmp_offset);
      break;
    case EOP: printf("Executing EOP(%d) \n", IR_meta->opcode); break;
    default: printf("Opcode: %d\n", IR_meta->opcode); assert(false);
  }
}

void assign_pipeline_registers_for_the_next_cycle() {
  struct ctrl_signals *control     = &arch_state.control;
  struct instr_meta *IR_meta       = &arch_state.IR_meta;
  struct pipe_regs *curr_pipe_regs = &arch_state.curr_pipe_regs;
  struct pipe_regs *next_pipe_regs = &arch_state.next_pipe_regs;

  if (control->IRWrite) {
    curr_pipe_regs->IR = next_pipe_regs->IR;
    printf("PC %d: ", curr_pipe_regs->pc / 4);
    set_up_IR_meta(curr_pipe_regs->IR, IR_meta);
  }
  curr_pipe_regs->ALUOut = next_pipe_regs->ALUOut;
  curr_pipe_regs->A      = next_pipe_regs->A;
  curr_pipe_regs->B      = next_pipe_regs->B;
  // We write if PCWrite or PCWriteCond and Cond
  if (control->PCWrite || (control->PCWriteCond && !next_pipe_regs->ALUOut)) {
    check_address_is_word_aligned(next_pipe_regs->pc);
    curr_pipe_regs->pc = next_pipe_regs->pc;
  }
}

int main(int argc, const char *argv[]) {
  /*--------------------------------------
  /------- Global Variable Init ----------
  /--------------------------------------*/
  parse_arguments(argc, argv);
  arch_state_init(&arch_state);
  ///@students WARNING: Do NOT change/move/remove main's code above this
  /// point!
  while (true) {
    ///@students: Fill/modify the function bodies of the 7 functions below,
    /// Do NOT modify the main() itself, you only need to
    /// write code inside the definitions of the functions called below.

    FSM();

    instruction_fetch();

    decode_and_read_RF();

    execute();

    memory_access();

    write_back();

    // printf( "$3: %d, $6: %d, $31; %d\n", arch_state.registers[3],
    // arch_state.registers[6], arch_state.registers[31]);

    assign_pipeline_registers_for_the_next_cycle();

    ///@students WARNING: Do NOT change/move/remove code below this point!
    marking_after_clock_cycle();
    arch_state.clock_cycle++;
    // Check exit statements
    if (arch_state.state == EXIT_STATE) {  // I.E. EOP instruction!
      printf("Exiting because the exit state was reached \n");
      break;
    }
    if (arch_state.clock_cycle == BREAK_POINT) {
      printf(
          "Exiting because the break point (%u) was reached \n",
          BREAK_POINT);
      break;
    }
  }
  marking_at_the_end();
}
