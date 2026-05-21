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
  } else if (!is_range) {
    // IF(condition, true_val, false_val) — three comma-separated arguments
    if (strcmp(fn, "IF") == 0) {
      float cond = expr(p);
      skipws(p);
      if (*p->p != ',') return NAN;
      p->p++;
      skipws(p);
      float tval = expr(p);
      skipws(p);
      if (*p->p != ',') return NAN;
      p->p++;
      skipws(p);
      float fval = expr(p);
      result = (cond != 0.0f && !isnan(cond)) ? tval : fval;
    } else {
      float arg = expr(p);
      // Optional second argument for ROUND/ROUNDUP/ROUNDDOWN
      float arg2 = 0;
      int has_arg2 = 0;
      if (strcmp(fn, "ROUND") == 0 || strcmp(fn, "ROUNDUP") == 0 ||
          strcmp(fn, "ROUNDDOWN") == 0) {
        skipws(p);
        if (*p->p == ',') {
          p->p++;
          skipws(p);
          arg2 = expr(p);
          has_arg2 = 1;
        }
      }
      if (strcmp(fn, "SUM") == 0)
        result = arg;
      else if (strcmp(fn, "AVERAGE") == 0)
        result = arg;
      else if (strcmp(fn, "MIN") == 0)
        result = arg;
      else if (strcmp(fn, "MAX") == 0)
        result = arg;
      else if (strcmp(fn, "COUNT") == 0)
        result = !isnan(arg) ? 1.0f : 0.0f;
      else if (strcmp(fn, "ABS") == 0)
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
      else
        return NAN;
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
    return expr(p);
  }
  if (*p->p == '(') {
    p->p++;
    float v = expr(p);
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
