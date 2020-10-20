#include <stdio.h>
#define INPUT_SIZE   10000
#define MAX_KEY_SIZE 4

int main(void) {
  char input_text[INPUT_SIZE + 1];
  char key_text[8 * MAX_KEY_SIZE + 1];
  FILE *f = fopen("input_xor.txt", "r");
  if (!f) {
    return 1;
  }
  fread(input_text, sizeof(char), INPUT_SIZE + 1, f);
  fclose(f);
  f = fopen("key_xor.txt", "r");
  if (!f) {
    return 1;
  }
  fread(key_text, sizeof(char), 8 * MAX_KEY_SIZE + 1, f);
  fclose(f);
  char key[MAX_KEY_SIZE];
  register char *key_begin = key; // $t7
  register char *key_end;         // $t8
  int num_bits;
  for (num_bits = 0; key_text[num_bits] != '\n'; num_bits++)
    ;
  int num_bytes = num_bits >> 3;
  key_end = key_begin + num_bytes;
  for (int i = 0; i < num_bytes; i++) {
    char byte_value = 0;
    for (int j = 0; j < 8; j++) {
      byte_value <<= 1;
      byte_value += key_text[i * 8 + j] == '1';
    }
    key_begin[i] = byte_value;
  }
  register char *p = input_text;  // pointer to current input
  register char *q = key_begin;   // pointer to current key symbol
  while (*p != '\0') {
    if (*p == ' ' || *p == '\n') {
      putchar(*p);
    } else {
      putchar(*p ^ *q);
    }
    p++;
    q++;
    if (q == key_end) {
      q = key_begin;
    }
  }
  return 0;
}