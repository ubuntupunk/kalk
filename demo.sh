#!/usr/bin/env bash
# kalk - Interactive Demo Script
#
# This script builds kalk and opens it with a feature-rich demo spreadsheet.
#
# For an asciinema recording:
#   asciinema rec demo.cast -c "./demo.sh"
#
set -euo pipefail
cd "$(dirname "$0")"

echo "=== kalk Demo ==="
echo ""
echo "Features you can explore:"
echo "  • Formulas with arithmetic (+, -, *, /)"
echo "  • 47 functions: SUM, AVERAGE, COUNTIF, VLOOKUP, text functions"
echo "  • Date auto-detection and date arithmetic"
echo "  • Date functions: TODAY, DATEDIF, YEAR, MONTH, DAY, WEEKDAY"
echo "  • Text functions: UPPER, LOWER, TRIM, CONCATENATE, LEN"
echo "  • Conditional formatting and cell colors (/F C, /F B, /F N)"
echo "  • Data sorting with /O"
echo "  • Undo/Redo with Ctrl+Z / Ctrl+Y"
echo "  • Formula autocomplete while typing"
echo "  • Multi-sheet support (/W E, /N, /P)"
echo "  • Date picker popup (/F P)"
echo ""
echo "Press any key to start kalk with demo.csv..."
read -n 1 -s

# Build if needed
if [ ! -f kalk ]; then
  echo "Building kalk..."
  make clean 2>/dev/null || true
  make 2>&1
fi

echo ""
echo "Opening kalk with demo.csv..."
echo "Press / for commands, arrow keys to navigate, type to enter data."
echo "Press Ctrl+C to quit."
echo ""
sleep 1

exec ./kalk demo.csv
