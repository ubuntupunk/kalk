#define _POSIX_C_SOURCE 200809L
#include "cell.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int CW = 8;  // column display width

struct cell* cell(struct grid* g, int c, int r) {
  return (c >= 0 && c < NCOL && r >= 0 && r < NROW) ? &g->cells[c][r] : NULL;
}

void recalc(struct grid* g) {
  for (int pass = 0; pass < 100; pass++) {
    int changed = 0;
    for (int r = 0; r < NROW; r++)
      for (int c = 0; c < NCOL; c++) {
        struct cell* cl = &g->cells[c][r];
        if (cl->type != FORMULA) continue;
        struct parser p = {cl->text, cl->text, g};
        float v = cmp(&p);
        if (v != cl->val) changed = 1;
        cl->val = v;
      }
    if (!changed) break;
  }
}

// Emit a cell reference into buf, preserving $ markers.
static int emitref(char* buf, int bufsz, int rc, int rr, int ac, int ar) {
  int oi = 0;
  if (ac && oi < bufsz) buf[oi++] = '$';
  oi += snprintf(buf + oi, bufsz - oi, "%s", col(rc));
  if (ar && oi < bufsz) buf[oi++] = '$';
  oi += snprintf(buf + oi, bufsz - oi, "%d", rr + 1);
  return oi;
}

// Rewrite cell references in all formulas after swapping two adjacent rows or columns.
// axis='R': rows a and b swapped. axis='C': columns a and b swapped.
static void fixrefs(struct grid* g, int axis, int a, int b) {
  for (int r = 0; r < NROW; r++)
    for (int c = 0; c < NCOL; c++) {
      struct cell* cl = &g->cells[c][r];
      if (cl->type != FORMULA) continue;
      char out[MAXIN] = {0};
      int oi = 0, changed = 0;
      const char* s = cl->text;
      while (*s && oi < MAXIN - 8) {
        int rc, rr, ac, ar, n = refabs(s, &rc, &rr, &ac, &ar);
        if (n) {
          if (axis == 'R') {
            if (rr == a)
              rr = b, changed = 1;
            else if (rr == b)
              rr = a, changed = 1;
          } else {
            if (rc == a)
              rc = b, changed = 1;
            else if (rc == b)
              rc = a, changed = 1;
          }
          oi += emitref(out + oi, MAXIN - oi, rc, rr, ac, ar);
          s += n;
        } else {
          out[oi++] = *s++;
        }
      }
      out[oi] = '\0';
      if (changed) strncpy(cl->text, out, MAXIN - 1);
    }
}

// Shift cell references after insert/delete.
// axis='R' for row, 'C' for column. pos=where inserted/deleted. dir=+1 insert, -1 delete.
static void shiftrefs(struct grid* g, int axis, int pos, int dir) {
  for (int r = 0; r < NROW; r++)
    for (int c = 0; c < NCOL; c++) {
      struct cell* cl = &g->cells[c][r];
      if (cl->type != FORMULA) continue;
      char out[MAXIN] = {0};
      int oi = 0, changed = 0;
      const char* s = cl->text;
      while (*s && oi < MAXIN - 8) {
        int rc, rr, ac, ar, n = refabs(s, &rc, &rr, &ac, &ar);
        if (n) {
          if (axis == 'R') {
            if (dir > 0 && rr >= pos)
              rr++, changed = 1;
            else if (dir < 0 && rr > pos)
              rr--, changed = 1;
          } else {
            if (dir > 0 && rc >= pos)
              rc++, changed = 1;
            else if (dir < 0 && rc > pos)
              rc--, changed = 1;
          }
          oi += emitref(out + oi, MAXIN - oi, rc, rr, ac, ar);
          s += n;
        } else {
          out[oi++] = *s++;
        }
      }
      out[oi] = '\0';
      if (changed) strncpy(cl->text, out, MAXIN - 1);
    }
}

void insertrow(struct grid* g, int at) {
  for (int c = 0; c < NCOL; c++)
    for (int r = NROW - 1; r > at; r--) g->cells[c][r] = g->cells[c][r - 1];
  for (int c = 0; c < NCOL; c++) g->cells[c][at] = (struct cell){0};
  shiftrefs(g, 'R', at, +1);
  g->dirty = 1;
}

