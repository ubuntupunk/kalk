#define _POSIX_C_SOURCE 200809L
#include "cell.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int CW = 8;  // column display width

// Function name list for autocomplete (alphabetical, NULL-terminated)
const char* const func_names[] = {
    "ABS", "AVERAGE", "CEILING", "CONCATENATE", "COS", "COUNT", "COUNTIF",
    "DATE", "FIND", "FLOOR", "HLOOKUP", "IF", "INT", "LEFT", "LEN", "LOG",
    "LOWER", "MAX", "MID", "MIN", "MOD", "NOW", "PI", "POWER", "RAND",
    "RANDBETWEEN", "RIGHT", "ROUND", "ROUNDDOWN", "ROUNDUP", "SIN", "SQRT",
    "SUM", "SUMIF", "TAN", "TODAY", "TRIM", "UPPER", "VLOOKUP",
    NULL
};

// Multi-sheet support
struct grid* sheets[MAXSHEETS];
int cur_sheet = 0;
int n_sheets = 1;
char sheet_names[MAXSHEETS][SHEETNAMELEN];

void init_sheets(void) {
    for (int i = 0; i < MAXSHEETS; i++) sheets[i] = NULL;
    sheets[0] = calloc(1, sizeof(struct grid));
    strncpy(sheet_names[0], "Sheet1", SHEETNAMELEN - 1);
    n_sheets = 1;
    cur_sheet = 0;
}

struct grid* curgrid(void) {
    return sheets[cur_sheet];
}

void newsheet(const char* name) {
    if (n_sheets >= MAXSHEETS) return;
    sheets[n_sheets] = calloc(1, sizeof(struct grid));
    strncpy(sheet_names[n_sheets], name, SHEETNAMELEN - 1);
    n_sheets++;
}

void delsheet(void) {
    if (n_sheets <= 1) return;
    free(sheets[n_sheets - 1]);
    sheets[n_sheets - 1] = NULL;
    if (cur_sheet >= n_sheets - 1) cur_sheet = n_sheets - 2;
    n_sheets--;
}

void renamesheet(int idx, const char* name) {
    if (idx < 0 || idx >= n_sheets) return;
    strncpy(sheet_names[idx], name, SHEETNAMELEN - 1);
}


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
        struct parser p;
        p.s = cl->text;
        p.p = cl->text;
        p.g = g;
        p.arg_str[0] = '\0';
        p.has_str_result = 0;
        float v = cmp(&p);
        if (v != cl->val) changed = 1;
        cl->val = v;
        if (p.has_str_result) {
          strncpy(cl->strval, p.arg_str, MAXIN - 1);
          cl->is_str = 1;
        } else if (!isnan(v)) {
          snprintf(cl->strval, MAXIN, "%g", v);
          cl->is_str = 0;
        } else {
          cl->strval[0] = '\0';
          cl->is_str = 0;
        }
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
    cl->strval[0] = '\0';
    cl->is_str = 0;
    recalc(g);
    return;
  }

  strncpy(cl->text, input, MAXIN - 1);
  g->dirty = 1;
  cl->strval[0] = '\0';
  cl->is_str = 0;

  if (input[0] == '+' || input[0] == '-' || input[0] == '(' || input[0] == '@' || input[0] == '=') {
    cl->type = FORMULA;
  } else if (isdigit(input[0]) || input[0] == '.') {
    char* end;
    double v = strtod(input, &end);
    cl->type = (*end == '\0') ? NUM : FORMULA;
    if (cl->type == NUM) {
      cl->val = v;
      snprintf(cl->strval, MAXIN, "%g", v);
    }
  } else {
    cl->type = LABEL;
    cl->val = 0;
    cl->is_str = 1;
    strncpy(cl->strval, input, MAXIN - 1);
  }
  recalc(g);
}

