# SnapTracker

SnapTracker is a C project designed to help you keep track of your files by creating snapshot files that store metadata about each file. The project supports both macOS and Linux, and includes a bash script to identify and handle potentially dangerous files. This project was developed as an individual university project.

## Table of Contents

1. [Features](#features)
2. [How It Works](#how-it-works)
3. [Usage](#usage)
    - [Compilation](#compilation)
    - [Running SnapTracker](#running-snaptracker)
    - [Example Commands](#example-commands)
    - [Bash Script](#bash-script)
4. [Project Structure](#project-structure)

## Features

- Create snapshot files for specified files and directories.
- Metadata in snapshots includes file path, name, size, last modified time, inode, and permissions.
- Use of processes and pipes to enhance performance.
- Customizable output file for snapshots.
- Isolation of potentially dangerous files based on specific criteria.

## How It Works

When you run SnapTracker with some files or directories as arguments, the program generates snapshot files for all the specified files. A snapshot file contains metadata that helps you track changes to your files over time, similar to version control systems like Git.

### Example Snapshot File
```text
Path: directory/file
Name: file
Size: 12 bytes
Last Modified: 2024-04-07 06:57:08
Inode: 50475123
Permissions: rw-r--r--
```

## Usage

To use SnapTracker, you need to compile the C files using `gcc`. It's important to note that in macOS, gcc refers to Apple Clang. Therefore, there are separate source files for macOS and Linux. Additionally, the project includes a bash script for identifying potentially dangerous files.

### Compilation

Compile the project using the following command:
```sh
gcc -s isolated_directory example-directory your_source_file.c
```
Replace your_source_file.c with the appropriate source file for your operating system (main.c or main_linux.c).

### Running SnapTracker

Run the program with the following arguments:

- `-o output_directory`: Specifies the output directory where all snapshots will be created. This argument must be the first if present.
- `-s isolated_directory`: Specifies the directory where all potentially dangerous files will be sent. This argument follows the output argument if present.
- Up to 10 directories: Specifies the directories for which snapshots will be created.

Example command:
```sh
./snaptracker -o output -s isolated dir1 dir2 dir3
```

### Example Commands

<details>
  <summary>Example Commands</summary>
  
  1. With output and isolated file arguments:
     ```sh
     ./snaptracker -o output-directory -s isolated-directory dir1 dir2
     ```

  2. With only the isolated file argument:
     ```sh
     ./snaptracker -s isolated-directory dir1 dir2
     ```

</details>

### Bash Script

The included bash script identifies potentially dangerous files, that have no permissions, based on the following criteria:
- Files containing non-ASCII characters.
- Files containing specific words, like `dangerous`, `malicious`, `corrupted`, etc.
- Files with a lot of characters in a few lines.

The script moves these files to the file specified with the -s argument.

### Project Structure

- `main.c`: Source code for macOS.
- `main_linux.c`: Source code for Linux.
- `analyze_file.sh`: Bash script for identifying dangerous files.
