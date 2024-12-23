#!/bin/bash

# Check if the required arguments are provided
if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <directory>"
    exit 1
fi

# Check if the specified directory exists
if [ ! -d "$1" ]; then
    echo "Error: Directory '$1' does not exist."
    exit 1
fi

# Get the directory to be archived and the output archive name
directory="$(cd $1; pwd)"
output_archive="$(basename $directory).tar.gz"

# Create the tar archive without the outer directory
tar -czf "$output_archive" -C "$directory" src CMakeLists.txt
echo "Archive created: $output_archive"