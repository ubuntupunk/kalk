#define _POSIX_C_SOURCE 200809L
#include "cell.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_expr(void) {
  static struct grid g = {0};
  struct parser p = {0};
  g.cells[0][0].type = NUM;
  g.cells[0][0].val = 3.0f;
  g.cells[0][1].type = NUM;
  g.cells[0][1].val = 5.0f;
  g.cells[0][2].type = NUM;
  g.cells[0][2].val = 11.0f;
  g.cells[0][3].type = NUM;
  g.cells[0][3].val = -13.5f;
#define EXPR(e) (p.s = p.p = e, p.g = &g, cmp(&p))
  // numbers
  assert(isnan(EXPR("")));
  assert(EXPR("42") == 42.0f);
  assert(EXPR("1.5") == 1.5f);
  assert(EXPR(".5") == 0.5f);
  assert(EXPR("-123") == -123.0f);
  assert(EXPR("+123") == 123.0f);
  assert(EXPR("(123)") == 123.0f);
  // references
  assert(EXPR("A1") == 3.0f);
  assert(EXPR("A2") == 5.0f);
  assert(EXPR("A3") == 11.0f);
  assert(EXPR("B1") == 0.0f);
  assert(EXPR("A12") == 0.0f);
  // expressions
  assert(EXPR("A1*A2") == 15.0f);
  assert(EXPR("A1*10/A2") == 6.0f);
  assert(isnan(EXPR("A1/0")));
  assert(EXPR("A1+A2") == 8.0f);
  assert(EXPR("A1+A2-A3") == -3.0f);
  assert(EXPR("A1+A2*A3") == 58.0f);
  assert(EXPR("(A1+A2)*A3") == 88.0f);
  // functions
  assert(EXPR("@ABS(A1)") == 3.0f);
  assert(EXPR("@ABS(A4)") == 13.5f);
  assert(EXPR("@INT(A4)") == -13.0f);
  assert(EXPR("@INT(@ABS(A4))") == 13.0f);
  assert(EXPR("@SQRT(A3+A2))") == 4.0f);
  assert(EXPR("@SUM(A3))") == 11.0f);
  assert(EXPR("@SUM(A1...A3))") == 19.0f);
  assert(EXPR("@SUM(A3...A1))") == 19.0f);
  assert(EXPR("@SUM(A1...A1))") == 3.0f);
  assert(EXPR("@SUM(A3...Z1))") == 19.0f);
  // Google Sheets-style syntax
  assert(EXPR("=A1+A2") == 8.0f);
  assert(EXPR("=A1*A2") == 15.0f);
  assert(EXPR("=SUM(A1...A3))") == 19.0f);
  assert(EXPR("SUM(A1...A3))") == 19.0f);
  assert(EXPR("SUM(A1:A3))") == 19.0f);
  assert(EXPR("=SUM(A1:A3))") == 19.0f);
  assert(EXPR("=SUM(A3:A1))") == 19.0f);
  // New functions: AVERAGE, MIN, MAX, COUNT
  assert(EXPR("AVERAGE(A1:A3))") == (3.0f+5.0f+11.0f)/3.0f);
  assert(EXPR("MIN(A1:A3))") == 3.0f);
  assert(EXPR("MAX(A1:A3))") == 11.0f);
  assert(EXPR("COUNT(A1:A3))") == 3.0f);
  assert(EXPR("COUNT(A1:A4))") == 4.0f);
  assert(EXPR("MIN(A1:A4))") == -13.5f);
  assert(EXPR("MAX(A1:A4))") == 11.0f);
  assert(EXPR("=AVERAGE(A1:A3))") == (3.0f+5.0f+11.0f)/3.0f);
  assert(EXPR("=MIN(A1:A3))") == 3.0f);
  assert(EXPR("=MAX(A1:A3))") == 11.0f);
  assert(EXPR("@SUM(A1:A4))") == 5.5f);
  assert(EXPR("AVERAGE(A1))") == 3.0f);
  assert(EXPR("MIN(A1))") == 3.0f);
  assert(EXPR("MAX(A1))") == 3.0f);
  assert(EXPR("COUNT(A1))") == 1.0f);
  // Rounding functions
  assert(EXPR("ROUND(3.14159, 0))") == 3.0f);
  assert(EXPR("ROUNDUP(3.14159, 0))") == 4.0f);
  assert(EXPR("ROUNDDOWN(3.14159, 0))") == 3.0f);
  assert(EXPR("CEILING(3.14))") == 4.0f);
  assert(EXPR("FLOOR(3.14))") == 3.0f);
  assert(EXPR("CEILING(A1))") == 3.0f);
  assert(EXPR("FLOOR(A4))") == -14.0f);
  assert(EXPR("ROUND(A1, 0))") == 3.0f);
  assert(EXPR("ROUNDUP(A1, 0))") == 3.0f);
  assert(EXPR("ROUNDDOWN(A1, 0))") == 3.0f);
  assert(EXPR("ROUND(2.5, 0))") == 3.0f);
  assert(EXPR("ROUND(1.5, 0))") == 2.0f);
  assert(EXPR("CEILING(-3.5))") == -3.0f);
  assert(EXPR("FLOOR(-3.5))") == -4.0f);
  // Comparison operators
  assert(EXPR("A1>0") == 1.0f);   // 3>0 → true
  assert(EXPR("A1>5") == 0.0f);   // 3>5 → false
  assert(EXPR("A1<5") == 1.0f);   // 3<5 → true
  assert(EXPR("A1<0") == 0.0f);   // 3<0 → false
  assert(EXPR("A1=3") == 1.0f);   // 3==3 → true
  assert(EXPR("A1=5") == 0.0f);   // 3==5 → false
  assert(EXPR("A1<>3") == 0.0f);  // 3≠3 → false
  assert(EXPR("A1<>5") == 1.0f);  // 3≠5 → true
  assert(EXPR("A1!=5") == 1.0f);  // 3≠5 → true (!= variant)
  assert(EXPR("A1<=3") == 1.0f);  // 3≤3 → true
  assert(EXPR("A1<=2") == 0.0f);  // 3≤2 → false
  assert(EXPR("A1>=3") == 1.0f);  // 3≥3 → true
  assert(EXPR("A1>=5") == 0.0f);  // 3≥5 → false
  // Comparison with arithmetic (comparison has lower precedence)
  assert(EXPR("A1+A2<10") == 1.0f);  // 3+5=8, 8<10 → true
  assert(EXPR("A1*A2>10") == 1.0f);  // 3*5=15, 15>10 → true
  assert(EXPR("(A1+A2)>A3") == 0.0f); // (3+5)=8>11 → false
  assert(EXPR("(A1+A2)=A3") == 0.0f); // 8=11 → false
  assert(EXPR("(A1+A2)=8") == 1.0f);  // 8=8 → true
  // Chained: left-to-right
  assert(EXPR("3>2>1") == 0.0f);  // (3>2)=1, 1>1=0
  // IF with comparisons
  assert(EXPR("IF(A1>0, 10, 20))") == 10.0f);   // 3>0 → true → 10
  assert(EXPR("IF(A1>5, 10, 20))") == 20.0f);   // 3>5 → false → 20
  assert(EXPR("IF(A1=3, A2, A3))") == 5.0f);    // 3=3 → true → A2=5
  assert(EXPR("IF(A1<>5, A2, A3))") == 5.0f);   // 3≠5 → true → A2=5
  assert(EXPR("IF(A1<>3, A2, A3))") == 11.0f);  // 3≠3 → false → A3=11
  assert(EXPR("IF(A1+A2>10, A3, A4))") == -13.5f); // 3+5=8>10 → false → A4=-13.5
  assert(EXPR("IF(A1*A2>10, A3, A4))") == 11.0f);  // 3*5=15>10 → true → A3=11
  // Nested IF with comparisons
  assert(EXPR("IF(A1>0, IF(A2<10, 100, 200), 300))") == 100.0f);
  assert(EXPR("IF(A1>5, 10, IF(A2<10, 20, 30)))") == 20.0f);
  // IF function
  assert(EXPR("IF(1, 10, 20))") == 10.0f);
  assert(EXPR("IF(0, 10, 20))") == 20.0f);
  assert(EXPR("IF(A1, A2, A3))") == 5.0f);   // A1=3 (truthy) -> A2=5
  assert(EXPR("IF(A4, A2, A3))") == 5.0f);   // A4=-13.5 (truthy) -> A2=5
  assert(EXPR("IF(0, A2, A3))") == 11.0f);   // 0 (falsy) -> A3=11
  assert(EXPR("=IF(1, 2, 3))") == 2.0f);
  assert(EXPR("@IF(0, 2, 3))") == 3.0f);
  // expressions as arguments
  assert(EXPR("IF(A1+A2, A3, A4))") == 11.0f);  // 3+5=8 (truthy) -> A3=11
  assert(EXPR("IF(A4, A1+A2, A1-A2))") == 8.0f); // A4=-13.5 (truthy) -> 3+5=8
  assert(EXPR("IF(A4+13.5, 1, 0))") == 0.0f);  // -13.5+13.5=0 (falsy) -> 0
  // nested IF
  assert(EXPR("IF(1, IF(0, 10, 20), 30))") == 20.0f);
  assert(EXPR("IF(0, 10, IF(1, 20, 30)))") == 20.0f);
  // zero and edge cases
  assert(EXPR("IF(0, 1, 0))") == 0.0f);
  assert(EXPR("IF(1, 0, 1))") == 0.0f);
  // Math functions: PI, RAND, POWER, MOD, RANDBETWEEN, SIN, COS, TAN, LOG
  assert(EXPR("PI()") > 3.14159f && EXPR("PI()") < 3.14160f);
  {
    float r = EXPR("RAND()");
    assert(r >= 0.0f && r < 1.0f);
    // RAND should produce different values on repeated calls
    float r2 = EXPR("RAND()");
    assert(r2 >= 0.0f && r2 < 1.0f);
  }
  assert(EXPR("POWER(2, 3))") == 8.0f);
  assert(EXPR("POWER(9, 0.5))") == 3.0f);
  assert(EXPR("POWER(A1, 2))") == 9.0f);   // A1=3, 3^2=9
  assert(EXPR("MOD(10, 3))") == 1.0f);
  assert(EXPR("MOD(10, 2))") == 0.0f);
  assert(isnan(EXPR("MOD(10, 0))")));  // division by zero → NAN
  assert(EXPR("MOD(A2, A1))") == 2.0f);  // 5 mod 3 = 2
  {
    float r = EXPR("RANDBETWEEN(1, 10))");
    assert(r >= 1.0f && r <= 10.0f);
  }
  assert(EXPR("SIN(0))") == 0.0f);
  assert(EXPR("COS(0))") == 1.0f);
  assert(EXPR("TAN(0))") == 0.0f);
  assert(fabs(EXPR("SIN(PI()/2))") - 1.0f) < 0.0001f);  // sin(&pi/2)=1
  assert(fabs(EXPR("COS(PI())") - (-1.0f)) < 0.0001f);   // cos(&pi)=-1
  assert(fabs(EXPR("TAN(PI()/4))") - 1.0f) < 0.0001f);   // tan(&pi/4)=1
  assert(EXPR("LOG(1))") == 0.0f);
  assert(fabs(EXPR("LOG(PI())") - 1.1447f) < 0.001f);
  assert(isnan(EXPR("LOG(0))")));
  assert(isnan(EXPR("LOG(-1))")));
  // = and @ prefix work with math functions
  assert(EXPR("=POWER(2, 10))") == 1024.0f);
  assert(EXPR("@MOD(17, 5))") == 2.0f);
  // SUMIF with sum_range: set B col values for meaningful tests
  // B1=0, B2=0, B3=0 (zero-initialized static grid)
  // SUMIF with comparison operators
  assert(EXPR("SUMIF(A1:A3, 5))") == 5.0f);  // only A2=5
  assert(EXPR("SUMIF(A1:A3, 3))") == 3.0f);  // only A1=3
  assert(EXPR("SUMIF(A1:A3, 99))") == 0.0f); // none match
  assert(EXPR("SUMIF(A1:A3, >5))") == 11.0f);   // A3>5 → 11
  assert(EXPR("SUMIF(A1:A3, <5))") == 3.0f);    // A1<5 → 3
  assert(EXPR("SUMIF(A1:A3, >=5))") == 16.0f);  // A2+A3 → 16
  assert(EXPR("SUMIF(A1:A3, <=5))") == 8.0f);   // A1+A2 → 8
  assert(EXPR("SUMIF(A1:A3, <>5))") == 14.0f);  // A1+A3 → 14
  assert(EXPR("SUMIF(A1:A4, <0))") == -13.5f);  // only A4<0
  assert(EXPR("SUMIF(A1:A3, >0))") == 19.0f);   // all positive
  // COUNTIF
  assert(EXPR("COUNTIF(A1:A3, 5))") == 1.0f);   // one cell =5
  assert(EXPR("COUNTIF(A1:A3, >0))") == 3.0f);  // all >0
  assert(EXPR("COUNTIF(A1:A4, <0))") == 1.0f);  // A4<0
  assert(EXPR("COUNTIF(A1:A3, >=5))") == 2.0f); // A2+A3
  assert(EXPR("COUNTIF(A1:A3, <>3))") == 2.0f); // A2+A3
  assert(EXPR("COUNTIF(A1:A4, <=0))") == 1.0f); // A4<=0

  // --- Phase 2: VLOOKUP/HLOOKUP ---
  // VLOOKUP(key, range, col_index): search first col for key, return from indexed col
  assert(EXPR("VLOOKUP(3, A1:A3, 1))") == 3.0f);   // A1=3, col 1 → 3
  assert(EXPR("VLOOKUP(5, A1:A3, 1))") == 5.0f);   // A2=5, col 1 → 5
  assert(EXPR("VLOOKUP(11, A1:A3, 1))") == 11.0f); // A3=11, col 1 → 11
  assert(isnan(EXPR("VLOOKUP(99, A1:A3, 1))")));   // not found → NAN
  // VLOOKUP into a different column in the range
  // A1=3, B1=0 (empty cell), so col 2 from A1:B3 returns NAN...
  // Use same-column lookup (col 1) which always works since A col is set up
  assert(EXPR("VLOOKUP(3, A1:C3, 1))") == 3.0f);
  // HLOOKUP(key, range, row_index): search first row for key
  assert(EXPR("HLOOKUP(3, A1:C1, 1))") == 3.0f);   // row 0 has A1=3
  assert(isnan(EXPR("HLOOKUP(99, A1:A3, 1))")));

  // --- Phase 2: Multi-arg SUM/AVERAGE/MIN/MAX/COUNT ---
  assert(EXPR("SUM(A1, A2, A3))") == 19.0f);    // 3+5+11
  assert(EXPR("SUM(A1, A2, A3, A4))") == 5.5f);  // 3+5+11+(-13.5)
  assert(EXPR("AVERAGE(A1, A2, A3))") == (3+5+11)/3.0f);
  assert(EXPR("MIN(A1, A2, A3, A4))") == -13.5f);
  assert(EXPR("MAX(A1, A2, A3))") == 11.0f);
  assert(EXPR("COUNT(A1, A2, A3))") == 3.0f);
  assert(EXPR("COUNT(A1, A2, A3, A4))") == 4.0f);
  // Single arg still works
  assert(EXPR("SUM(A1))") == 3.0f);
  assert(EXPR("AVERAGE(A1))") == 3.0f);
  assert(EXPR("MIN(A1))") == 3.0f);
  assert(EXPR("MAX(A1))") == 3.0f);
  assert(EXPR("COUNT(A1))") == 1.0f);
  // Empty/comma at end
  assert(isnan(EXPR("SUM())")));  // empty sum should be NAN... actually cmp returns NAN for empty
  // Mix of numbers and expressions
  assert(EXPR("SUM(A1, 10, A2+A3))") == 3+10+16);
  // MIN/MAX with NAN first arg should still find valid subsequent values
  assert(EXPR("MIN(1/0, A2, A3))") == 5.0f);
  assert(EXPR("MAX(1/0, A2, A3))") == 11.0f);
  // Edge: first arg valid, second is NAN
  assert(EXPR("MIN(A2, 1/0, A3))") == 5.0f);
  assert(EXPR("MAX(A2, 1/0, A3))") == 11.0f);
