/* Lee el JSON que genera save_nfa (opcion -o del validador) y arma una
 * pagina HTML con el diagrama del automata, para no depender de Graphviz
 * ni de ninguna libreria externa al momento de revisar el trabajo.
 *
 * Uso: ./nfa_visualizer entrada.json salida.html
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct
{
    int symbol;
    int out1;
    int out2;
} estado_vis;

static char *leer_archivo_completo(const char *ruta)
{
    FILE *f = fopen(ruta, "rb");
    if (f == NULL)
    {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long tam = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc((size_t)tam + 1);
    if (buffer == NULL)
    {
        fclose(f);
        return NULL;
    }

    size_t leidos = fread(buffer, 1, (size_t)tam, f);
    buffer[leidos] = '\0';
    fclose(f);
    return buffer;
}

/* Busca la llave literal (ej. "\"out1\":") y devuelve el entero que sigue.
 * En *fin se deja el cursor justo despues del numero, para encadenar
 * la busqueda de la siguiente llave sin volver a leer desde el principio */
static int leer_entero_despues_de(const char *texto, const char *llave, const char **fin)
{
    const char *p = strstr(texto, llave);
    if (p == NULL)
    {
        if (fin != NULL) *fin = texto;
        return 0;
    }

    p += strlen(llave);
    char *siguiente;
    long valor = strtol(p, &siguiente, 10);
    if (fin != NULL) *fin = siguiente;
    return (int)valor;
}

