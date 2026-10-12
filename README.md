# Midterm Project: Implementing a Simplified `ls(1)`

## Project Information

- **Course:** Systems Programming
- **Project:** Midterm Project — Implement `ls(1)`
- **Student:** Nguyen Huu Quoc Bao
- **Student ID:** 24IT019
- **Programming language:** C
- **Target environment:** Linux / POSIX-compatible systems
- **Reference manual:** The NetBSD `ls(1)` manual provided for this assignment (`ls.pdf`)
- **GitHub repository:** https://github.com/baonhq1406/NguyenHuuQuocBao_24IT019_midterm

## 1. Project Overview

This project implements a simplified version of the UNIX `ls` command in C. The program lists files and directories and supports the subset of command-line options specified in the supplied `ls(1)` manual.

The executable is named `myls` so that it does not conflict with the system-provided `ls` command. The implementation obtains file and directory information through filesystem interfaces such as `opendir()`, `readdir()`, `lstat()`, `stat()`, `readlink()`, and `closedir()`. It does not invoke the system `ls` command to produce its output.

The project is divided into multiple source files and a shared header file. A `Makefile` is provided to compile the program, and a shell-based smoke test checks common features.

## 2. Implemented Options

The supported command-line syntax is:

```text
myls [-AacdFfhiklnqRrSstuw] [file ...]
```

| Option | Description |
|---|---|
| `-A` | Include entries whose names begin with a dot, excluding `.` and `..`. This is the default hidden-file mode when the program is run by the superuser, unless another hidden-file option is specified. |
| `-a` | Include all entries, including `.` and `..`. |
| `-c` | Use the file status-change time instead of modification time for `-t` sorting and long-format timestamps. The last of `-c` and `-u` determines the selected time. |
| `-d` | List directory operands as entries instead of listing their contents. The most recently specified option between `-d` and `-R` determines whether recursive directory listing is enabled. |
| `-F` | Append `/` to directories, `*` to executable regular files, `@` to symbolic links, `=` to sockets, and `\|` to FIFOs. Linux does not provide the NetBSD whiteout file type through normal file metadata, so the `%` marker is not generated. |
| `-f` | Disable sorting. |
| `-h` | Display sizes in a human-readable format for `-s` and `-l`. The last of `-h` and `-k` determines the size mode. |
| `-i` | Display the inode number for each entry. |
| `-k` | Display `-s` block counts in 1024-byte units. The last of `-h` and `-k` determines the size mode. |
| `-l` | Use long format, including file type and permissions, link count, owner, group, size, timestamp, and pathname. |
| `-n` | Use long format and display numeric user and group IDs instead of resolving names. The last of `-l` and `-n` determines whether IDs are numeric. |
| `-q` | Replace non-printable filename characters with `?`. |
| `-R` | Recursively list subdirectories. Symbolic links are not followed during recursive descent. The most recently specified option between `-R` and `-d` determines whether recursion is enabled. |
| `-r` | Reverse the selected sort order. |
| `-S` | Sort entries by size, largest first. |
| `-s` | Display allocated filesystem space in blocks. The default block unit is 512 bytes; `BLOCKSIZE` may change the unit unless overridden by `-h` or `-k`. |
| `-t` | Sort by the selected file timestamp, newest first. The default timestamp is modification time. |
| `-u` | Use the last-access time for `-t` sorting and long-format timestamps. The last of `-c` and `-u` determines the selected time. |
| `-w` | Print non-printable filename characters without replacing them. The last of `-q` and `-w` determines the output mode. |

When no operand is given, the program lists the current directory. For multiple operands, non-directory operands are processed before directory operands; each group is sorted separately unless sorting is disabled with `-f`. By default, entries are printed one per line.

## 3. Project Structure

```text
NguyenHuuQuocBao_24IT019_midterm/
├── Makefile
├── README.md
├── .gitignore
├── include/
│   └── ls.h
├── src/
│   ├── main.c
│   ├── options.c
│   ├── util.c
│   ├── format.c
│   └── listing.c
└── tests/
    └── smoke_test.sh
```

### Source file responsibilities