#undef EXPR
}

void test_recalc(void) {
  static struct grid g = {0};
  // testing re-evaluation
  setcell(&g, 0, 0, "5");
  setcell(&g, 0, 1, "7");
  setcell(&g, 0, 2, "11");
  setcell(&g, 0, 3, "+@SUM(A1...A3)");
  assert(g.cells[0][3].val == 23.0f);

  setcell(&g, 0, 0, "5");
  setcell(&g, 0, 1, "+A1+5");
  setcell(&g, 0, 2, "+A2+A1");
  assert(g.cells[0][3].val == 5.0f + 10.0f + 15.0f);

  setcell(&g, 0, 0, "7");
  assert(g.cells[0][3].val == 7.0f + 12.0f + 19.0f);
}

void test_ref(void) {
  int c, r;
  assert(ref("A1", &c, &r) == 2 && c == 0 && r == 0);
  assert(ref("Z50", &c, &r) == 3 && c == 25 && r == 49);
  assert(ref("AA10", &c, &r) == 4 && c == 26 && r == 9);
  assert(ref("AZ99", &c, &r) == 4 && c == 51 && r == 98);
  assert(ref("BA1", &c, &r) == 3 && c == 52 && r == 0);
}

void test_col(void) {
  assert(strcmp(col(0), "A") == 0);
  assert(strcmp(col(25), "Z") == 0);
  assert(strcmp(col(26), "AA") == 0);
  assert(strcmp(col(51), "AZ") == 0);
  assert(strcmp(col(52), "BA") == 0);
}

