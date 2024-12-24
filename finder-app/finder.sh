#!/bin/sh
# finder.sh - Script to count files and matching lines in a directory
# Author: Joseph Garwood

# Exit if fewer than two arguments are provided
if [ "$#" -lt 2 ]; then
    echo "Error: Missing arguments."
    echo "Usage: $0 <directory-path> <search-string>"
    exit 1
fi

# Assign arguments to variables
filesdir=$1
searchstr=$2

# Check if the first argument is a valid directory
if [ ! -d "$filesdir" ]; then
    echo "Error: $filesdir is not a valid directory."
    exit 1
fi

# Count the number of files in the directory and subdirectories
file_count=$(find "$filesdir" -type f | wc -l)

# Count the number of matching lines containing the search string
match_count=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)

# Print the results
echo "The number of files are $file_count and the number of matching lines are $match_count"

# Exit successfully
exit 0
