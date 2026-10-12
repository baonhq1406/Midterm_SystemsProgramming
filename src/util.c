#include "ls.h"

#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <unistd.h>

char *join_path(const char *directory, const char *name)
{
    size_t directory_length;
    size_t name_length;
    bool needs_slash;
    char *joined;

    if (directory == NULL || name == NULL) {
        return NULL;
    }

    directory_length = strlen(directory);
    name_length = strlen(name);
    needs_slash = directory_length > 0 && directory[directory_length - 1] != '/';

    if (directory_length > SIZE_MAX - name_length - (needs_slash ? 2U : 1U)) {
        return NULL;
    }

    joined = malloc(directory_length + name_length + (needs_slash ? 2U : 1U));
    if (joined == NULL) {
        return NULL;
    }

    memcpy(joined, directory, directory_length);
    if (needs_slash) {
        joined[directory_length++] = '/';
    }
    memcpy(joined + directory_length, name, name_length);
    joined[directory_length + name_length] = '\0';
    return joined;
}

void print_safe_text(const char *text, const Options *options)
{
    NonprintMode mode;
    mbstate_t state;
    const char *cursor;
    bool replace_nonprintable;

    if (text == NULL) {
        return;
    }

    mode = options->nonprint_mode;
    if (mode == NONPRINT_DEFAULT) {
        replace_nonprintable = isatty(STDOUT_FILENO) != 0;
    } else {
        replace_nonprintable = (mode == NONPRINT_QUESTION);
    }

    if (!replace_nonprintable) {
        fputs(text, stdout);
        return;
    }

    memset(&state, 0, sizeof(state));
    cursor = text;
    while (*cursor != '\0') {
        wchar_t wc;
        size_t remaining = strlen(cursor);
        size_t consumed = mbrtowc(&wc, cursor, remaining, &state);

        if (consumed == (size_t)-1 || consumed == (size_t)-2) {
            /* Invalid byte sequence: replace one byte and restart decoding. */
            putchar('?');
            ++cursor;
            memset(&state, 0, sizeof(state));
            continue;
        }
        if (consumed == 0) {
            break;
        }

        if (iswprint((wint_t)wc) != 0) {
            (void)fwrite(cursor, 1, consumed, stdout);
        } else {
            putchar('?');
        }
        cursor += consumed;
    }
}

char *humanize_size(uintmax_t bytes)
{
    static const char *const units[] = {"B", "K", "M", "G", "T", "P", "E"};
    long double value = (long double)bytes;
    size_t unit = 0;
    char buffer[64];
    char *result;

    while (value >= 1024.0L && unit + 1 < sizeof(units) / sizeof(units[0])) {
        value /= 1024.0L;
        ++unit;
    }

    if (unit == 0) {
        (void)snprintf(buffer, sizeof(buffer), "%" PRIuMAX "%s", bytes, units[unit]);
    } else {
        (void)snprintf(buffer, sizeof(buffer), "%.1Lf%s", value, units[unit]);
    }

    result = malloc(strlen(buffer) + 1);
    if (result != NULL) {
        strcpy(result, buffer);
    }
    return result;
}

uintmax_t allocated_bytes(const struct stat *st)
{
    if (st->st_blocks < 0) {
        return 0;
    }
    if ((uintmax_t)st->st_blocks > UINTMAX_MAX / 512U) {
        return UINTMAX_MAX;
    }
    return (uintmax_t)st->st_blocks * 512U;
}

uintmax_t block_count(const struct stat *st, const Options *options)
{
    uintmax_t bytes = allocated_bytes(st);
    size_t unit_size = options->block_size == 0 ? 512 : options->block_size;

    if (options->size_mode == SIZE_MODE_KILOBYTES && options->show_blocks) {
        unit_size = 1024;
    }
    if (unit_size == 0) {
        unit_size = 512;
    }
    return bytes / unit_size + ((bytes % unit_size) != 0 ? 1U : 0U);
}

struct timespec entry_time(const struct stat *st, FileTimeMode mode)
{
    switch (mode) {
    case FILE_TIME_CTIME: return st->st_ctim;
    case FILE_TIME_ATIME: return st->st_atim;
    case FILE_TIME_MTIME:
    default: return st->st_mtim;
    }
}

char file_type_char(mode_t mode)
{
    if (S_ISDIR(mode)) return 'd';
    if (S_ISLNK(mode)) return 'l';
    if (S_ISCHR(mode)) return 'c';
    if (S_ISBLK(mode)) return 'b';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
    return '-';
}

char file_classification_suffix(const struct stat *st)
{
    mode_t mode = st->st_mode;

    if (S_ISDIR(mode)) return '/';
    if (S_ISLNK(mode)) return '@';
    if (S_ISFIFO(mode)) return '|';
    if (S_ISSOCK(mode)) return '=';
    if (S_ISREG(mode) && (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0) return '*';
    return '\0';
}

char *read_symlink_target(const char *path)
{
    size_t capacity = 128;
    char *buffer;

    for (;;) {
        ssize_t length;

        if (capacity > SIZE_MAX - 1) {
            return NULL;
        }
        buffer = malloc(capacity + 1);
        if (buffer == NULL) {
            return NULL;
        }

        length = readlink(path, buffer, capacity);
        if (length < 0) {
            free(buffer);
            return NULL;
        }
        if ((size_t)length < capacity) {
            buffer[length] = '\0';
            return buffer;
        }

        free(buffer);
        if (capacity > SIZE_MAX / 2) {
            return NULL;
        }
        capacity *= 2;
    }
}
