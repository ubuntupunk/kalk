#define _POSIX_C_SOURCE 200809L
#include "cell.h"
#include <ctype.h>
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int vcols(void) { return (COLS - GW) / CW > 0 ? (COLS - GW) / CW : 1; }
static int vrows(void) { return (LINES - 5) > 0 ? (LINES - 5) : 1; }

static void fmtcell(struct grid* g, struct cell* cl, char* fb, int cw) {
  if (!cl || cl->type == EMPTY) {
    memset(fb, ' ', cw);
    fb[cw] = '\0';
  } else if (cl->type == LABEL) {
    snprintf(fb, cw + 1, "%-*.*s", cw, cw, cl->text[0] == '"' ? cl->text + 1 : cl->text);
  } else if (cl->is_str && cl->strval[0]) {
    snprintf(fb, cw + 1, "%-*.*s", cw, cw, cl->strval);
  } else if (isnan(cl->val)) {
    snprintf(fb, cw + 1, "%*s", cw, "ERROR");
  } else {
    char t[MAXIN] = {0}, fmt = cl->fmt;
    if (!fmt || fmt == 'D') fmt = g->fmt;
    if (fmt == '$') {
      snprintf(t, sizeof(t), "%.2f", cl->val);
    } else if (fmt == '%') {
      snprintf(t, sizeof(t), "%.2f%%", cl->val * 100);
    } else if (fmt == 'T' || fmt == 'U' || fmt == 'u' || fmt == 't') {
      long days = (long)cl->val;
      double frac = (double)cl->val - days;
      time_t t_ = days * 86400LL + (time_t)(frac * 86400.0 + 0.5);
      struct tm* tm_ = gmtime(&t_);
      if (tm_) {
        int has_time = (fabs(frac) > 0.0001);
        const char* fmtstr = "%Y-%m-%d";
        if (fmt == 'U') fmtstr = "%m/%d/%Y";
        else if (fmt == 'u') fmtstr = "%d-%b-%Y";
        else if (fmt == 't') fmtstr = "%H:%M:%S";
        else if (fmt == 'T' && has_time) fmtstr = "%Y-%m-%d %H:%M";
        strftime(t, sizeof(t), fmtstr, tm_);
      } else {
        snprintf(t, sizeof(t), "%g", cl->val);
      }
      fmt = 'L';
    } else if (fmt == '*') {
      for (int i = 0; i < cw && i < cl->val; i++) t[i] = '*';
      fmt = 'L';
    } else if (fmt == 'I' || (cl->val == (long)cl->val && fabs(cl->val) < 1e9)) {
      snprintf(t, sizeof(t), "%ld", (long)cl->val);
    } else {
      snprintf(t, sizeof(t), "%g", cl->val);
    }
    snprintf(fb, cw + 1, fmt == 'L' ? "%-*s" : "%*s", cw, t);
  }
}

static void draw(struct grid* g, const char* mode, const char* buf) {
  erase();

  int lc = g->tc;         // locked title columns
  int lr = g->tr;         // locked title rows
  int fc = vcols() - lc;  // free scrollable column slots
  int fr = vrows() - lr;  // free scrollable row slots
  if (fc < 1) fc = 1;
  if (fr < 1) fr = 1;

  // Status bar
  attron(A_BOLD | A_REVERSE);
  move(0, 0);
  clrtoeol();
  struct cell* cur = cell(g, g->cc, g->cr);
  mvprintw(0, 0, " %s%d", col(g->cc), g->cr + 1);
  if (cur && cur->type == NUM)
    printw("  %.10g", cur->val);
  else if (cur && cur->type == FORMULA) {
    if (cur->is_str)
      printw("  %s = \"%s\"", cur->text, cur->strval);
    else
      printw("  %s = %s%.10g", cur->text, isnan(cur->val) ? "ERR " : "",
             isnan(cur->val) ? 0.0 : cur->val);
  }
  else if (cur && cur->type == LABEL)
    printw("  %s", cur->text);
  mvprintw(0, COLS - 6, "%s", mode);
  attroff(A_BOLD | A_REVERSE);

  // Edit line
  move(1, 0);
  clrtoeol();
  if (mode)
    mvprintw(1, 0, "%s_", buf);
  else if (cur && cur->type != EMPTY)
    mvprintw(1, 0, "  %s", cur->text);

  /* Row 2: column headers */
  attron(A_BOLD | A_REVERSE);
  move(2, 0);
  clrtoeol();
  for (int ci = 0; ci < lc + fc; ci++) {
    int c = (ci < lc) ? ci : g->vc + (ci - lc);
    if (c >= NCOL) break;
    mvprintw(2, GW + ci * CW, "%*s", CW, col(c));
  }
  attroff(A_BOLD | A_REVERSE);

  // Grid rows: locked title rows first, then scrollable rows
  for (int ri = 0; ri < lr + fr; ri++) {
    int row = (ri < lr) ? ri : g->vr + (ri - lr);
    if (row >= NROW) continue;
    int y = 3 + ri;
    int is_locked_row = (ri < lr);

    move(y, 0);
    clrtoeol();
    attron(A_REVERSE);
    mvprintw(y, 0, "%*d ", GW - 1, row + 1);
    attroff(A_REVERSE);

    for (int ci = 0; ci < lc + fc; ci++) {
      int c = (ci < lc) ? ci : g->vc + (ci - lc);
      if (c >= NCOL) break;
      int is_locked_col = (ci < lc);

      struct cell* cl = cell(g, c, row);
      char fb[64];
      fmtcell(g, cl, fb, CW);

      int is_cur = (c == g->cc && row == g->cr);
      int is_locked = (is_locked_row || is_locked_col);

      // Apply cell foreground+background color (skip for cursor cell to keep selection visible)
      int pair_id = 0;
      if (cl && !is_cur) {
        int fg = cl->color, bg = cl->bg;
        if (cl->cond[0] && cl->type != EMPTY && cl->type != LABEL && (fg > 0 || bg > 0)) {
          int cop; float cv;
          if (parse_cond(cl->cond, &cop, &cv)) {
            int match = 0;
            switch (cop) {
              case 1: match = (cl->val > cv); break;
              case 2: match = (cl->val < cv); break;
              case 3: match = (cl->val >= cv); break;
              case 4: match = (cl->val <= cv); break;
              case 5: match = (cl->val != cv); break;
              default: match = (cl->val == cv); break;
            }
            if (match) pair_id = fg * 8 + bg;
          }
        } else if (fg > 0 || bg > 0) {
          pair_id = fg * 8 + bg;
        }
      }

      // Cell text attributes (bold/italic)
      int cell_attr = 0;
      if (cl && cl->attr) {
        if (cl->attr & 1) cell_attr |= A_BOLD;
        if (cl->attr & 2) cell_attr |= A_ITALIC;
      }
      if (cell_attr) attron(cell_attr);
      if (pair_id) attron(COLOR_PAIR(pair_id));

      if (is_cur || is_locked) attron(A_REVERSE);
      if (is_locked && !is_cur) attron(A_BOLD);
      mvprintw(y, GW + ci * CW, "%s", fb);
      if (is_locked && !is_cur) attroff(A_BOLD);
      if (is_cur || is_locked) attroff(A_REVERSE);
      if (pair_id) attroff(COLOR_PAIR(pair_id));
      if (cell_attr) attroff(cell_attr);
    }
  }

  // Tab bar (sheet names)
  move(LINES - 1, 0);
  clrtoeol();
  for (int i = 0; i < n_sheets; i++) {
    int color = sheet_colors[i];
    int pair_id = 0;
    if (i == cur_sheet) attron(A_REVERSE);
    if (color > 0 && color <= 7) {
      pair_id = color * 8 + 0;
      if (i == cur_sheet) pair_id = color * 8 + (color > 4 ? 0 : 7);
      attron(COLOR_PAIR(pair_id));
    }
    mvprintw(LINES - 1, i * 12, " %s ", sheet_names[i]);
    if (pair_id) attroff(COLOR_PAIR(pair_id));
    if (i == cur_sheet) attroff(A_REVERSE);
  }
}