- **`src/main.c`** — Initializes locale settings, parses command-line options, and starts the listing operation.
- **`src/options.c`** — Initializes option defaults and parses the supported flags.
- **`src/listing.c`** — Processes operands, reads directory entries, sorts entries, handles recursive traversal, and reports filesystem errors.
- **`src/format.c`** — Formats normal and long listings, including permissions, ownership, sizes, timestamps, suffixes, and symbolic-link targets.
- **`src/util.c`** — Provides utility functions for path construction, safe filename printing, human-readable sizes, block counts, timestamp selection, file-type classification, and symbolic-link reading.
- **`include/ls.h`** — Defines shared data structures, option types, and function declarations.
- **`tests/smoke_test.sh`** — Runs automated smoke tests against common listing behaviors.
- **`Makefile`** — Builds the executable, runs tests, and removes generated build files.
- **`.gitignore`** — Excludes the compiled executable and object files from version control.

## 4. Build Requirements

The project is intended for a Linux/POSIX environment and requires:

- A C compiler such as GCC or Clang.
- GNU Make or a compatible `make` utility.
- Standard POSIX/Linux filesystem interfaces used by the implementation.

## 5. Build and Run

Open a terminal and change to the project root directory.

### Compile

```sh
make
```

The build creates an executable named `myls` in the project root.

### Examples

List the current directory:

```sh
./myls
```

Include hidden entries:

```sh
./myls -a
```

Display a long listing of `/etc`:

```sh
./myls -l /etc
```

Display inode numbers and allocated block counts:

```sh
./myls -i -s .
```

Append file-type indicators:

```sh
./myls -F
```

Recursively list a directory:

```sh
./myls -R /tmp
```

Display numeric owner and group IDs:

```sh
./myls -n /etc/passwd
```

Sort by modification time or size:

```sh
./myls -t
./myls -S
```

Treat a directory operand as an entry:

```sh
./myls -d /tmp
```

### Run tests

```sh
make test
```

The smoke test creates a temporary directory containing ordinary and hidden files, an executable file, a symbolic link, and a subdirectory. It checks common options including hidden-file listing, long format, symbolic-link output, numeric IDs, inode and block output, filename suffixes, recursion, directory-as-entry behavior, character-printing modes, sorting, and size formatting.

The project was built on Ubuntu with `make`, and the provided test target completed with the message `smoke tests passed`. These smoke tests cover common cases but are not an exhaustive proof of exact compatibility with every behavior of the NetBSD manual.

### Clean generated files

```sh
make clean
```

This removes the `myls` executable and generated `.o` object files.

## 6. Implementation Notes and Platform Differences

- Directory entries are read using directory APIs, and file metadata is obtained with `lstat()` so that symbolic links can be identified without dereferencing them when they are listed as entries.
- Recursive traversal skips symbolic links, preventing recursive descent through linked directories and reducing the risk of symlink loops.
- Long format displays file type and permission bits, link count, owner, group, size, timestamp, and pathname. Numeric IDs are used with `-n` or when an owner/group name cannot be resolved.
- File sizes for `-s` are based on allocated blocks reported by Linux metadata. Linux reports `st_blocks` in 512-byte units, and `BLOCKSIZE` is considered for block-count formatting.
- The reference manual is for NetBSD. NetBSD-specific file types such as whiteouts and archive-state entries are not available through normal Linux `stat` metadata, so those type markers cannot be reproduced on this target platform.
- Human-readable sizes use binary units (`B`, `K`, `M`, `G`, and larger units).
- This is a simplified implementation of the specified option subset, not a complete replacement for every feature of every system's `ls` command.

## 7. Error Handling and Exit Status

The program reports command-line option errors and filesystem errors to standard error. For example, if a path cannot be accessed or a directory cannot be read, an explanatory message is printed. The program returns a non-zero status when option parsing or a listing operation fails; successful execution returns zero.

## 8. Repository and Submission

The project source code is hosted at the following GitHub repository:

**https://github.com/baonhq1406/NguyenHuuQuocBao_24IT019_midterm**

The repository contains the C source files, header file, `Makefile`, this report, `.gitignore`, and smoke-test script. Compiled binaries and `.o` files should not be committed.

Before submission, verify that the latest version is pushed to the repository and submit the repository URL and report through the course e-learning system according to the instructor's instructions.

## 9. Conclusion

This project applies C programming and Linux filesystem interfaces to implement a modular command-line utility. It demonstrates argument parsing, directory traversal, file metadata retrieval, sorting, output formatting, symbolic-link handling, error reporting, and basic automated testing. The result is a working simplified `ls` implementation organized into maintainable source modules and built using a `Makefile`.
