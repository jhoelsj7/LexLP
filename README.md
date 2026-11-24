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
| Semana 2 (22/09) | `ID` + `TEXTO` + palabras reservadas + tabla de símbolos | Implementado |
| Semana 3 (29/09) | Operadores + símbolos especiales + comentarios + integración completa |  Implementado |
| Sintáctico – Entregable 1 (13/11) | Integración lexer→parser, `PROGRAMA`, `PRINCIPAL`, `BLOQUE`, declaraciones, asignaciones, errores básicos | Implementado |
| Sintáctico – Entregable 2 (24/11) | AST, recuperación de errores (varios errores por ejecución), pruebas de funciones/IO/control | Implementado |

### Expresiones regulares implementadas

Variables base: `D = [0-9]` y `L = [a-zA-Z_]`.

| ER | Token | Ejemplo |
|---|---|---|
| `D+` | `NUM_INT` | `20` |
| `D+\.D+` | `NUM_DEC` | `15.5` |
| `L(L\|D)*` | `ID` | `edad` |
| `".*"` | `TEXTO` | `"hola"` |
| `int, float, char, boolean, void, if, else, for, while, scanf, println, main, return, func` | `INT, FLOAT, CHAR, BOOLEAN, VOID, IF, ELSE, FOR, WHILE, SCANF, PRINTLN, MAIN, RETURN, FUNC` | `if` |

Una palabra reservada tiene prioridad sobre `ID`: `if` produce `<IF>`, pero
`if2` sigue siendo un `ID` porque no coincide exactamente con la palabra
reservada.

### Operadores, símbolos especiales y comentarios (Semana 3)

| Lexema | Categoría | Token impreso |
|---|---|---|
| `=` | Asignación | `<=>` |
| `+` `-` `*` `/` `%` | Aritméticos | `<+>` `<->` `<*>` `</>` `<%>` |
| `&&` `\|\|` `!` | Lógicos | `<&&>` `<\|\|>` `<!>` |
| `>` `>=` `<` `<=` `!=` `==` | Relacionales | `<COMP>` (categoría única para las 6, según especificación) |
| `(` `)` `[` `]` `{` `}` `,` `;` | Símbolos especiales | `<(>` `<)>` `<[>` `<]>` `<{>` `<}>` `<,>` `<;>` |
| `//.*\n` | Comentario de línea | se descarta: no genera token ni error |

`/` se distingue de un comentario mirando el carácter siguiente: si también
es `/`, todo lo que sigue hasta el fin de línea se descarta; si no, `/` es el
operador de división.

Cualquier carácter (o cadena de texto sin cerrar) que no encaje en ninguna
de las reglas anteriores se reporta como `ERROR_LEXICO`, con su línea y
columna exactas.

---

## Estructura del proyecto

```
LexLP/
├── src/
│   ├── main.cpp        # LexLP: lectura del archivo y reportes de salida del lexer
│   ├── mainParser.cpp  # ParserLP: archivo .lp -> Lexer -> Parser -> ACEPTADO/RECHAZADO
│   ├── Parser.h/.cpp   # analizador sintáctico por descenso recursivo
│   ├── Lexer.h/.cpp    # el autómata: recorre el fuente y reconoce tokens
│   └── Token.h/.cpp    # estructura Token y nombres de los tipos
├── tests/
│   ├── prueba_semana1.lp          # números enteros y decimales (fase actual)
│   ├── prueba_semana1_limites.lp  # casos límite: 15.  .5  3..14  @
│   ├── prueba_basica.lp                    # variables y tabla de símbolos (Semana 2)
│   ├── prueba_identificadores.lp           # ID vs. palabras reservadas (if2, Int, IF, _privado...)
│   ├── prueba_identificadores_repetidos.lp # un ID repetido reutiliza su índice en la tabla
│   ├── prueba_texto_casos.lp               # TEXTO: normal, vacío y sin cerrar
│   ├── prueba_operadores_aritmeticos.lp    # + - * / %
│   ├── prueba_comparaciones.lp             # >= != == (categoría COMP)
│   ├── prueba_logicos.lp                   # && y !
│   ├── prueba_comentarios.lp               # // se descarta, no genera token
│   ├── prueba_completa.lp                  # programa completo en LP (Semana 3)
│   ├── prueba_errores.lp                   # errores léxicos (@, $)
│   ├── expected/                           # salida esperada de cada prueba (para run_tests.sh)
│   ├── run_tests.sh               # corre todas las pruebas y compara contra expected/
│   ├── sintactico/                # pruebas del parser: valido_*.lp y error_*.lp
│   │   └── salidas/               # salida de cada prueba (evidencia de ejecución)
│   └── run_tests_parser.sh        # corre las pruebas del parser
├── output/
│   ├── tokens.txt      # generado al ejecutar
│   ├── errores.txt     # generado al ejecutar
│   └── sintactico.txt  # resultado del último análisis sintáctico
├── compilar.bat
└── README.md
```

