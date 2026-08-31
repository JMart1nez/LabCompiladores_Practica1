#ifndef REGEX_H
#define REGEX_H

// Estructura exigida por main.c para iterar sobre los caracteres
typedef struct {
    char value;
} RegexItem;

typedef struct {
    RegexItem *items; // Arreglo de caracteres dinámico
    int size;
} regex;

// Firma de la función principal que llamamos desde main.c
regex parse_regex(const char *regex_str);

#endif