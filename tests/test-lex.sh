#!/bin/sh

test_dir=tests

# Loop through all the input files
find "$test_dir" -type f -name "test-*.in" | while read -r infile; do
    # Extract the base name without the extension
    base_name=$(basename "$infile" .in)

    # Define the output file
    outfile=$test_dir/$base_name.out
    resfile=$test_dir/$base_name.res
    errfile=$test_dir/$base_name.err

    # Run lex on the input file
    printf '%s' "$(cat "$infile")" | ./lexer > "$resfile"

    # Compare lex output with the content of the corresponding .out file
    diff "$outfile" "$resfile" > "$errfile"
    if [ $? -eq 0 ]; then
        printf 'Test %s: Passed\n' "$base_name"
        rm "$resfile" "$errfile"
    else
        printf 'Test %s: Failed, see %s\n' "$base_name" "$errfile"
    fi
done
