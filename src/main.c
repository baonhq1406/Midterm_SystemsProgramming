#include "ls.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    Options options;
    int first_operand = 1;

    /* Make printable-character handling respect the user's locale. */
    (void)setlocale(LC_CTYPE, "");
    (void)setlocale(LC_TIME, "");

    options_init(&options);
    if (options_parse(argc, argv, &options, &first_operand) != 0) {
        print_usage(argv[0]);
        return 2;
    }

    return run_listing(argc, argv, first_operand, &options);
}
