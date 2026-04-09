#!/bin/bash

# Determine directory of this script
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

if [ -z "$1" ]; then
    echo "Usage: $0 <grammar_file> [parser_method]"
    echo "Example: $0 example_grammar.txt SLR1"
    echo "Parser methods: LR0, SLR1, CLR1, LALR1"
    exit 1
fi

GRAMMAR_FILE="$1"
METHOD="${2:-SLR1}"

# Extract base name
BASENAME=$(basename "$GRAMMAR_FILE" | cut -d. -f1)
OUT_FILE="${DIR}/out_${BASENAME}.json"

echo "Running $METHOD parser on $GRAMMAR_FILE..."

# Pipe the grammar file to the executable
cat "$GRAMMAR_FILE" | "$DIR/cpp/bin/lr_lab.exe" "$METHOD" > "$OUT_FILE"

if [ $? -ne 0 ]; then
    echo "Error running the parser. Please check if the C++ executable is built."
    exit 1
fi

echo "Saved JSON output to $OUT_FILE"
echo "Opening $OUT_FILE..."

# Detect OS and open file
if command -v start &> /dev/null; then
    start "$OUT_FILE" # Windows (Git Bash/MSYS)
elif command -v xdg-open &> /dev/null; then
    xdg-open "$OUT_FILE" # Linux
elif command -v open &> /dev/null; then
    open "$OUT_FILE" # macOS
else
    echo "Could not detect program to open the file. It is saved as $OUT_FILE."
fi
