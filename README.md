**Regex to NFA Converter**

Programa que convierte expresiones regulares en Autómatas Finitos No Deterministas (NFA) utilizando el algoritmo de construcción de Thompson.

**Requisitos Previos:**

* **Docker Desktop** (Para la ejecución en Windows)
* **WSL2 / Linux** (Para compilación directa y desarrollo local)
* **CMake** (v3.10 o superior) y **GCC/G++** (Para compilación manual)

**Compilación y Ejecución con Docker**

El proyecto incluye un entorno Docker preconfigurado que instala las herramientas necesarias, compila y ejecuta el validador automático.

1. **Construir la imagen de Docker:**
   ```bash
   docker build -t nfa .

2. **Ejecutar el validador:**
   ```bash
   docker run --rm nfa

**Compilación manual (CMake)**

```bash
cmake -B regex_to_nfa/build -S regex_to_nfa
cmake --build regex_to_nfa/build
```

Esto genera tres ejecutables dentro de `regex_to_nfa/build`: `regex_to_nfa` (el validador), `unit_tests` y `nfa_visualizer`.

**Pruebas unitarias**

Cubren el parser, la construcción de Thompson y `match_nfa`.

```bash
./regex_to_nfa/build/unit_tests
# o bien:
ctest --test-dir regex_to_nfa/build
```

**Visualizador del NFA**

Genera un HTML con el diagrama del autómata a partir del JSON que produce `regex_to_nfa -o`.

```bash
echo "a(b|c)*" | ./regex_to_nfa/build/regex_to_nfa -o nfa.json
./regex_to_nfa/build/nfa_visualizer nfa.json salida.html
```

Abre `salida.html` en el navegador para ver el diagrama.
