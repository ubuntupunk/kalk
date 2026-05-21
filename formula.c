#define _POSIX_C_SOURCE 200809L
#include "cell.h"
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void skipws(struct parser* p) { for (; isspace(*p->p); p->p++); }

static float number(struct parser* p) {
  char* end;
  float v = strtof(p->p, &end);
  if (end == p->p) return NAN;
  p->p = end;
  return v;
}

int refabs(const char* s, int* col, int* row, int* absc, int* absr) {
  const char* p = s;
  *absc = *absr = 0;
  if (*p == '$') {
    *absc = 1;
    p++;
  }
  if (!isalpha(*p)) return 0;
  *col = toupper(*p++) - 'A' + 1;
  if (isalpha(*p)) *col = *col * 26 + (toupper(*p++) - 'A' + 1);
  if (*p == '$') {
    *absr = 1;
    p++;
  }
  char* end;
  int n = strtol(p, &end, 10);
  if (n <= 0 || end == p) return 0;
  *row = n - 1, *col = *col - 1;
  return (int)(end - s);
}

int ref(const char* s, int* col, int* row) {
  int ac, ar;
  return refabs(s, col, row, &ac, &ar);
}

static float cellval(struct parser* p) {
  int c, r, ac, ar, n = refabs(p->p, &c, &r, &ac, &ar);
  if (!n) return NAN;
  p->p += n;
  struct cell* cl = cell(p->g, c, r);
  if (!cl) return NAN;
  return cl->val;
}

// Parse condition string like ">5", ">=10", "<0", "<=100", "=42", "<>7", "5"
// Returns op: 0=EQ, 1=GT, 2=LT, 3=GE, 4=LE, 5=NE and the numeric value.
static int parse_cond(const char* s, int* op, float* val) {
  while (isspace(*s)) s++;
  *op = 0; // default: equals
  if (*s == '>') { *op = 1; s++; if (*s == '=') { *op = 3; s++; } }
  else if (*s == '<') { *op = 2; s++; if (*s == '>') { *op = 5; s++; } else if (*s == '=') { *op = 4; s++; } }
  else if (*s == '=') { *op = 0; s++; }
  else if (*s == '!' && *(s+1) == '=') { *op = 5; s += 2; }
  while (isspace(*s)) s++;
  char* end;
  *val = strtof(s, &end);
  return end > s; // success if we consumed at least one digit
}

// Comparison operators: < > <= >= = <> !=
// Lower precedence than + and -, so A1+5>B2 parses as (A1+5)>B2
float cmp(struct parser* p) {
  float l = expr(p);
  for (;;) {
    skipws(p);
    if (*p->p == '<') {
      p->p++;
      if (*p->p == '=') { p->p++; float r = expr(p); l = (l <= r) ? 1.0f : 0.0f; }
      else if (*p->p == '>') { p->p++; float r = expr(p); l = (l != r) ? 1.0f : 0.0f; }
      else { float r = expr(p); l = (l < r) ? 1.0f : 0.0f; }
    } else if (*p->p == '>') {
      p->p++;
      if (*p->p == '=') { p->p++; float r = expr(p); l = (l >= r) ? 1.0f : 0.0f; }
      else { float r = expr(p); l = (l > r) ? 1.0f : 0.0f; }
    } else if (*p->p == '=') {
      p->p++;
      float r = expr(p);
      l = (l == r) ? 1.0f : 0.0f;
    } else if (*p->p == '!' && *(p->p + 1) == '=') {
      p->p += 2;
      float r = expr(p);
      l = (l != r) ? 1.0f : 0.0f;
    } else {
      break;
    }
  }
  return l;
}

// Helper: read raw condition text between current position and next ',' or ')'
// into a buffer, advance p->p past it.
static int read_cond_text(struct parser* p, char* buf, int bufsz) {
  const char* start = p->p;
  while (*p->p && *p->p != ',' && *p->p != ')') p->p++;
  int len = p->p - start;
  if (len >= bufsz) len = bufsz - 1;
  memcpy(buf, start, len);
  buf[len] = '\0';
  return len > 0;
}

// Helper: parse a range from current position, advance p->p past it.
static int parse_range(struct parser* p, int* c1, int* r1, int* c2, int* r2) {
  const char* q = p->p;
  int n = ref(q, c1, r1);
  if (!n) return 0;
  q += n;
  if ((*q == '.' && *(q+1) == '.' && *(q+2) == '.') || *q == ':') {
    q += (*q == ':') ? 1 : 3;
    n = ref(q, c2, r2);
    if (n) { p->p = q + n; return 1; }
  }
  return 0;
}