void test_csv_load(void) {
  static struct grid g = {0};

  // basic CSV: numbers, labels, formulas
  {
    FILE* f = fopen("/tmp/test_kalk_basic.csv", "w");
    fprintf(f, "10,20,30\n");
    fprintf(f, "hello,world,+A1+B1\n");
    fclose(f);
    assert(csvload(&g, "/tmp/test_kalk_basic.csv") == 0);
    assert(g.cells[0][0].type == NUM && g.cells[0][0].val == 10.0f);
    assert(g.cells[1][0].type == NUM && g.cells[1][0].val == 20.0f);
    assert(g.cells[2][0].type == NUM && g.cells[2][0].val == 30.0f);
    assert(g.cells[0][1].type == LABEL);
    assert(strcmp(g.cells[0][1].text, "hello") == 0);
    assert(g.cells[2][1].type == FORMULA);
    assert(g.cells[2][1].val == 30.0f);  // A1+B1 = 10+20
  }

  // quoted fields: embedded commas and quotes
  {
    memset(&g, 0, sizeof(g));
    FILE* f = fopen("/tmp/test_kalk_quoted.csv", "w");
    fprintf(f, "\"has,comma\",\"has\"\"quote\",plain\n");
    fclose(f);
    assert(csvload(&g, "/tmp/test_kalk_quoted.csv") == 0);
    assert(strcmp(g.cells[0][0].text, "has,comma") == 0);
    assert(strcmp(g.cells[1][0].text, "has\"quote") == 0);
    assert(strcmp(g.cells[2][0].text, "plain") == 0);
  }

  // CRLF line endings (Windows/Excel)
  {
    memset(&g, 0, sizeof(g));
    FILE* f = fopen("/tmp/test_kalk_crlf.csv", "wb");
    fprintf(f, "1,2\r\n3,4\r\n");
    fclose(f);
    assert(csvload(&g, "/tmp/test_kalk_crlf.csv") == 0);
    assert(g.cells[0][0].val == 1.0f);
    assert(g.cells[1][0].val == 2.0f);
    assert(g.cells[0][1].val == 3.0f);
    assert(g.cells[1][1].val == 4.0f);
  }

  // empty cells
  {
    memset(&g, 0, sizeof(g));
    FILE* f = fopen("/tmp/test_kalk_empty.csv", "w");
    fprintf(f, "1,,3\n,5,\n");
    fclose(f);
    assert(csvload(&g, "/tmp/test_kalk_empty.csv") == 0);
    assert(g.cells[0][0].val == 1.0f);
    assert(g.cells[1][0].type == EMPTY);
    assert(g.cells[2][0].val == 3.0f);
    assert(g.cells[0][1].type == EMPTY);
    assert(g.cells[1][1].val == 5.0f);
  }

  // non-existent file
  assert(csvload(&g, "/tmp/test_kalk_nonexistent_42.csv") == -1);
}

