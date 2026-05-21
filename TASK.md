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

- [ ] **String cell values** — store strings that can be used in formulas
- [ ] **Text functions** — `LEN`, `LEFT`, `RIGHT`, `MID`, `UPPER`, `LOWER`, `TRIM`, `FIND`, `CONCATENATE`

### Phase 4: Data operations

- [x] **Data sorting** — `/O` command to sort rows by a column
- [ ] **Auto-fill** — replicate patterns (1, 2, 3... or Mon, Tue, Wed...) via `/R`
- [x] **Cell colors** — `/F` C set cell color (0-7), `/F` N set conditional formatting rule

### Phase 5: Polish & UX

- [ ] **Formula autocomplete** — show function suggestions while typing
- [ ] **Cell formatting: date/time** — display timestamps
- [ ] **Multi-sheet support** — tabs like Google Sheets workbook

---

## Legend

- [ ] Not started
- [~] In progress
- [x] Done
