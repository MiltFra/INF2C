#include <stdio.h>
#define INPUT_SIZE 10000

int main(void) {
  char input_text[INPUT_SIZE + 1];
  char book_text[INPUT_SIZE + 1];
  FILE *f = fopen("input_book_cipher.txt", "r");
  if (!f) {
    return 1;
  }
  fread(input_text, sizeof(char), INPUT_SIZE + 1, f);
  fclose(f);
  f = fopen("book.txt", "r");
  if (!f) {
    return 1;
  }
  fread(book_text, sizeof(char), INPUT_SIZE + 1, f);
  fclose(f);

  register char *p = input_text;  // $t0
  register char *q;               // $t1
  register int line, word;        // $t2, $t3
  register char new_line = 1;     // $t4
  while (*p != '\0') {
    line = word = 0;
    while (*p != ' ') {
      line *= 10;
      line += *p - '0';
      p++;
    }
    p++;
    while (*p != '\n') {
      word *= 10;
      word += *p - '0';
      p++;
    }
    p++;
    q = book_text;
    while (line != 1) {
      if (*q == '\n') {
        line--;
      }
      q++;
    }
    while (word != 1) {
      if (*q == ' ') {
        word--;
      }
      if (*q == '\n') {
        putchar(*q);
        new_line = 1;
        break;
      }
      q++;
    }
    if (*q == '\n') {
      continue;
    }
    if (new_line) {
      new_line = 0;
    } else {
      putchar(' ');
    }
    while (*q != ' ' && *q != '\n') {
      putchar(*q);
      q++;
    }
  }
  if (!new_line) {
    putchar('\n');
  }
  return 0;
}