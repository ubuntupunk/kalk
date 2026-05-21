# kalk

![screenshot](kalk.png)

A minimal spreadsheet for the terminal. No dependencies beyond ncurses.

	$ kalk budget.csv

Inspired by VisiCalc and mostly compatible with it. Reads and writes CSV.
Supports formulas with cell references, basic functions, cell formatting,
row/column operations, and frozen titles. Supports both traditional
(`@SUM(A1...A4)`) and Google Sheets-style (`=SUM(A1:A4)`) syntax.

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
	editor.c     Terminal UI (ncurses)
	kalk.c      Main entry point
	test.c      Unit tests

## Usage

Arrow keys navigate. Type a number or formula to enter data. Formulas
start with `+`, `-`, `(`, `@`, or `=`. Anything else is a label.

Press `/` for commands:

	/B          Blank cell
	/C          Clear sheet
	/DR /DC     Delete row/column
	/IR /IC     Insert row/column
	/F_         Format cell (L R I G D $ % *)
	/GC         Set column width
	/GF_        Set global format
	/M          Move row/column (arrow keys to drag)
	/R          Replicate (copy with relative refs)
	/SL /SS     Load/Save CSV
	/SQ         Save and quit
	/TV/TH/TB/TN  Lock title rows/columns
	/Q          Quit

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

| Function | Description | Example |
|---|---|---|
| `SUM` | Sum of values in range | `=SUM(A1:A10)` |
| `AVERAGE` | Mean of values | `=AVERAGE(A1:A10)` |
| `MIN` | Minimum value | `=MIN(A1:A10)` |
| `MAX` | Maximum value | `=MAX(A1:A10)` |
| `COUNT` | Count of numeric cells | `=COUNT(A1:A10)` |
| `ABS` | Absolute value | `=ABS(A1)` |
| `INT` | Integer truncation | `=INT(A1)` |
| `SQRT` | Square root | `=SQRT(A1)` |
| `ROUND` | Round to N decimal places | `=ROUND(A1, 2)` |
| `ROUNDUP` | Round up | `=ROUNDUP(A1, 2)` |
| `ROUNDDOWN` | Round down | `=ROUNDDOWN(A1, 2)` |
| `CEILING` | Round up to integer | `=CEILING(A1)` |
| `FLOOR` | Round down to integer | `=FLOOR(A1)` |
| `IF` | Conditional: if cond is truthy, return a else b | `=IF(A1>5, A2, A3)` |
| `POWER` | Raise to a power | `=POWER(A1, 2)` |
| `MOD` | Modulo (remainder) | `=MOD(A1, 3)` |
| `PI` | The constant π | `=PI()` |
| `RAND` | Random number [0, 1) | `=RAND()` |
| `RANDBETWEEN` | Random integer in [a, b] | `=RANDBETWEEN(1, 10)` |
| `SIN` | Sine (radians) | `=SIN(A1)` |
| `COS` | Cosine (radians) | `=COS(A1)` |
| `TAN` | Tangent (radians) | `=TAN(A1)` |
| `LOG` | Natural logarithm | `=LOG(A1)` |

### Comparison operators

Comparisons return 1 (true) or 0 (false):

| Operator | Meaning | Example |
|---|---|---|
| `<` | Less than | `=A1<5` |
| `>` | Greater than | `=A1>5` |
| `<=` | Less than or equal | `=A1<=5` |
| `>=` | Greater than or equal | `=A1>=5` |
| `=` | Equal to | `=A1=3` |
| `<>` | Not equal to | `=A1<>5` |
| `!=` | Not equal to (alt.) | `=A1!=5` |

Comparisons have lower precedence than arithmetic, so `A1+B2>10` parses as `(A1+B2)>10`.

Cell references adjust automatically on replicate, insert, and delete.
Use `$` for absolute references: `$A$1` (fixed), `$A1` (fixed column),
`A$1` (fixed row).

## License

MIT