---

## Compilación y ejecución

```bat
compilar.bat
LexLP.exe tests\prueba_semana1.lp
```

### Funcionalidades adicionales

- **Coloreado sintáctico en consola**: cada categoría de token se imprime en
  un color distinto (números, identificadores, texto, palabras reservadas,
  errores en rojo). Los archivos exportados a `output/` quedan siempre en
  texto plano, sin códigos de color.

- **Visualización paso a paso**: `LexLP.exe <archivo.lp> --visualizar`
  recorre el archivo token por token, resaltando en el texto fuente
  completo cuál es el lexema que se acaba de reconocer, y espera Enter
  antes de continuar con el siguiente. Útil para explicar el
  funcionamiento del autómata en vivo.

- **Pruebas automatizadas**: `tests/run_tests.sh` compila el proyecto y
  corre todos los `.lp` de `tests/` que tengan un resultado guardado en
  `tests/expected/`, comparándolo con `diff`. Si un cambio de
  comportamiento es intencional, `tests/run_tests.sh --actualizar`
  regenera esa referencia.

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

- **El campo `atributo` del token** vale `-1` para todo token que no sea
  `ID`. Para un `ID` guarda su posición en la tabla de símbolos, y la
  impresión pasa automáticamente de `<TIPO>` a `<ID,0>` sin necesidad de
  tocar `Token::aCadena()`.

- **Tabla de símbolos como lista de identificadores únicos, en orden de
  aparición.** Al leer un `ID` se busca linealmente en la tabla: si ya
  existe se reutiliza su índice, si no, se agrega al final. Ese índice es
  el `atributo` del token. Es una búsqueda lineal (no un `map`) porque el
  volumen de identificadores de un programa fuente es pequeño y así el
  código queda igual de directo que el resto del autómata.

- **Palabras reservadas se resuelven después de reconocer el `ID` completo.**
  El lexema se compara contra una tabla fija (`int`, `if`, `main`, ...); si
  coincide exactamente se devuelve el token reservado (`<IF>`, `<MAIN>`,
  ...) y NO se registra en la tabla de símbolos. Por eso `if2` sigue siendo
  un `ID` normal: la coincidencia es exacta, no por prefijo.

- **Una cadena de texto (`".*"`) sin comilla de cierre en la misma línea**
  se reporta como `ERROR_LEXICO` (con el texto leído hasta el corte), en
  vez de dejar que el analizador arrastre el error a las líneas
  siguientes.


---

# Analizador Sintáctico (ParserLP)

## Flujo

```
archivo.lp ──► Lexer::analizarConFin() ──► vector<Token> (termina en FIN_ARCHIVO) ──► Parser::analizar() ──► ACEPTADO / RECHAZADO
```

El parser **no lee caracteres**: solo consume los tokens del lexer. Si hay
errores léxicos, se reportan y no se intenta el análisis sintáctico (los
tokens no serían confiables).

## Compilar y ejecutar

```bat
compilar.bat
ParserLP.exe tests\sintactico\valido_minimo.lp
ParserLP.exe tests\sintactico\error_falta_puntoycoma.lp
```

Manual: `g++ -std=c++17 -Wall -o ParserLP.exe src\mainParser.cpp src\Parser.cpp src\Lexer.cpp src\Token.cpp`

Código de salida: `0` aceptado, `1` rechazado (error léxico o sintáctico), `2` archivo no encontrado.
El resultado también se guarda en `output/sintactico.txt`.

Pruebas: `bash tests/run_tests_parser.sh` (los `valido_*.lp` deben aceptarse
y los `error_*.lp` rechazarse; la salida de cada uno queda en
`tests/sintactico/salidas/`).

## Ejemplo

