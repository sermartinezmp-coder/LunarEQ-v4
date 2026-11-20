# LUNAR EQ — AETHEREON AUDIO (v2.0)

EQ paramétrico VST3 de 16 slots.

## Fuentes (opcional pero recomendado)
Copia estos 5 archivos en la carpeta `fonts/` antes de compilar:
Jost-Light.ttf, Jost-Regular.ttf, Jost-Medium.ttf, Inter_18pt-Regular.ttf, Inter_18pt-Medium.ttf.
Si faltan, el plugin compila igual y usa la fuente del sistema.

## Compilar
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
