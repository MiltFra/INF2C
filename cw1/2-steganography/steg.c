#include <stdio.h>
#define INPUT_SIZE 10000

int print_word_and_skip_line(char **p) {
  while (**p != ' ' && **p != '\n' && **p != '\0') {
    putchar(**p);
    (*p)++;
  }
  while (**p != '\n' && **p != '\0') {
    (*p)++;
  }
  return *((*p)++) == '\0';
}

int process_line(char **p, int n, char *first) {
  int num_words = 0;
  char is_space;
  char is_eol;
  char is_eof;
  while (num_words++ < n) {
    do {
      is_space = **p == ' ';
      is_eol   = **p == '\n';
      is_eof   = **p == '\0';
      (*p)++;
    } while (!is_space && !is_eol && !is_eof);
    if (is_eof) {
      return 1;
    }
  }
  if (is_space) {
    return print_word_and_skip_line(p);
  } else {
    putchar('\n');
    return 0;
  }
}

int main(void) {
  char input_text[INPUT_SIZE + 1];
  FILE *f = fopen("task2_in.txt", "r");
  if (!f) {
    return 1;
  }
  fread(input_text, sizeof(char), INPUT_SIZE + 1, f);
  char *p    = input_text;
  int n      = 0;
  char first = 1;
  while (!process_line(&p, n++, &first))
    ;
}