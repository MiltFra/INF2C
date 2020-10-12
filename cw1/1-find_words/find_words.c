#include <stdio.h>

int main(void) {
  char c;
  while ((c = getchar()) != EOF) {
    if (c == ' ') {
      putchar('\n');
    } else {
      putchar(c);
    }
  }
}