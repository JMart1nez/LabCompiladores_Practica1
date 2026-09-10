#include "../src/regex.h"
#include "../src/nfa.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static int pruebas_totales = 0;
static int pruebas_ok = 0;

#define CHECA(cond, descripcion) \
    do { \
        pruebas_totales++; \
        if (cond) { pruebas_ok++; } \
        else { printf("  [FALLO] %s (linea %d)\n", descripcion, __LINE__); } \
    } while (0)

/* parse_regex termina el programa con exit(1) cuando la entrada es invalida,
 * asi que la unica forma de comprobar ese caso sin matar el resto de las
 * pruebas es correrlo en un proceso hijo y revisar como murio */
static int falla_con_entrada_invalida(const char *regex_str)
{
    /* si no se vacia el buffer antes de fork(), el hijo hereda una copia
     * y la vuelve a imprimir al terminar con exit(), duplicando lineas */
    fflush(stdout);

    pid_t pid = fork();
    if (pid == 0)
    {
        freopen("/dev/null", "w", stderr);
        parse_regex(regex_str);
        _exit(0);
    }

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 1;
}

static char *postfijo_como_cadena(regex r)
{
    char *out = malloc((size_t)r.size + 1);
    for (int i = 0; i < r.size; i++)
    {
        out[i] = r.items[i].value;
    }
    out[r.size] = '\0';
    return out;
}

static void prueba_parser(void)
{
    printf("Parser (regex -> postfijo)\n");

    struct { const char *entrada; const char *esperado; } casos[] = {
        { "a",        "a" },
        { "ab",       "ab." },
        { "a|b",      "ab|" },
        { "a*",       "a*" },
        { "a+",       "a+" },
        { "a(b|c)*",  "abc|*." },
        { "",         "" },
    };

    for (size_t i = 0; i < sizeof(casos) / sizeof(casos[0]); i++)
    {
        regex r = parse_regex(casos[i].entrada);
        char *postfijo = postfijo_como_cadena(r);

        char msg[128];
        snprintf(msg, sizeof(msg), "\"%s\" -> \"%s\"", casos[i].entrada, casos[i].esperado);
        CHECA(strcmp(postfijo, casos[i].esperado) == 0, msg);

        free(postfijo);
        free(r.items);
    }

    printf("Rechazo de expresiones invalidas\n");
    const char *invalidas[] = { "a|", "*a", "(a", "a)", ")(", "|" };
    for (size_t i = 0; i < sizeof(invalidas) / sizeof(invalidas[0]); i++)
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "\"%s\" debe abortar el programa", invalidas[i]);
        CHECA(falla_con_entrada_invalida(invalidas[i]), msg);
    }
}

static void prueba_construccion_nfa(void)
{
    printf("Construccion del NFA (Thompson)\n");

    regex r = parse_regex("a");
    nfa n = regex_to_nfa(r);
    CHECA(n.size == 2, "un solo simbolo produce dos estados");
    CHECA(n.start != n.accept, "inicio y aceptacion quedan separados");
    CHECA(n.states[n.start].symbol == 'a', "el estado inicial consume 'a'");
    CHECA(n.states[n.start].out1 == n.accept, "el estado inicial llega al de aceptacion");
    free_nfa(&n);
    free(r.items);

    regex vacia = parse_regex("");
    nfa nv = regex_to_nfa(vacia);
    CHECA(nv.size == 1, "la regex vacia produce un solo estado");
    CHECA(nv.start == nv.accept, "en la regex vacia inicio y aceptacion coinciden");
    free_nfa(&nv);
    free(vacia.items);
}

static bool acepta(const char *patron, const char *cadena)
{
    regex r = parse_regex(patron);
    nfa n = regex_to_nfa(r);
    bool resultado = match_nfa(n, cadena, strlen(cadena));
    free_nfa(&n);
    free(r.items);
    return resultado;
}

static void prueba_match(void)
{
    printf("Simulacion del NFA (match_nfa)\n");

    /* Ejemplo del manual: a(b|c)*. La fe de erratas aclara que "ac" si pertenece */
    CHECA(acepta("a(b|c)*", "a") == true,        "a(b|c)* acepta \"a\"");
    CHECA(acepta("a(b|c)*", "ab") == true,       "a(b|c)* acepta \"ab\"");
    CHECA(acepta("a(b|c)*", "ac") == true,       "a(b|c)* acepta \"ac\" (fe de erratas)");
    CHECA(acepta("a(b|c)*", "abcbcbc") == true,  "a(b|c)* acepta repeticiones largas");
    CHECA(acepta("a(b|c)*", "abcca") == false,   "a(b|c)* rechaza \"abcca\"");
    CHECA(acepta("a(b|c)*", "ca") == false,      "a(b|c)* rechaza \"ca\"");
    CHECA(acepta("a(b|c)*", "") == false,        "a(b|c)* rechaza la cadena vacia");

    CHECA(acepta("a*", "") == true,     "a* acepta la cadena vacia");
    CHECA(acepta("a*", "aaaa") == true, "a* acepta repeticiones de 'a'");
    CHECA(acepta("a*", "b") == false,   "a* rechaza simbolos fuera del alfabeto");

    CHECA(acepta("a+", "") == false, "a+ exige al menos una repeticion");
    CHECA(acepta("a+", "a") == true, "a+ acepta una sola 'a'");

    CHECA(acepta("", "") == true,  "la regex vacia acepta la cadena vacia");
    CHECA(acepta("", "a") == false, "la regex vacia rechaza cualquier otra cadena");
}

static void prueba_save_nfa(void)
{
    printf("Serializacion a JSON (save_nfa)\n");

    regex r = parse_regex("a|b");
    nfa n = regex_to_nfa(r);

    const char *ruta = "tests_tmp_nfa.json";
    CHECA(save_nfa(&n, ruta) == true, "save_nfa escribe el archivo sin error");

    FILE *f = fopen(ruta, "r");
    CHECA(f != NULL, "el archivo generado se puede volver a abrir");
    if (f != NULL)
    {
        char buffer[4096];
        size_t leidos = fread(buffer, 1, sizeof(buffer) - 1, f);
        buffer[leidos] = '\0';
        CHECA(strstr(buffer, "\"start\"") != NULL,  "el JSON incluye el estado inicial");
        CHECA(strstr(buffer, "\"accept\"") != NULL, "el JSON incluye el estado de aceptacion");
        fclose(f);
    }
    remove(ruta);

    free_nfa(&n);
    free(r.items);
}

int main(void)
{
    prueba_parser();
    prueba_construccion_nfa();
    prueba_match();
    prueba_save_nfa();

    printf("\n%d/%d pruebas pasaron\n", pruebas_ok, pruebas_totales);
    return pruebas_ok == pruebas_totales ? 0 : 1;
}
