#ifndef TOKEN_H
#define TOKEN_H

#include <string>

// ---------------------------------------------------------------------------
// Tipos de token del lenguaje LP.
//
// Variables base de la especificacion:
//      D = [0-9]
//      L = [a-zA-Z_]
//
// FASE ACTUAL (Semana 2): numericos + identificadores + texto + palabras
// reservadas + operadores + simbolos especiales. El comentario (COMENT =
// //.*\n) se reconoce pero no genera token: se descarta igual que un espacio.
// ---------------------------------------------------------------------------
enum class TipoToken {
    NUM_INT,        // ER:  D+                 ejemplo: 20
    NUM_DEC,        // ER:  D+ \. D+            ejemplo: 15.5
    ID,             // ER:  L (L|D)*            ejemplo: edad
    TEXTO,          // ER:  " .* "              ejemplo: "hola"

    // Palabras reservadas
    INT, FLOAT, CHAR, BOOLEAN, VOID,
    IF, ELSE, FOR, WHILE,
    SCANF, PRINTLN, MAIN, RETURN,

    // Operador de comparacion/relacional: > >= < <= != ==
    COMP,

    // Asignacion
    ASIGNACION,     // =

    // Aritmeticos
    SUMA, RESTA, MULT, DIV, MOD,               // + - * / %

    // Logicos
    AND, OR, NOT,                              // && || !

    // Simbolos especiales
    PAR_IZQ, PAR_DER,      // ( )
    COR_IZQ, COR_DER,      // [ ]
    LLAVE_IZQ, LLAVE_DER,  // { }
    COMA, PUNTOCOMA,       // , ;

    ERROR_LEXICO,   // caracter (o cadena) que no pertenece al alfabeto de LP
    FIN_ARCHIVO     // marca interna: se llego al final de la entrada
};

// Nombre del tipo de token tal como debe aparecer en los reportes.
// Los tokens con nombre propio (NUM_INT, ID, COMP, las reservadas, ...)
// devuelven su nombre. Los "tokens directos" (=, +, (, ...) devuelven su
// propio simbolo, tal como pide la especificacion.
std::string nombreTipoToken(TipoToken tipo);

// ---------------------------------------------------------------------------
// Un token reconocido en el programa fuente.
// 'atributo' guarda la posicion en la tabla de simbolos cuando el token es
// un identificador (ID). Para el resto de tokens vale -1 = no aplica.
// ---------------------------------------------------------------------------
struct Token {
    TipoToken   tipo;
    std::string lexema;    // el texto exacto que se leyo del archivo
    int         atributo;  // posicion en la tabla de simbolos, -1 si no aplica
    int         linea;     // linea donde inicia el lexema (empieza en 1)
    int         columna;   // columna donde inicia el lexema (empieza en 1)

    Token(TipoToken tipo,
          const std::string& lexema,
          int linea,
          int columna,
          int atributo = -1);

    // Formato pedido para la lista de tokens:  <NUM_INT>   y luego  <ID,0>
    std::string aCadena() const;
};

#endif // TOKEN_H
