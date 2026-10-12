#include "ls.h"
#include <dirent.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    char *path;
} Operand;

typedef struct {
    Entry *items;
    size_t count;
    size_t capacity;
} EntryVector;

typedef struct {
    Operand *items;
    size_t count;
    size_t capacity;
} OperandVector;

static const Options *sort_options;

static void free_entry(Entry *entry)
{
    if (entry == NULL) return;
    free(entry->name);
    free(entry->path);
    entry->name = NULL;
    entry->path = NULL;
}

static void free_entries(EntryVector *entries)
{
    size_t i;
    for (i = 0; i < entries->count; ++i) free_entry(&entries->items[i]);
    free(entries->items);
    entries->items = NULL;
    entries->count = entries->capacity = 0;
}

static int append_entry(EntryVector *entries, const char *name, const char *path,
                        const struct stat *metadata)
{
    Entry *larger;
    Entry new_entry;

    if (entries->count == entries->capacity) {
        size_t new_capacity = entries->capacity == 0 ? 16 : entries->capacity * 2;

        if (new_capacity < entries->capacity ||
            new_capacity > SIZE_MAX / sizeof(*larger)) {
            errno = ENOMEM;
            return -1;
        }

        larger = realloc(entries->items, new_capacity * sizeof(*larger));
        if (larger == NULL) return -1;

        entries->items = larger;
        entries->capacity = new_capacity;
    }

    memset(&new_entry, 0, sizeof(new_entry));
    new_entry.name = strdup(name);
    new_entry.path = strdup(path);

    if (new_entry.name == NULL || new_entry.path == NULL) {
        free_entry(&new_entry);
        return -1;
    }

    new_entry.st = *metadata;
    entries->items[entries->count++] = new_entry;

    return 0;
}

static int compare_timespec(struct timespec left, struct timespec right)
{
    if (left.tv_sec < right.tv_sec) return -1;
    if (left.tv_sec > right.tv_sec) return 1;
    if (left.tv_nsec < right.tv_nsec) return -1;
    if (left.tv_nsec > right.tv_nsec) return 1;
    return 0;
}

static int compare_entries(const void *left_pointer, const void *right_pointer)
{
    const Entry *left = left_pointer;
    const Entry *right = right_pointer;
    int result = 0;

    switch (sort_options->sort_mode) {
    case SORT_TIME:
        result = compare_timespec(
            entry_time(&right->st, sort_options->time_mode),
            entry_time(&left->st, sort_options->time_mode));

        if (result == 0) result = strcmp(left->name, right->name);
        break;

    case SORT_SIZE:
        if (left->st.st_size < right->st.st_size) result = 1;
        else if (left->st.st_size > right->st.st_size) result = -1;
        else result = strcmp(left->name, right->name);
        break;

    case SORT_NONE:
        return 0;

    case SORT_NAME:
    default:
        result = strcmp(left->name, right->name);
        break;
    }

    return sort_options->reverse ? -result : result;
}

static int compare_operands(const void *left_pointer, const void *right_pointer)
{
    const Operand *left = left_pointer;
    const Operand *right = right_pointer;
    int result = strcmp(left->path, right->path);

    return sort_options != NULL && sort_options->reverse ? -result : result;
}

static bool include_name(const char *name, const Options *options)
{
    if (options->hidden_mode == HIDDEN_ALL) return true;

    if (options->hidden_mode == HIDDEN_ALMOST_ALL) {
        return strcmp(name, ".") != 0 && strcmp(name, "..") != 0;
    }

    return name[0] != '.';
}

static int read_directory(const char *path, const Options *options,
                          EntryVector *entries)
{
    DIR *directory;
    struct dirent *item;
    int read_error = 0;

    directory = opendir(path);

    if (directory == NULL) {
        fprintf(stderr, "ls: %s: ", path);
        perror(NULL);
        return -1;
    }

    for (;;) {
        char *child_path;
        struct stat metadata;

        errno = 0;
        item = readdir(directory);

        if (item == NULL) {
            if (errno != 0) {
                fprintf(stderr, "ls: error reading directory %s: ", path);
                perror(NULL);
                read_error = 1;
            }
            break;
        }

        if (!include_name(item->d_name, options)) continue;

        child_path = join_path(path, item->d_name);

        if (child_path == NULL) {
            fprintf(stderr, "ls: unable to allocate path for %s\n",
                    item->d_name);
            read_error = 1;
            continue;
        }

        if (lstat(child_path, &metadata) != 0) {
            fprintf(stderr, "ls: %s: ", child_path);
            perror(NULL);
            read_error = 1;
            free(child_path);
            continue;
        }

        if (append_entry(entries, item->d_name, child_path, &metadata) != 0) {
            fprintf(stderr, "ls: unable to store directory entry\n");
            read_error = 1;
            free(child_path);
            break;
        }

        free(child_path);
    }

    if (closedir(directory) != 0) {
        fprintf(stderr, "ls: error closing directory %s: ", path);
        perror(NULL);
        read_error = 1;
    }

    return read_error ? -1 : 0;
}

