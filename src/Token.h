#ifndef TOKEN_H
#define TOKEN_H

#include <string>

// ---------------------------------------------------------------------------
// Tipos de token del lenguaje LP.
// FASE ACTUAL (Semana 1): solo se reconocen los numericos.
// Los demas tipos (ID, TEXTO, palabras reservadas, operadores, simbolos)
// se agregan en las Semanas 2 y 3 anadiendo valores a este enum.
// ---------------------------------------------------------------------------
enum class TipoToken {
    NUM_INT,        // ER:  D+          ejemplo: 20
    NUM_DEC,        // ER:  D+ \. D+    ejemplo: 15.5
    ERROR_LEXICO,   // caracter que no pertenece al alfabeto de LP
    FIN_ARCHIVO     // marca interna: se llego al final de la entrada
};

// Nombre del tipo de token tal como debe aparecer en los reportes.
std::string nombreTipoToken(TipoToken tipo);

// ---------------------------------------------------------------------------
// Un token reconocido en el programa fuente.
// 'atributo' guardara la posicion en la tabla de simbolos cuando el token
// sea un identificador (Semana 2). Por ahora vale -1 = no aplica.
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
