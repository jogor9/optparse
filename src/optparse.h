#ifndef OPTPARSE_OPTPARSE_H
#define OPTPARSE_OPTPARSE_H

#if __STDC_VERSION__ >= 199900
#define OPT_PARSE_RESTRICT restrict
#define OPT_PARSE_ARRAY static
#elif __GNUC__
#define OPT_PARSE_RESTRICT __restrict__
#define OPT_PARSE_ARRAY
#else
#define OPT_PARSE_RESTRICT
#define OPT_PARSE_ARRAY
#endif

#include <stddef.h>
#include <stdio.h>

#define OPT_PARSE_NONOPT_VALUE 1

typedef int OptProc(int val,
                    char *OPT_PARSE_RESTRICT arg,
                    void *OPT_PARSE_RESTRICT user);

enum OptParseArg {
        OPT_PARSE_NONE,
        OPT_PARSE_REQUIRED,
        OPT_PARSE_OPTIONAL,
};

struct OptSpec {
        OptProc *proc;
        const char *name;
        const char *desc;
        const char *arg_name;
        enum OptParseArg arg_type;
        int val;
};

enum OptParseOrder {
        OPT_PARSE_PERMUTE,
        OPT_PARSE_STRICT,
        OPT_PARSE_WRAP
};

enum OptParse {
        OPT_PARSE_SUCCESS,
        OPT_PARSE_UNRECOGNIZED,
        OPT_PARSE_REQUIRES_ARG,
        OPT_PARSE_PROC_FAILURE,
        OPT_PARSE_OUT_OF_MEMORY,
};

size_t opt_parse_longopts_struct_size(size_t nmemb);

enum OptParse opt_parse_static(
        enum OptParseOrder order,
        size_t n,
        struct OptSpec opts_spec[OPT_PARSE_ARRAY OPT_PARSE_RESTRICT n],
        char optstr[OPT_PARSE_ARRAY OPT_PARSE_RESTRICT 3 * n + 3],
        void *OPT_PARSE_RESTRICT longopts,
        int argc,
        char *argv[OPT_PARSE_ARRAY argc + 1],
        void *OPT_PARSE_RESTRICT user);

enum OptParse opt_parse(
        enum OptParseOrder order,
        size_t n,
        struct OptSpec opts_spec[OPT_PARSE_ARRAY OPT_PARSE_RESTRICT n],
        int argc,
        char *argv[OPT_PARSE_ARRAY argc + 1],
        void *OPT_PARSE_RESTRICT user);
void opt_parse_print_help(
        FILE *f,
        size_t specs_sz,
        struct OptSpec specs[OPT_PARSE_ARRAY OPT_PARSE_RESTRICT specs_sz]);

#endif