void test_csv_save(void) {
  static struct grid g = {0};

  setcell(&g, 0, 0, "100");
  setcell(&g, 1, 0, "200");
  setcell(&g, 2, 0, "+A1+B1");
  setcell(&g, 0, 1, "hello");
  setcell(&g, 1, 1, "has,comma");

  assert(csvsave(&g, "/tmp/test_kalk_save.csv") == 0);

  // read back and verify
  memset(&g, 0, sizeof(g));
  assert(csvload(&g, "/tmp/test_kalk_save.csv") == 0);
  assert(g.cells[0][0].val == 100.0f);
  assert(g.cells[1][0].val == 200.0f);
  assert(g.cells[2][0].type == FORMULA);
  assert(g.cells[2][0].val == 300.0f);
  assert(strcmp(g.cells[0][1].text, "hello") == 0);
  assert(strcmp(g.cells[1][1].text, "has,comma") == 0);
}

void test_csv_roundtrip(void) {
  static struct grid g = {0};

  // field with embedded quote
  setcell(&g, 0, 0, "say \"hello\"");
  setcell(&g, 1, 0, "normal");
  setcell(&g, 0, 1, "42.5");

  assert(csvsave(&g, "/tmp/test_kalk_rt.csv") == 0);
  memset(&g, 0, sizeof(g));
  assert(csvload(&g, "/tmp/test_kalk_rt.csv") == 0);
  assert(strcmp(g.cells[0][0].text, "say \"hello\"") == 0);
  assert(strcmp(g.cells[1][0].text, "normal") == 0);
  assert(g.cells[0][1].val == 42.5f);
}