void movecmd(struct grid* g) {
  int origc = g->cc, origr = g->cr;
  char src[16];
  snprintf(src, sizeof(src), "%s%d", col(origc), origr + 1);
  for (;;) {
    draw(g, "MOVE", "");
    if (g->cc == origc && g->cr == origr)
      mvprintw(1, 0, "Source: %s  (move cursor, Esc cancel)", src);
    else
      mvprintw(1, 0, "%s...%s%d  (Enter confirm, Esc cancel)", src, col(g->cc), g->cr + 1);
    clrtoeol();
    refresh();
    int k = getch();
    if (k == 27) {  // cancel: undo all swaps
      if (g->cc != origc) {
        while (g->cc < origc) swapcol(g, g->cc, g->cc + 1), g->cc++;
        while (g->cc > origc) swapcol(g, g->cc, g->cc - 1), g->cc--;
      } else {
        while (g->cr < origr) swaprow(g, g->cr, g->cr + 1), g->cr++;
        while (g->cr > origr) swaprow(g, g->cr, g->cr - 1), g->cr--;
      }
      recalc(g);
      break;
    } else if (k == 10 || k == 13 || k == KEY_ENTER) {
      if (g->cc != origc || g->cr != origr) g->dirty = 1;
      recalc(g);
      break;
    } else if (k == KEY_UP && g->cc == origc) {
      int lo = g->tr > 0 ? g->tr : 0;
      if (g->cr > lo) {
        swaprow(g, g->cr, g->cr - 1);
        g->cr--;
      }
    } else if (k == KEY_DOWN && g->cc == origc) {
      if (g->cr < NROW - 1) {
        swaprow(g, g->cr, g->cr + 1);
        g->cr++;
      }
    } else if (k == KEY_LEFT && g->cr == origr) {
      int lo = g->tc > 0 ? g->tc : 0;
      if (g->cc > lo) {
        swapcol(g, g->cc, g->cc - 1);
        g->cc--;
      }
    } else if (k == KEY_RIGHT && g->cr == origr) {
      if (g->cc < NCOL - 1) {
        swapcol(g, g->cc, g->cc + 1);
        g->cc++;
      }
    }
  }
}

// Helper: format a range string into buf (uses static col() buffer carefully)
static void fmtrange(char* buf, int sz, int c1, int r1, int c2, int r2) {
  if (c1 == c2 && r1 == r2) {
    snprintf(buf, sz, "%s%d", col(c1), r1 + 1);
  } else {
    char a[16];
    snprintf(a, sizeof(a), "%s%d", col(c1), r1 + 1);
    snprintf(buf, sz, "%s...%s%d", a, col(c2), r2 + 1);
  }
}

// Select a range: anchor is fixed, cursor moves to define other corner.
// Both cursor keys and typed input work. Returns 1 on Enter, 0 on Esc.
static int selectrange(struct grid* g, const char* prompt, int ac, int ar, int* c1, int* r1,
                       int* c2, int* r2) {
  char buf[MAXIN] = {0};
  int n = 0, typed = 0;
  g->cc = ac;
  g->cr = ar;
  for (;;) {
    char rng[MAXIN + 4];
    if (typed) {
      snprintf(rng, sizeof(rng), "%s_", buf);
    } else {
      fmtrange(rng, sizeof(rng), ac < g->cc ? ac : g->cc, ar < g->cr ? ar : g->cr,
               ac > g->cc ? ac : g->cc, ar > g->cr ? ar : g->cr);
    }
    draw(g, "REPL", "");
    mvprintw(1, 0, "%s %s", prompt, rng);
    clrtoeol();
    refresh();
    int ch = getch();
    if (ch == 27) return 0;
    if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
      if (typed) {
        const char* p = buf;
        int k = ref(p, c1, r1);
        if (!k) return 0;
        p += k;
        *c2 = *c1, *r2 = *r1;
        if (p[0] == '.' && p[1] == '.' && p[2] == '.') {
          p += 3;
          if (!ref(p, c2, r2)) return 0;
        }
      } else {
        *c1 = ac < g->cc ? ac : g->cc;
        *r1 = ar < g->cr ? ar : g->cr;
        *c2 = ac > g->cc ? ac : g->cc;
        *r2 = ar > g->cr ? ar : g->cr;
      }
      if (*c1 > *c2) {
        int t = *c1;
        *c1 = *c2;
        *c2 = t;
      }
      if (*r1 > *r2) {
        int t = *r1;
        *r1 = *r2;
        *r2 = t;
      }
      return 1;
    } else if (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_LEFT || ch == KEY_RIGHT) {
      typed = 0;
      buf[0] = '\0';
      n = 0;
      if (ch == KEY_UP && g->cr > 0)
        g->cr--;
      else if (ch == KEY_DOWN && g->cr < NROW - 1)
        g->cr++;
      else if (ch == KEY_LEFT && g->cc > 0)
        g->cc--;
      else if (ch == KEY_RIGHT && g->cc < NCOL - 1)
        g->cc++;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      typed = 1;
      if (n > 0) buf[--n] = '\0';
    } else if (n < MAXIN - 1 && ch >= 32 && ch < 127) {
      typed = 1;
      buf[n++] = toupper(ch);
      buf[n] = '\0';
    }
  }
}

