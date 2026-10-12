#ifndef LS_H
#define LS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef enum {
    HIDDEN_NORMAL = 0,
    HIDDEN_ALL,
    HIDDEN_ALMOST_ALL
} HiddenMode;

typedef enum {
    SORT_NAME = 0,
    SORT_TIME,
    SORT_SIZE,
    SORT_NONE
} SortMode;

typedef enum {
    FILE_TIME_MTIME = 0,
    FILE_TIME_CTIME,
    FILE_TIME_ATIME
} FileTimeMode;

typedef enum {
    NONPRINT_DEFAULT = 0,
    NONPRINT_QUESTION,
    NONPRINT_RAW
} NonprintMode;

typedef enum {
    SIZE_MODE_DEFAULT = 0,
    SIZE_MODE_KILOBYTES,
    SIZE_MODE_HUMAN
} SizeMode;

typedef struct {
    HiddenMode hidden_mode;
    SortMode sort_mode;
    FileTimeMode time_mode;
    NonprintMode nonprint_mode;
    SizeMode size_mode;
    bool long_format;
    bool numeric_ids;
    bool classify;
    bool show_inode;
    bool show_blocks;
    bool recursive;
    bool directory_as_file;
    bool reverse;
    bool no_sort;
    size_t block_size; /* Used by -s when neither -h nor -k overrides it. */
} Options;

typedef struct {
    char *name;       /* Text displayed to the user. */
    char *path;       /* Path used for filesystem operations. */
    struct stat st;   /* lstat() metadata; symlinks are not dereferenced here. */
} Entry;

typedef struct {
    bool wrote_output;
} OutputState;

void options_init(Options *options);
int options_parse(int argc, char **argv, Options *options, int *first_operand);
void print_usage(const char *program_name);

char *join_path(const char *directory, const char *name);
void print_safe_text(const char *text, const Options *options);
char *humanize_size(uintmax_t bytes);
uintmax_t allocated_bytes(const struct stat *st);
uintmax_t block_count(const struct stat *st, const Options *options);
struct timespec entry_time(const struct stat *st, FileTimeMode mode);
char file_type_char(mode_t mode);
char file_classification_suffix(const struct stat *st);
char *read_symlink_target(const char *path);

void print_entry(const Entry *entry, const Options *options);
int run_listing(int argc, char **argv, int first_operand, const Options *options);

#endif /* LS_H */
