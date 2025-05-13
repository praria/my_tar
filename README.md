# Welcome to My Tar
*******************

## Description
`my_tar` - A reimplementation of the Unix `tar` utility, written in C.
It creates, lists, extracts, updates, and appends `.tar` archives using low-level system calls only.

## Features
- Creates tar archives (`-cf`)
- List contents of a tar archive (`-tf`)
- Extracts contents (`-xf`)
- Update existing files in archive if newer (`-uf`)
- Append new files to archive (`-rf`)
- Uses only allowed syscalls (no other functions/ system calls are used which are not in this list: chmod, close|(2), free(3), fstat(2), getpwuid, getgrgid, getxattr, listxattr, lseek(2), major, malloc(3), minor, mkdir(2), open(2), opendir, read(2), readdir, readlink, stat(2), lstat(2), symlink, unlink(2), time, ctime, utime, write(2))
- Error handling when a file doesn't exist. It prints the message - my_tar: filename: Cannot stat: No such file or directory

## Installation
```bash
./make            # buids the program my_tar
./make clean      # removes object files
./make fclean     # removes object files and binary
./make re         # cleans and rebuilds the program```


## Usage
```bash
./my_tar -cf archive.tar file1 [file2 ...]      # Create archive
./my_tar -tf archive.tar                        # List archive contents
./my_tar -xf archive.tar                        # Extract archive
./my_tar -uf archive.tar file1 [file2 ...]      # Update files (if newer)
./my_tar -rf archive.tar file1 [file2 ...]      # Append new files


### The Core Team
-- Prakash Shrestha -- 