# kalk — Google Sheets Feature Parity

Progress tracking for making kalk more like Google Sheets.

## Current state

**14 functions implemented:** SUM, AVERAGE, MIN, MAX, COUNT, ABS, INT, SQRT, ROUND, ROUNDUP, ROUNDDOWN, CEILING, FLOOR, IF
**3 syntaxes:** `@FUNC(...)`, `=FUNC(...)`, `FUNC(...)`
**2 range separators:** `...` and `:`
**Multi-file split:** cell.h, cell.c, formula.c, csv.c, editor.c, kalk.c, test.c — ~1,500 lines total

---

## Task list

### Phase 1: Formula parser improvements (high-impact basics)

- [x] **Comparison operators** — `<`, `>`, `<=`, `>=`, `=`, `<>`, `!=` for `IF(A1>5, ...)` and general use
- [x] **More math functions` — `POWER`, `MOD`, `PI`, `RAND`, `RANDBETWEEN`, `SIN`, `COS`, `TAN`, `LOG`

### Phase 2: Analytical functions (high-value spreadsheet features)

- [x] **SUMIF / COUNTIF** — conditional sum/count over a range with comparison operators
- [x] **VLOOKUP / HLOOKUP** — exact-match lookup in a table range
- [x] **SUM with multiple arguments** — `SUM(A1, B1, C1)` (comma-separated lists for SUM/AVERAGE/MIN/MAX/COUNT)

### Phase 3: String/type system (foundational)

- [x] **String cell values** — store strings that can be used in formulas
- [x] **Text functions** — `LEN`, `LEFT`, `RIGHT`, `MID`, `UPPER`, `LOWER`, `TRIM`, `FIND`, `CONCATENATE`

### Phase 4: Data operations

- [x] **Data sorting** — `/O` command to sort rows by a column
- [x] **Auto-fill** — replicate patterns (1, 2, 3... or Mon, Tue, Wed...) via `/A`
- [x] **Cell colors** — `/F` C set cell color (0-7), `/F` N set conditional formatting rule

### Phase 5: Polish & UX

- [x] **Formula autocomplete** — show function suggestions while typing (`/E` entry mode)
- [x] **Cell formatting: date/time** — `'T'` format displays dates from serial numbers; `DATE()`, `NOW()`, `TODAY()` functions
- [x] **Multi-sheet support** — tab bar at bottom, `/N` `/P` next/prev sheet, `/W E` new sheet, `/W R` rename, `/W D` delete

---

## Future phases

### Phase 6: Multi-Sheet Enhancements

- [x] **Copy/paste across sheets** — clipboard-based yank (`/Y`), cut (`/X`), paste (`/V`); works across sheets
- [x] **Cross-sheet references** — `Sheet2!A1` syntax for formula references to other sheets
- [x] **Sheet reordering** — `/W L` / `/W R` to move tabs left/right
- [x] **Sheet colors** — `/W C` to assign a tab color (0-7)
- [x] **CSV per sheet** — each sheet has its own `filename` field; save/load per sheet
- [x] **Import sheet from CSV** — `/W I` loads a CSV into a new sheet
- [x] **Sheet navigation popup** — `/E` shows all sheets with numbered list for quick switching

### Phase 7: Date/Time System

- [x] **Time component in dates** — show `HH:MM` alongside dates; store fractional days for time
- [x] **More date functions** — `YEAR()`, `MONTH()`, `DAY()`, `HOUR()`, `MINUTE()`, `SECOND()`, `WEEKDAY()`, `DATEDIF()`
- [x] **Auto-detect date input** — when the user types "2024-01-15" or "1/15/2024", auto-set format to `'T'`
- [x] **Date arithmetic** — adding days to a date (e.g. `A1+7` shifts a date by 7 days)
- [x] **Custom date formats** — `YYYY-MM-DD`, `MM/DD/YYYY`, `DD-Mon-YYYY`, etc.
- [x] **Date picker popup** — a mini calendar UI to pick a date when in date-formatted cell
- [x] **Duration/time formatting** — `HH:MM:SS` display via `/F S` format

### Phase 8: Formula Autocomplete Enhancements

- [x] **Nested autocomplete** — show suggestions for functions inside expressions: `IF(ISBLANK(A1), SUM(`
- [x] **Argument hints** — show expected arguments for the selected function (e.g. `SUM(number1, [number2], ...)`)
- [x] **Function description tooltip** — brief description of what the function does
- [x] **Fuzzy matching** — match partial function names (e.g. "rou" matches ROUND, ROUNDUP, ROUNDDOWN)
- [x] **Cell range autocomplete** — suggest adjacent cell ranges after typing `SUM(`
- [x] **History-based suggestions** — rank recently used functions higher
- [x] **Multi-line popup with scrolling** — if many matches, allow scrolling through them

### Phase 9: Date Arithmetic, Duration Format, Range Autocomplete

- [x] **Date arithmetic** — `A1+7` on a date cell displays as shifted date via format propagation in `recalc()`
- [x] **Duration format** — `/F S` to display cells as `HH:MM:SS` (e.g., 1.5 → `36:00:00`)
- [x] **Cell range autocomplete** — suggest adjacent ranges after typing `SUM(` in the autocomplete popup

### Phase 10: Undo/Redo

- [x] **Undo/Redo stack** — `struct undocell` + `struct undoentry` snapshot system, 100-entry stack with 4096 cells max per entry
- [x] **Swap-based undo/redo** — `undo_perform`/`redo_perform` swap current↔stored cells; no separate redo stack needed
- [x] **Instrumented operations** — insert/delete row/col, sort, autofill, replicate, paste, cut, and cell edits all record undo snapshots
- [x] **Ctrl+Z / Ctrl+Y** — keyboard shortcuts work in both navigation loop and entry mode
- [x] **Recursion guard** — `in_undo` flag prevents re-entrant undo calls during `recalc()`
- [x] **Redo stack invalidation** — new edit after undo clears redo entries

---

## Legend

- [ ] Not started
- [~] In progress
- [x] Done
