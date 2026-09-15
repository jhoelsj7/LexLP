# LexLP — Analizador Léxico para el lenguaje LP

**Curso:** Compiladores
**Docente:** Prof. Nury Y. Arosquipa Yanque
**Lenguaje de implementación:** C++17

Analizador léxico que lee un programa fuente escrito en LP y lo transforma en
una secuencia de tokens, reportando además los errores léxicos con su ubicación
exacta (línea y columna).

El autómata está implementado **a mano**: cada expresión regular de la
especificación se traduce directamente a código que recorre el texto carácter
por carácter. No se utiliza ninguna librería de expresiones regulares ni
generadores tipo Lex/Flex.

---

## Estado por fases

| Entrega | Contenido | Estado |
|---|---|---|
| Semana 1 (15/09) | Lectura del archivo + `NUM_INT` + `NUM_DEC` |  Implementado |
### Expresiones regulares implementadas

| ER | Token | Ejemplo |
|---|---|---|
| `D = [0-9]` | — | — |
| `D+` | `NUM_INT` | `20` |
| `D+\.D+` | `NUM_DEC` | `15.5` |

Cualquier otro carácter se reporta por ahora como `ERROR_LEXICO`, porque los
demás tokens aún no forman parte de esta fase.

---

## Estructura del proyecto

```
LexLP/
├── src/
│   ├── main.cpp        # lectura del archivo y reportes de salida
│   ├── Lexer.h/.cpp    # el autómata: recorre el fuente y reconoce tokens
│   └── Token.h/.cpp    # estructura Token y nombres de los tipos
├── tests/
│   ├── prueba_semana1.lp         # números enteros y decimales (fase actual)
│   ├── prueba_semana1_limites.lp # casos límite: 15.  .5  3..14  @
│   ├── prueba_basica.lp     # variables y tabla de símbolos (Semana 2)
│   ├── prueba_completa.lp   # programa completo en LP (Semana 3)
│   └── prueba_errores.lp    # errores léxicos (Semana 3)
├── output/
│   ├── tokens.txt      # generado al ejecutar
│   └── errores.txt     # generado al ejecutar
├── compilar.bat
└── README.md
```

---

## Compilación y ejecución

```bat
compilar.bat
LexLP.exe tests\prueba_semana1.lp
```

O de forma manual:

```bat
g++ -std=c++17 -Wall -o LexLP.exe src\main.cpp src\Lexer.cpp src\Token.cpp
```

Si no se indica archivo, se usa `tests/prueba_semana1.lp` por defecto.

---

## Decisiones de diseño

- **Lectura completa del archivo en memoria.** El analizador necesita poder
  mirar el carácter siguiente (*lookahead*) aunque se encuentre al final de una
  línea; leer línea por línea rompería esa capacidad.

- **Lookahead de un carácter para el punto decimal.** El punto solo se consume
  si después viene al menos un dígito. Por eso `15.5` produce `NUM_DEC`,
  mientras que `15.` produce `NUM_INT(15)` y el punto queda suelto para
  reportarse como error. Esto respeta la ER `D+\.D+`, que exige dígitos a
  ambos lados del punto.

- **Línea y columna se actualizan al consumir cada carácter**, dentro de
  `avanzar()`. Así todo token conoce su ubicación de inicio sin necesidad de
  recorrer el texto una segunda vez.

- **Los errores no entran en la lista de tokens.** Se acumulan en una lista
  aparte, de modo que el análisis continúa y se reportan *todos* los errores
  del archivo en una sola ejecución, en lugar de detenerse en el primero.

- **El campo `atributo` del token ya existe** (vale `-1` mientras no aplique).
  En la Semana 2 guardará la posición del identificador en la tabla de
  símbolos, y la impresión pasará automáticamente de `<NUM_INT>` a `<ID,0>`
  sin modificar el resto del código.
