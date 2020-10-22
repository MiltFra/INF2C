#include <stdio.h>
#define INPUT_SIZE   10000
#define MAX_KEY_SIZE 4

int compare_substring(char *p, char *q) {
  while (1) {
    if (*p == '\0') {
      break;
    }
    if (*p == '\n') {
      *p = ' ';
    }
    if (*p != *q) {
      break;
    }
    p++;
    q++;
  }
  return *q == '\0';
}

int main(void) {
  char input_text[INPUT_SIZE + 1];
  char hint_text[INPUT_SIZE + 1];
  char decrypt_text[INPUT_SIZE + 1];
  char key_text[10];
  FILE *f = fopen("input_xor_crack.txt", "r");
  if (!f) {
    return 1;
  }
  fread(input_text, sizeof(char), INPUT_SIZE + 1, f);
  fclose(f);
  f = fopen("hint.txt", "r");
  if (!f) {
    return 1;
  }
  fread(hint_text, sizeof(char), INPUT_SIZE + 1, f);
  fclose(f);
  char *h;
  for (h = hint_text; *h != '\n' && *h != '\0'; h++) {
  }
  *h               = '\0';
  register int key = 0;
  register int i;
  register char c;
  register char *p;
  do {
    i = 0;
    c = input_text[i];
    while (c != '\0') {
      if (c != ' ' && c != '\n') {
        c ^= key;
      }
      decrypt_text[i] = c;
      i++;
      c = input_text[i];
    }
    p = decrypt_text;
    while (*p != 0) {
      if (compare_substring(p, hint_text)) {
        break;
      }
      p++;
    }
    if (*p != 0) {
      break;
    }
    key++;
  } while (key != 0x100);
  if (key == 0) {
    return -1;
  }
  i           = 8;
  key_text[8] = '\n';
  key_text[9] = '\0';
  while (i) {
    i--;
    c = 48;
    c += key & 1;
    key_text[i] = c;
    key >>= 1;
  }
  printf("%s", key_text);
}