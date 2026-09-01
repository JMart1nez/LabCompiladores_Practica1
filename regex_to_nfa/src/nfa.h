#ifndef NFA_H
#define NFA_H

#include <stdbool.h>
#include <stddef.h>

#include "regex.h"

#define epsilon -1 /* Transicion que no consume entrada */
#define no_edge -1 /* Ausencia de arista */

/* En la construccion de Thompson ningun estado necesita mas de dos
 * aristas salientes, un estado que consume un simbolo tiene exactamente una (out1),
 * un estado de division tiene dos transiciones epsilon
 * y el estado de aceptacion no tiene ninguna 
 */
typedef struct
{
    int symbol; /* Caracter consumido psilon */
    int out1; /* Indice del estado destino, o no_edge */
    int out2; /* Segundo destino, solo valido si symbol == epsilon */
} state;

typedef struct
{
    state *states;
    int size; /* Numero de estados en uso */
    int capacity; /* Capacidad del arreglo */
    int start; /* Indice del estado inicial */
    int accept; /* Indice del estado de aceptacion */
} nfa;

/* Construye el NFA a partir de la regex en postfijo */
nfa regex_to_nfa(regex r);

/* Simula el NFA sobre s Devuelve true si la cadena es aceptada */
bool match_nfa(nfa n, const char *s, size_t len);

void free_nfa(nfa *n);

bool save_nfa(const nfa *n, const char *path);

#endif 