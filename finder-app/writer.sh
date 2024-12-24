#!/bin/sh
# writer.sh - Script to write a string to a specified file
# Author: Joseph Garwood

# Check if the required arguments are provided
if [ "$#" -lt 2 ]; then
    echo "Error: Missing arguments."
    echo "Usage: $0 <file-path> <write-string>"
    exit 1
fi

# Assign arguments to variables
writefile=$1
writestr=$2

# Create the directory path if it doesn't exist
mkdir -p "$(dirname "$writefile")"

# Attempt to create/write to the file
if ! echo "$writestr" > "$writefile"; then
    echo "Error: Could not write to file $writefile."
    exit 1
fi

# Exit successfully
exit 0