const char* cell_str(struct grid* g, int c, int r) {
  struct cell* cl = cell(g, c, r);
  if (!cl) return "";
  if (cl->type == LABEL) return cl->text;
  if (cl->strval[0]) return cl->strval;
  return "";
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

// Date/time name tables for auto-fill pattern detection
static const char* const day_short[] = {"MON","TUE","WED","THU","FRI","SAT","SUN"};
static const char* const day_full[] = {"MONDAY","TUESDAY","WEDNESDAY","THURSDAY","FRIDAY","SATURDAY","SUNDAY"};
static const char* const mon_short[] = {"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
static const char* const mon_full[] = {"JANUARY","FEBRUARY","MARCH","APRIL","MAY","JUNE","JULY","AUGUST","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER"};

// Case-insensitive string comparison (POSIX)
static int my_strcasecmp(const char* a, const char* b) {
  for (; *a && *b; a++, b++) {
    int da = toupper(*a), db = toupper(*b);
    if (da != db) return da - db;
  }
  return toupper(*a) - toupper(*b);
}

// Find index of s in a string table, or -1 if not found
static int in_table(const char* s, const char* const* table, int n) {
  for (int i = 0; i < n; i++)
    if (my_strcasecmp(s, table[i]) == 0) return i;
  return -1;
}

// Auto-fill: detect pattern from seed cells and extend
// fill_right=0 fill downward, fill_right=1 fill rightward
// seed_c,seed_r = first seed cell; seed_n = number of seed cells (consecutive in fill direction)
// count = number of new cells to fill beyond the seed
void autofill(struct grid* g, int seed_c, int seed_r, int seed_n, int count, int fill_right) {
  if (seed_n < 1 || count < 1) return;
  if (seed_n > 16) seed_n = 16;
  if (count > 1024) count = 1024;  // safety cap

  // Collect seed values
  float vals[16];
  char texts[16][MAXIN];
  int is_numeric = 1;
  int all_empty = 1;

  for (int i = 0; i < seed_n; i++) {
    int c = fill_right ? (seed_c + i) : seed_c;
    int r = fill_right ? seed_r : (seed_r + i);
    struct cell* cl = cell(g, c, r);
    texts[i][0] = '\0';
    vals[i] = 0;
    if (cl && cl->type != EMPTY) {
      all_empty = 0;
      strncpy(texts[i], cell_str(g, c, r), MAXIN - 1);
      if (cl->type == NUM || cl->type == FORMULA) {
        vals[i] = cl->val;
        if (isnan(vals[i])) is_numeric = 0;
      } else {
        is_numeric = 0;
      }
    } else {
      is_numeric = 0;
    }
  }

  if (all_empty) return;

  // Pattern detection
  // 0=copy/tile, 1=linear arithmetic, 2=day_short, 3=day_full, 4=mon_short, 5=mon_full
  int ptype = 0;
  float step = 1;
  int date_idx = 0, date_n = 7;
  const char* const* date_table = day_short;

  // Try linear arithmetic pattern
  if (is_numeric && seed_n >= 2) {
    float diff = vals[1] - vals[0];
    int consistent = 1;
    for (int i = 2; i < seed_n; i++) {
      float d = vals[i] - vals[i - 1];
      if (fabs(d - diff) > 0.0001f) { consistent = 0; break; }
    }
    if (consistent && !isnan(diff) && diff != 0) {
      ptype = 1;
      step = diff;
    } else if (consistent && diff == 0) {
      // All same value, still linear with step=0
      ptype = 1;
      step = 0;
    }
  }

  // Try date name patterns
  if (ptype == 0 && texts[0][0]) {
    int idx;
    if ((idx = in_table(texts[0], day_short, 7)) >= 0) {
      ptype = 2; date_idx = idx; date_table = day_short; date_n = 7;
    } else if ((idx = in_table(texts[0], day_full, 7)) >= 0) {
      ptype = 3; date_idx = idx; date_table = day_full; date_n = 7;
    } else if ((idx = in_table(texts[0], mon_short, 12)) >= 0) {
      ptype = 4; date_idx = idx; date_table = mon_short; date_n = 12;
    } else if ((idx = in_table(texts[0], mon_full, 12)) >= 0) {
      ptype = 5; date_idx = idx; date_table = mon_full; date_n = 12;
    }
    // Verify consecutive entries for multi-cell seeds
    if (ptype >= 2 && seed_n >= 2) {
      for (int i = 1; i < seed_n; i++) {
        int expected = (date_idx + i) % date_n;
        if (in_table(texts[i], date_table, date_n) != expected) {
          ptype = 0;  // not consecutive, fall back to copy
          break;
        }
      }
    }
  }

  // Fill target cells
  for (int i = 0; i < count; i++) {
    int target_idx = seed_n + i;
    int tc = fill_right ? (seed_c + target_idx) : seed_c;
    int tr = fill_right ? seed_r : (seed_r + target_idx);
    if (tc >= NCOL || tr >= NROW) break;

    char buf[MAXIN];

    switch (ptype) {
      case 1: {  // linear arithmetic
        float v = vals[seed_n - 1] + step * (i + 1);
        snprintf(buf, sizeof(buf), "%g", v);
        setcell(g, tc, tr, buf);
        break;
      }
      case 2: case 3:  // day names
      case 4: case 5: {  // month names
        int idx = (date_idx + target_idx) % date_n;
        snprintf(buf, sizeof(buf), "%s", date_table[idx]);
        setcell(g, tc, tr, buf);
        break;
      }
      default: {  // copy/tile
        int src_idx = target_idx % seed_n;
        int src_c = fill_right ? (seed_c + src_idx) : seed_c;
        int src_r = fill_right ? seed_r : (seed_r + src_idx);
        replicatecell(g, src_c, src_r, tc, tr);
        break;
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