void replcmd(struct grid* g) {
  int sc1, sr1, sc2, sr2;
  int origc = g->cc, origr = g->cr;

  // Phase 1: select source range (anchor = current cell)
  if (!selectrange(g, "Source:", origc, origr, &sc1, &sr1, &sc2, &sr2)) return;

  // Phase 2: pick target top-left corner, target size = source size
  int sw = sc2 - sc1 + 1, sh = sr2 - sr1 + 1;
  char srcstr[32];
  fmtrange(srcstr, sizeof(srcstr), sc1, sr1, sc2, sr2);
  g->cc = sc1, g->cr = sr1;  // start target selection from source position
  char buf[MAXIN] = {0};
  int n = 0, typed = 0;
  for (;;) {
    char tgt[MAXIN + 4];
    int tc = typed ? -1 : g->cc, tr = typed ? -1 : g->cr;
    if (typed) {
      snprintf(tgt, sizeof(tgt), "%s_", buf);
    } else {
      fmtrange(tgt, sizeof(tgt), tc, tr, tc + sw - 1, tr + sh - 1);
    }
    draw(g, "REPL", "");
    mvprintw(1, 0, "%s to: %s", srcstr, tgt);
    clrtoeol();
    refresh();
    int ch = getch();
    if (ch == 27) return;
    if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
      int tc1, tr1;
      if (typed) {
        int dummy;
        int k = ref(buf, &tc1, &tr1);
        if (!k) return;
      } else {
        tc1 = g->cc;
        tr1 = g->cr;
      }
      for (int r = 0; r < sh; r++)
        for (int c = 0; c < sw; c++) replicatecell(g, sc1 + c, sr1 + r, tc1 + c, tr1 + r);
      recalc(g);
      g->dirty = 1;
      return;
    } else if (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_LEFT || ch == KEY_RIGHT) {
      typed = 0;
      buf[0] = '\0';
      n = 0;
      if (ch == KEY_UP && g->cr > 0)
        g->cr--;
      else if (ch == KEY_DOWN && g->cr < NROW - 1)
        g->cr++;
      else if (ch == KEY_LEFT && g->cc > 0)
        g->cc--;
      else if (ch == KEY_RIGHT && g->cc < NCOL - 1)
        g->cc++;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      typed = 1;
      if (n > 0) buf[--n] = '\0';
    } else if (n < MAXIN - 1 && ch >= 32 && ch < 127) {
      typed = 1;
      buf[n++] = toupper(ch);
      buf[n] = '\0';
    }
  }
}

void autofillcmd(struct grid* g) {
  mvprintw(1, 0, "Auto-fill: (D)own or (R)ight?    "), clrtoeol();
  int dir = toupper(getch());
  if (dir != 'D' && dir != 'R') return;

  mvprintw(1, 0, "Count: "), clrtoeol();
  char cbuf[8] = {0};
  int cn = 0;
  for (;;) {
    mvprintw(1, 7, "%s_", cbuf);
    int k = getch();
    if (k == 27) return;
    if (k == 10 || k == 13 || k == KEY_ENTER) break;
    else if (k == KEY_BACKSPACE || k == 127 || k == 8) {
      if (cn > 0) cbuf[--cn] = '\0';
    } else if (isdigit(k) && cn < (int)sizeof(cbuf) - 2) {
      cbuf[cn++] = k;
      cbuf[cn] = '\0';
    }
  }
  int count = (cn > 0) ? atoi(cbuf) : 0;
  if (count <= 0) return;

  // Detect seed range from cells above (down) or left (right) of current cursor
  int seed_n = 0;
  int max_seed = 16;
  if (dir == 'D') {
    for (int r = g->cr - 1; r >= 0 && seed_n < max_seed; r--) {
      struct cell* cl = cell(g, g->cc, r);
      if (!cl || cl->type == EMPTY) break;
      seed_n++;
    }
    if (seed_n < 1) return;
    autofill(g, g->cc, g->cr - seed_n, seed_n, count, 0);
  } else {
    for (int c = g->cc - 1; c >= 0 && seed_n < max_seed; c--) {
      struct cell* cl = cell(g, c, g->cr);
      if (!cl || cl->type == EMPTY) break;
      seed_n++;
    }
    if (seed_n < 1) return;
    autofill(g, g->cc - seed_n, g->cr, seed_n, count, 1);
  }
  recalc(g);
  g->dirty = 1;
}

// Date picker: mini calendar popup
void datepicker(struct grid* g) {
  // Get current cell value as date, default to today
  time_t now = time(NULL);
  struct cell* cl = cell(g, g->cc, g->cr);
  int has_date = 0;
  int year, month, day;
  if (cl && cl->type == NUM) {
    time_t t = (time_t)(cl->val * 86400);
    struct tm* tm_ = localtime(&t);
    if (tm_) {
      year = tm_->tm_year;
      month = tm_->tm_mon;
      day = tm_->tm_mday;
      has_date = 1;
    }
  }
  if (!has_date) {
    struct tm* tm_ = localtime(&now);
    year = tm_->tm_year;
    month = tm_->tm_mon;
    day = tm_->tm_mday;
  }

  int ay = year, am = month, ad = day;
  int sel = 0;  // 0=day, 1=month, 2=year
  int exit = 0;
  while (!exit) {
    // Build calendar for current month
    struct tm caltm = {0};
    caltm.tm_year = ay;
    caltm.tm_mon = am;
    caltm.tm_mday = 1;
    mktime(&caltm);
    int first_dow = caltm.tm_wday;  // 0=Sun
    int days_in_month;
    {
      struct tm tmp = caltm;
      tmp.tm_mon++;
      mktime(&tmp);
      days_in_month = (int)((mktime(&tmp) - mktime(&caltm)) / 86400);
    }

    // Draw calendar on lines 2-10 (over grid columns header + first rows)
    char title[32];
    strftime(title, sizeof(title), "%B %Y", &caltm);
    for (int r = 0; r < 9; r++) {
      move(2 + r, 2);
      clrtoeol();
    }
    attron(A_BOLD);
    mvprintw(2, 2, "%s", title);
    attroff(A_BOLD);
    mvprintw(3, 2, "Su Mo Tu We Th Fr Sa");
    int d = 1;
    for (int w = 0; w < 6 && d <= days_in_month; w++) {
      move(4 + w, 2);
      for (int dow = 0; dow < 7; dow++) {
        if ((w == 0 && dow < first_dow) || d > days_in_month) {
          printw("   ");
        } else {
          int is_today = (ay == year && am == month && d == ad);
          int is_cur = (sel == 0 && d == day && ay == year && am == month);
          if (is_cur) attron(A_REVERSE);
          else if (is_today) attron(A_BOLD);
          mvprintw(4 + w, 2 + dow * 3, "%2d", d);
          if (is_cur) attroff(A_REVERSE);
          else if (is_today) attroff(A_BOLD);
          d++;
        }
      }
    }
    mvprintw(10, 2, "Tab: field  Arrow: adjust  Enter: select  Esc: cancel");
    int ch = getch();
    if (ch == 27) { exit = 1; }
    else if (ch == 9) { sel = (sel + 1) % 3; }  // Tab
    else if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
      // Confirm selection
      if (sel == 0) {
        struct tm setm = {0};
        setm.tm_year = ay;
        setm.tm_mon = am;
        setm.tm_mday = day;
        time_t t = mktime(&setm);
        if (t != (time_t)-1) {
          float serial = (float)((double)t / 86400.0);
          char buf[MAXIN];
          snprintf(buf, sizeof(buf), "%g", serial);
          setcell(g, g->cc, g->cr, buf);
          struct cell* cl2 = cell(g, g->cc, g->cr);
          if (cl2) cl2->fmt = 'T';
          g->dirty = 1;
        }
      }
      exit = 1;
    }
    else if (ch == KEY_UP) {
      if (sel == 0) day -= 7;
      else if (sel == 1) am = (am - 1 + 12) % 12;
      else if (sel == 2) ay--;
    }
    else if (ch == KEY_DOWN) {
      if (sel == 0) day += 7;
      else if (sel == 1) am = (am + 1) % 12;
      else if (sel == 2) ay++;
    }
    else if (ch == KEY_LEFT) {
      if (sel == 0) day--;
      else if (sel == 1) am = (am - 1 + 12) % 12;
      else if (sel == 2) ay--;
    }
    else if (ch == KEY_RIGHT) {
      if (sel == 0) day++;
      else if (sel == 1) am = (am + 1) % 12;
      else if (sel == 2) ay++;
    }
    // Clamp day to valid range
    {
      struct tm caltm2 = {0};
      caltm2.tm_year = ay;
      caltm2.tm_mon = am;
      caltm2.tm_mday = 1;
      mktime(&caltm2);
      // Calculate days in month
      struct tm tmp2 = caltm2;
      tmp2.tm_mon++;
      time_t t1 = mktime(&caltm2);
      time_t t2 = mktime(&tmp2);
      int dim = (int)((t2 - t1) / 86400);
      if (day < 1) day = 1;
      if (day > dim) day = dim;
    }
  }
}

