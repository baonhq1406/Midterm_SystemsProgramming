#include "ls.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static size_t parse_block_size(const char *value)
{
    char *end = NULL;
    unsigned long long number;
    unsigned long long multiplier = 1;

    if (value == NULL || *value == '\0') {
        return 512;
    }

    errno = 0;
    number = strtoull(value, &end, 10);
    if (errno != 0 || end == value || number == 0) {
        return 512;
    }

    if (*end != '\0') {
        if (end[1] != '\0') {
            return 512;
        }
        switch (*end) {
        case 'k': case 'K': multiplier = 1024ULL; break;
        case 'm': case 'M': multiplier = 1024ULL * 1024ULL; break;
        case 'g': case 'G': multiplier = 1024ULL * 1024ULL * 1024ULL; break;
        default: return 512;
        }
    }

    if (number > (unsigned long long)SIZE_MAX / multiplier) {
        return 512;
    }
    return (size_t)(number * multiplier);
}

void options_init(Options *options)
{
    const char *blocksize;

    memset(options, 0, sizeof(*options));
    options->hidden_mode = (geteuid() == 0) ? HIDDEN_ALMOST_ALL : HIDDEN_NORMAL;
    options->sort_mode = SORT_NAME;
    options->time_mode = FILE_TIME_MTIME;
    options->nonprint_mode = NONPRINT_DEFAULT;
    options->size_mode = SIZE_MODE_DEFAULT;

    blocksize = getenv("BLOCKSIZE");
    options->block_size = parse_block_size(blocksize);
}

int options_parse(int argc, char **argv, Options *options, int *first_operand)
{
    int i;

    for (i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        size_t j;

        if (strcmp(arg, "--") == 0) {
            ++i;
            break;
        }
        if (arg[0] != '-' || arg[1] == '\0') {
            break;
        }

        for (j = 1; arg[j] != '\0'; ++j) {
            switch (arg[j]) {
            case 'A': options->hidden_mode = HIDDEN_ALMOST_ALL; break;
            case 'a': options->hidden_mode = HIDDEN_ALL; break;
            case 'c': options->time_mode = FILE_TIME_CTIME; break;
            case 'd':
                options->directory_as_file = true;
                options->recursive = false;
                break;
            case 'F': options->classify = true; break;
            case 'f':
                options->no_sort = true;
                options->sort_mode = SORT_NONE;
                break;
            case 'h': options->size_mode = SIZE_MODE_HUMAN; break;
            case 'i': options->show_inode = true; break;
            case 'k': options->size_mode = SIZE_MODE_KILOBYTES; break;
            case 'l':
                options->long_format = true;
                options->numeric_ids = false;
                break;
            case 'n':
                options->long_format = true;
                options->numeric_ids = true;
                break;
            case 'q': options->nonprint_mode = NONPRINT_QUESTION; break;
            case 'R':
                options->recursive = true;
                options->directory_as_file = false;
                break;
            case 'r': options->reverse = true; break;
            case 'S':
                options->sort_mode = SORT_SIZE;
                break;
            case 's': options->show_blocks = true; break;
            case 't': options->sort_mode = SORT_TIME; break;
            case 'u': options->time_mode = FILE_TIME_ATIME; break;
            case 'w': options->nonprint_mode = NONPRINT_RAW; break;
            default:
                fprintf(stderr, "ls: illegal option -- %c\n", arg[j]);
                return -1;
            }
        }
    }

    *first_operand = i;
    return 0;
}

void print_usage(const char *program_name)
{
    fprintf(stderr,
            "usage: %s [-AacdFfhiklnqRrSstuw] [file ...]\n",
            program_name != NULL ? program_name : "ls");
}
