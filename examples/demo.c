// You can get the include directory for `optparse.h` by inspecting the include
// directories target property of the CMake library

#include <stdio.h>
#include <stdlib.h>

#include "optparse.h"

struct MyData {
        int loglevel;
};

int print_help(int val, char *arg, void *user);

int set_loglevel(int val, char *arg, void *user)
{
        struct MyData *mydata = user;
        mydata->loglevel = atoi(arg);
        return 1;
}

int error_option(int val, char *arg, void *user)
{
        if (val == ':')
                fprintf(stderr, "option %s requires an argument\n", arg);
        else
                fprintf(stderr, "unrecognized option: %s\n", arg);

        exit(EXIT_FAILURE);
}

#define array_size(arr) (sizeof(arr) / (sizeof(*(arr))))

static struct OptSpec argdesc[] = {
        {
         .val = 'h',
         .name = "help",
         .desc = "Show help",
         .proc = print_help,
         },
        {
         .val = 'v',
         .name = "loglevel",
         .desc = "Set the loglevel (0 - default, -1 - quiet, 1 - verbose)",
         .arg_type = OPT_PARSE_ARG_REQUIRED,
         .proc = set_loglevel,
         },
        { .val = '?', .proc = error_option },
        { .val = ':', .proc = error_option },
};

int print_help(int val, char *arg, void *user)
{
        opt_parse_print_help(stdout, array_size(argdesc), argdesc);
        exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
        struct MyData mydata;

        switch (opt_parse(OPT_PARSE_ORDER_PERMUTE,
                          array_size(argdesc),
                          argdesc,
                          argc,
                          argv,
                          &mydata)) {
        case OPT_PARSE_ERR_SUCCESS:
                puts("Arguments successfully parsed");
                exit(EXIT_SUCCESS);
        default:
                abort();
        }
}