```
$ ParserLP.exe tests/sintactico/error_falta_puntoycoma.lp
Error sintactico en linea 3, columna 5: se esperaba ';' pero se encontro 'edad' (identificador).
  Sugerencia: probablemente falta ';' al final de la linea 2, despues de 'edad'.

Resultado: PROGRAMA RECHAZADO (error sintactico)
```

## Implementación: descenso recursivo

Cada no terminal de la gramática LP es un método de `Parser` con el mismo
nombre (`PROGRAMA` → `programa()`, `LISTA_VARIABLES` → `listaVariables()`,
`EXPRESION_ADITIVA2` → `expresionAditiva2()`, ...). La producción está
escrita como comentario encima de cada método.

| Utilidad | Rol |
|---|---|
| `actual()` | token de *lookahead* (no lo consume) |
| `esperar(tipo, desc)` | *match*: consume el token si es el esperado, si no lanza el error |
| `iniciaComando()`, `iniciaExpresion()`, `esTipoBase()` | conjuntos PRIMERO para elegir la producción |

- **Producciones con ε**: si el token actual está en el PRIMERO de la
  alternativa, se sigue; si no, se toma ε sin consumir nada.
- **Recursión por la derecha** (`LISTA_ID`, `EXPRESION_ADITIVA2`, ...):
  `X → a Y X | ε` se implementa con un `while`; es equivalente y más simple.
- **Precedencia** (de menor a mayor): `||` → `&&` → `comp` → `+ -` → `* / %`
  → `FACTOR`. Sale directamente de la cadena de no terminales de la gramática.

## Terminales de la gramática ↔ tokens del lexer

| Gramática | Token |
|---|---|
| `id`, `num_int`, `num_dec`, `texto` | `ID`, `NUM_INT`, `NUM_DEC`, `TEXTO` |
| `comp` | `COMP` (`>`, `>=`, `<`, `<=`, `!=`, `==`) |
| `func` | `FUNC` (palabra reservada agregada al lexer para esta etapa) |
| `= + - * / % && \|\| !` | `ASIGNACION SUMA RESTA MULT DIV MOD AND OR NOT` |
| `( ) [ ] { } , ;` | `PAR_IZQ PAR_DER COR_IZQ COR_DER LLAVE_IZQ LLAVE_DER COMA PUNTOYCOMA` |

## Decisiones de implementación

- **`void main()`**: la gramática dice `PRINCIPAL → main ( ) BLOQUE`, pero los
  ejemplos del enunciado usan `void main()`. Como `void` también inicia una
  `DECLARACION_FUNCION`, en `listaFunciones()` se mira un token más (LL(2)):
  si después de `void` viene `main`, se termina la lista de funciones y
  `principal()` acepta ese `void` opcional.
- **Arreglos**: según `TIPO → TIPO_BASE DIMENSION` la dimensión va pegada al
  tipo: `float[10] notas;` (no `float notas[10];`). El acceso `notas[i]` se
  permite en expresiones, `scanf` y `println` (`DIMENSION2`), pero no a la
  izquierda de `=`, porque `ASIGNACION → id = COMPLEMENTO ;`.
- **Llamada como comando** (`func f(x)`): la gramática no pone `;` después de
  `FUNCION`; se acepta un `;` opcional para no rechazar `func f(x);`.
- **`return` exige expresión** (`return EXPRESION ;`): `return;` es error.
- **`for` no se reconoce**: no existe producción para él en la gramática.
- **Declaraciones antes de comandos** en cada `BLOQUE`; si aparece un tipo
  después de un comando, el mensaje lo indica explícitamente.
- **Errores (Entregable 1)**: modo pánico — al primer error se lanza
  `ErrorSintactico` y se detiene el análisis. El mensaje incluye línea,
  columna, lo esperado y el token encontrado; si falta un cierre (`;`, `)`,
  `}`, `]`) y el token encontrado está en una línea posterior, se sugiere la
  línea anterior como origen. La recuperación para reportar varios errores y
  la construcción del AST corresponden al Entregable 2.

## Casos de prueba (`tests/sintactico/`)

