#ifndef CELL_H
#define CELL_H

#define MAXIN 128                     // max cell input length
#define NCOL 256                      // max number of columns
#define NROW 1024                     // max number of rows
#define GW 4                          // row-number gutter
extern int CW;                        // column display width

enum { EMPTY, NUM, LABEL, FORMULA };  // cell types

struct cell {
  int type;
  float val;
  char text[MAXIN];  // raw user input
  int fmt;  // 0=general 'I'=integer 'D'=default '$'=dollar '%'=percent '*'=graph 'L'=left 'R'=right
};

struct grid {
  struct cell cells[NCOL][NROW];
  int cc, cr, vc, vr, tc, tr, fmt, dirty;
  const char* filename;
};

struct parser {
  const char *s, *p;
  struct grid* g;
};

// Cell operations
struct cell* cell(struct grid* g, int c, int r);
void setcell(struct grid* g, int c, int r, const char* input);
void recalc(struct grid* g);

// Row/column operations
void insertrow(struct grid* g, int at);
void insertcol(struct grid* g, int at);
void deleterow(struct grid* g, int at);
void deletecol(struct grid* g, int at);
void swaprow(struct grid* g, int a, int b);
void swapcol(struct grid* g, int a, int b);

// Cell replication
void replicatecell(struct grid* g, int sc, int sr, int dc, int dr);

// Column name utility
char* col(int c);

// Formula parser
float expr(struct parser* p);
float cmp(struct parser* p);
int ref(const char* s, int* col, int* row);
int refabs(const char* s, int* col, int* row, int* absc, int* absr);

// CSV I/O
int csvload(struct grid* g, const char* filename);
int csvsave(struct grid* g, const char* filename);

// Editor main loop
void loop(struct grid* g);

// Entry mode (called from loop)
void entry(struct grid* g, int label, int ch);

// Navigation mode (called from loop)
void nav(struct grid* g);

// Replicate command (called from command)
void replcmd(struct grid* g);

// Move command (called from command)
void movecmd(struct grid* g);

// Command handler (called from loop)
int command(struct grid* g);

#endif