static void escribir_arista(FILE *out, const double *x, const double *y,
                             int origen, int destino, const char *etiqueta,
                             double desplazamiento)
{
    double x1 = x[origen], y1 = y[origen];
    double x2 = x[destino], y2 = y[destino];

    if (origen == destino)
    {
        double cy = y1 - 70;
        fprintf(out,
            "<path class=\"arista\" d=\"M%.1f,%.1f C%.1f,%.1f %.1f,%.1f %.1f,%.1f\" marker-end=\"url(#flecha)\" />\n",
            x1 - 14, y1 - 18, x1 - 40, cy, x1 + 40, cy, x1 + 14, y1 - 18);
        fprintf(out,
            "<text class=\"simbolo\" x=\"%.1f\" y=\"%.1f\" text-anchor=\"middle\">%s</text>\n",
            x1, cy - 6, etiqueta);
        return;
    }

    double dx = x2 - x1, dy = y2 - y1;
    double largo = sqrt(dx * dx + dy * dy);
    if (largo < 1e-6) largo = 1e-6;

    double ux = dx / largo, uy = dy / largo;
    double px = -uy, py = ux;

    /* se acorta el segmento en los extremos para no tapar los circulos
     * de los estados y se desvia el punto de control para separar
     * aristas paralelas (los dos out de un mismo estado de division) */
    double sx = x1 + ux * 24, sy = y1 + uy * 24;
    double exf = x2 - ux * 24, eyf = y2 - uy * 24;
    double mx = (x1 + x2) / 2 + px * desplazamiento;
    double my = (y1 + y2) / 2 + py * desplazamiento;

    fprintf(out,
        "<path class=\"arista\" d=\"M%.1f,%.1f Q%.1f,%.1f %.1f,%.1f\" marker-end=\"url(#flecha)\" />\n",
        sx, sy, mx, my, exf, eyf);
    fprintf(out,
        "<text class=\"simbolo\" x=\"%.1f\" y=\"%.1f\" text-anchor=\"middle\">%s</text>\n",
        mx, my - 6, etiqueta);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <nfa.json> <salida.html>\n", argv[0]);
        return 1;
    }

    char *contenido = leer_archivo_completo(argv[1]);
    if (contenido == NULL)
    {
        fprintf(stderr, "No se pudo abrir '%s'\n", argv[1]);
        return 1;
    }

    const char *cursor;
    int inicio = leer_entero_despues_de(contenido, "\"start\":", &cursor);
    int aceptacion = leer_entero_despues_de(contenido, "\"accept\":", &cursor);

    estado_vis *estados = NULL;
    int total = 0, capacidad = 0;

    const char *p = contenido;
    while ((p = strstr(p, "\"symbol\":")) != NULL)
    {
        estado_vis e;
        const char *tras_symbol, *tras_out1, *tras_out2;

        e.symbol = leer_entero_despues_de(p, "\"symbol\":", &tras_symbol);
        e.out1   = leer_entero_despues_de(tras_symbol, "\"out1\":", &tras_out1);
        e.out2   = leer_entero_despues_de(tras_out1, "\"out2\":", &tras_out2);

        if (total == capacidad)
        {
            capacidad = (capacidad == 0) ? 16 : capacidad * 2;
            estado_vis *tmp = realloc(estados, (size_t)capacidad * sizeof(estado_vis));
            if (tmp == NULL)
            {
                fprintf(stderr, "Sin memoria al leer los estados.\n");
                free(estados);
                free(contenido);
                return 1;
            }
            estados = tmp;
        }
        estados[total++] = e;

        p = tras_out2;
    }
    free(contenido);

    if (total == 0 || inicio < 0 || inicio >= total)
    {
        fprintf(stderr, "El JSON no tiene un NFA valido.\n");
        free(estados);
        return 1;
    }

    /* BFS desde el estado inicial para acomodar el dibujo en niveles */
    int *nivel = malloc((size_t)total * sizeof(int));
    int *cola = malloc((size_t)total * sizeof(int));
    for (int i = 0; i < total; i++) nivel[i] = -1;

    int frente = 0, final = 0;
    nivel[inicio] = 0;
    cola[final++] = inicio;
    while (frente < final)
    {
        int actual = cola[frente++];
        int vecinos[2] = { estados[actual].out1, estados[actual].out2 };
        for (int k = 0; k < 2; k++)
        {
            int v = vecinos[k];
            if (v >= 0 && v < total && nivel[v] == -1)
            {
                nivel[v] = nivel[actual] + 1;
                cola[final++] = v;
            }
        }
    }
    free(cola);

    int nivel_max = 0;
    for (int i = 0; i < total; i++) if (nivel[i] > nivel_max) nivel_max = nivel[i];
    for (int i = 0; i < total; i++)
    {
        if (nivel[i] == -1)
        {
            nivel[i] = nivel_max + 1;
        }
    }
    for (int i = 0; i < total; i++) if (nivel[i] > nivel_max) nivel_max = nivel[i];

    int *conteo = calloc((size_t)nivel_max + 1, sizeof(int));
    int *fila = malloc((size_t)total * sizeof(int));
    for (int i = 0; i < total; i++)
    {
        fila[i] = conteo[nivel[i]]++;
    }

    int max_por_nivel = 0;
    for (int l = 0; l <= nivel_max; l++) if (conteo[l] > max_por_nivel) max_por_nivel = conteo[l];

    const double espacio_x = 160.0, espacio_y = 110.0, margen = 70.0;
    double *x = malloc((size_t)total * sizeof(double));
    double *y = malloc((size_t)total * sizeof(double));
    for (int i = 0; i < total; i++)
    {
        x[i] = margen + nivel[i] * espacio_x;
        y[i] = margen + fila[i] * espacio_y;
    }

    double ancho = margen * 2 + nivel_max * espacio_x + 40;
    double alto = margen * 2 + (max_por_nivel - 1) * espacio_y + 40;
    if (alto < 200) alto = 200;

    FILE *out = fopen(argv[2], "w");
    if (out == NULL)
    {
        fprintf(stderr, "No se pudo crear '%s'\n", argv[2]);
        free(estados); free(nivel); free(conteo); free(fila); free(x); free(y);
        return 1;
    }

    fprintf(out,
        "<!DOCTYPE html>\n<html lang=\"es\">\n<head>\n<meta charset=\"UTF-8\">\n"
        "<title>NFA generado</title>\n"
        "<style>\n"
        "  body { font-family: Arial, sans-serif; background: #fafafa; padding: 24px; }\n"
        "  .estado, .aceptacion { fill: white; stroke: #333; stroke-width: 2; }\n"
        "  .etiqueta { font-size: 13px; fill: #222; }\n"
        "  .simbolo { font-size: 13px; fill: #005b96; font-weight: bold; }\n"
        "  .arista { fill: none; stroke: #666; stroke-width: 1.5; }\n"
        "</style>\n</head>\n<body>\n"
        "<h2>Automata Finito No Determinista</h2>\n"
        "<p>Estado inicial: q%d &nbsp;&nbsp; Estado de aceptacion: q%d &nbsp;&nbsp; Total de estados: %d</p>\n"
        "<svg width=\"%.0f\" height=\"%.0f\">\n"
        "<defs>\n"
        "  <marker id=\"flecha\" markerWidth=\"10\" markerHeight=\"10\" refX=\"9\" refY=\"3\" orient=\"auto\" markerUnits=\"strokeWidth\">\n"
        "    <path d=\"M0,0 L0,6 L9,3 z\" fill=\"#666\" />\n"
        "  </marker>\n"
        "</defs>\n",
        inicio, aceptacion, total, ancho, alto);

    /* flecha suelta que marca cual es el estado inicial */
    fprintf(out,
        "<line class=\"arista\" x1=\"%.1f\" y1=\"%.1f\" x2=\"%.1f\" y2=\"%.1f\" marker-end=\"url(#flecha)\" />\n",
        x[inicio] - 45, y[inicio], x[inicio] - 22, y[inicio]);

    for (int i = 0; i < total; i++)
    {
        char etiqueta[8];
        if (estados[i].symbol == -1)
        {
            strcpy(etiqueta, "\xCE\xB5"); /* epsilon en UTF-8 */
        }
        else
        {
            etiqueta[0] = (char)estados[i].symbol;
            etiqueta[1] = '\0';
        }

        if (estados[i].out1 >= 0 && estados[i].out1 < total)
        {
            escribir_arista(out, x, y, i, estados[i].out1, etiqueta, 0.0);
        }
        if (estados[i].out2 >= 0 && estados[i].out2 < total)
        {
            escribir_arista(out, x, y, i, estados[i].out2, etiqueta, 40.0);
        }
    }

    for (int i = 0; i < total; i++)
    {
        if (i == aceptacion)
        {
            fprintf(out, "<circle class=\"aceptacion\" cx=\"%.1f\" cy=\"%.1f\" r=\"24\" />\n", x[i], y[i]);
            fprintf(out, "<circle class=\"aceptacion\" cx=\"%.1f\" cy=\"%.1f\" r=\"18\" />\n", x[i], y[i]);
        }
        else
        {
            fprintf(out, "<circle class=\"estado\" cx=\"%.1f\" cy=\"%.1f\" r=\"20\" />\n", x[i], y[i]);
        }
        fprintf(out,
            "<text class=\"etiqueta\" x=\"%.1f\" y=\"%.1f\" text-anchor=\"middle\" dominant-baseline=\"middle\">q%d</text>\n",
            x[i], y[i] + 1, i);
    }

    fprintf(out, "</svg>\n</body>\n</html>\n");
    fclose(out);

    printf("Visualizacion generada en %s (%d estados)\n", argv[2], total);

    free(estados); free(nivel); free(conteo); free(fila); free(x); free(y);
    return 0;
}