//
//  /A                   Auto-fill pattern (linear, date names, or copy)
//  /B                   Blank current cell value (keep formatting)
//  /C                   Clear entire spreadsheet (keep formatting)
//  /F(L/R/I/G/D/$/%/*/T/t/U/u) Set cell format
//  /F P                 Date picker popup calendar
//  /DR, /DC             Delete row/column
//  /IR, /IC             Insert row/column
//  /GC                  Set column width
//  /GF(L/R/I/G/D/$/%/*) Set default column format
//  /E                   Sheet explorer (list all sheets)
//  /M                   Move row/column
//  /R                   Replicate cell
//  /SL                  Load CSV file
//  /SS                  Save CSV file
//  /SQ                  Save and quit
//  /T(V/H/B/N)          Lock rows/columns
//  /N                   Next sheet
//  /P                   Previous sheet
//  /W(E/R/D/I/L/R/C)    Sheet: New/Rename/Delete/Import/MoveL/MoveR/Color
//  /Y                   Yank (copy) cells to clipboard
//  /V                   Paste cells from clipboard
//  /X                   Cut cells to clipboard (yank + clear)
//  /Q                   Quit (prompts if unsaved)
//
int command(struct grid* g) {
  draw(g, "CMD", "");
  mvprintw(1, 0, "Command: A B C D E F G I M N P Q R S T V W X Y"), clrtoeol();
  refresh();
  int ch = toupper(getch());
  if (ch == 'B') {  // blank current cell
    setcell(g, g->cc, g->cr, "");
    recalc(g);
  } else if (ch == 'C') {  // clear sheet
    mvprintw(1, 0, "Clear entire sheet? (y/N)"), clrtoeol();
    ch = getch();
    if (ch == 'y' || ch == 'Y') {
      for (int r = 0; r < NROW; r++)
        for (int c = 0; c < NCOL; c++) g->cells[c][r] = (struct cell){0};
      g->dirty = 1;
    }
  } else if (ch == 'D') {  // delete row/column
    mvprintw(1, 0, "Delete (R)ow or (C)olumn?"), clrtoeol();
    ch = toupper(getch());
    if (ch == 'R')
      deleterow(g, g->cr);
    else if (ch == 'C')
      deletecol(g, g->cc);
    recalc(g);
  } else if (ch == 'I') {  // insert row/column
    mvprintw(1, 0, "Insert (R)ow or (C)olumn?"), clrtoeol();
    ch = toupper(getch());
    if (ch == 'R')
      insertrow(g, g->cr);
    else if (ch == 'C')
      insertcol(g, g->cc);
    recalc(g);
  } else if (ch == 'F') {  // change cell format/color/condition
    mvprintw(1, 0, "Fmt: L R I G D $ %% * T(d) U(u) t(ime) | Fg(C) | (B)g | Attr(O) | (N)cond | (X)clear | (P)icker"), clrtoeol();
    ch = toupper(getch());
    struct cell* cl = cell(g, g->cc, g->cr);
    if (strchr("LRIGD$%*TU", ch) || ch == 't' || ch == 'T') {
      // Distinguish 'T' (date YYYY-MM-DD) vs 't' (time HH:MM:SS)
      if (ch == 't') cl->fmt = 't';
      else if (ch == 'T') cl->fmt = 'T';
      else cl->fmt = ch;
    } else if (ch == 'P' || ch == 'p') {  // Date picker
      datepicker(g);
    } else if (ch == 'C') {
      mvprintw(1, 0, "Fg: 0=blk 1=Red 2=Grn 3=Yel 4=Blu 5=Mag 6=Cyn 7=Wht"), clrtoeol();
      ch = toupper(getch());
      int col = ch - '0';
      if (col >= 0 && col <= 7) cl->color = col;
    } else if (ch == 'B') {
      mvprintw(1, 0, "Bg: 0=blk 1=Red 2=Grn 3=Yel 4=Blu 5=Mag 6=Cyn 7=Wht"), clrtoeol();
      ch = toupper(getch());
      int col = ch - '0';
      if (col >= 0 && col <= 7) cl->bg = col;
      g->dirty = 1;
    } else if (ch == 'O') {
      mvprintw(1, 0, "Attr: 0=none 1=Bold 2=Italic 3=Both"), clrtoeol();
      ch = getch();
      int a = ch - '0';
      if (a >= 0 && a <= 3) cl->attr = a;
      g->dirty = 1;
    } else if (ch == 'X') {
      cl->color = 0;
      cl->bg = 0;
      cl->attr = 0;
      cl->cond[0] = '\0';
      g->dirty = 1;
    } else if (ch == 'N') {
      mvprintw(1, 0, "Cond (>5, <0, =10, <>7), Enter=none: "), clrtoeol();
      char cbuf[64] = {0};
      int cn = 0;
      for (;;) {
        mvprintw(1, 30, "%s_", cbuf);
        int k = getch();
        if (k == 27) break;
        if (k == 10 || k == 13 || k == KEY_ENTER) {
          strncpy(cl->cond, cbuf, sizeof(cl->cond) - 1);
          break;
        } else if (k == KEY_BACKSPACE || k == 127 || k == 8) {
          if (cn > 0) cbuf[--cn] = '\0';
        } else if (cn < (int)sizeof(cbuf) - 2 && k >= 32) {
          cbuf[cn++] = k;
          cbuf[cn] = '\0';
        }
      }
    }
  } else if (ch == 'E') {  // Sheet explorer popup
    mvprintw(1, 0, "Sheets: "), clrtoeol();
    for (int i = 0; i < n_sheets; i++) {
      char marker = (i == cur_sheet) ? '>' : ' ';
      mvprintw(1, 8 + i * 14, "%c%d.%s", marker, i + 1, sheet_names[i]);
    }
    mvprintw(1, 8 + n_sheets * 14, "(pick #, Esc)");
    clrtoeol();
    char sbuf[4] = {0};
    int sn = 0;
    for (;;) {
      mvprintw(0, COLS - 12, "SHEET");
      int k = getch();
      if (k == 27) break;
      if (k == 10 || k == 13 || k == KEY_ENTER) {
        int idx = atoi(sbuf) - 1;
        if (idx >= 0 && idx < n_sheets) {
          cur_sheet = idx;
          recalc(curgrid());
        }
        break;
      } else if (k == KEY_BACKSPACE || k == 127 || k == 8) {
        if (sn > 0) sbuf[--sn] = '\0';
      } else if (isdigit(k) && sn < (int)sizeof(sbuf) - 2) {
        sbuf[sn++] = k;
        sbuf[sn] = '\0';
      }
      // Redraw sheet list with current selection highlighted
      for (int i = 0; i < n_sheets; i++) {
        int pick = (sn > 0 && atoi(sbuf) - 1 == i);
        if (pick) attron(A_REVERSE);
        mvprintw(1, 8 + i * 14, "%s%2d.%s", i == cur_sheet ? ">" : "", i + 1, sheet_names[i]);
        clrtoeol();
        if (pick) attroff(A_REVERSE);
      }
      mvprintw(1, 8 + n_sheets * 14, "(pick #, Esc) %s", sbuf);
      clrtoeol();
      refresh();
    }
  } else if (ch == 'G') {  // change global settings
    mvprintw(1, 0, "Global: (C)ol width or (F)mt?"), clrtoeol();
    ch = toupper(getch());
    if (ch == 'C') {
      mvprintw(1, 0, "New column width (4-20)?"), clrtoeol();
      char buf[16] = {0};
      int n = 0;
      for (;;) {
        mvprintw(1, 30, "%s_", buf);
        int ch = getch();
        if (ch == 27) break;
        if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
          int w = strtol(buf, NULL, 10);
          if (w >= 4 && w <= 20) CW = w;
          break;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
          if (n > 0) buf[--n] = '\0';
        } else if (isdigit(ch) && n < sizeof(buf) - 1) {
          buf[n++] = ch;
          buf[n] = '\0';
        }
      }
    } else if (ch == 'F') {
      mvprintw(1, 0, "Format: L R I G D $ %% *"), clrtoeol();
      ch = toupper(getch());
      if (strchr("LRIGD$%*", ch)) g->fmt = ch;
    }
  } else if (ch == 'A') {
    autofillcmd(g);
  } else if (ch == 'N') {  // next sheet
    if (cur_sheet < n_sheets - 1) cur_sheet++;
    recalc(curgrid());
  } else if (ch == 'P') {  // previous sheet
    if (cur_sheet > 0) cur_sheet--;
    recalc(curgrid());
  } else if (ch == 'W') {  // worksheet management
    mvprintw(1, 0, "Sheet: N(e)w (R)ename (D)elete (I)mport (L)eft (R)i (C)olor?"), clrtoeol();
    ch = toupper(getch());
    if (ch == 'E') {  // New
      char nbuf[32] = {0};
      int nn = 0;
      snprintf(nbuf, sizeof(nbuf), "Sheet%d", n_sheets + 1);
      nn = strlen(nbuf);
      mvprintw(1, 0, "New sheet name: %s_", nbuf), clrtoeol();
      for (;;) {
        mvprintw(1, 16, "%s_", nbuf);
        int k = getch();
        if (k == 27) break;
        if (k == 10 || k == 13 || k == KEY_ENTER) {
          newsheet(nbuf);
          cur_sheet = n_sheets - 1;
          break;
        } else if (k == KEY_BACKSPACE || k == 127 || k == 8) {
          if (nn > 0) nbuf[--nn] = '\0';
        } else if (nn < (int)sizeof(nbuf) - 2 && k >= 32) {
          nbuf[nn++] = k;
          nbuf[nn] = '\0';
        }
      }
    } else if (ch == 'R') {  // Rename
      mvprintw(1, 0, "Rename to: "), clrtoeol();
      char nbuf[32] = {0};
      int nn = 0;
      strncpy(nbuf, sheet_names[cur_sheet], sizeof(nbuf) - 1);
      nn = strlen(nbuf);
      for (;;) {
        mvprintw(1, 11, "%s_", nbuf);
        int k = getch();
        if (k == 27) break;
        if (k == 10 || k == 13 || k == KEY_ENTER) {
          renamesheet(cur_sheet, nbuf);
          break;
        } else if (k == KEY_BACKSPACE || k == 127 || k == 8) {
          if (nn > 0) nbuf[--nn] = '\0';
        } else if (nn < (int)sizeof(nbuf) - 2 && k >= 32) {
          nbuf[nn++] = k;
          nbuf[nn] = '\0';
        }
      }
    } else if (ch == 'D') {  // Delete
      if (n_sheets > 1) {
        delsheet();
        recalc(curgrid());
      }
    } else if (ch == 'I') {  // Import from CSV into new sheet
      mvprintw(1, 0, "Import CSV file: "), clrtoeol();
      char fbuf[256] = {0};
      int fn = 0;
      for (;;) {
        mvprintw(1, 18, "%s_  ", fbuf);
        int k = getch();
        if (k == 27) break;
        if (k == 10 || k == 13 || k == KEY_ENTER) {
          if (fn > 0) {
            char sname[SHEETNAMELEN];
            const char* base = strrchr(fbuf, '/');
            base = base ? base + 1 : fbuf;
            snprintf(sname, SHEETNAMELEN, "%.*s", (int)(strcspn(base, ".")), base);
            // Clean up name: replace non-alphanumeric with underscore
            for (int i = 0; sname[i]; i++) if (!isalnum(sname[i]) && sname[i] != '_') sname[i] = '_';
            if (sname[0] == '\0') snprintf(sname, SHEETNAMELEN, "Sheet%d", n_sheets + 1);
            newsheet(sname);
            cur_sheet = n_sheets - 1;
            struct grid* ng = curgrid();
            if (csvload(ng, fbuf) == 0) {
              ng->filename = strdup(fbuf);
              ng->dirty = 0;
            } else {
              // Loading failed, remove the empty sheet
              delsheet();
              mvprintw(1, 0, "Failed to load: %s. Press any key.", fbuf), clrtoeol();
              getch();
            }
          }
          break;
        } else if (k == KEY_BACKSPACE || k == 127 || k == 8) {
          if (fn > 0) fbuf[--fn] = '\0';
        } else if (fn < (int)sizeof(fbuf) - 1 && k >= 32) {
          fbuf[fn++] = k;
          fbuf[fn] = '\0';
        }
      }
    } else if (ch == 'L' || ch == 'R') {  // Move tab left/right
      int dir = (ch == 'L') ? -1 : 1;
      int new_idx = cur_sheet + dir;
      if (new_idx >= 0 && new_idx < n_sheets) {
        swapsheets(cur_sheet, new_idx);
      }
    } else if (ch == 'C') {  // Sheet tab color
      mvprintw(1, 0, "Tab color: 0=blk 1=Red 2=Grn 3=Yel 4=Blu 5=Mag 6=Cyn 7=Wht"), clrtoeol();
      ch = getch();
      int col = ch - '0';
      if (col >= 0 && col <= 7) setsheetcolor(cur_sheet, col);
    }
  } else if (ch == 'Y') {  // Yank (copy to clipboard)
    int sc1, sr1, sc2, sr2;
    int origc = g->cc, origr = g->cr;
    if (selectrange(g, "Yank:", origc, origr, &sc1, &sr1, &sc2, &sr2)) {
      yank_cells(g, sc1, sr1, sc2, sr2);
    }
  } else if (ch == 'V') {  // Paste from clipboard
    if (clip_w > 0 && clip_h > 0) {
      paste_cells(g, g->cc, g->cr);
    }
  } else if (ch == 'X') {  // Cut (yank + clear)
    int sc1, sr1, sc2, sr2;
    int origc = g->cc, origr = g->cr;
    if (selectrange(g, "Cut:", origc, origr, &sc1, &sr1, &sc2, &sr2)) {
      cut_cells(g, sc1, sr1, sc2, sr2);
    }
  } else if (ch == 'M') {
    movecmd(g);
  } else if (ch == 'R') {
    replcmd(g);
  } else if (ch == 'T') {
    mvprintw(1, 0, "Lock (V)ertical, (H)orizontal, (B)oth, or (N)one?"), clrtoeol();
    ch = toupper(getch());
    if (ch == 'V') {
      g->tc = g->cc + 1, g->tr = 0;
      g->cc++;  // move cursor past locked columns
    } else if (ch == 'H') {
      g->tr = g->cr + 1, g->tc = 0;
      g->cr++;  // move cursor past locked rows
    } else if (ch == 'B') {
      g->tc = g->cc + 1, g->tr = g->cr + 1;
      g->cc++;
      g->cr++;
    } else if (ch == 'N') {
      g->tc = g->tr = 0;
      g->vc = g->vr = 0;
    }
  } else if (ch == 'O') {  // sort rows by column
    mvprintw(1, 0, "Sort by column (e.g. A, B, AA): "), clrtoeol();
    char sbuf[16] = {0};
    int sn = 0;
    for (;;) {
      mvprintw(1, 25, "%s_", sbuf);
      int k = getch();
      if (k == 27) break;
      if (k == 10 || k == 13 || k == KEY_ENTER) {
        // Append row number to make valid cell ref (user types "A", we use "A1")
        char refbuf[20];
        snprintf(refbuf, sizeof(refbuf), "%s1", sbuf);
        int c, r;
        if (ref(refbuf, &c, &r) && c >= 0 && c < NCOL) {
          sortbycol(g, c);
          recalc(g);
          g->dirty = 1;
        }
        break;
      } else if (k == KEY_BACKSPACE || k == 127 || k == 8) {
        if (sn > 0) sbuf[--sn] = '\0';
      } else if (isalpha(k) && sn < (int)sizeof(sbuf) - 2) {
        sbuf[sn++] = toupper(k);
        sbuf[sn] = '\0';
      }
    }
  } else if (ch == 'S') {
    mvprintw(1, 0, "Storage: (L)oad, (S)ave, or save & (Q)uit?"), clrtoeol();
    ch = toupper(getch());
    if (ch == 'L') {
      // prompt for filename
      mvprintw(1, 0, "Load file: "), clrtoeol();
      char fbuf[256] = {0};
      int fn = 0;
      if (g->filename) {
        strncpy(fbuf, g->filename, sizeof(fbuf) - 1);
        fn = strlen(fbuf);
      }
      for (;;) {
        mvprintw(1, 11, "%s_  ", fbuf);
        int ch = getch();
        if (ch == 27) break;
        if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
          if (fn > 0) {
            for (int r = 0; r < NROW; r++)
              for (int c = 0; c < NCOL; c++) g->cells[c][r] = (struct cell){0};
            if (csvload(g, fbuf) == 0) {
              g->filename = strdup(fbuf);
              g->dirty = 0;
            } else {
              mvprintw(1, 0, "Failed to load: %s. Press any key.", fbuf), clrtoeol();
              getch();
            }
          }
          break;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
          if (fn > 0) fbuf[--fn] = '\0';
        } else if (fn < (int)sizeof(fbuf) - 1 && ch >= 32) {
          fbuf[fn++] = ch;
          fbuf[fn] = '\0';
        }
      }
    } else if (ch == 'S' || ch == 'Q') {
      int quit = (ch == 'Q');
      const char* fn = g->filename;
      if (!fn) {
        // prompt for filename
        mvprintw(1, 0, "Save as: "), clrtoeol();
        static char sbuf[256] = {0};
        int sn = 0;
        sbuf[0] = '\0';
        for (;;) {
          mvprintw(1, 9, "%s_  ", sbuf);
          int ch = getch();
          if (ch == 27) {
            fn = NULL;
            break;
          }
          if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
            fn = (sn > 0) ? sbuf : NULL;
            break;
          } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (sn > 0) sbuf[--sn] = '\0';
          } else if (sn < (int)sizeof(sbuf) - 1 && ch >= 32) {
            sbuf[sn++] = ch;
            sbuf[sn] = '\0';
          }
        }
      }
      if (fn) {
        if (csvsave(g, fn) == 0) {
          g->filename = fn;
          g->dirty = 0;
          if (quit) return 1;
        } else {
          mvprintw(1, 0, "Failed to save: %s. Press any key.", fn), clrtoeol();
          getch();
        }
      }
    }
  } else if (ch == 'Q') {
    if (g->dirty) {
      mvprintw(1, 0, "Unsaved changes. Quit anyway? (y/N)"), clrtoeol();
      ch = getch();
      if (ch == 'y' || ch == 'Y') return 1;
    } else {
      return 1;
    }
  }
  return 0;
}