| Archivo | Esperado | Qué prueba |
|---|---|---|
| `valido_minimo.lp` | Aceptado | ejemplo mínimo del enunciado (4.2) |
| `valido_declaraciones.lp` | Aceptado | listas de ids, arreglos, todos los tipos, expresiones con precedencia |
| `valido_integral.lp` | Aceptado | función con parámetro, `if/else`, `while`, `func`, `scanf`, `println`, `return` |
| `error_falta_puntoycoma.lp` | Rechazado | ejemplo incorrecto del enunciado (4.3) |
| `error_llave_sin_cerrar.lp` | Rechazado | falta `}` del bloque |
| `error_parentesis.lp` | Rechazado | `(` sin cerrar en una expresión |
| `error_token_inesperado.lp` | Rechazado | `edad = = 20;` |
| `error_declaracion_tras_comando.lp` | Rechazado | declaración después de un comando |
| `error_sin_main.lp` | Rechazado | variable global / falta `main` |

Además, `tests/sintactico/validos/` (v01–v04: mínimo, integral, funciones con arreglos, expresiones) y
`tests/sintactico/invalidos/` (i01–i08, incluye `return;` sin expresión, error léxico y falta de `main`).
En Windows también se pueden correr con `probar_parser.bat` (después de `compilar.bat`).

---

# Analizador Sintáctico – Entregable 2

## Qué se agregó

| Parte | Dónde | Descripción |
|---|---|---|
| AST | `src/Ast.h` (nuevo), `src/Parser.h/.cpp` | cada método del parser devuelve el nodo del árbol que reconoció |
| Recuperación de errores | `src/Parser.cpp` | el análisis continúa después de un error y se reportan **todos** en una sola ejecución |
| Opción `--ast` | `src/mainParser.cpp` | imprime el AST en consola; el AST de un programa aceptado siempre se guarda en `output/ast.txt` |
| Pruebas nuevas | `tests/sintactico/validos/v05–v06`, `tests/sintactico/invalidos/i09–i11` | funciones, entrada/salida, control anidado y programas con varios errores |
| Evidencia del AST | `tests/sintactico/salidas/ast/` | `run_tests_parser.sh` guarda el AST de cada programa válido |

`Ast.h` es solo un header, así que `compilar.bat` y los comandos de compilación no cambian.

```bat
compilar.bat
ParserLP.exe tests\sintactico\validos\v06_control.lp --ast
ParserLP.exe tests\sintactico\invalidos\i09_varios_errores.lp
```

## Árbol de Sintaxis Abstracta (AST)

Todos los nodos tienen la misma forma: `tipo`, `valor` (nombre, operador o
constante), `linea` e `hijos`. El AST **no** guarda los símbolos que solo
sirven a la sintaxis (`( ) { } ; ,`) ni los no terminales auxiliares
(`EXPRESION_O2`, `LISTA_ID`, ...).

| Construcción LP | Nodo | Hijos |
|---|---|---|
| programa | `PROGRAMA` | `FUNCION`..., `PRINCIPAL` |
| `int suma(int a, int b) {...}` | `FUNCION int suma` | `PARAMETROS` (`PARAMETRO int a`...), `BLOQUE` |
| `main() {...}` | `PRINCIPAL main` | `BLOQUE` |
| `{ ... }` | `BLOQUE` | declaraciones y comandos en orden |
| `int a, b;` | `DECLARACION int` | `VARIABLE a`, `VARIABLE b` |
| `x = e;` | `ASIGNACION x` | expresión o `LLAMADA` |
| `scanf(x, v[i]);` | `LECTURA` | `ID`, `ACCESO_ARREGLO` |
| `println("t", x);` | `ESCRITURA` | `TEXTO`, `ID`, `NUM_INT`... |
| `if (c) {...} else {...}` | `SI` | `CONDICION`, `BLOQUE`, `SINO` → `BLOQUE` |
| `while (c) {...}` | `MIENTRAS` | `CONDICION`, `BLOQUE` |
| `return e;` | `RETORNO` | expresión |
| `func f(a, b)` | `LLAMADA f` | argumentos |
| `a + b`, `x >= 1`, `p && q` | `OPERACION <op>` | operando izquierdo, operando derecho |
| `!e` / `-x` | `NEGACION !` / `SIGNO -` | operando |
| `v[i][j]` | `ACCESO_ARREGLO v` | índices |
| `x`, `20`, `15.5`, `"hola"` | `ID`, `NUM_INT`, `NUM_DEC`, `TEXTO` | — |

- **Asociatividad por la izquierda:** cada `X2` recibe el operando izquierdo
  ya construido y lo envuelve en `OPERACION`, así `a - b - c` queda
  `(a - b) - c`.