static void print_total(const EntryVector *entries, const Options *options)
{
    uintmax_t bytes = 0;
    uintmax_t blocks = 0;
    size_t i;

    /*
     * Long format always prints the directory total.
     * With only -s, print the total only when stdout is a terminal,
     * as specified by the supplied ls(1) manual.
     */
    if (!options->long_format &&
        (!options->show_blocks || !isatty(STDOUT_FILENO))) {
        return;
    }

    for (i = 0; i < entries->count; ++i) {
        uintmax_t allocated = allocated_bytes(&entries->items[i].st);
        uintmax_t current_blocks = block_count(&entries->items[i].st, options);

        if (UINTMAX_MAX - bytes < allocated) {
            bytes = UINTMAX_MAX;
        } else {
            bytes += allocated;
        }

        if (UINTMAX_MAX - blocks < current_blocks) {
            blocks = UINTMAX_MAX;
        } else {
            blocks += current_blocks;
        }
    }

    if (options->size_mode == SIZE_MODE_HUMAN) {
        char *human = humanize_size(bytes);
        printf("total %s\n", human != NULL ? human : "?");
        free(human);
    } else {
        printf("total %ju\n", blocks);
    }
}

static int list_directory(const char *path, const char *label, bool show_header,
                          const Options *options, OutputState *output)
{
    EntryVector entries = {0};
    size_t i;
    int result = 0;

    if (show_header) {
        if (output->wrote_output) putchar('\n');
        print_safe_text(label, options);
        puts(":");
        output->wrote_output = true;
    }

    if (read_directory(path, options, &entries) != 0) {
        result = 1;
    }

    if (!options->no_sort && entries.count > 1) {
        sort_options = options;
        qsort(entries.items, entries.count, sizeof(*entries.items),
              compare_entries);
    }

    print_total(&entries, options);

    for (i = 0; i < entries.count; ++i) {
        print_entry(&entries.items[i], options);
        output->wrote_output = true;
    }

    if (options->recursive) {
        for (i = 0; i < entries.count; ++i) {
            Entry *entry = &entries.items[i];

            /* Do not follow symbolic links during recursive descent. */
            if (!S_ISDIR(entry->st.st_mode)) continue;

            if (strcmp(entry->name, ".") == 0 ||
                strcmp(entry->name, "..") == 0) {
                continue;
            }

            if (list_directory(entry->path, entry->path, true,
                               options, output) != 0) {
                result = 1;
            }
        }
    }

    free_entries(&entries);
    return result;
}

static int append_operand(OperandVector *operands, const char *path)
{
    Operand *larger;
    char *copy;

    if (operands->count == operands->capacity) {
        size_t new_capacity = operands->capacity == 0
                                  ? 8
                                  : operands->capacity * 2;

        if (new_capacity < operands->capacity ||
            new_capacity > SIZE_MAX / sizeof(*larger)) {
            errno = ENOMEM;
            return -1;
        }

        larger = realloc(operands->items, new_capacity * sizeof(*larger));
        if (larger == NULL) return -1;

        operands->items = larger;
        operands->capacity = new_capacity;
    }

    copy = strdup(path);
    if (copy == NULL) return -1;

    operands->items[operands->count].path = copy;
    ++operands->count;

    return 0;
}

static void free_operands(OperandVector *operands)
{
    size_t i;

    for (i = 0; i < operands->count; ++i) {
        free(operands->items[i].path);
    }

    free(operands->items);
    operands->items = NULL;
    operands->count = operands->capacity = 0;
}

static int process_file_operand(const char *path, const Options *options)
{
    struct stat metadata;
    Entry entry;

    if (lstat(path, &metadata) != 0) {
        fprintf(stderr, "ls: %s: ", path);
        perror(NULL);
        return 1;
    }

    entry.name = (char *)path;
    entry.path = (char *)path;
    entry.st = metadata;

    print_entry(&entry, options);
    return 0;
}

int run_listing(int argc, char **argv, int first_operand,
                const Options *options)
{
    OperandVector files = {0};
    OperandVector directories = {0};
    OutputState output = {false};
    int operand_count = argc - first_operand;
    int result = 0;
    int i;

    sort_options = options;

    if (operand_count == 0) {
        struct stat metadata;

        bool is_directory =
            stat(".", &metadata) == 0 &&
            S_ISDIR(metadata.st_mode) &&
            !options->directory_as_file;

        if (append_operand(is_directory ? &directories : &files, ".") != 0) {
            fprintf(stderr, "ls: memory allocation failed\n");
            return 1;
        }
    } else {
        for (i = first_operand; i < argc; ++i) {
            struct stat metadata;
            bool is_directory = false;

            if (!options->directory_as_file &&
                stat(argv[i], &metadata) == 0 &&
                S_ISDIR(metadata.st_mode)) {
                is_directory = true;
            }

            if (append_operand(is_directory ? &directories : &files,
                              argv[i]) != 0) {
                fprintf(stderr, "ls: memory allocation failed\n");
                free_operands(&files);
                free_operands(&directories);
                return 1;
            }
        }
    }

    if (!options->no_sort) {
        if (files.count > 1) {
            qsort(files.items, files.count, sizeof(*files.items),
                  compare_operands);
        }

        if (directories.count > 1) {
            qsort(directories.items, directories.count,
                  sizeof(*directories.items), compare_operands);
        }
    }

    for (i = 0; i < (int)files.count; ++i) {
        if (process_file_operand(files.items[i].path, options) != 0) {
            result = 1;
        }

        output.wrote_output = true;
    }

    for (i = 0; i < (int)directories.count; ++i) {
        bool show_header = options->recursive || operand_count > 1;

        if (list_directory(directories.items[i].path,
                           directories.items[i].path,
                           show_header, options, &output) != 0) {
            result = 1;
        }
    }

    free_operands(&files);
    free_operands(&directories);

    return result;
}