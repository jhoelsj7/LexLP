#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# Pruebas del analizador sintactico (ParserLP).
#
# Convencion de nombres en tests/sintactico/:
#   valido_*.lp  -> debe ser ACEPTADO  (codigo de salida 0)
#   error_*.lp   -> debe ser RECHAZADO (codigo de salida 1)
# La salida de cada caso se guarda en tests/sintactico/salidas/ como
# evidencia de ejecucion.
# ---------------------------------------------------------------------------
set -uo pipefail
cd "$(dirname "$0")/.."

BIN="$(mktemp -d)/ParserLP.exe"
g++ -std=c++17 -Wall -Wextra -static -o "$BIN" src/mainParser.cpp src/Parser.cpp src/Lexer.cpp src/Token.cpp || exit 1
mkdir -p tests/sintactico/salidas

total=0; fallidas=0
for archivo in tests/sintactico/*.lp; do
    base=$(basename "$archivo" .lp)
    "$BIN" "$archivo" > "tests/sintactico/salidas/$base.txt"
    codigo=$?
    case "$base" in
        valido_*) esperado=0 ;;
        *)        esperado=1 ;;
    esac
    total=$((total + 1))
    if [[ $codigo -eq $esperado ]]; then
        echo "OK      $base"
    else
        echo "FALLO   $base (salida $codigo, se esperaba $esperado)"
        fallidas=$((fallidas + 1))
    fi
done
rm -rf "$(dirname "$BIN")"
echo ""
echo "Total: $total pruebas, $fallidas fallidas."
[[ $fallidas -eq 0 ]]