// navigation mode: enter cell reference to jump to (e.g. B12)
void nav(struct grid* g) {
  char buf[MAXIN] = {0}, n = 0;
  draw(g, "GOTO", "");
  for (;;) {
    mvprintw(1, 0, "> %s_", buf);
    clrtoeol();
    int ch = getch();
    if (ch == 27) break;
    if (ch == 10 || ch == 13 || ch == KEY_ENTER || ch == 9) {
      int c, r;
      if (ref(buf, &c, &r)) g->cc = c, g->cr = r;
      break;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (n > 0) buf[--n] = '\0';
    } else if ((isalpha(ch) || isdigit(ch)) && strlen(buf) < MAXIN - 2) {
      int c, r, i = n;
      buf[i++] = toupper(ch);
      if (isalpha(ch)) buf[i++] = '1';
      if (ref(buf, &c, &r) && r < NROW && c < NCOL) n++;
      buf[n] = '\0';
    }
  }
}

// --- Autocomplete helpers ---

// Find the trailing alpha fragment in a buffer for nested autocomplete.
// E.g., "SUM(A1, S" -> "S", "IF(A1>5,A" -> "A"
// Returns fragment length, 0 if none.
static int ac_find_fragment(const char* buf, char* frag, int fragsz) {
  int len = strlen(buf);
  if (len < 1) return 0;
  // Find last continuous alpha sequence at end of buffer
  int i = len;
  while (i > 0 && isalpha((unsigned char)buf[i-1])) i--;
  int flen = len - i;
  if (flen < 1 || flen >= fragsz) return 0;
  for (int j = 0; j < flen; j++) frag[j] = toupper((unsigned char)buf[i+j]);
  frag[flen] = '\0';
  return flen;
}