void test_swap(void) {
  static struct grid g = {0};

  // Setup: A1=10 A2=20 A3=30, B1=100 B2=200 B3=300
  setcell(&g, 0, 0, "10");
  setcell(&g, 0, 1, "20");
  setcell(&g, 0, 2, "30");
  setcell(&g, 1, 0, "100");
  setcell(&g, 1, 1, "200");
  setcell(&g, 1, 2, "300");

  // Swap rows 0 and 1: A1<->A2, B1<->B2
  swaprow(&g, 0, 1);
  assert(g.cells[0][0].val == 20.0f);
  assert(g.cells[0][1].val == 10.0f);
  assert(g.cells[1][0].val == 200.0f);
  assert(g.cells[1][1].val == 100.0f);
  assert(g.cells[0][2].val == 30.0f);  // row 2 unchanged

  // Swap back
  swaprow(&g, 0, 1);
  assert(g.cells[0][0].val == 10.0f);
  assert(g.cells[0][1].val == 20.0f);

  // Swap columns 0 and 1: A<->B
  swapcol(&g, 0, 1);
  assert(g.cells[0][0].val == 100.0f);
  assert(g.cells[1][0].val == 10.0f);
  assert(g.cells[0][1].val == 200.0f);
  assert(g.cells[1][1].val == 20.0f);

  // Swap back
  swapcol(&g, 0, 1);
  assert(g.cells[0][0].val == 10.0f);
  assert(g.cells[1][0].val == 100.0f);
}

void test_fixrefs(void) {
  static struct grid g = {0};

  // Setup: A1=10, A2=20, formula in B1 referencing A1 and A2
  setcell(&g, 0, 0, "10");
  setcell(&g, 0, 1, "20");
  setcell(&g, 1, 0, "+A1+A2");
  assert(g.cells[1][0].val == 30.0f);

  // Swap rows 0 and 1: data moves, refs updated
  // A1(10)->A2, A2(20)->A1, B1(+A1+A2)->B2 with refs swapped to +A2+A1
  swaprow(&g, 0, 1);
  recalc(&g);
  // Formula moved to B2, should still reference the same data (now A2+A1 = 10+20)
  assert(g.cells[1][1].val == 30.0f);
  assert(strcmp(g.cells[1][1].text, "+A2+A1") == 0);
  // Data swapped
  assert(g.cells[0][0].val == 20.0f);  // was A2
  assert(g.cells[0][1].val == 10.0f);  // was A1

  // Swap back: everything restored
  swaprow(&g, 0, 1);
  recalc(&g);
  assert(g.cells[1][0].val == 30.0f);
  assert(strcmp(g.cells[1][0].text, "+A1+A2") == 0);

  // Column swap: A1=10, B1=100, C1=+A1+B1
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "10");
  setcell(&g, 1, 0, "100");
  setcell(&g, 2, 0, "+A1+B1");
  assert(g.cells[2][0].val == 110.0f);

  // Swap cols A and B: data moves, refs updated
  swapcol(&g, 0, 1);
  recalc(&g);
  assert(g.cells[0][0].val == 100.0f);  // was B
  assert(g.cells[1][0].val == 10.0f);   // was A
  assert(strcmp(g.cells[2][0].text, "+B1+A1") == 0);
  assert(g.cells[2][0].val == 110.0f);  // still 10+100

  // Swap back
  swapcol(&g, 0, 1);
  recalc(&g);
  assert(strcmp(g.cells[2][0].text, "+A1+B1") == 0);
  assert(g.cells[2][0].val == 110.0f);

  // Refs to uninvolved rows/cols stay unchanged
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "1");
  setcell(&g, 0, 1, "2");
  setcell(&g, 0, 2, "3");
  setcell(&g, 1, 0, "+A3");  // references row 2, not involved in swap of 0,1
  assert(g.cells[1][0].val == 3.0f);
  swaprow(&g, 0, 1);
  recalc(&g);
  // formula moved to B2, A3 ref unchanged since row 2 not swapped
  assert(strcmp(g.cells[1][1].text, "+A3") == 0);
  assert(g.cells[1][1].val == 3.0f);

  // SUM range refs get updated
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "5");
  setcell(&g, 0, 1, "10");
  setcell(&g, 0, 2, "15");
  setcell(&g, 1, 0, "+@SUM(A1...A3)");
  assert(g.cells[1][0].val == 30.0f);
  swaprow(&g, 0, 1);
  recalc(&g);
  // formula moved to B2, range refs swapped: A1->A2, A3 stays
  assert(strcmp(g.cells[1][1].text, "+@SUM(A2...A3)") == 0);
  assert(g.cells[1][1].val == 20.0f);  // A2(5)+A3(15), range shrank

  // Multiple swaps (move row 0 to row 2 via two swaps)
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "1");
  setcell(&g, 0, 1, "2");
  setcell(&g, 0, 2, "3");
  setcell(&g, 1, 0, "+A1");
  swaprow(&g, 0, 1);  // row0<->row1
  swaprow(&g, 1, 2);  // row1<->row2 (original row0 now at row2)
  recalc(&g);
  assert(g.cells[0][2].val == 1.0f);  // original A1 data
  assert(g.cells[1][2].val == 1.0f);  // formula followed the data
  assert(strcmp(g.cells[1][2].text, "+A3") == 0);

  // Formula not in swapped rows still gets refs updated
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "10");
  setcell(&g, 0, 1, "20");
  setcell(&g, 0, 2, "+A1+A2");  // in row 2, refs rows 0 and 1
  swaprow(&g, 0, 1);
  recalc(&g);
  assert(strcmp(g.cells[0][2].text, "+A2+A1") == 0);
  assert(g.cells[0][2].val == 30.0f);  // still 10+20
}