float func(struct parser* p) {
  char fn[16] = {0};
  for (int i = 0; isalpha(*p->p) && i < sizeof(fn) - 1;) fn[i++] = toupper(*p->p++);

  if (*p->p != '(') return NAN;
  p->p++;
  skipws(p);

  // Try to parse range: A1...B5
  int c1, r1, c2, r2, is_range = 0;
  const char* q = p->p;
  int n = ref(q, &c1, &r1);
  if (n) {
    q += n;
    if ((*q == '.' && *(q + 1) == '.' && *(q + 2) == '.') || *q == ':') {
      q += (*q == ':') ? 1 : 3;
      n = ref(q, &c2, &r2);
      if (n) {
        p->p = q + n;
        is_range = 1;
      }
    }
  }

  float result = 0;
  // --- Range functions: SUM, AVERAGE, MIN, MAX, COUNT ---
  if (is_range && (strcmp(fn, "SUM") == 0 || strcmp(fn, "AVERAGE") == 0 ||
                   strcmp(fn, "MIN") == 0 || strcmp(fn, "MAX") == 0 ||
                   strcmp(fn, "COUNT") == 0)) {
    if (c1 > c2) { int t = c1; c1 = c2; c2 = t; }
    if (r1 > r2) { int t = r1; r1 = r2; r2 = t; }
    int count = 0;
    float sum = 0, min_val = INFINITY, max_val = -INFINITY;
    for (int c = c1; c <= c2; c++)
      for (int r = r1; r <= r2; r++) {
        struct cell* cl = cell(p->g, c, r);
        if (cl && cl->type != EMPTY && cl->type != LABEL) {
          float v = cl->val;
          sum += v;
          if (v < min_val) min_val = v;
          if (v > max_val) max_val = v;
          count++;
        }
      }
    if (strcmp(fn, "SUM") == 0)
      result = sum;
    else if (strcmp(fn, "AVERAGE") == 0)
      result = count > 0 ? sum / count : NAN;
    else if (strcmp(fn, "MIN") == 0)
      result = count > 0 ? min_val : NAN;
    else if (strcmp(fn, "MAX") == 0)
      result = count > 0 ? max_val : NAN;
    else if (strcmp(fn, "COUNT") == 0)
      result = (float)count;

  // --- Range functions: SUMIF, COUNTIF ---
  } else if (is_range && (strcmp(fn, "SUMIF") == 0 || strcmp(fn, "COUNTIF") == 0)) {
    // Advance past comma, then read condition as raw text until next ',' or ')'
    char cond_buf[64];
    if (*p->p == ',') p->p++;
    skipws(p);
    if (!read_cond_text(p, cond_buf, sizeof(cond_buf))) return NAN;

    int cond_op;
    float cond_val;
    int cond_ok = parse_cond(cond_buf, &cond_op, &cond_val);

    // Optional sum_range for SUMIF
    int has_sr = 0;
    int sc1, sr1, sc2, sr2;
    skipws(p);
    if (*p->p == ',' && strcmp(fn, "SUMIF") == 0) {
      p->p++;
      skipws(p);
      has_sr = parse_range(p, &sc1, &sr1, &sc2, &sr2);
    }

    if (!cond_ok) return NAN;

    if (c1 > c2) { int t = c1; c1 = c2; c2 = t; }
    if (r1 > r2) { int t = r1; r1 = r2; r2 = t; }
    if (has_sr && sc1 > sc2) { int t = sc1; sc1 = sc2; sc2 = t; }
    if (has_sr && sr1 > sr2) { int t = sr1; sr1 = sr2; sr2 = t; }

    float sum = 0;
    int count = 0;
    for (int c = c1; c <= c2; c++)
      for (int r = r1; r <= r2; r++) {
        struct cell* cl = cell(p->g, c, r);
        if (!cl || cl->type == EMPTY || cl->type == LABEL) continue;
        float cv = cl->val;
        int match = 0;
        switch (cond_op) {
          case 1: match = (cv > cond_val); break;
          case 2: match = (cv < cond_val); break;
          case 3: match = (cv >= cond_val); break;
          case 4: match = (cv <= cond_val); break;
          case 5: match = (cv != cond_val); break;
          default: match = (cv == cond_val); break; // 0 = EQ
        }
        if (match) {
          if (has_sr) {
            int dr = r - r1, dc = c - c1;
            int tr = sr1 + dr, tc = sc1 + dc;
            if (tr >= sr1 && tr <= sr2 && tc >= sc1 && tc <= sc2) {
              struct cell* scl = cell(p->g, tc, tr);
              if (scl && scl->type != EMPTY && scl->type != LABEL)
                sum += scl->val;
            }
            count++;
          } else {
            sum += cv;
            count++;
          }
        }
      }

    result = strcmp(fn, "SUMIF") == 0 ? sum : (float)count;

  // --- Non-range functions ---
  } else if (!is_range) {
    // VLOOKUP(search_key, range, col_index, [is_sorted])
    if (strcmp(fn, "VLOOKUP") == 0) {
      float key = cmp(p);
      skipws(p);
      if (*p->p != ',') return NAN;
      p->p++;
      skipws(p);
      if (!parse_range(p, &c1, &r1, &c2, &r2)) return NAN;
      skipws(p);
      if (*p->p != ',') return NAN;
      p->p++;
      skipws(p);
      float col_idx_f = cmp(p);
      int col_idx = (int)col_idx_f - 1; // 1-based → 0-based
      // Optional is_sorted (ignored, always exact match)
      skipws(p);
      if (*p->p == ',') { p->p++; skipws(p); cmp(p); }
      if (col_idx < 0) return NAN;
      if (c1 > c2) { int t = c1; c1 = c2; c2 = t; }
      if (r1 > r2) { int t = r1; r1 = r2; r2 = t; }
      int target_c = c1 + col_idx;
      if (target_c > c2) return NAN;
      result = NAN;
      for (int r = r1; r <= r2; r++) {
        struct cell* cl = cell(p->g, c1, r);
        if (cl && cl->type != EMPTY && cl->type != LABEL && cl->val == key) {
          struct cell* tcl = cell(p->g, target_c, r);
          if (tcl && tcl->type != EMPTY && tcl->type != LABEL) result = tcl->val;
          break;
        }
      }

    // HLOOKUP(key, range, row_index, [is_sorted])
    } else if (strcmp(fn, "HLOOKUP") == 0) {
      float key = cmp(p);
      skipws(p);
      if (*p->p != ',') return NAN;
      p->p++;
      skipws(p);
      if (!parse_range(p, &c1, &r1, &c2, &r2)) return NAN;
      skipws(p);
      if (*p->p != ',') return NAN;
      p->p++;
      skipws(p);
      float row_idx_f = cmp(p);
      int row_idx = (int)row_idx_f - 1; // 1-based → 0-based
      skipws(p);
      if (*p->p == ',') { p->p++; skipws(p); cmp(p); }
      if (row_idx < 0) return NAN;
      if (c1 > c2) { int t = c1; c1 = c2; c2 = t; }
      if (r1 > r2) { int t = r1; r1 = r2; r2 = t; }
      int target_r = r1 + row_idx;
      if (target_r > r2) return NAN;
      result = NAN;
      for (int c = c1; c <= c2; c++) {
        struct cell* cl = cell(p->g, c, r1);
        if (cl && cl->type != EMPTY && cl->type != LABEL && cl->val == key) {
          struct cell* tcl = cell(p->g, c, target_r);
          if (tcl && tcl->type != EMPTY && tcl->type != LABEL) result = tcl->val;
          break;
        }
      }

    // Zero-argument functions: PI(), RAND()
    } else if (strcmp(fn, "PI") == 0) {
      result = 3.141592653589793f;
    } else if (strcmp(fn, "RAND") == 0) {
      result = (float)rand() / (float)RAND_MAX;
    } else if (strcmp(fn, "IF") == 0) {
      float cond = cmp(p);
      skipws(p);
      if (*p->p != ',') return NAN;
      p->p++;
      skipws(p);
      float tval = cmp(p);
      skipws(p);
      if (*p->p != ',') return NAN;
      p->p++;
      skipws(p);
      float fval = cmp(p);
      result = (cond != 0.0f && !isnan(cond)) ? tval : fval;
    } else {
      float arg = cmp(p);

      // Multi-argument aggregating functions: SUM(A1, B1, C1), etc.
      if (strcmp(fn, "SUM") == 0 || strcmp(fn, "AVERAGE") == 0 ||
          strcmp(fn, "MIN") == 0 || strcmp(fn, "MAX") == 0 ||
          strcmp(fn, "COUNT") == 0) {
        float total = 0;
        float minv = 0;
        float maxv = 0;
        int has_valid = 0;
        int cnt = 0;

        if (!isnan(arg)) {
          total = minv = maxv = arg;
          has_valid = 1;
          cnt = 1;
        }

        for (;;) {
          skipws(p);
          if (*p->p != ',') break;
          p->p++;
          skipws(p);
          float next = cmp(p);
          if (!isnan(next)) {
            cnt++;
            if (has_valid) {
              total += next;
              if (next < minv) minv = next;
              if (next > maxv) maxv = next;
            } else {
              total = minv = maxv = next;
              has_valid = 1;
            }
          }
        }

        if (strcmp(fn, "SUM") == 0)
          result = has_valid ? total : NAN;
        else if (strcmp(fn, "AVERAGE") == 0)
          result = cnt > 0 ? total / cnt : NAN;
        else if (strcmp(fn, "MIN") == 0)
          result = has_valid ? minv : NAN;
        else if (strcmp(fn, "MAX") == 0)
          result = has_valid ? maxv : NAN;
        else if (strcmp(fn, "COUNT") == 0)
          result = (float)cnt;

      } else {
        // Optional second argument for two-arg functions
        float arg2 = 0;
        int has_arg2 = 0;
        if (strcmp(fn, "ROUND") == 0 || strcmp(fn, "ROUNDUP") == 0 ||
            strcmp(fn, "ROUNDDOWN") == 0 || strcmp(fn, "POWER") == 0 ||
            strcmp(fn, "MOD") == 0 || strcmp(fn, "RANDBETWEEN") == 0) {
          skipws(p);
          if (*p->p == ',') {
            p->p++;
            skipws(p);
            arg2 = cmp(p);
            has_arg2 = 1;
          }
        }
        if (strcmp(fn, "ABS") == 0)
          result = fabs(arg);
        else if (strcmp(fn, "INT") == 0)
          result = (int)arg;
        else if (strcmp(fn, "SQRT") == 0)
          result = arg >= 0 ? sqrt(arg) : NAN;
        else if (strcmp(fn, "ROUND") == 0) {
          int places = has_arg2 ? (int)arg2 : 0;
          double f = pow(10.0, places);
          result = round(arg * f) / f;
        } else if (strcmp(fn, "ROUNDUP") == 0) {
          int places = has_arg2 ? (int)arg2 : 0;
          double f = pow(10.0, places);
          result = ceil(arg * f) / f;
        } else if (strcmp(fn, "ROUNDDOWN") == 0) {
          int places = has_arg2 ? (int)arg2 : 0;
          double f = pow(10.0, places);
          result = floor(arg * f) / f;
        } else if (strcmp(fn, "CEILING") == 0)
          result = ceil(arg);
        else if (strcmp(fn, "FLOOR") == 0)
          result = floor(arg);
        else if (strcmp(fn, "POWER") == 0)
          result = pow(arg, arg2);
        else if (strcmp(fn, "MOD") == 0)
          result = arg2 != 0 ? fmod(arg, arg2) : NAN;
        else if (strcmp(fn, "RANDBETWEEN") == 0)
          result = arg + (float)rand() / (float)RAND_MAX * (arg2 - arg);
        else if (strcmp(fn, "SIN") == 0)
          result = sin(arg);
        else if (strcmp(fn, "COS") == 0)
          result = cos(arg);
        else if (strcmp(fn, "TAN") == 0)
          result = cos(arg) != 0 ? tan(arg) : NAN;
        else if (strcmp(fn, "LOG") == 0)
          result = arg > 0 ? log(arg) : NAN;
        else
          return NAN;
      }
    }
  } else {
    return NAN;
  }

  skipws(p);
  if (*p->p != ')') return NAN;
  p->p++;
  return result;
}