// Fuzzy match: check if all chars in query appear in order in target.
// E.g., "rou" matches both "ROUND" and "ROUNDUP".
static int fuzzy_match(const char* query, const char* target) {
  while (*query) {
    while (*target && toupper((unsigned char)*target) != (unsigned char)*query) target++;
    if (!*target) return 0;
    query++;
    target++;
  }
  return 1;
}

// Match score: 0=none, 1=prefix, 2=fuzzy
static int ac_match_score(const char* query, const char* func) {
  int qlen = strlen(query);
  // Prefix match
  int match = 1;
  for (int i = 0; i < qlen; i++)
    if (toupper((unsigned char)func[i]) != (unsigned char)query[i]) { match = 0; break; }
  if (match) return 1;
  // Fuzzy match (subsequence)
  return fuzzy_match(query, func) ? 2 : 0;
}

// History rank: 0 = most recent, -1 = not in history
static int ac_history_rank(int idx) {
  for (int i = 0; i < func_history_count; i++)
    if (func_history[i] == idx) return i;
  return -1;
}

// Record a function as used (bring to front of history)
static void ac_record_usage(int idx) {
  int found = -1;
  for (int i = 0; i < func_history_count; i++) {
    if (func_history[i] == idx) { found = i; break; }
  }
  if (found >= 0) {
    for (int i = found; i > 0; i--) func_history[i] = func_history[i-1];
  } else {
    if (func_history_count < FUNC_HISTORY_MAX) func_history_count++;
    for (int i = func_history_count - 1; i > 0; i--) func_history[i] = func_history[i-1];
  }
  func_history[0] = idx;
}

