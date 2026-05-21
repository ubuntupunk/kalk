#define _POSIX_C_SOURCE 200809L
#include "cell.h"
#include <stdio.h>
#include <string.h>

static int csvfield(FILE* f, char* buf, int bufsz, int* eol, int* eof) {
  int c, n = 0, quoted = 0;
  *eol = *eof = 0;
  buf[0] = '\0';

  c = fgetc(f);
  if (c == EOF) {
    *eof = 1;
    return 0;
  }

  if (c == '"') {
    quoted = 1;
    for (;;) {
      c = fgetc(f);
      if (c == EOF) {
        *eof = 1;
        break;
      }
      if (c == '"') {
        c = fgetc(f);
        if (c == '"') {
          if (n < bufsz - 1) buf[n++] = '"';
        } else {
          if (c == '\r') c = fgetc(f);
          if (c == '\n' || c == EOF) {
            *eol = 1;
            if (c == EOF) *eof = 1;
          }
          break;
        }
      } else {
        if (n < bufsz - 1) buf[n++] = c;
      }
    }
  } else {
    for (;;) {
      if (c == ',' || c == '\n' || c == EOF) {
        if (c == '\n' || c == EOF) {
          *eol = 1;
          if (c == EOF) *eof = 1;
        }
        break;
      }
      if (c == '\r') {
        c = fgetc(f);
        continue;
      }
      if (n < bufsz - 1) buf[n++] = c;
      c = fgetc(f);
    }
  }
  buf[n] = '\0';
  return 1;
}

int csvload(struct grid* g, const char* filename) {
  FILE* f = fopen(filename, "r");
  if (!f) return -1;
  char buf[MAXIN];
  int row = 0, col = 0, eol, eof;

  while (csvfield(f, buf, sizeof(buf), &eol, &eof)) {
    if (buf[0] && row < NROW && col < NCOL) setcell(g, col, row, buf);
    if (eol) {
      row++, col = 0;
    } else
      col++;
    if (eof) break;
  }
  fclose(f);
  return 0;
}

static int csvneedsquote(const char* s) {
  for (; *s; s++)
    if (*s == ',' || *s == '"' || *s == '\n' || *s == '\r') return 1;
  return 0;
}

static void csvwritefield(FILE* f, const char* s) {
  if (csvneedsquote(s)) {
    fputc('"', f);
    for (; *s; s++) {
      if (*s == '"') fputc('"', f);
      fputc(*s, f);
    }
    fputc('"', f);
  } else {
    fputs(s, f);
  }
}

int csvsave(struct grid* g, const char* filename) {
  FILE* f = fopen(filename, "w");
  if (!f) return -1;

  int maxr = -1, maxc = -1;
  for (int r = 0; r < NROW; r++)
    for (int c = 0; c < NCOL; c++)
      if (g->cells[c][r].type != EMPTY) {
        if (r > maxr) maxr = r;
        if (c > maxc) maxc = c;
      }

  for (int r = 0; r <= maxr; r++) {
    for (int c = 0; c <= maxc; c++) {
      if (c > 0) fputc(',', f);
      struct cell* cl = &g->cells[c][r];
      if (cl->type == EMPTY) continue;
      csvwritefield(f, cl->text);
    }
    fputc('\n', f);
  }
  fclose(f);
  return 0;
}
