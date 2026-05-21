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
    static const int cmap[] = {COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
                               COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE};
    for (int fg = 0; fg < 8; fg++)
      for (int bg = 0; bg < 8; bg++)
        if (fg > 0 || bg > 0)  // skip pair 0 (unused default)
          init_pair(fg * 8 + bg, cmap[fg], cmap[bg]);
  }
  loop(&g);
  endwin();
  return 0;
}