// Clear popup area (lines 2-10)
static void ac_clear_popup(void) {
  for (int r = 2; r <= 10; r++) {
    move(r, 2);
    clrtoeol();
  }
}

// Compute autocomplete matches with nested context + fuzzy matching + history ranking
static int ac_matches(const char* buf, int* indices, int max) {
  char frag[32];
  if (!ac_find_fragment(buf, frag, sizeof(frag))) return 0;
  
  int qlen = strlen(frag);
  if (qlen < 1) return 0;
  
  // Score all functions, collect matches
  int scores[NFUNCS];
  int idxbuf[NFUNCS];
  int n = 0;
  for (int i = 0; func_names[i]; i++) {
    int sc = ac_match_score(frag, func_names[i]);
    if (sc > 0) {
      // Apply history boost: recently used functions get higher priority
      int hrank = ac_history_rank(i);
      if (hrank >= 0 && sc == 2) sc = 1;  // fuzzy match + in history -> treat as prefix
      scores[n] = sc;
      idxbuf[n] = i;
      n++;
    }
  }
  
  // Sort: prefix matches first, then fuzzy; within each group, history rank (most recent first)
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      int swap = 0;
      if (scores[j] < scores[i]) swap = 1;
      else if (scores[j] == scores[i]) {
        int hi = ac_history_rank(idxbuf[i]);
        int hj = ac_history_rank(idxbuf[j]);
        if (hi < 0) hi = 9999;
        if (hj < 0) hj = 9999;
        if (hj < hi) swap = 1;
      }
      if (swap) {
        int ts = scores[i]; scores[i] = scores[j]; scores[j] = ts;
        int ti = idxbuf[i]; idxbuf[i] = idxbuf[j]; idxbuf[j] = ti;
      }
    }
  }
  
  // Copy to output (limit to max)
  int out_n = n < max ? n : max;
  for (int i = 0; i < out_n; i++) indices[i] = idxbuf[i];
  return out_n;
}

