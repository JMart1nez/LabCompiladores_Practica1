#include "nfa.h"

#include <stdlib.h>

static nfa nfa_new(void)
{
    nfa n;
    n.states = NULL;
    n.size = 0;
    n.capacity = 0;
    n.start = no_edge;
    n.accept = no_edge;
    return n;
}

/* Agrega un estado al arreglo y devuelve su indice o no_edge si no hubo memoria
 * Los estados se referencian por indice y no por puntero por la fun
 * realloc que puede mover el bloque completo a otra direccion, 
 * lo que invalidaria cualquier puntero guardado antes y así los índices siguen siendo válidos
*/
static int nfa_add_state(nfa *n, int symbol, int out1, int out2)
{
    if (n->size == n->capacity)
    {
        int new_cap = (n->capacity == 0) ? 32 : n->capacity * 2;
        state *tmp = realloc(n->states, (size_t)new_cap * sizeof(state));
        if (tmp == NULL)
        {
            return no_edge;
        }
        n->states   = tmp;
        n->capacity = new_cap;
    }

    n->states[n->size].symbol = symbol;
    n->states[n->size].out1 = out1;
    n->states[n->size].out2 = out2;

    return n->size++;
}

void free_nfa(nfa *n)
{
    if (n == NULL)
    {
        return;
    }

    free(n->states);
    n->states = NULL;
    n->size = 0;
    n->capacity = 0;
    n->start = no_edge;
    n->accept = no_edge;
}

/* NFA que no acepta ninguna cadena, o sea dos estados sin conexion entre si
 * Se devuelve cuando la expresion regular viene mal formada, el programa responde 0 */
static nfa nfa_reject_all(void)
{
    nfa n = nfa_new();
    n.start = nfa_add_state(&n, epsilon, no_edge, no_edge);
    n.accept = nfa_add_state(&n, epsilon, no_edge, no_edge);
    return n;
}


/* 
 * Un subautomata es un subNFA con un unico estado inicial y un unico estado de aceptacion que todavia no tiene aristas salientes
 */
typedef struct
{
    int start;
    int accept;
} subautomata;

/* 
 * Algoritmo de Thompson
 */
nfa regex_to_nfa(regex r)
{
    nfa n = nfa_new();

    /* Expresion vacia, un solo estado que es inicio y aceptacion a la vez, 
     * el automata acepta unicamente la cadena vacia 
     */
    if (r.size == 0 || r.items == NULL)
    {
        int s = nfa_add_state(&n, epsilon, no_edge, no_edge);
        n.start = s;
        n.accept = s;
        return n;
    }

    /* Cada token empuja a lo más un subautomata, asi que la pila nunca
     * necesita mas entradas que los tokens  que hay en la expresion 
     */
    subautomata *stack = malloc((size_t)r.size * sizeof(subautomata));
    int top = 0;

    if (stack == NULL)
    {
        return nfa_reject_all();
    }

    for (int i = 0; i < r.size; i++)
    {
        char c = r.items[i].value;

        switch (c)
        {
            /* Concatenacion f1 f2
             *
             * No se crean estados nuevos, el aceptador de f1 deja de
             * serlo y se convierte en una transicion epsilon que entra
             * al inicio de f2
             */
            case '.':
            {
                if (top < 2)
                {
                    goto malformed;
                }

                /* El ultimo subautomata apilado es el operando derecho */
                subautomata f2 = stack[--top];
                subautomata f1 = stack[--top];

                n.states[f1.accept].symbol = epsilon;
                n.states[f1.accept].out1 = f2.start;

                stack[top].start = f1.start;
                stack[top].accept = f2.accept;
                top++;
                break;
            }

            /* Union f1 | f2
             * Un estado inicial nuevo se divide hacia ambos operandos
             * con dos transiciones epsilon y los aceptadores viejos
             * se unen en un aceptador nuevo
             */
            case '|':
            {
                if (top < 2)
                {
                    goto malformed;
                }

                subautomata f2 = stack[--top];
                subautomata f1 = stack[--top];

                int acc = nfa_add_state(&n, epsilon, no_edge, no_edge);
                int st = nfa_add_state(&n, epsilon, f1.start, f2.start);

                if (acc == no_edge || st == no_edge)
                {
                    goto malformed;
                }

                n.states[f1.accept].out1 = acc;
                n.states[f2.accept].out1 = acc;

                stack[top].start = st;
                stack[top].accept = acc;
                top++;
                break;
            }

            /* Cerradura de Kleene f*
             * El estado inicial nuevo puede entrar al subautomata o saltarlo por completo,
             * el aceptador viejo puede regresar al inicio del subautomata o salir hacia el aceptador nuevo
             *
             * El ciclo epsilon que se forma aqui es el que obliga a marcar los estados ya visitados durante la
             * cerradura epsilon
            */
            case '*':
            {
                if (top < 1)
                {
                    goto malformed;
                }

                subautomata f = stack[--top];

                int acc = nfa_add_state(&n, epsilon, no_edge, no_edge);
                int st = nfa_add_state(&n, epsilon, f.start, acc);

                if (acc == no_edge || st == no_edge)
                {
                    goto malformed;
                }

                n.states[f.accept].out1 = f.start;
                n.states[f.accept].out2 = acc;

                stack[top].start = st;
                stack[top].accept = acc;
                top++;
                break;
            }

            /*
             * Cerradura positiva  f+
             * Igual que la de Kleene pero sin la arista que salta el subautomata de modo que se exige al menos una repeticion
             */
            case '+':
            {
                if (top < 1)
                {
                    goto malformed;
                }

                subautomata f = stack[--top];

                int acc = nfa_add_state(&n, epsilon, no_edge, no_edge);
                int st = nfa_add_state(&n, epsilon, f.start, no_edge);

                if (acc == no_edge || st == no_edge)
                {
                    goto malformed;
                }

                n.states[f.accept].out1 = f.start;
                n.states[f.accept].out2 = acc;

                stack[top].start = st;
                stack[top].accept = acc;
                top++;
                break;
            }

            /* Un parentesis en el postfijo significa que la expresion
             * venia desbalanceada, es invalida
             */
            case '(':
            case ')':
                goto malformed;

            /* Caso base s0--c--> s1
            */
            default:
            {
                /* Se crea primero el destino porque su indice hace
                 * falta para construir el estado que lo apunta 
                 */
                int s1 = nfa_add_state(&n, epsilon, no_edge, no_edge);
                int s0 = nfa_add_state(&n, (unsigned char)c, s1, no_edge);

                if (s1 == no_edge || s0 == no_edge)
                {
                    goto malformed;
                }

                stack[top].start = s0;
                stack[top].accept = s1;
                top++;
                break;
            }
        }
    }

    /* Una expresion bien formada deja exactamente un subautomata o sea el
     * automata completo, cero o mas de uno significa que faltaban o
     * sobraban operandos
     */
    if (top != 1)
    {
        goto malformed;
    }

    n.start = stack[0].start;
    n.accept = stack[0].accept;

    free(stack);
    return n;

malformed:
    free(stack);
    free_nfa(&n);
    return nfa_reject_all();
}