void insertcol(struct grid* g, int at) {
  for (int r = 0; r < NROW; r++)
    for (int c = NCOL - 1; c > at; c--) g->cells[c][r] = g->cells[c - 1][r];
  for (int r = 0; r < NROW; r++) g->cells[at][r] = (struct cell){0};
  shiftrefs(g, 'C', at, +1);
  g->dirty = 1;
}

void deleterow(struct grid* g, int at) {
  shiftrefs(g, 'R', at, -1);
  for (int c = 0; c < NCOL; c++)
    for (int r = at; r < NROW - 1; r++) g->cells[c][r] = g->cells[c][r + 1];
  for (int c = 0; c < NCOL; c++) g->cells[c][NROW - 1] = (struct cell){0};
  g->dirty = 1;
}

void deletecol(struct grid* g, int at) {
  shiftrefs(g, 'C', at, -1);
  for (int r = 0; r < NROW; r++)
    for (int c = at; c < NCOL - 1; c++) g->cells[c][r] = g->cells[c + 1][r];
  for (int r = 0; r < NROW; r++) g->cells[NCOL - 1][r] = (struct cell){0};
  g->dirty = 1;
}

void setcell(struct grid* g, int c, int r, const char* input) {
  struct cell* cl = cell(g, c, r);
  if (!cl) return;
  if (!*input) {
    *cl = (struct cell){0};
    recalc(g);
    return;
  }

  strncpy(cl->text, input, MAXIN - 1);
  g->dirty = 1;

  if (input[0] == '+' || input[0] == '-' || input[0] == '(' || input[0] == '@' || input[0] == '=') {
    cl->type = FORMULA;
  } else if (isdigit(input[0]) || input[0] == '.') {
    char* end;
    double v = strtod(input, &end);
    cl->type = (*end == '\0') ? NUM : FORMULA;
    if (cl->type == NUM) cl->val = v;
  } else {
    cl->type = LABEL;
    cl->val = 0;
  }
  recalc(g);
}

char* col(int c) {
  static char buf[16] = {0};
  buf[0] = (c < 26) ? ('A' + c) : ('A' + (c) / 26 - 1);
  buf[1] = (c < 26) ? '\0' : ('A' + (c) % 26);
  return buf;
}

void swaprow(struct grid* g, int a, int b) {
  for (int c = 0; c < NCOL; c++) {
    struct cell tmp = g->cells[c][a];
    g->cells[c][a] = g->cells[c][b];
    g->cells[c][b] = tmp;
  }
  fixrefs(g, 'R', a, b);
}

void swapcol(struct grid* g, int a, int b) {
  for (int r = 0; r < NROW; r++) {
    struct cell tmp = g->cells[a][r];
    g->cells[a][r] = g->cells[b][r];
    g->cells[b][r] = tmp;
  }
  fixrefs(g, 'C', a, b);
}

void sortbycol(struct grid* g, int col) {
  // Bubble sort: put EMPTY/LABEL cells at bottom, sort NUM/FORMULA by value ascending
  for (int i = 0; i < NROW - 1; i++) {
    for (int j = 0; j < NROW - 1 - i; j++) {
      struct cell* a = &g->cells[col][j];
      struct cell* b = &g->cells[col][j + 1];
      int a_empty = (a->type == EMPTY || a->type == LABEL);
      int b_empty = (b->type == EMPTY || b->type == LABEL);
      if (a_empty && !b_empty) {
        swaprow(g, j, j + 1);
      } else if (!a_empty && !b_empty && a->val > b->val) {
        swaprow(g, j, j + 1);
      }
    }
  }
}

void replicatecell(struct grid* g, int sc, int sr, int dc, int dr) {
  struct cell* src = cell(g, sc, sr);
  struct cell* dst = cell(g, dc, dr);
  if (!src || !dst) return;
  if (src->type == EMPTY) {
    *dst = (struct cell){0};
    return;
  }
  *dst = *src;
  if (src->type != FORMULA) return;

  int dcol = dc - sc, drow = dr - sr;
  char out[MAXIN] = {0};
  int oi = 0;
  const char* s = src->text;
  while (*s && oi < MAXIN - 8) {
    int rc, rr, ac, ar, n = refabs(s, &rc, &rr, &ac, &ar);
    if (n) {
      if (!ac) rc += dcol;
      if (!ar) rr += drow;
      oi += emitref(out + oi, MAXIN - oi, rc, rr, ac, ar);
      s += n;
    } else {
      out[oi++] = *s++;
    }
  }
  out[oi] = '\0';
  strncpy(dst->text, out, MAXIN - 1);
}
