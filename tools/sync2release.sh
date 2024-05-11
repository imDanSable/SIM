#!/bin/bash

# Function to show usage
usage() {
    echo "Usage: $0 [-d] [-v] <src> <trg>"
    echo "  -d  Dry run mode (pass to rsync as --dry-run)"
    echo "  -v  Verbose mode with detailed output"
    exit 1
}

# Initialize flags
dry_run_flag=""
verbose_flag=""
hardcoded_flags="-i --delete --checksum"

# Parse optional flags
while getopts "dv" opt; do
    case $opt in
        d) dry_run_flag="--dry-run" ;;
        v) verbose_flag="-v" ;;
        *) usage ;;
    esac
done
shift $((OPTIND - 1))

# Check for correct number of remaining arguments
if [ "$#" -ne 2 ]; then
    usage
fi

# Assign arguments to variables
src="$1"
trg="$2"

# Define the directories and files to sync
directories=("src" "res" "dep" ".github")
files=("README.md" "CHANGELOG.md" "plugin.json")

# Define directories to ignore
ignore_dirs=("test") 

# Create rsync exclude patterns for ignored directories
exclude_patterns=""
for ignore in "${ignore_dirs[@]}"; do
    exclude_patterns+="--exclude=$ignore "
done


colorize() {
    grep -vE '^\.(f|d)..t' | sed -E \
        -e 's/^>f\+.*$/\x1b[32m&\x1b[0m/' \
        -e 's/^cd\+.*$/\x1b[32m&\x1b[0m/' \
        -e 's/^>f.st.*$/\x1b[33m&\x1b[0m/' \
        -e 's/^\*deleting.*$/\x1b[31m&\x1b[0m/' # Red for deletions
}

# Iterate over each directory and perform the rsync
for dir in "${directories[@]}"; do
    echo -e "\n\n\e[1;37mPROCESSING DIRECTORY: $dir\e[0m"
    echo "===================="
    rsync -a $dry_run_flag $verbose_flag $hardcoded_flags $exclude_patterns "$src/$dir/" "$trg/$dir/" | colorize
done

# Sync specific files directly
echo -e "\n\n\e[1;37mNOW PROCESSING FILES...\e[0m"
echo "===================="
for file in "${files[@]}"; do
    echo -e "\e[1;37mProcessing file: $file\e[0m"
    rsync -a $dry_run_flag $verbose_flag $hardcoded_flags "$src/$file" "$trg/$file" | colorize
done
