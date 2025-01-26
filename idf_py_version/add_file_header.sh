#!/bin/bash

# This script finds all .c and .h files, excluding the 'build' directory,
# and adds a comment at the top with the file's relative path.

# Find all .c and .h files starting from the current directory, excluding 'build' directory
find . -path ./build -prune -o -type f \( -name "*.c" -o -name "*.h" \) -print | while IFS= read -r file; do
    # Get the relative path by removing the leading './'
    filepath="${file#./}"
    # Escape slashes and special characters for sed
    escaped_filepath=$(printf '%s\n' "$filepath" | sed 's/[\/&]/\\&/g')
    # Check if the file already contains the comment
    if ! grep -q "^/\* $escaped_filepath \*/" "$file"; then
        # Insert the comment at the top of the file
        sed -i '' "1s;^;/* $filepath */\n\n;" "$file"
        echo "Added header to $filepath"
    else
        echo "Header already exists in $filepath"
    fi
done

