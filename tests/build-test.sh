#!/bin/sh

test_exe=$1
test_dir=$2

# Loop through all the input files
find "$test_dir" -type f -name "test-*.in" | while read -r infile; do
    # Extract the base name without the extension
    base_name=$(basename "$infile" .in)

    # Define the output file
    outfile=$test_dir/$base_name.out

    # Run lex on the input file
    printf '%s' "$(cat "$infile")" | "$test_exe" > "$outfile"
done