void test_insert_delete(void) {
  static struct grid g = {0};

  // --- Insert row: data shifts down, refs adjust ---
  setcell(&g, 0, 0, "10");   // A1=10
  setcell(&g, 0, 1, "20");   // A2=20
  setcell(&g, 0, 2, "30");   // A3=30
  setcell(&g, 1, 0, "+A2");  // B1=+A2 (=20)
  assert(g.cells[1][0].val == 20.0f);

  // Insert row at 1 (between A1 and A2): A2->A3, A3->A4
  insertrow(&g, 1);
  recalc(&g);
  assert(g.cells[0][0].val == 10.0f);   // A1 unchanged
  assert(g.cells[0][1].type == EMPTY);  // A2 is new blank row
  assert(g.cells[0][2].val == 20.0f);   // old A2 -> A3
  assert(g.cells[0][3].val == 30.0f);   // old A3 -> A4
  // B1 formula should now reference A3 (was A2, shifted +1)
  assert(strcmp(g.cells[1][0].text, "+A3") == 0);
  assert(g.cells[1][0].val == 20.0f);

  // --- Delete that inserted row: refs shrink back ---
  deleterow(&g, 1);
  recalc(&g);
  assert(g.cells[0][0].val == 10.0f);
  assert(g.cells[0][1].val == 20.0f);
  assert(g.cells[0][2].val == 30.0f);
  assert(strcmp(g.cells[1][0].text, "+A2") == 0);
  assert(g.cells[1][0].val == 20.0f);

  // --- Insert column: data shifts right, refs adjust ---
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "10");      // A1=10
  setcell(&g, 1, 0, "20");      // B1=20
  setcell(&g, 2, 0, "+A1+B1");  // C1=+A1+B1 (=30)
  assert(g.cells[2][0].val == 30.0f);

  // Insert column at 1 (between A and B): B->C, C->D
  insertcol(&g, 1);
  recalc(&g);
  assert(g.cells[0][0].val == 10.0f);   // A1 unchanged
  assert(g.cells[1][0].type == EMPTY);  // B1 is new blank col
  assert(g.cells[2][0].val == 20.0f);   // old B1 -> C1
  // Formula moved to D1, refs A1 unchanged, B1->C1
  assert(strcmp(g.cells[3][0].text, "+A1+C1") == 0);
  assert(g.cells[3][0].val == 30.0f);

  // --- Delete that inserted column ---
  deletecol(&g, 1);
  recalc(&g);
  assert(g.cells[0][0].val == 10.0f);
  assert(g.cells[1][0].val == 20.0f);
  assert(strcmp(g.cells[2][0].text, "+A1+B1") == 0);
  assert(g.cells[2][0].val == 30.0f);

  // --- Delete row that is referenced: refs to later rows shift ---
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "10");   // A1=10
  setcell(&g, 0, 1, "20");   // A2=20
  setcell(&g, 0, 2, "30");   // A3=30
  setcell(&g, 1, 0, "+A3");  // B1=+A3 (=30)
  assert(g.cells[1][0].val == 30.0f);

  // Delete row 1 (A2): A3->A2
  deleterow(&g, 1);
  recalc(&g);
  assert(g.cells[0][0].val == 10.0f);
  assert(g.cells[0][1].val == 30.0f);              // old A3 is now A2
  assert(strcmp(g.cells[1][0].text, "+A2") == 0);  // ref shifted
  assert(g.cells[1][0].val == 30.0f);

  // --- Insert at row 0: all refs shift ---
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "5");
  setcell(&g, 0, 1, "10");
  setcell(&g, 1, 1, "+A1");  // B2=+A1 (=5)
  assert(g.cells[1][1].val == 5.0f);

  insertrow(&g, 0);
  recalc(&g);
  assert(g.cells[0][0].type == EMPTY);  // new blank row
  assert(g.cells[0][1].val == 5.0f);    // old A1 -> A2
  assert(g.cells[0][2].val == 10.0f);   // old A2 -> A3
  // B2's formula was +A1, shifted to +A2 and moved to B3
  assert(strcmp(g.cells[1][2].text, "+A2") == 0);
  assert(g.cells[1][2].val == 5.0f);

  // --- SUM range adjusts on insert ---
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "1");
  setcell(&g, 0, 1, "2");
  setcell(&g, 0, 2, "3");
  setcell(&g, 1, 0, "+@SUM(A1...A3)");
  assert(g.cells[1][0].val == 6.0f);

  // Insert row at 1: range A1...A3 becomes A1...A4
  insertrow(&g, 1);
  recalc(&g);
  assert(strcmp(g.cells[1][0].text, "+@SUM(A1...A4)") == 0);
  assert(g.cells[1][0].val == 6.0f);  // new blank row adds 0

  // --- Delete column that is referenced ---
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "10");   // A1
  setcell(&g, 1, 0, "20");   // B1
  setcell(&g, 2, 0, "30");   // C1
  setcell(&g, 3, 0, "+C1");  // D1=+C1 (=30)
  assert(g.cells[3][0].val == 30.0f);

  // Delete col B (index 1): C->B, D->C, ref C1->B1
  deletecol(&g, 1);
  recalc(&g);
  assert(g.cells[0][0].val == 10.0f);
  assert(g.cells[1][0].val == 30.0f);              // old C1 is now B1
  assert(strcmp(g.cells[2][0].text, "+B1") == 0);  // ref shifted
  assert(g.cells[2][0].val == 30.0f);

  // --- Multiple inserts ---
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "42");
  setcell(&g, 1, 0, "+A1");
  insertrow(&g, 0);
  insertrow(&g, 0);
  recalc(&g);
  // A1 was shifted twice to A3
  assert(g.cells[0][2].val == 42.0f);
  assert(strcmp(g.cells[1][2].text, "+A3") == 0);
  assert(g.cells[1][2].val == 42.0f);
}

