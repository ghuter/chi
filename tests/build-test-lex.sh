#!/bin/sh

TEST_DIR="tests"

# Loop through all the input files
find "$TEST_DIR" -type f -name "test-*.in" | while read -r infile; do
    # Extract the base name without the extension
    base_name=$(basename "$infile" .in)

    # Define the output file
    outfile="$TEST_DIR/$base_name.out"

    # Run lex on the input file
    printf "%s" "$(cat "$infile")" | ./lex > "$outfile"
done
