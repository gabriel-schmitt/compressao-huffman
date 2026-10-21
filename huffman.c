#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc != 4) {
    printf("uso: %s c|d <entrada> <saida>\n", argv[0]);
    return 1;
  }

  return 0;
}
