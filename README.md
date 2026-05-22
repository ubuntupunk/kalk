# kalk

![screenshot](kalk.png)

A minimal spreadsheet for the terminal. No dependencies beyond ncurses.

    $ kalk budget.csv

Inspired by VisiCalc and mostly compatible with it. Reads and writes CSV.
Supports formulas with cell references, 35+ functions (including text
functions like LEN, UPPER, CONCATENATE), cell formatting with colors and
bold/italic text, conditional formatting, row/column operations,
data sorting, cell replication with relative reference adjustment, and
frozen titles. Supports both traditional (`@SUM(A1...A4)`) and Google
Sheets-style (`=SUM(A1:A4)`) syntax.

This is the Power Sheets version, if you want the simple VisiCalc version,
it can be found [here](https://github.com/zserge/kalk).

## Build

    make
    make install        # installs to /usr/local
    make install PREFIX=/usr

Requires a C99 compiler and ncurses.

## Source files

    cell.h      Shared header — types, constants, declarations
    cell.c      Grid and cell operations
    formula.c   Formula parser (arithmetic, functions)
    csv.c       CSV file I/O
    editor.c    Terminal UI (ncurses)
    kalk.c      Main entry point
    test.c      Unit tests

## Usage

Arrow keys navigate. Type a number or formula to enter data. Formulas
start with `+`, `-`, `(`, `@`, or `=`. Anything else is a label.

Press `/` for commands:

    /A            Auto-fill (detect patterns: 1,2,3... or Mon,Tue,Wed...)
    /B            Blank cell
    /C            Clear sheet
    /DR /DC       Delete row/column
    /IR /IC       Insert row/column
    /F_           Format cell (L R I G D $ % *)
    /FC           Set foreground color (0=blk 1=Red 2=Grn 3=Yel 4=Blu 5=Mag 6=Cyn 7=Wht)
    /FB           Set background color (same 0-7 palette)
    /FO           Set text attribute (0=none 1=Bold 2=Italic 3=Both)
    /FN           Set conditional format rule (e.g. >5, <0, =10, <>7)
    /FX           Clear all cell colors/attributes/condition
    /GC           Set column width
    /GF_          Set global format
    /M            Move row/column (arrow keys to drag)
    /O            Sort rows by column (type column letter, e.g. A)
    /R            Replicate (copy with relative refs)
    /SL /SS       Load/Save CSV
    /SQ           Save and quit
    /TV/TH/TB/TN  Lock title rows/columns
    /Q            Quit

Other keys:

    >           Go to cell (type reference)
    !           Force recalculation
    "           Enter label
    Backspace   Clear cell
    Tab         Next column
    Enter       Next row
    Home        Jump to A1
    Ctrl-C      Quit

## Formulas

### Traditional syntax

Arithmetic: `+A1*B2-3`, `(A1+A2)/2`

Functions: `@SUM(A1...A10)`, `@ABS(x)`, `@INT(x)`, `@SQRT(x)`

### Google Sheets-style syntax

`=SUM(A1:A10)` — prefix with `=` and use `:` for ranges.

Functions can also be used without `@` or `=`: `SUM(A1:A10)`.

The two syntaxes are fully compatible and can be mixed.

### Functions

| Function        | Description                                             | Example                         |
| --------------- | ------------------------------------------------------- | ------------------------------- |
| `SUM`           | Sum of values (range or comma-separated)                | `=SUM(A1:A10)`                  |
| `AVERAGE`       | Mean of values (range or comma-separated)               | `=AVERAGE(A1:A10)`              |
| `MIN`           | Minimum value (range or comma-separated)                | `=MIN(A1:A10)`                  |
| `MAX`           | Maximum value (range or comma-separated)                | `=MAX(A1:A10)`                  |
| `COUNT`         | Count of numeric cells (range or comma-separated)       | `=COUNT(A1:A10)`                |
| `SUMIF`         | Sum range cells matching a condition                    | `=SUMIF(A1:A10, ">5")`          |
| `COUNTIF`       | Count range cells matching a condition                  | `=COUNTIF(A1:A10, "<>0")`       |
| `VLOOKUP`       | Search column for key, return from indexed column       | `=VLOOKUP(5, A1:B10, 2)`        |
| `HLOOKUP`       | Search row for key, return from indexed row             | `=HLOOKUP(5, A1:Z10, 3)`        |
| `LEN`           | Length of a string                                      | `=LEN(A1)`                      |
| `FIND`          | Position of substring (1-indexed, NAN if not found)     | `=FIND("abc", A1)`              |
| `LEFT`          | First N characters of a string                          | `=LEFT(A1, 3)`                  |
| `RIGHT`         | Last N characters of a string                           | `=RIGHT(A1, 3)`                 |
| `MID`           | Substring starting at position (1-indexed)              | `=MID(A1, 2, 3)`                |
| `UPPER`         | Convert to uppercase                                    | `=UPPER(A1)`                    |
| `LOWER`         | Convert to lowercase                                    | `=LOWER(A1)`                    |
| `TRIM`          | Remove leading/trailing whitespace                      | `=TRIM(A1)`                     |
| `CONCATENATE`   | Join strings together (comma-separated args)            | `=CONCATENATE(A1, " ", A2)`     |
| `ABS`           | Absolute value                                          | `=ABS(A1)`                      |
| `INT`           | Integer truncation                                      | `=INT(A1)`                      |
| `SQRT`          | Square root                                             | `=SQRT(A1)`                     |
| `ROUND`         | Round to N decimal places                               | `=ROUND(A1, 2)`                 |
| `ROUNDUP`       | Round up                                                | `=ROUNDUP(A1, 2)`               |
| `ROUNDDOWN`     | Round down                                              | `=ROUNDDOWN(A1, 2)`             |
| `CEILING`       | Round up to integer                                     | `=CEILING(A1)`                  |
| `FLOOR`         | Round down to integer                                   | `=FLOOR(A1)`                    |
| `IF`            | Conditional: if cond, return a else b                   | `=IF(A1>5, A2, A3)`             |
| `POWER`         | Raise to a power                                        | `=POWER(A1, 2)`                 |
| `MOD`           | Modulo (remainder)                                      | `=MOD(A1, 3)`                   |
| `PI`            | The constant pi                                         | `=PI()`                         |
| `RAND`          | Random number [0, 1)                                    | `=RAND()`                       |
| `RANDBETWEEN`   | Random integer in [a, b]                                | `=RANDBETWEEN(1, 10)`           |
| `SIN`           | Sine (radians)                                          | `=SIN(A1)`                      |
| `COS`           | Cosine (radians)                                        | `=COS(A1)`                      |
| `TAN`           | Tangent (radians)                                       | `=TAN(A1)`                      |
| `LOG`           | Natural logarithm                                       | `=LOG(A1)`                      |

String arguments can be cell references (A1), string literals (`"hello"`),
or expressions. Functions that return strings (UPPER, LOWER, TRIM, LEFT,
RIGHT, MID, CONCATENATE) produce a string cell value that displays in the
grid. Functions that accept string arguments (LEN, FIND) also accept
numeric values which are converted automatically.

Text functions can be nested naturally: `=LEN(UPPER(A1))` evaluates
to the length of the uppercased value of A1.

### SUMIF / COUNTIF conditions

Condition strings use the same operators as comparisons:

| Condition | Meaning               |
| --------- | --------------------- |
| `>5`      | Greater than 5        |
| `<0`      | Less than 0           |
| `>=10`    | Greater than or equal |
| `<=3`     | Less than or equal    |
| `=42`     | Equal to 42           |
| `<>7`     | Not equal to 7        |
| `5`       | Equal to 5 (plain)    |

SUMIF optionally accepts a third sum_range argument for summing a different
range than the one being tested: `=SUMIF(A1:A10, ">5", B1:B10)` sums the
corresponding B-column cells where A-column cells match the condition.

### Comparison operators

Comparisons return 1 (true) or 0 (false):

| Operator | Meaning               | Example  |
| -------- | --------------------- | -------- |
| `<`      | Less than             | `=A1<5`  |
| `>`      | Greater than          | `=A1>5`  |
| `<=`     | Less than or equal    | `=A1<=5` |
| `>=`     | Greater than or equal | `=A1>=5` |
| `=`      | Equal to              | `=A1=3`  |
| `<>`     | Not equal to          | `=A1<>5` |
| `!=`     | Not equal to (alt.)   | `=A1!=5` |

Comparisons have lower precedence than arithmetic, so `A1+B2>10` parses as
`(A1+B2)>10`. Chained comparisons evaluate left-to-right: `3>2>1` becomes
`(3>2)>1 = 1>1 = 0`.

### Absolute references

Cell references adjust automatically on replicate, insert, and delete.
Use `$` for absolute references: `$A$1` (fixed), `$A1` (fixed column),
`A$1` (fixed row).

## License

MIT