float primary(struct parser* p) {
  skipws(p);
  if (!*p->p) return NAN;
  if (*p->p == '+') p->p++;
  if (*p->p == '-') {
    p->p++;
    return -primary(p);
  }
  if (*p->p == '@') {
    p->p++;
    return func(p);
  }
  if (*p->p == '=') {
    p->p++;
    return cmp(p);
  }
  if (*p->p == '(') {
    p->p++;
    float v = cmp(p);
    skipws(p);
    if (*p->p != ')') return NAN;
    p->p++;
    return v;
  }
  if (isdigit(*p->p) || *p->p == '.') return number(p);

  // Google Sheets-style function call (no @ prefix): SUM(...)
  if (isalpha(*p->p)) {
    const char* saved = p->p;
    float v = func(p);
    if (!isnan(v)) return v;
    p->p = saved;
  }

  return cellval(p);
}

float term(struct parser* p) {
  float l = primary(p);
  for (;;) {
    skipws(p);
    char op = *p->p;
    if (op != '*' && op != '/') break;
    p->p++;
    float r = primary(p);
    if (op == '*')
      l *= r;
    else if (r == 0)
      return NAN;
    else
      l /= r;
  }
  return l;
}

float expr(struct parser* p) {
  float l = term(p);
  for (;;) {
    skipws(p);
    char op = *p->p;
    if (op != '+' && op != '-') break;
    p->p++;
    float r = term(p);
    l = (op == '+') ? l + r : l - r;
  }
  return l;
}
