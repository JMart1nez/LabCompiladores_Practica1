**Practica 1**
**Equipo**
- Castro Hernández Rafael
- Martínez Leal José María
- Ortíz Vásquez Gustavo Angel
- Gomez Calva Carlos Manuel

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
