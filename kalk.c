#define _POSIX_C_SOURCE 200809L
#include "cell.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
  static struct grid g = {0};
  if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
    fprintf(stderr, "Usage: %s sheet.csv\n", argv[0]);
    exit(1);
  }
  if (argc > 1) {
    if (csvload(&g, argv[1]) < 0) {
      perror("Failed to load CSV file");
      exit(1);
    }
    g.filename = argv[1];
  }
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  curs_set(0);
  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
  }
  loop(&g);
  endwin();
  return 0;
}