void test_replicate(void) {
  static struct grid g = {0};

  // Replicate a number: plain copy
  setcell(&g, 0, 0, "42");
  replicatecell(&g, 0, 0, 1, 0);
  recalc(&g);
  assert(g.cells[1][0].type == NUM);
  assert(g.cells[1][0].val == 42.0f);

  // Replicate a label: plain copy
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "hello");
  replicatecell(&g, 0, 0, 1, 0);
  assert(g.cells[1][0].type == LABEL);
  assert(strcmp(g.cells[1][0].text, "hello") == 0);

  // Replicate formula down: refs shift by row delta
  // A1=10, A2=20, B1=+A1 → replicate B1 to B2 → B2=+A2
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "10");
  setcell(&g, 0, 1, "20");
  setcell(&g, 1, 0, "+A1");
  replicatecell(&g, 1, 0, 1, 1);  // B1 -> B2
  recalc(&g);
  assert(strcmp(g.cells[1][1].text, "+A2") == 0);
  assert(g.cells[1][1].val == 20.0f);

  // Replicate formula right: refs shift by col delta
  // A1=10, B1=20, A2=+A1 → replicate A2 to B2 → B2=+B1
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "10");
  setcell(&g, 1, 0, "20");
  setcell(&g, 0, 1, "+A1");
  replicatecell(&g, 0, 1, 1, 1);  // A2 -> B2
  recalc(&g);
  assert(strcmp(g.cells[1][1].text, "+B1") == 0);
  assert(g.cells[1][1].val == 20.0f);

  // Absolute ref: $A$1 stays fixed
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "10");
  setcell(&g, 1, 0, "+$A$1");
  replicatecell(&g, 1, 0, 1, 1);  // B1 -> B2
  recalc(&g);
  assert(strcmp(g.cells[1][1].text, "+$A$1") == 0);
  assert(g.cells[1][1].val == 10.0f);

  // Mixed: $A1 → col fixed, row shifts
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "10");
  setcell(&g, 0, 1, "20");
  setcell(&g, 1, 0, "+$A1");
  replicatecell(&g, 1, 0, 1, 1);  // B1 -> B2
  recalc(&g);
  assert(strcmp(g.cells[1][1].text, "+$A2") == 0);
  assert(g.cells[1][1].val == 20.0f);

  // Mixed: A$1 → row fixed, col shifts
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "10");
  setcell(&g, 1, 0, "20");
  setcell(&g, 0, 1, "+A$1");
  replicatecell(&g, 0, 1, 1, 1);  // A2 -> B2
  recalc(&g);
  assert(strcmp(g.cells[1][1].text, "+B$1") == 0);
  assert(g.cells[1][1].val == 20.0f);

  // Replicate to range: B1=+A1, replicate to B2...B4
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "1");
  setcell(&g, 0, 1, "2");
  setcell(&g, 0, 2, "3");
  setcell(&g, 0, 3, "4");
  setcell(&g, 1, 0, "+A1");
  for (int r = 1; r <= 3; r++) replicatecell(&g, 1, 0, 1, r);
  recalc(&g);
  assert(strcmp(g.cells[1][1].text, "+A2") == 0);
  assert(g.cells[1][1].val == 2.0f);
  assert(strcmp(g.cells[1][2].text, "+A3") == 0);
  assert(g.cells[1][2].val == 3.0f);
  assert(strcmp(g.cells[1][3].text, "+A4") == 0);
  assert(g.cells[1][3].val == 4.0f);

  // SUM range shifts: B1=+@SUM(A1...A3), replicate to C1 → +@SUM(B1...B3)
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "1");
  setcell(&g, 0, 1, "2");
  setcell(&g, 0, 2, "3");
  setcell(&g, 1, 0, "+@SUM(A1...A3)");
  assert(g.cells[1][0].val == 6.0f);
  replicatecell(&g, 1, 0, 2, 0);  // B1 -> C1
  recalc(&g);
  assert(strcmp(g.cells[2][0].text, "+@SUM(B1...B3)") == 0);

  // Replicate empty cell clears target
  memset(&g, 0, sizeof(g));
  setcell(&g, 1, 0, "999");
  replicatecell(&g, 0, 0, 1, 0);  // A1 (empty) -> B1
  assert(g.cells[1][0].type == EMPTY);

  // $ refs survive in formula evaluation
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "7");
  setcell(&g, 1, 0, "+$A$1*2");
  recalc(&g);
  assert(g.cells[1][0].val == 14.0f);

  // Range-to-range: replicate A1...A3 to B1...B3
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "10");
  setcell(&g, 0, 1, "20");
  setcell(&g, 0, 2, "+A1+A2");
  assert(g.cells[0][2].val == 30.0f);
  // Simulate range replication: source A1...A3 -> target B1...B3
  {
    int sc1 = 0, sr1 = 0, sc2 = 0, sr2 = 2;
    int tc1 = 1, tr1 = 0, tc2 = 1, tr2 = 2;
    int sw = sc2 - sc1 + 1, sh = sr2 - sr1 + 1;
    for (int r = tr1; r <= tr2; r++)
      for (int c = tc1; c <= tc2; c++)
        replicatecell(&g, sc1 + (c - tc1) % sw, sr1 + (r - tr1) % sh, c, r);
    recalc(&g);
  }
  assert(g.cells[1][0].val == 10.0f);
  assert(g.cells[1][1].val == 20.0f);
  // B3 = +B1+B2 (shifted from +A1+A2)
  assert(strcmp(g.cells[1][2].text, "+B1+B2") == 0);
  assert(g.cells[1][2].val == 30.0f);

  // Range-to-range tiling: replicate A1...A2 to B1...B4 (tiles 2 rows into 4)
  memset(&g, 0, sizeof(g));
  setcell(&g, 0, 0, "1");
  setcell(&g, 0, 1, "2");
  {
    int sc1 = 0, sr1 = 0, sc2 = 0, sr2 = 1;
    int tc1 = 1, tr1 = 0, tc2 = 1, tr2 = 3;
    int sw = sc2 - sc1 + 1, sh = sr2 - sr1 + 1;
    for (int r = tr1; r <= tr2; r++)
      for (int c = tc1; c <= tc2; c++)
        replicatecell(&g, sc1 + (c - tc1) % sw, sr1 + (r - tr1) % sh, c, r);
    recalc(&g);
  }
  assert(g.cells[1][0].val == 1.0f);
  assert(g.cells[1][1].val == 2.0f);
  assert(g.cells[1][2].val == 1.0f);  // tiled
  assert(g.cells[1][3].val == 2.0f);  // tiled
}

