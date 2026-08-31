#include "regex.h"
#include "nfa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

void print_postfix(regex r)
{
    for (int i = 0; i < r.size; i++)
    {
        printf("%c", r.items[i].value);
    }
    printf("\n");
}

void test_strings_stdin(const char *regex_str)
{
    regex r = parse_regex(regex_str);
    nfa n = regex_to_nfa(r);

    char buf[1024];
    while (fgets(buf, sizeof(buf), stdin))
    {
        buf[strcspn(buf, "\r\n")] = '\0';
        int result = match_nfa(n, buf, strlen(buf));
        printf("%d", result ? 1 : 0);
    }
    printf("\n");

    free_nfa(&n);
}

int serialize_nfa_from_regex(const char *regex_str, const char *output_path)
{
    regex r = parse_regex(regex_str);
    nfa n = regex_to_nfa(r);

    bool ok = save_nfa(&n, output_path);
    free_nfa(&n);

    if (!ok)
    {
        fprintf(stderr, "Error: No se pudo serializar el NFA en '%s'.\n", output_path);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int opt;
    char regex_str[1024];
    char *output_file = NULL;
    int mode = 0;

    while ((opt = getopt(argc, argv, "rto:")) != -1)
    {
        switch (opt)
        {
            case 'r':
                if (mode != 0)
                {
                    fprintf(stderr, "Error: Solo puedes usar una opcion de modo entre -r, -t o -o.\n");
                    return 1;
                }
                mode = 'r';
                break;
            case 't':
                if (mode != 0)
                {
                    fprintf(stderr, "Error: Solo puedes usar una opcion de modo entre -r, -t o -o.\n");
                    return 1;
                }
                mode = 't';
                break;
            case 'o':
                if (mode != 0)
                {
                    fprintf(stderr, "Error: Solo puedes usar una opcion de modo entre -r, -t o -o.\n");
                    return 1;
                }
                mode = 'o';
                output_file = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -r | -t | -o <archivo.nfa>\n", argv[0]);
                return 1;
        }
    }

    if (mode == 0)
    {
        fprintf(stderr, "Usage: %s -r | -t | -o <archivo.nfa>\n", argv[0]);
        return 1;
    }

    if (!fgets(regex_str, sizeof(regex_str), stdin))
    {
        return 1;
    }
    regex_str[strcspn(regex_str, "\r\n")] = '\0';

    if (mode == 'r')
    {
        print_postfix(parse_regex(regex_str));
        return 0;
    }

    if (mode == 't')
    {
        test_strings_stdin(regex_str);
        return 0;
    }

    return serialize_nfa_from_regex(regex_str, output_file);
}