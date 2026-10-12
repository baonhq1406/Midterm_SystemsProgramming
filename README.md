# Midterm Project: Simplified `ls(1)`

## Project report

**Course project:** Midterm Project - Implement `ls(1)`  
**Language:** C (POSIX/Linux)  
**Reference manual:** the supplied NetBSD `ls(1)` manual (`ls.pdf`)  
**GitHub repository:** `https://github.com/baonhq1406/Midterm_SystemsProgramming` *(replace `YOUR_USERNAME` and `Name_studentID_midterm` with your actual GitHub username and required repository name before submission)*

## 1. Overview

This project implements a modular, simplified version of the UNIX `ls` command. It uses directory and metadata system calls such as `opendir()`, `readdir()`, `lstat()`, `stat()`, `readlink()`, and `closedir()` to list filesystem entries.

The implementation follows the option subset listed in the supplied manual. It is written for a POSIX/Linux environment and does not call the system `ls` command to generate its output.

## 2. Implemented options

| Option | Behavior |
|---|---|
| `-A` | Include dot-prefixed entries but omit `.` and `..`; enabled by default when run as superuser unless overridden by another hidden-file option. |
| `-a` | Include all entries, including `.` and `..`. |
| `-c` | Use status-change time for `-t` sorting and long-format timestamps; the last of `-c` and `-u` determines the selected time. |
| `-d` | List directory operands as entries instead of listing their contents; disables recursion until a later `-R`. |
| `-F` | Append `/`, `*`, `@`, `=`, or `|` for directories, executable regular files, symbolic links, sockets, and FIFOs. Linux has no standard whiteout file type, so `%` is not generated. |
| `-f` | Disable sorting. |
| `-h` | Show sizes in human-readable units for `-s` and `-l`; the last of `-h` and `-k` determines the size mode. A later `-k` returns long-format sizes to bytes and makes `-s` use kilobytes. |
| `-i` | Print inode numbers. |
| `-k` | Report `-s` block counts in 1024-byte units. |
| `-l` | Use long format with owner and group names when available. |
| `-n` | Use long format with numeric owner and group IDs. |
| `-q` | Replace non-printable filename characters with `?`. |
| `-R` | Recursively list subdirectories; a later `-d` disables recursion. Symbolic links are not followed during recursive descent. |
| `-r` | Reverse the selected sort order. |
| `-S` | Sort by file size, largest first. |
| `-s` | Print allocated filesystem space in blocks. The default block unit is 512 bytes, subject to `BLOCKSIZE`. |
| `-t` | Sort by the selected file timestamp, newest first. |
| `-u` | Use last-access time for `-t` sorting and long-format timestamps; the last of `-c` and `-u` determines the selected time. |
| `-w` | Print filename characters without replacing non-printable characters. Overrides a prior `-q`. |

When more than one operand is supplied, non-directory operands are listed before directory operands, with each group sorted lexicographically (reversed by `-r`) unless `-f` is specified. The default output is one entry per line. Long format includes file mode, link count, owner, group, size, timestamp, and pathname. Symbolic-link targets are shown after ` -> ` in long format.

### Platform notes and boundaries

- This project targets Linux/POSIX and uses Linux's `st_blocks` allocation counts (in 512-byte units) as the basis for `-s` and directory totals.
- Filesystem-specific NetBSD whiteouts and archive-state file types are not available through normal Linux `stat` metadata, so those NetBSD-only type markers cannot be emitted.
- The supplied manual describes timezone selection through `TZ`; this program relies on the standard C library's local-time conversion and honors the process environment.
- The human-readable output uses binary size units (`K`, `M`, `G`, etc.).

## 3. Project structure

```text
Name_studentID_midterm/
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

- `main.c`: initializes the locale and starts option parsing.
- `options.c`: initializes options and parses the supported command-line flags.
- `listing.c`: reads directories, classifies operands, sorts entries, and handles recursion.
- `format.c`: prints the default and long formats, permissions, IDs, timestamps, and symbolic-link targets.
- `util.c`: provides path joining, character-safe printing, size conversion, timestamp access, file-type classification, and symlink reading.
- `include/ls.h`: shared structs, enums, and function declarations.

## 4. Requirements

- Modular source organization using several `.c` files and a shared `.h` file.
- Compiler warnings enabled by the provided Makefile.
- Dynamic allocation and error checks for paths, directory entries, and formatting strings.
- No recursive descent through symbolic links, preventing common symlink loops.
- `.gitignore` excludes the compiled `myls` binary and object files.
- A smoke-test script exercises common options and filesystem entry types.

## 5. Build and run

Use a Linux terminal from the project root:

```sh
make
```

The executable is named `myls` to avoid confusion with the system command. Examples:

```sh
./myls
./myls -a
./myls -l /etc
./myls -i -s .
./myls -F
./myls -R /tmp
./myls -n /etc/passwd
./myls -t
./myls -S
./myls -d /tmp
```

Run the smoke tests with:

```sh
make test
```

Remove generated binaries and object files with:

```sh
make clean
```

## 6. GitHub submission

Create a repository named exactly `Name_studentID_midterm`, replacing the placeholder parts with the naming format required by the instructor. From inside the project directory, run:

```sh
git init
git add Makefile README.md .gitignore include src tests
git commit -m "Implement simplified ls midterm project"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/Name_studentID_midterm.git
git push -u origin main
```

Replace `YOUR_USERNAME` and the repository name as needed. If the GitHub repository already exists, use its actual URL for `git remote add origin`. Before submission, check that no executable or `.o` files are tracked:

```sh
git status --short
git ls-files | grep -E '(^|/)(myls|.*\.o)$' || true
```

Submit the actual GitHub URL in the e-learning system and ensure the URL recorded near the top of this report matches the repository you created.
# Midterm_SystemsProgramming
