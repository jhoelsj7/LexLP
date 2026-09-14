#include "Token.h"

std::string nombreTipoToken(TipoToken tipo) {
    switch (tipo) {
        case TipoToken::NUM_INT:      return "NUM_INT";
        case TipoToken::NUM_DEC:      return "NUM_DEC";
        case TipoToken::ERROR_LEXICO: return "ERROR_LEXICO";
        case TipoToken::FIN_ARCHIVO:  return "FIN_ARCHIVO";
    }
    return "DESCONOCIDO";
}

Token::Token(TipoToken tipo,
             const std::string& lexema,
             int linea,
             int columna,
             int atributo)
    : tipo(tipo), lexema(lexema), atributo(atributo), linea(linea), columna(columna) {}

std::string Token::aCadena() const {
    // Cuando el token tenga atributo (identificadores, Semana 2) se imprime
    // como <ID,0>. Mientras el atributo sea -1 se imprime solo el nombre.
    if (atributo >= 0) {
        return "<" + nombreTipoToken(tipo) + "," + std::to_string(atributo) + ">";
    }
    return "<" + nombreTipoToken(tipo) + ">";
}
