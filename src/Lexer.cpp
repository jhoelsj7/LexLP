#include "Lexer.h"

Lexer::Lexer(const std::string& fuente)
    : fuente_(fuente), pos_(0), linea_(1), columna_(1) {}

// D = [0-9]
bool Lexer::esDigito(char c) {
    return c >= '0' && c <= '9';
}

bool Lexer::finEntrada() const {
    return pos_ >= fuente_.size();
}

char Lexer::actual() const {
    if (finEntrada()) return '\0';
    return fuente_[pos_];
}

// Mira un caracter mas adelante sin consumirlo.
// Es lo que permite distinguir  "15.5"  (NUM_DEC)  de  "15."  (NUM_INT y luego error).
char Lexer::siguiente() const {
    if (pos_ + 1 >= fuente_.size()) return '\0';
    return fuente_[pos_ + 1];
}

// Consume el caracter actual y mantiene actualizados linea y columna,
// que es lo que despues permite localizar los errores lexicos.
char Lexer::avanzar() {
    char c = fuente_[pos_];
    pos_++;
    if (c == '\n') {
        linea_++;
        columna_ = 1;
    } else {
        columna_++;
    }
    return c;
}

void Lexer::omitirEspacios() {
    while (!finEntrada()) {
        char c = actual();
        // '\r' aparece en los archivos guardados en Windows (fin de linea CRLF)
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            avanzar();
        } else {
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Reconoce numeros. Traduce directamente estas dos expresiones regulares:
//
//      D+        -> NUM_INT
//      D+ \. D+  -> NUM_DEC
//
// El punto solo se consume si DESPUES viene al menos un digito. Por eso
// "15.5" da NUM_DEC, mientras que "15." da NUM_INT(15) y el punto queda
// suelto para reportarse como error lexico.
// ---------------------------------------------------------------------------
Token Lexer::leerNumero() {
    int lineaInicio   = linea_;
    int columnaInicio = columna_;
    std::string lexema;

    // parte entera: D+
    while (!finEntrada() && esDigito(actual())) {
        lexema += avanzar();
    }

    // parte decimal opcional: \. D+
    if (actual() == '.' && esDigito(siguiente())) {
        lexema += avanzar();                 // consume el '.'
        while (!finEntrada() && esDigito(actual())) {
            lexema += avanzar();
        }
        return Token(TipoToken::NUM_DEC, lexema, lineaInicio, columnaInicio);
    }

    return Token(TipoToken::NUM_INT, lexema, lineaInicio, columnaInicio);
}

Token Lexer::siguienteToken() {
    omitirEspacios();

    if (finEntrada()) {
        return Token(TipoToken::FIN_ARCHIVO, "", linea_, columna_);
    }

    char c = actual();

    if (esDigito(c)) {
        return leerNumero();
    }

    // AVANCE SEMANA 1: todavia no se implementan identificadores, palabras
    // reservadas, operadores ni simbolos especiales. Cualquier otro caracter
    // se reporta como error lexico con su linea y columna exactas.
    int lineaInicio   = linea_;
    int columnaInicio = columna_;
    std::string lexema(1, avanzar());

    Token error(TipoToken::ERROR_LEXICO, lexema, lineaInicio, columnaInicio);
    errores_.push_back(error);
    return error;
}

std::vector<Token> Lexer::analizarTodo() {
    std::vector<Token> tokens;
    while (true) {
        Token t = siguienteToken();
        if (t.tipo == TipoToken::FIN_ARCHIVO) break;
        // los errores no entran a la lista de tokens: van al reporte de errores
        if (t.tipo == TipoToken::ERROR_LEXICO) continue;
        tokens.push_back(t);
    }
    return tokens;
}
