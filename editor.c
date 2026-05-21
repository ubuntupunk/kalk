#define _POSIX_C_SOURCE 200809L
#include "cell.h"
#include <ctype.h>
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int vcols(void) { return (COLS - GW) / CW > 0 ? (COLS - GW) / CW : 1; }
static int vrows(void) { return (LINES - 4) > 0 ? (LINES - 4) : 1; }

static void fmtcell(struct grid* g, struct cell* cl, char* fb, int cw) {
  if (!cl || cl->type == EMPTY) {
    memset(fb, ' ', cw);
    fb[cw] = '\0';
  } else if (cl->type == LABEL) {
    snprintf(fb, cw + 1, "%-*.*s", cw, cw, cl->text[0] == '"' ? cl->text + 1 : cl->text);
  } else if (isnan(cl->val)) {
    snprintf(fb, cw + 1, "%*s", cw, "ERROR");
  } else {
    char t[MAXIN] = {0}, fmt = cl->fmt;
    if (!fmt || fmt == 'D') fmt = g->fmt;
    if (fmt == '$') {
      snprintf(t, sizeof(t), "%.2f", cl->val);
    } else if (fmt == '%') {
      snprintf(t, sizeof(t), "%.2f%%", cl->val * 100);
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
  else if (cur && cur->type == FORMULA)
    printw("  %s = %s%.10g", cur->text, isnan(cur->val) ? "ERR " : "",
           isnan(cur->val) ? 0.0 : cur->val);
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

//
//  /B                   Blank current cell value (keep formatting)
//  /C                   Clear entire spreadsheet (keep formatting)
//  /F(L/R/I/G/D/$/%/*)  Set cell format: Left/Right/Integer/General/Dollar/Percent
//  /DR, /DC             Delete row/column
//  /IR, /IC             Insert row/column
//  /GC                  Set column width
//  /GF(L/R/I/G/D/$/%/*) Set default column format
//  /M                   Move row/column
//  /R                   Replicate cell
//  /SL                  Load CSV file
//  /SS                  Save CSV file
//  /SQ                  Save and quit
//  /T(V/H/B/N)          Lock rows/columns
//  /Q                   Quit (prompts if unsaved)
//
int command(struct grid* g) {
  draw(g, "CMD", "");
  mvprintw(1, 0, "Command: B C F D I G M R S T Q"), clrtoeol();
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
    mvprintw(1, 0, "Fmt: L R I G D $ %% * | Fg(C) | (B)g | Attr(O) | (N)cond | (X)clear"), clrtoeol();
    ch = toupper(getch());
    struct cell* cl = cell(g, g->cc, g->cr);
    if (strchr("LRIGD$%*", ch)) cl->fmt = ch;
    else if (ch == 'C') {
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

// entry mode: edit cell content, label mode if label=1 or ch is non-formula starter
void entry(struct grid* g, int label, int ch) {
  char buf[MAXIN] = {0};
  int n = 0;
  draw(g, "ENTRY", "");
  if (ch) buf[n++] = ch;
  for (;;) {
    mvprintw(1, 0, "> %s_", buf);
    clrtoeol();
    int ch = getch();
    if (ch == 27) break;
    if (ch == 10 || ch == 13 || ch == KEY_ENTER) {
      setcell(g, g->cc, g->cr, buf);
      if (g->cr < NROW - 1) g->cr++;
      break;
    } else if (ch == 9) {
      setcell(g, g->cc, g->cr, buf);
      if (g->cc < NCOL - 1) g->cc++;
      break;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
      if (n > 0) buf[--n] = '\0';
    } else if (n < MAXIN - 1) {
      buf[n++] = ch;
      buf[n] = '\0';
    }
  }
}

void loop(struct grid* g) {
  for (;;) {
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
