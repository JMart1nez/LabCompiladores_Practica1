#include "regex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Aplicación del algoritmo de Shunting-Yard

// Función auxiliar para saber si un carácter es un operando (letras/números)
int is_operand(char c) {
    return isalnum((unsigned char)c);
}

// Determina la prioridad de los operadores
int precedence(char c) {
    switch (c) {
        case '*':           // Estrella de Kleene
        case '+': return 3; // Cerradura positiva
        case '.': return 2; // Concatenación
        case '|': return 1; // Or
        default: return 0;
    }
}

// Agregar puntos '.' donde hay una concatenación implícita
void add_explicit_concat(const char *input, char *output) {
    int j = 0;
    int len = strlen(input);
    for (int i = 0; i < len; i++) {
        char c1 = input[i];
        output[j++] = c1;
        
        if (i + 1 < len) {
            char c2 = input[i + 1];
            // Regla de concatenación: 
            // Si c1 es operando, '*', o ')'
            // Y c2 es operando o '(' entonces concatenamos
            if ((is_operand(c1) || c1 == '*' || c1 == '+' || c1 == ')') &&
                (is_operand(c2) || c2 == '(')) {
                output[j++] = '.';
            }
        }
    }
    output[j] = '\0';
}

// Algoritmo de Shunting-Yard
regex parse_regex(const char *regex_str) {
    /* Validar antes de convertir evita que a) termine convertido en a.
     * Se conserva el alfabeto alfanumerico y el limite del main original. */
    int depth = 0;
    int need_operand = 1;
    size_t input_len = strlen(regex_str);
    if (input_len > 1023) {
        fprintf(stderr, "Error: expresion demasiado larga (maximo 1023).\n");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < input_len; i++) {
        char c = regex_str[i];
        if (is_operand(c)) {
            need_operand = 0;
        } else if (c == '(') {
            depth++;
            need_operand = 1;
        } else if (c == ')') {
            if (depth == 0 || need_operand) goto invalid;
            depth--;
        } else if (c == '*' || c == '+') {
            if (need_operand) goto invalid;
        } else if (c == '|' || c == '.') {
            if (need_operand) goto invalid;
            need_operand = 1;
        } else {
            goto invalid;
        }
    }
    if (depth != 0 || (input_len != 0 && need_operand)) goto invalid;

    char with_concat[2048];
    add_explicit_concat(regex_str, with_concat);

    int len = strlen(with_concat);
    
    // Prepara la estructura de salida requerida en main.c
    regex result;
    result.items = malloc(len * sizeof(RegexItem));
    result.size = 0;
    if (len > 0 && result.items == NULL) {
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(EXIT_FAILURE);
    }

    // Pila temporal para operadores
    char stack[2048];
    int top = -1;

    for (int i = 0; i < len; i++) {
        char c = with_concat[i];

        if (is_operand(c)) {
            // Operandos van directo a la salida
            result.items[result.size++].value = c;
        } else if (c == '(') {
            // Paréntesis de apertura entra a la pila
            stack[++top] = c;
        } else if (c == ')') {
            // Vaciamos pila a la salida hasta encontrar el '('
            while (top != -1 && stack[top] != '(') {
                result.items[result.size++].value = stack[top--];
            }
            if (top != -1) top--; // Eliminamos el '('
        } else {
            // Operadores (*, ., |)
            while (top != -1 && precedence(stack[top]) >= precedence(c)) {
                result.items[result.size++].value = stack[top--];
            }
            stack[++top] = c;
        }
    }

    // Vaciamos cualquier operador restante de la pila
    while (top != -1) {
        result.items[result.size++].value = stack[top--];
    }

    return result;
invalid:
    fprintf(stderr, "Error: expresion regular invalida.\n");
    exit(EXIT_FAILURE);
}
