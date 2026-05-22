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
  int color;  // 0=default, 1-7=foreground color
  int bg;      // 0=default, 1-7=background color
  int attr;    // 0=normal 1=A_BOLD 2=A_ITALIC 3=A_BOLD|A_ITALIC
  char cond[64];  // conditional format rule e.g. ">5", "=0" (empty = none)
  char strval[MAXIN];  // evaluated string value (for LABELs and string-returning formulas)
  int is_str;          // 1 if cell value is a string result (from string-returning formula)
};

struct grid {
  struct cell cells[NCOL][NROW];
  int cc, cr, vc, vr, tc, tr, fmt, dirty;
  const char* filename;
};

struct parser {
  const char *s, *p;
  struct grid* g;
  char arg_str[MAXIN];  // current string argument/result (side channel)
  int has_str_result;   // 1 if current expression produced a string result
};

// Function name list for autocomplete (null-terminated)
#define NFUNCS 40
extern const char* const func_names[];

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

// Auto-fill: detect pattern from seed cells and extend
// fill_right=0 fill down, fill_right=1 fill right
void autofill(struct grid* g, int seed_c, int seed_r, int seed_n, int count, int fill_right);

// Sorting
void sortbycol(struct grid* g, int col);

// Column name utility
char* col(int c);

// String value from a cell (text for LABEL, strval for formulas, formatted for NUM)
const char* cell_str(struct grid* g, int c, int r);

// Formula parser
float expr(struct parser* p);
float cmp(struct parser* p);
int ref(const char* s, int* col, int* row);
int refabs(const char* s, int* col, int* row, int* absc, int* absr);
int parse_cond(const char* s, int* op, float* val);

// CSV I/O
int csvload(struct grid* g, const char* filename);
int csvsave(struct grid* g, const char* filename);

// Editor main loop
void loop(void);

// Entry mode (called from loop)
void entry(struct grid* g, int label, int ch);

// Navigation mode (called from loop)
void nav(struct grid* g);

// Replicate command (called from command)
void replcmd(struct grid* g);

// Move command (called from command)
void movecmd(struct grid* g);

// Auto-fill command (called from command)
void autofillcmd(struct grid* g);

// Command handler (called from loop)
int command(struct grid* g);

// Multi-sheet support
#define MAXSHEETS 26
#define SHEETNAMELEN 32
#define CLIPBOARD_MAX 4096  // max cells in clipboard

extern struct grid* sheets[MAXSHEETS];
extern int cur_sheet;
extern int n_sheets;
extern char sheet_names[MAXSHEETS][SHEETNAMELEN];
extern int sheet_colors[MAXSHEETS];  // 0=default, 1-7=tab color

struct grid* curgrid(void);
void newsheet(const char* name);
void delsheet(void);
void renamesheet(int idx, const char* name);
void init_sheets(void);
void swapsheets(int a, int b);
void setsheetcolor(int idx, int color);
int sheetbyname(const char* name);

// Clipboard for cross-sheet copy/paste
struct clipcell {
  int type;
  float val;
  char text[MAXIN];
  char strval[MAXIN];
  int is_str;
  int fmt, color, bg, attr;
  char cond[64];
};

extern struct clipcell clipboard[CLIPBOARD_MAX];
extern int clip_w, clip_h;  // dimensions of clipboard content (0 if empty)

void yank_cells(struct grid* g, int c1, int r1, int c2, int r2);
void paste_cells(struct grid* g, int tc, int tr);
void cut_cells(struct grid* g, int c1, int r1, int c2, int r2);

#endif
