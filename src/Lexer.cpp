#include "Lexer.h"
#include <unordered_map>

Lexer::Lexer(const std::string& fuente)
    : fuente_(fuente), pos_(0), linea_(1), columna_(1) {}

// D = [0-9]
bool Lexer::esDigito(char c) {
    return c >= '0' && c <= '9';
}

// L = [a-zA-Z_]
bool Lexer::esLetra(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::finEntrada() const {
    return pos_ >= fuente_.size();
}

char Lexer::actual() const {
    if (finEntrada()) return '\0';
    return fuente_[pos_];
}

// Mira un caracter mas adelante sin consumirlo.
// Es lo que permite distinguir  "15.5"  (NUM_DEC)  de  "15."  (NUM_INT y luego error),
// y tambien detectar "//" antes de decidir si "/" es un DIV o el inicio de un comentario.
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

// Espacios/tabs/saltos de linea y comentarios (COMENT = //.*\n) se saltan
// por igual: ninguno de los dos genera token. Se repite en bucle porque
// despues de un comentario puede venir mas espacio o otro comentario.
void Lexer::omitirEspacios() {
    while (!finEntrada()) {
        char c = actual();
        // '\r' aparece en los archivos guardados en Windows (fin de linea CRLF)
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            avanzar();
        } else if (c == '/' && siguiente() == '/') {
            avanzar(); // primer '/'
            avanzar(); // segundo '/'
            while (!finEntrada() && actual() != '\n') {
                avanzar();
            }
            // el '\n' final del comentario lo consume la proxima vuelta del bucle
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

// Palabras reservadas de LP (exactas, en minusculas). Cualquier otro
// lexema que cumpla L(L|D)* es un ID comun.
bool Lexer::esPalabraReservada(const std::string& lexema, TipoToken& tipo) {
    static const std::unordered_map<std::string, TipoToken> reservadas = {
        {"int",     TipoToken::INT},
        {"float",   TipoToken::FLOAT},
        {"char",    TipoToken::CHAR},
        {"boolean", TipoToken::BOOLEAN},
        {"void",    TipoToken::VOID},
        {"if",      TipoToken::IF},
        {"else",    TipoToken::ELSE},
        {"for",     TipoToken::FOR},
        {"while",   TipoToken::WHILE},
        {"scanf",   TipoToken::SCANF},
        {"println", TipoToken::PRINTLN},
        {"main",    TipoToken::MAIN},
        {"return",  TipoToken::RETURN},
    };
    auto it = reservadas.find(lexema);
    if (it == reservadas.end()) return false;
    tipo = it->second;
    return true;
}

// Tabla de simbolos: un ID nuevo se agrega al final; uno repetido reutiliza
// su indice. Asi 'atributo' siempre apunta a la posicion correcta.
int Lexer::indiceSimbolo(const std::string& lexema) {
    for (size_t i = 0; i < tablaSimbolos_.size(); ++i) {
        if (tablaSimbolos_[i] == lexema) {
            return static_cast<int>(i);
        }
    }
    tablaSimbolos_.push_back(lexema);
    return static_cast<int>(tablaSimbolos_.size()) - 1;
}

// ER:  L (L|D)*
// Si el lexema resulta ser una palabra reservada se devuelve el token
// correspondiente (INT, IF, MAIN, ...); si no, es un ID y se registra
// (o se recupera) su posicion en la tabla de simbolos.
Token Lexer::leerIdentificador() {
    int lineaInicio   = linea_;
    int columnaInicio = columna_;
    std::string lexema;

    lexema += avanzar(); // primer caracter: L
    while (!finEntrada() && (esLetra(actual()) || esDigito(actual()))) {
        lexema += avanzar();
    }

    TipoToken tipoReservada;
    if (esPalabraReservada(lexema, tipoReservada)) {
        return Token(tipoReservada, lexema, lineaInicio, columnaInicio);
    }

    int indice = indiceSimbolo(lexema);
    return Token(TipoToken::ID, lexema, lineaInicio, columnaInicio, indice);
}

// ER:  " .* "
// Se lee todo hasta encontrar la comilla de cierre. Si se llega a un salto
// de linea o al final del archivo antes de cerrarla, la cadena queda mal
// formada y se reporta como error lexico (con el texto leido hasta ahi).
Token Lexer::leerTexto() {
    int lineaInicio   = linea_;
    int columnaInicio = columna_;
    std::string lexema;

    lexema += avanzar(); // comilla de apertura '"'
    while (!finEntrada() && actual() != '"' && actual() != '\n') {
        lexema += avanzar();
    }

    if (!finEntrada() && actual() == '"') {
        lexema += avanzar(); // comilla de cierre
        return Token(TipoToken::TEXTO, lexema, lineaInicio, columnaInicio);
    }

    Token error(TipoToken::ERROR_LEXICO, lexema, lineaInicio, columnaInicio);
    errores_.push_back(error);
    return error;
}

// Operadores y simbolos especiales. Varios operadores comparten su primer
// caracter con otro token mas corto (=, ==) (!, !=) (>, >=) (<, <=), asi que
// se usa un caracter de lookahead para decidir cual de los dos es.
Token Lexer::leerSimbolo() {
    int lineaInicio   = linea_;
    int columnaInicio = columna_;
    char c = avanzar();

    switch (c) {
        case '+': return Token(TipoToken::SUMA,      "+", lineaInicio, columnaInicio);
        case '-': return Token(TipoToken::RESTA,      "-", lineaInicio, columnaInicio);
        case '*': return Token(TipoToken::MULT,       "*", lineaInicio, columnaInicio);
        case '/': return Token(TipoToken::DIV,        "/", lineaInicio, columnaInicio);
        case '%': return Token(TipoToken::MOD,        "%", lineaInicio, columnaInicio);

        case '(': return Token(TipoToken::PAR_IZQ,    "(", lineaInicio, columnaInicio);
        case ')': return Token(TipoToken::PAR_DER,    ")", lineaInicio, columnaInicio);
        case '[': return Token(TipoToken::COR_IZQ,    "[", lineaInicio, columnaInicio);
        case ']': return Token(TipoToken::COR_DER,    "]", lineaInicio, columnaInicio);
        case '{': return Token(TipoToken::LLAVE_IZQ,  "{", lineaInicio, columnaInicio);
        case '}': return Token(TipoToken::LLAVE_DER,  "}", lineaInicio, columnaInicio);
        case ',': return Token(TipoToken::COMA,       ",", lineaInicio, columnaInicio);
        case ';': return Token(TipoToken::PUNTOCOMA,  ";", lineaInicio, columnaInicio);

        case '=':
            if (actual() == '=') { avanzar(); return Token(TipoToken::COMP, "==", lineaInicio, columnaInicio); }
            return Token(TipoToken::ASIGNACION, "=", lineaInicio, columnaInicio);

        case '>':
            if (actual() == '=') { avanzar(); return Token(TipoToken::COMP, ">=", lineaInicio, columnaInicio); }
            return Token(TipoToken::COMP, ">", lineaInicio, columnaInicio);

        case '<':
            if (actual() == '=') { avanzar(); return Token(TipoToken::COMP, "<=", lineaInicio, columnaInicio); }
            return Token(TipoToken::COMP, "<", lineaInicio, columnaInicio);

        case '!':
            if (actual() == '=') { avanzar(); return Token(TipoToken::COMP, "!=", lineaInicio, columnaInicio); }
            return Token(TipoToken::NOT, "!", lineaInicio, columnaInicio);

        case '&':
            if (actual() == '&') { avanzar(); return Token(TipoToken::AND, "&&", lineaInicio, columnaInicio); }
            break; // '&' suelto no pertenece al alfabeto de LP

        case '|':
            if (actual() == '|') { avanzar(); return Token(TipoToken::OR, "||", lineaInicio, columnaInicio); }
            break; // '|' suelto no pertenece al alfabeto de LP

        default:
            break;
    }

    // Cualquier caracter que no encajo en ninguna regla anterior.
    std::string lexema(1, c);
    Token error(TipoToken::ERROR_LEXICO, lexema, lineaInicio, columnaInicio);
    errores_.push_back(error);
    return error;
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

    if (esLetra(c)) {
        return leerIdentificador();
    }

    if (c == '"') {
        return leerTexto();
    }

    return leerSimbolo();
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
