#include "ls.h"

#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <unistd.h>

static void make_mode_string(mode_t mode, char text[11])
{
    text[0] = file_type_char(mode);
    text[1] = (mode & S_IRUSR) ? 'r' : '-';
    text[2] = (mode & S_IWUSR) ? 'w' : '-';
    text[3] = (mode & S_ISUID) ? ((mode & S_IXUSR) ? 's' : 'S')
                              : ((mode & S_IXUSR) ? 'x' : '-');
    text[4] = (mode & S_IRGRP) ? 'r' : '-';
    text[5] = (mode & S_IWGRP) ? 'w' : '-';
    text[6] = (mode & S_ISGID) ? ((mode & S_IXGRP) ? 's' : 'S')
                              : ((mode & S_IXGRP) ? 'x' : '-');
    text[7] = (mode & S_IROTH) ? 'r' : '-';
    text[8] = (mode & S_IWOTH) ? 'w' : '-';
    text[9] = (mode & S_ISVTX) ? ((mode & S_IXOTH) ? 't' : 'T')
                              : ((mode & S_IXOTH) ? 'x' : '-');
    text[10] = '\0';
}

static char *owner_name(uid_t uid, bool numeric)
{
    struct passwd *entry;
    char buffer[64];
    const char *name;
    char *copy;

    entry = numeric ? NULL : getpwuid(uid);
    if (entry != NULL) {
        name = entry->pw_name;
    } else {
        (void)snprintf(buffer, sizeof(buffer), "%ju", (uintmax_t)uid);
        name = buffer;
    }

    copy = malloc(strlen(name) + 1);
    if (copy != NULL) strcpy(copy, name);
    return copy;
}

static char *group_name(gid_t gid, bool numeric)
{
    struct group *entry;
    char buffer[64];
    const char *name;
    char *copy;

    entry = numeric ? NULL : getgrgid(gid);
    if (entry != NULL) {
        name = entry->gr_name;
    } else {
        (void)snprintf(buffer, sizeof(buffer), "%ju", (uintmax_t)gid);
        name = buffer;
    }

    copy = malloc(strlen(name) + 1);
    if (copy != NULL) strcpy(copy, name);
    return copy;
}

static char *entry_size_text(const struct stat *st, const Options *options)
{
    char buffer[64];
    char *human;
    uintmax_t bytes = st->st_size < 0 ? 0 : (uintmax_t)st->st_size;

    if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode)) {
        (void)snprintf(buffer, sizeof(buffer), "%u, %u",
                       major(st->st_rdev), minor(st->st_rdev));
    } else if (options->size_mode == SIZE_MODE_HUMAN) {
        human = humanize_size(bytes);
        return human;
    } else {
        (void)snprintf(buffer, sizeof(buffer), "%ju", bytes);
    }
    human = malloc(strlen(buffer) + 1);
    if (human != NULL) strcpy(human, buffer);
    return human;
}

static void print_name_and_suffix(const Entry *entry, const Options *options)
{
    print_safe_text(entry->name, options);
    if (options->classify) {
        char suffix = file_classification_suffix(&entry->st);
        if (suffix != '\0') putchar(suffix);
    }
}

static void print_link_target(const Entry *entry, const Options *options)
{
    char *target;

    if (!options->long_format || !S_ISLNK(entry->st.st_mode)) return;
    target = read_symlink_target(entry->path);
    if (target == NULL) return;

    fputs(" -> ", stdout);
    print_safe_text(target, options);
    free(target);
}

void print_entry(const Entry *entry, const Options *options)
{
    char mode_text[11];
    char *size_text = NULL;
    char *owner = NULL;
    char *group = NULL;
    char date_text[64];
    struct timespec chosen_time;
    struct tm local_time;

    if (options->show_inode) {
        printf("%ju ", (uintmax_t)entry->st.st_ino);
    }

    if (options->show_blocks) {
        if (options->size_mode == SIZE_MODE_HUMAN) {
            char *human = humanize_size(allocated_bytes(&entry->st));
            printf("%s ", human != NULL ? human : "?");
            free(human);
        } else {
            printf("%ju ", block_count(&entry->st, options));
        }
    }

    if (options->long_format) {
        make_mode_string(entry->st.st_mode, mode_text);
        owner = owner_name(entry->st.st_uid, options->numeric_ids);
        group = group_name(entry->st.st_gid, options->numeric_ids);
        size_text = entry_size_text(&entry->st, options);

        if (owner == NULL || group == NULL || size_text == NULL) {
            fprintf(stderr, "ls: memory allocation failed while formatting entry\n");
            free(owner);
            free(group);
            free(size_text);
            owner = group = size_text = NULL;
            printf("%s %3ju ? ? ? ", mode_text, (uintmax_t)entry->st.st_nlink);
        } else {
            printf("%s %3ju %-8s %-8s %8s ", mode_text,
                   (uintmax_t)entry->st.st_nlink, owner, group, size_text);
        }
        free(owner);
        free(group);
        free(size_text);

        chosen_time = entry_time(&entry->st, options->time_mode);
        if (localtime_r(&chosen_time.tv_sec, &local_time) != NULL &&
            strftime(date_text, sizeof(date_text), "%b %e %H:%M", &local_time) > 0) {
            printf("%s ", date_text);
        } else {
            fputs("??? ?? ??:?? ", stdout);
        }
    }

    print_name_and_suffix(entry, options);
    print_link_target(entry, options);
    putchar('\n');
}
