# kalk

![screenshot](kalk.png)

A minimal spreadsheet for the terminal. No dependencies beyond ncurses.

    $ kalk budget.csv

Inspired by VisiCalc and mostly compatible with it. Reads and writes CSV.
Supports formulas with cell references, 47 functions (including text
functions like LEN, UPPER, CONCATENATE, date/time functions like DATE,
DATEDIF, WEEKDAY), cell formatting with colors and bold/italic text,
conditional formatting, row/column operations, data sorting, cell
replication with relative reference adjustment, frozen titles,
multi-sheet support with cross-sheet references, undo/redo (Ctrl+Z/Y),
and formula autocomplete with fuzzy matching. Supports both traditional
(`@SUM(A1...A4)`) and Google Sheets-style (`=SUM(A1:A4)`) syntax.

This is the breakout Power Sheets version, with extra features, if you want the simple VisiCalc 500 lines in one file version,
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
    /E            Sheet explorer (list all sheets, pick by number)
    /IR /IC       Insert row/column
    /F_           Format cell (L R I G D $ % * T U u t S)
    /FC           Set foreground color (0=blk 1=Red 2=Grn 3=Yel 4=Blu 5=Mag 6=Cyn 7=Wht)
    /FB           Set background color (same 0-7 palette)
    /FO           Set text attribute (0=none 1=Bold 2=Italic 3=Both)
    /FN           Set conditional format rule (e.g. >5, <0, =10, <>7)
    /FP           Date picker popup (mini calendar)
    /FX           Clear all cell colors/attributes/condition
    /GC           Set column width
    /GF_          Set global format
    /M            Move row/column (arrow keys to drag)
    /N            Next sheet
    /O            Sort rows by column (type column letter, e.g. A)
    /P            Previous sheet
    /R            Replicate (copy with relative refs)
    /SL /SS       Load/Save CSV
    /SQ           Save and quit
    /TV/TH/TB/TN  Lock title rows/columns
    /V            Paste cells from clipboard
    /WE           New sheet
    /WR           Rename current sheet
    /WD           Delete current sheet
    /WI           Import CSV into a new sheet
    /WL /WR       Move sheet tab left/right
    /WC           Set sheet tab color
    /X            Cut cells (yank + clear)
    /Y            Yank (copy) cells to clipboard
    /Q            Quit

Cross-sheet references: `Sheet2!A1` in formulas references cell A1 on Sheet2.

Copy/paste across sheets: yank (`/Y`) or cut (`/X`) cells, navigate to another sheet, paste (`/V`).

Other keys:

    >           Go to cell (type reference)
    !           Force recalculation
    "           Enter label
    Backspace   Clear cell
    Tab         Next column / cycle autocomplete suggestions
    Enter       Next row / accept autocomplete suggestion
    Up/Down     Navigate autocomplete suggestions
    Home        Jump to A1
    Ctrl-C      Quit
    Ctrl-Z      Undo
    Ctrl-Y      Redo

## Formulas

### Traditional syntax

Arithmetic: `+A1*B2-3`, `(A1+A2)/2`

Functions: `@SUM(A1...A10)`, `@ABS(x)`, `@INT(x)`, `@SQRT(x)`

### Google Sheets-style syntax

`=SUM(A1:A10)` — prefix with `=` and use `:` for ranges.

Functions can also be used without `@` or `=`: `SUM(A1:A10)`.

The two syntaxes are fully compatible and can be mixed.

### Functions