// entry mode: edit cell content, label mode if label=1 or ch is non-formula starter
void entry(struct grid* g, int label, int ch) {
  char buf[MAXIN] = {0};
  int n = 0, ac_sel = 0, ac_n = 0, ac_indices[30];
  int ac_active = 0;  // 1 if popup is being shown
  draw(g, "ENTRY", "");
  if (ch && !label) {
    buf[n++] = ch;
    buf[n] = '\0';
    ac_n = ac_matches(buf, ac_indices, 30);
    ac_sel = 0;
    ac_active = (ac_n > 0);
  }
  for (;;) {
    mvprintw(1, 0, "> %s_", buf);
    clrtoeol();
    // Draw autocomplete popup
    if (ac_active && ac_n > 0) {
      int max_vis = 7;  // max visible items at once
      int start = ac_sel - max_vis / 2;
      if (start < 0) start = 0;
      if (start + max_vis > ac_n) start = ac_n - max_vis;
      if (start < 0) start = 0;

      // Line 2: Argument hint (sig of currently selected function)
      move(2, 2);
      clrtoeol();
      int sel_idx = ac_indices[ac_sel];
      attron(A_BOLD);
      mvprintw(2, 2, "%s", func_sigs[sel_idx]);
      attroff(A_BOLD);

      // Lines 3-9: Function names, one per line
      int disp = 0;
      for (int i = start; i < ac_n && disp < max_vis; i++, disp++) {
        int row = 3 + disp;
        move(row, 2);
        clrtoeol();
        int idx = ac_indices[i];
        int is_cur = (i == ac_sel);
        if (is_cur) attron(A_REVERSE);
        // Prefix hint: show first letters of match in bold
        mvprintw(row, 2, "%-20s", func_names[idx]);
        if (is_cur) attroff(A_REVERSE);
      }

      // Line 10: Description of selected function
      move(10, 2);
      clrtoeol();
      mvprintw(10, 2, "%s", func_descs[sel_idx]);

      // Scroll indicator if there are more items
      if (start > 0) mvprintw(3, 25, "^");
      if (start + max_vis < ac_n) mvprintw(3 + max_vis - 1, 25, "v");
    }

    int ch = getch();
    if (ch == 27) {
      if (ac_active) { ac_active = 0; ac_clear_popup(); continue; }
      break;
    }
    if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
      if (ac_active && ac_n > 0) {
        // Accept selected autocomplete: replace trailing alpha with func_name + "("
        const char* fn = func_names[ac_indices[ac_sel]];
        int flen = strlen(fn);
        // Find the trailing alpha fragment and replace it
        int i = n;
        while (i > 0 && isalpha((unsigned char)buf[i-1])) i--;
        int fraglen = n - i;
        if (fraglen > 0 && flen + 1 < MAXIN - (int)sizeof(buf) + i) {
          memcpy(buf + i, fn, flen);
          buf[i + flen] = '(';
          n = i + flen + 1;
          buf[n] = '\0';
        }
        // Record this function in history
        ac_record_usage(ac_indices[ac_sel]);
        ac_active = 0;
        ac_clear_popup();
        // After accepting, update autocomplete for nested context
        ac_n = ac_matches(buf, ac_indices, 30);
        ac_sel = 0;
        ac_active = (ac_n > 0);
        continue;
      }
      ac_clear_popup();
      setcell(g, g->cc, g->cr, buf);
      if (g->cr < NROW - 1) g->cr++;
      break;
    } else if (ch == 9) {  // Tab: cycle forward through matches
      if (ac_active && ac_n > 0) {
        ac_sel = (ac_sel + 1) % ac_n;
        continue;
      }
      ac_clear_popup();
      setcell(g, g->cc, g->cr, buf);
      if (g->cc < NCOL - 1) g->cc++;
      break;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (n > 0) buf[--n] = '\0';
      if (!label) {
        ac_n = ac_matches(buf, ac_indices, 30);
        ac_sel = 0;
        ac_active = (ac_n > 0);
        if (!ac_active) ac_clear_popup();
      }
    } else if (ch == KEY_UP) {
      if (ac_active && ac_n > 0) {
        ac_sel = (ac_sel > 0) ? ac_sel - 1 : ac_n - 1;
      }
    } else if (ch == KEY_DOWN) {
      if (ac_active && ac_n > 0) {
        ac_sel = (ac_sel + 1) % ac_n;
      }
    } else if (n < MAXIN - 1) {
      buf[n++] = ch;
      buf[n] = '\0';
      if (!label) {
        ac_n = ac_matches(buf, ac_indices, 30);
        ac_sel = 0;
        ac_active = (ac_n > 0);
        if (!ac_active) ac_clear_popup();
      }
    }
  }
  ac_clear_popup();
}

void loop(void) {
  for (;;) {
    struct grid* g = curgrid();
    int lc = g->tc;
    int lr = g->tr;
    int fc = vcols() - lc;
    if (fc < 1) fc = 1;
    int fr = vrows() - lr;
    if (fr < 1) fr = 1;

    // Clamp cursor out of locked title area
    if (lc > 0 && g->cc < lc) g->cc = lc;
    if (lr > 0 && g->cr < lr) g->cr = lr;

    // Viewport must not overlap with locked area
    if (lc > 0 && g->vc < lc) g->vc = lc;
    if (lr > 0 && g->vr < lr) g->vr = lr;

    // Keep cursor visible in scrollable region
    if (g->cc >= lc) {
      if (g->cc < g->vc) g->vc = g->cc;
      if (g->cc >= g->vc + fc) g->vc = g->cc - fc + 1;
    }
    if (g->cr >= lr) {
      if (g->cr < g->vr) g->vr = g->cr;
      if (g->cr >= g->vr + fr) g->vr = g->cr - fr + 1;
    }
    draw(g, "READY", "");
    int ch = getch();

    if (ch == (0x1f & 'c'))  // Ctrl+C: quit
      break;
    else if (ch == KEY_UP && g->cr > lr)
      g->cr--;
    else if (ch == KEY_DOWN && g->cr < NROW - 1)
      g->cr++;
    else if (ch == KEY_LEFT && g->cc > lc)
      g->cc--;
    else if (ch == KEY_RIGHT && g->cc < NCOL - 1)
      g->cc++;
    else if (ch == KEY_HOME) {
      g->cc = lc;
      g->cr = lr;
    } else if (ch == 9 && g->cc < NCOL - 1)  // Tab: next cell
      g->cc++;
    else if (ch == 10 || ch == 13 || ch == KEY_ENTER) {  // Enter: next row
      if (g->cr < NROW - 1) g->cr++;
    } else if (ch == 127 || ch == 8 || ch == KEY_BACKSPACE) {  // Bsp/Del: clear cell
      struct cell* cl = cell(g, g->cc, g->cr);
      if (cl) *cl = (struct cell){0};
      recalc(g);
    } else if (ch == '!') {  // Recalculate
      recalc(g);
    } else if (ch == '/') {
      if (command(g)) break;
    } else if (ch == '>') {
      nav(g);
    } else if (ch == '"') {
      entry(g, 1, 0);
    } else if (ch == '+' || ch == '-' || ch == '(' || ch == '@' || ch == '=' || ch == '.' || isdigit(ch)) {
      entry(g, 0, ch);
    } else if (ch >= 32 && ch < 127) {
      entry(g, 1, ch);
    }
  }
}
