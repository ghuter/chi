#!/bin/sh

test_dir=tests

# Loop through all the input files
find "$test_dir" -type f -name "test-*.in" | while read -r infile; do
    # Extract the base name without the extension
    base_name=$(basename "$infile" .in)

    # Define the output file
    outfile=$test_dir/$base_name.out

    # Run lex on the input file
    printf '%s' "$(cat "$infile")" | ./lexer > "$outfile"
done