- **La precedencia la da la forma del árbol:** `a + b * c` deja la `*` más
  abajo que la `+`. Por eso `( EXPRESION )` no genera un nodo propio.

Ejemplo (`ParserLP.exe programa.lp --ast`):

```
PROGRAMA  (linea 1)
`-- PRINCIPAL main  (linea 1)
    `-- BLOQUE  (linea 1)
        |-- DECLARACION int  (linea 2)
        |   `-- VARIABLE edad  (linea 2)
        `-- ASIGNACION edad  (linea 3)
            `-- OPERACION +  (linea 3)
                |-- NUM_INT 20  (linea 3)
                `-- NUM_INT 1  (linea 3)
```

## Recuperación de errores: modo pánico con sincronización

1. El método que detecta el error lanza `ErrorSintactico`, igual que en el
   Entregable 1, con el mismo mensaje (línea, columna, lo esperado y lo
   encontrado).
2. Lo atrapan los **puntos de recuperación**: cada `COMANDO`, cada
   declaración de `LISTA_VARIABLES`, el cierre de `BLOQUE` y las cabeceras
   de función y de `main`.
3. Se registra el error y se descartan tokens hasta un **token de
   sincronización**:

| Token | Acción |
|---|---|
| `;` | se consume: termina la instrucción con error |
| `}` | no se consume: lo cierra el `BLOQUE` que corresponde |
| `{` | se salta el bloque completo, que pertenece a la instrucción con error |
| `scanf`, `println`, `if`, `while`, `return` | inicio del siguiente comando |
| comando o tipo al inicio de una **línea nueva** | caso típico: falta `;` y la siguiente instrucción está debajo |

- **Sin errores en cascada:** un mismo token nunca genera dos errores.
- **Declaración tardía:** después del error, la declaración se analiza igual
  y el bloque continúa.
- **Error en una cabecera** (`int suma(int a int b)`): se salta hasta el `{`
  del cuerpo y se analiza el cuerpo normalmente.
- Si la recuperación llega al fin de archivo (por ejemplo, falta la `}`
  final), el análisis termina ahí.
- Un programa con un solo error produce exactamente la misma salida que en
  el Entregable 1. Con varios errores, la salida dice `Fase sintactica:  N errores`
  y los lista en orden.

Ejemplo (`tests/sintactico/invalidos/i09_varios_errores.lp`):

```
Fase sintactica:  5 errores

Error sintactico en linea 4, columna 5: se esperaba ';' pero se encontro 'float'.
  Sugerencia: probablemente falta ';' al final de la linea 3, despues de 'edad'.

Error sintactico en linea 5, columna 16: se esperaba una expresion (identificador, numero, texto, '!' o '(') pero se encontro ';'.

Error sintactico en linea 7, columna 26: se esperaba ')' pero se encontro ';'.

Error sintactico en linea 8, columna 19: se esperaba una expresion (identificador, numero, texto, '!' o '(') pero se encontro ')'.

Error sintactico en linea 11, columna 11: se esperaba una expresion (identificador, numero, texto, '!' o '(') pero se encontro ';'.

Resultado: PROGRAMA RECHAZADO (error sintactico)
```

## Casos de prueba nuevos

| Archivo | Esperado | Qué prueba |
|---|---|---|
| `validos/v05_funciones_io.lp` | Aceptado | funciones `float`/`int`/`void` con varios parámetros y arreglos, llamadas con `func`, `scanf` y `println` con varios términos |
| `validos/v06_control.lp` | Aceptado | `while` con `if/else` anidados, `&&`, `\|\|`, `!`, signo unario y `%` |
| `invalidos/i09_varios_errores.lp` | Rechazado (5 errores) | falta `;`, expresión incompleta, `)` faltante en `println`, condición vacía, `return;` |
| `invalidos/i10_errores_funciones.lp` | Rechazado (6 errores) | error en la cabecera de una función, en `scanf`, en `func`, en `println`, en `if` y `= =` |
| `invalidos/i11_errores_declaraciones.lp` | Rechazado (3 errores) | `int a, ;`, `float[] b;`, declaración después de un comando |

`bash tests/run_tests_parser.sh` corre las 26 pruebas: las 21 del Entregable 1
siguen dando exactamente la misma salida.