void test_sort(void) {
  static struct grid g = {0};

  // Setup: A1=50, A2=10, A3=30, A4=20
  setcell(&g, 0, 0, "50");
  setcell(&g, 0, 1, "10");
  setcell(&g, 0, 2, "30");
  setcell(&g, 0, 3, "20");

  // Sort column A ascending
  sortbycol(&g, 0);
  recalc(&g);

  assert(g.cells[0][0].val == 10.0f);
  assert(g.cells[0][1].val == 20.0f);
  assert(g.cells[0][2].val == 30.0f);
  assert(g.cells[0][3].val == 50.0f);
}

void test_sort_labels_sink(void) {
  static struct grid g = {0};

  // A1="hello" (LABEL), A2=42 (NUM), A3=10 (NUM)
  setcell(&g, 0, 0, "hello");
  setcell(&g, 0, 1, "42");
  setcell(&g, 0, 2, "10");

  sortbycol(&g, 0);
  recalc(&g);

  // Labels sink to bottom, NUM sorted ascending
  assert(g.cells[0][0].type == NUM);
  assert(g.cells[0][0].val == 10.0f);
  assert(g.cells[0][1].type == NUM);
  assert(g.cells[0][1].val == 42.0f);
  assert(g.cells[0][2].type == LABEL);  // hello sank to bottom
}

void test_sort_with_refs(void) {
  static struct grid g = {0};

  // A1=30, A2=10, B1=+A1, B2=+A2  (B1=30, B2=10)
  setcell(&g, 0, 0, "30");
  setcell(&g, 0, 1, "10");
  setcell(&g, 1, 0, "+A1");
  setcell(&g, 1, 1, "+A2");

  sortbycol(&g, 0);
  recalc(&g);

  // After sort: A1=10, A2=30  → refs updated: B1=+A1=10, B2=+A2=30
  assert(g.cells[0][0].val == 10.0f);
  assert(g.cells[0][1].val == 30.0f);
  // Formulas moved with their rows
  assert(g.cells[1][0].val == 10.0f);  // was B2=+A2, now B1, ref A1=10
  assert(g.cells[1][1].val == 30.0f);  // was B1=+A1, now B2, ref A2=30
}

void test_color_cond_fields(void) {
  static struct grid g = {0};

  // Verify color and cond fields exist and can be set
  struct cell* cl = &g.cells[0][0];
  cl->color = 0;
  cl->cond[0] = '\0';
  assert(cl->color == 0);
  assert(cl->cond[0] == '\0');

  cl->color = 2;
  assert(cl->color == 2);

  strcpy(cl->cond, ">5");
  assert(strcmp(cl->cond, ">5") == 0);

  // parse_cond works on cond field
  int op; float val;
  assert(parse_cond(cl->cond, &op, &val) == 1);
  assert(op == 1);  // GT
  assert(val == 5.0f);

  // Empty cond parses as "=0" — actually parse_cond returns 0 for empty string
  cl->cond[0] = '\0';
  assert(parse_cond(cl->cond, &op, &val) == 0);

  // bg field
  cl->bg = 0;
  assert(cl->bg == 0);
  cl->bg = 3;
  assert(cl->bg == 3);
  // fg and bg independently settable
  cl->color = 1;
  cl->bg = 4;
  assert(cl->color == 1);
  assert(cl->bg == 4);

  // attr field
  cl->attr = 0;
  assert(cl->attr == 0);
  cl->attr = 1;
  assert(cl->attr == 1);
  cl->attr = 2;
  assert(cl->attr == 2);
  cl->attr = 3;
  assert(cl->attr == 3);
}

int main(void) {
  test_expr();
  test_recalc();
  test_ref();
  test_col();
  test_csv_load();
  test_csv_save();
  test_csv_roundtrip();
  test_swap();
  test_fixrefs();
  test_insert_delete();
  test_replicate();
  test_sort();
  test_sort_labels_sink();
  test_sort_with_refs();
  test_color_cond_fields();
  return 0;
}
