#!/bin/sh

TEST_DIR="tests"

# Loop through all the input files
find "$TEST_DIR" -type f -name "test-*.in" | while read -r infile; do
    # Extract the base name without the extension
    base_name=$(basename "$infile" .in)

    # Define the output file
    outfile="$TEST_DIR/$base_name.out"
    resfile="$TEST_DIR/$base_name.res"
    errfile="$TEST_DIR/$base_name.err"

    # Run lex on the input file
    printf "%s" "$(cat "$infile")" | ./lex > "$resfile"

    # Compare lex output with the content of the corresponding .out file
    diff "$outfile" "$resfile" > "$errfile"
    if [ $? -eq 0 ]; then
        printf "Test %s: Passed\n" "$base_name"
        rm "$resfile" "$errfile"
    else
        printf "Test %s: Failed, see %s\n" "$base_name" "$errfile"
    fi
done
