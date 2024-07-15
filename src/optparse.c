#define _GNU_SOURCE

#include <ctype.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "optparse.h"

#define compare(a, b) (((b) > (a)) - ((a) > (b)))
#define max(a, b) ((a) > (b) ? (a) : (b))

static void make_optstr(
        enum OptParseOrder order,
        size_t n,
        struct OptSpec specs[OPT_PARSE_ARRAY OPT_PARSE_RESTRICT n],
        char short_opts[OPT_PARSE_ARRAY OPT_PARSE_RESTRICT 3 * n + 3],
        struct option long_opts[OPT_PARSE_ARRAY OPT_PARSE_RESTRICT n + 1])
{
        struct OptSpec *p = specs;
        struct option *q = long_opts;
        char *r = short_opts;

        if (order == OPT_PARSE_STRICT)
                *r++ = '+';
        else if (order == OPT_PARSE_WRAP)
                *r++ = '-';

        *r++ = ':';

        for (; p < specs + n; ++p) {
                if (p->val == OPT_PARSE_NONOPT_VALUE)
                        continue;
                if (p->name) {
                        q->name = p->name;
                        switch (p->arg_type) {
                        case OPT_PARSE_NONE:
                                q->has_arg = no_argument;
                                break;
                        case OPT_PARSE_REQUIRED:
                                q->has_arg = required_argument;
                                break;
                        case OPT_PARSE_OPTIONAL:
                                q->has_arg = optional_argument;
                                break;
                        }
                        q->val = p->val;
                        q->flag = NULL;
                        ++q;
                }

                if (!isgraph(p->val) || strchr(":;-?", p->val))
                        continue;
                *r++ = p->val;
                if (p->arg_type != OPT_PARSE_NONE)
                        *r++ = ':';
                if (p->arg_type == OPT_PARSE_OPTIONAL)
                        *r++ = ':';
        }

        memset(q, 0, sizeof(*q));
        *r = 0;
}

void opt_parse_print_help(
        FILE *f,
        size_t specs_sz,
        struct OptSpec specs[OPT_PARSE_ARRAY OPT_PARSE_RESTRICT specs_sz])
{
        const char *s, *b;
        struct OptSpec *p = specs;
        ptrdiff_t sz, max_len = 0, indent;

        fprintf(f, "usage: wallset [options...] [wallpapers...]\n\n");

        for (; p < specs + specs_sz; ++p) {
                sz = 8 // iniital indent
                   + (isgraph(p->val) != 0) * 2 // short opt
                   + (isgraph(p->val) != 0 && p->name) * 2 // comma if both opts
                   + (p->name ? 2 + strlen(p->name) : 0) // long opt
                   + (p->arg_name ? 1 + strlen(p->arg_name)
                                            + 2
                                                      * (p->arg_type
                                                         == OPT_PARSE_OPTIONAL)
                                  : 0) // argument
                   + 1; // trailing space
                max_len = max(max_len, sz);
        }

        for (; p < specs + specs_sz; ++p) {
                indent = fprintf(f, "        ");
                if (isgraph(p->val))
                        indent += fprintf(f, "-%c", p->val);
                if (isgraph(p->val) && p->name)
                        indent += fprintf(f, ", ");
                if (p->name)
                        indent += fprintf(f, "--%s", p->name);
                if (p->arg_name || (p->name && p->arg_type != OPT_PARSE_NONE)) {
                        s = p->arg_name;
                        if (!s) {
                                indent += fprintf(f, "=");
                                if (p->arg_type == OPT_PARSE_OPTIONAL)
                                        indent += fprintf(f, "[");
                                for (s = p->name; *s; ++s) {
                                        fprintf(f, "%c", toupper(*s));
                                }
                                if (p->arg_type == OPT_PARSE_OPTIONAL)
                                        indent += fprintf(f, "]");
                        } else {
                                indent += fprintf(
                                        f,
                                        p->arg_type == OPT_PARSE_OPTIONAL
                                                ? "=[%s]"
                                                : "=%s",
                                        s);
                        }
                }
                for (; indent != max_len; ++indent)
                        fprintf(f, " ");
                s = p->desc;
                while (s && *s) {
                        do {
                                b = s;
                                for (; *s && *s != ' '; ++s)
                                        ++indent;
                                fprintf(f, "%.*s ", (int)(s - b), b);
                                if (*s)
                                        ++s;
                                ++indent;
                        } while (*s && indent <= 80);
                        if (!*s)
                                break;
                        fprintf(f, "\n");
                        for (indent = 0; indent < max_len; ++indent)
                                fprintf(f, " ");
                }
                fprintf(f, "\n");
        }
}

static int opt_cmp(const void *pa, const void *pb)
{
        const struct OptSpec *a = pa, *b = pb;

        return compare(a->val, b->val);
}

size_t opt_parse_struct_option_size(size_t nmemb)
{
        return nmemb * sizeof(struct option);
}

enum OptParse opt_parse_static(
        enum OptParseOrder order,
        size_t n,
        struct OptSpec opts_spec[OPT_PARSE_ARRAY OPT_PARSE_RESTRICT n],
        char optstr[OPT_PARSE_ARRAY OPT_PARSE_RESTRICT 3 * n + 3],
        void *OPT_PARSE_RESTRICT l,
        int argc,
        char *argv[static argc + 1],
        void *OPT_PARSE_RESTRICT user)
{
        struct OptSpec spec = { 0 }, *p;
        struct option *longopts = l;
        char *arg;

        optind = 0;

        qsort(opts_spec, n, sizeof(*opts_spec), opt_cmp);

        make_optstr(order, n, opts_spec, optstr, longopts);

        while ((spec.val = getopt_long(argc, argv, optstr, longopts, NULL))
               != -1) {
                p = bsearch(&spec, opts_spec, n, sizeof(*opts_spec), opt_cmp);
                if (!p) {
                        if (spec.val == ':')
                                return OPT_PARSE_REQUIRES_ARG;
                        return OPT_PARSE_UNRECOGNIZED;
                }
                if (spec.val == '?' || spec.val == ':')
                        arg = argv[optind - 1];
                else
                        arg = optarg;
                if (!p->proc(spec.val, arg, user))
                        return OPT_PARSE_PROC_FAILURE;
        }

        return OPT_PARSE_SUCCESS;
}

enum OptParse opt_parse(
        enum OptParseOrder order,
        size_t n,
        struct OptSpec spec[OPT_PARSE_ARRAY OPT_PARSE_RESTRICT n],
        int argc,
        char *argv[OPT_PARSE_ARRAY argc + 1],
        void *OPT_PARSE_RESTRICT user)
{
        char *optstr = NULL;
        struct option *longs = NULL;
        enum OptParse status = OPT_PARSE_OUT_OF_MEMORY;

        optstr = malloc((3 * n + 3) * sizeof(*optstr));
        longs = malloc((n + 1) * sizeof(*longs));

        if (optstr && longs) {
                status = opt_parse_static(
                        order, n, spec, optstr, longs, argc, argv, user);
        }

        free(optstr);
        free(longs);

        return status;
}