| Function      | Description                                           | Example                     |
| ------------- | ----------------------------------------------------- | --------------------------- |
| `SUM`         | Sum of values (range or comma-separated)              | `=SUM(A1:A10)`              |
| `AVERAGE`     | Mean of values (range or comma-separated)             | `=AVERAGE(A1:A10)`          |
| `MIN`         | Minimum value (range or comma-separated)              | `=MIN(A1:A10)`              |
| `MAX`         | Maximum value (range or comma-separated)              | `=MAX(A1:A10)`              |
| `COUNT`       | Count of numeric cells (range or comma-separated)     | `=COUNT(A1:A10)`            |
| `SUMIF`       | Sum range cells matching a condition                  | `=SUMIF(A1:A10, ">5")`      |
| `COUNTIF`     | Count range cells matching a condition                | `=COUNTIF(A1:A10, "<>0")`   |
| `VLOOKUP`     | Search column for key, return from indexed column     | `=VLOOKUP(5, A1:B10, 2)`    |
| `HLOOKUP`     | Search row for key, return from indexed row           | `=HLOOKUP(5, A1:Z10, 3)`    |
| `LEN`         | Length of a string                                    | `=LEN(A1)`                  |
| `FIND`        | Position of substring (1-indexed, NAN if not found)   | `=FIND("abc", A1)`          |
| `LEFT`        | First N characters of a string                        | `=LEFT(A1, 3)`              |
| `RIGHT`       | Last N characters of a string                         | `=RIGHT(A1, 3)`             |
| `MID`         | Substring starting at position (1-indexed)            | `=MID(A1, 2, 3)`            |
| `UPPER`       | Convert to uppercase                                  | `=UPPER(A1)`                |
| `LOWER`       | Convert to lowercase                                  | `=LOWER(A1)`                |
| `TRIM`        | Remove leading/trailing whitespace                    | `=TRIM(A1)`                 |
| `CONCATENATE` | Join strings together (comma-separated args)          | `=CONCATENATE(A1, " ", A2)` |
| `ABS`         | Absolute value                                        | `=ABS(A1)`                  |
| `INT`         | Integer truncation                                    | `=INT(A1)`                  |
| `SQRT`        | Square root                                           | `=SQRT(A1)`                 |
| `ROUND`       | Round to N decimal places                             | `=ROUND(A1, 2)`             |
| `ROUNDUP`     | Round up                                              | `=ROUNDUP(A1, 2)`           |
| `ROUNDDOWN`   | Round down                                            | `=ROUNDDOWN(A1, 2)`         |
| `CEILING`     | Round up to integer                                   | `=CEILING(A1)`              |
| `FLOOR`       | Round down to integer                                 | `=FLOOR(A1)`                |
| `IF`          | Conditional: if cond, return a else b                 | `=IF(A1>5, A2, A3)`         |
| `POWER`       | Raise to a power                                      | `=POWER(A1, 2)`             |
| `MOD`         | Modulo (remainder)                                    | `=MOD(A1, 3)`               |
| `PI`          | The constant pi                                       | `=PI()`                     |
| `RAND`        | Random number [0, 1)                                  | `=RAND()`                   |
| `RANDBETWEEN` | Random integer in [a, b]                              | `=RANDBETWEEN(1, 10)`       |
| `SIN`         | Sine (radians)                                        | `=SIN(A1)`                  |
| `COS`         | Cosine (radians)                                      | `=COS(A1)`                  |
| `TAN`         | Tangent (radians)                                     | `=TAN(A1)`                  |
| `LOG`         | Natural logarithm                                     | `=LOG(A1)`                  |
| `DATE`        | Create date from year, month, day                     | `=DATE(2024, 1, 15)`        |
| `TODAY`       | Current date                                          | `=TODAY()`                  |
| `NOW`         | Current date and time                                 | `=NOW()`                    |
| `YEAR`        | Extract year from a date serial                       | `=YEAR(A1)`                 |
| `MONTH`       | Extract month (1-12) from a date serial               | `=MONTH(A1)`                |
| `DAY`         | Extract day of month (1-31) from a date serial        | `=DAY(A1)`                  |
| `HOUR`        | Extract hour (0-23) from a date serial                | `=HOUR(A1)`                 |
| `MINUTE`      | Extract minute (0-59) from a date serial              | `=MINUTE(A1)`               |
| `SECOND`      | Extract second (0-59) from a date serial              | `=SECOND(A1)`               |
| `WEEKDAY`     | Day of week (1=Sun..7=Sat); optional type arg         | `=WEEKDAY(A1, 2)`           |
| `DATEDIF`     | Difference between dates ("Y"/"M"/"D"/"MD"/"YM"/"YD") | `=DATEDIF(A1, A2, "D")`     |

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

### Date/time system

Date serials are days since 1970-01-01. Typing a date like `2024-01-15`
or `1/15/2024` auto-detects the format and stores a serial number.
Date arithmetic works naturally: `=A1+7` on a date cell shifts it by 7
days. Use `/F P` to open a date picker popup for visual selection.

Cell formats for date/time display:

- `T` — YYYY-MM-DD (includes time if present)
- `U` — MM/DD/YYYY
- `u` — DD-Mon-YYYY (e.g. 15-Jan-2024)
- `t` — HH:MM:SS (time only)
- `S` — HH:MM:SS (duration, e.g. 1.5 displays as 36:00:00)

### Formula autocomplete

While typing a formula, a popup shows matching function names with
fuzzy matching (e.g. "rou" matches ROUND, ROUNDUP, ROUNDDOWN).
Use Tab or Up/Down to cycle, Enter to accept. The popup shows each
function's signature (argument hints) and description. Recently used
functions appear higher. Adjacent cell ranges are also suggested.

### Undo / Redo

Press Ctrl+Z to undo the last operation (cell edit, insert/delete row/col,
sort, paste, cut, replicate, autofill, format change, etc.). Press Ctrl+Y
to redo. Redo is invalidated after a new edit. 100 undo levels are stored
with up to 4096 cells per entry.

### Auto-fill patterns

The `/A` command detects and extends several pattern types from seed cells:

- **Linear arithmetic**: 1, 2, 3 → 4, 5, 6... or 10, 20, 30 → 40, 50...
- **Day names**: Mon, Tue, Wed... or Monday, Tuesday...
- **Month names**: Jan, Feb, Mar... or January, February...
- **Copy/tile**: any other pattern repeats the seed cells cyclically

## License

MIT
