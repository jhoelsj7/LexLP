#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "Token.h"

// ---------------------------------------------------------------------------
// Analizador lexico de LP.
//
// Funciona como un automata implementado a mano: recorre el texto fuente
// caracter por caracter y, segun el caracter actual, decide que token esta
// empezando. No se usa ninguna libreria de expresiones regulares: las ER de
// la especificacion estan traducidas directamente a codigo.
// ---------------------------------------------------------------------------
class Lexer {
public:
    explicit Lexer(const std::string& fuente);

    // Reconoce y devuelve el siguiente token del programa fuente.
    // Al llegar al final devuelve un token de tipo FIN_ARCHIVO.
    Token siguienteToken();

    // Recorre todo el fuente y devuelve la lista de tokens validos,
    // en el mismo orden en que aparecen. Los errores quedan aparte
    // y se consultan con errores().
    std::vector<Token> analizarTodo();

    const std::vector<Token>& errores() const { return errores_; }

private:
    std::string fuente_;   // todo el contenido del archivo .lp
    size_t      pos_;      // posicion del caracter que se esta mirando
    int         linea_;    // linea actual  (empieza en 1)
    int         columna_;  // columna actual(empieza en 1)

    std::vector<Token> errores_;

    // --- utilidades de recorrido ---
    bool finEntrada() const;
    char actual() const;        // caracter actual sin consumirlo
    char siguiente() const;     // caracter de adelante (lookahead), para el punto decimal
    char avanzar();             // consume el caracter actual y actualiza linea/columna

    void omitirEspacios();      // espacios, tabulaciones y saltos de linea

    // --- reconocedores (uno por cada ER de la especificacion) ---
    Token leerNumero();         // implementa  D+  y  D+\.D+

    static bool esDigito(char c);   // D = [0-9]
};

#endif // LEXER_H
