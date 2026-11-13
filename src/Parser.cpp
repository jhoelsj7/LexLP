#include "Parser.h"

// ===========================================================================
//  Utilidades sobre la secuencia de tokens
// ===========================================================================

Parser::Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {
    // Garantia: la lista siempre termina en FIN_ARCHIVO, asi actual() nunca
    // se sale del vector aunque el programa este incompleto.
    if (tokens_.empty() || tokens_.back().tipo != TipoToken::FIN_ARCHIVO) {
        int linea = tokens_.empty() ? 1 : tokens_.back().linea;
        tokens_.emplace_back(TipoToken::FIN_ARCHIVO, "", linea, 1);
    }
}

const Token& Parser::actual() const { return tokens_[pos_]; }

const Token& Parser::siguienteTok() const {
    return pos_ + 1 < tokens_.size() ? tokens_[pos_ + 1] : tokens_.back();
}

const Token& Parser::anterior() const {
    return pos_ > 0 ? tokens_[pos_ - 1] : tokens_[0];
}

bool Parser::es(TipoToken tipo) const { return actual().tipo == tipo; }

void Parser::avanzar() {
    if (!es(TipoToken::FIN_ARCHIVO)) pos_++;
}

// match(): si el token actual es el esperado lo consume; si no, error.
void Parser::esperar(TipoToken tipo, const std::string& descripcion) {
    if (es(tipo)) {
        avanzar();
        return;
    }
    error(descripcion);
}

// Nombre comprensible de un tipo de token para los mensajes de error.
std::string Parser::nombreLegible(TipoToken t) {
    switch (t) {
        case TipoToken::ID:           return "identificador";
        case TipoToken::NUM_INT:      return "numero entero";
        case TipoToken::NUM_DEC:      return "numero decimal";
        case TipoToken::TEXTO:        return "texto";
        case TipoToken::COMP:         return "operador de comparacion";
        case TipoToken::FIN_ARCHIVO:  return "fin de archivo";
        default:                      return "'" + nombreTipoToken(t) + "'";
    }
}

// "'edad' (identificador)"  /  "fin de archivo"
std::string Parser::describir(const Token& t) {
    switch (t.tipo) {
        case TipoToken::FIN_ARCHIVO:
            return "fin de archivo";
        case TipoToken::ID: case TipoToken::NUM_INT: case TipoToken::NUM_DEC:
        case TipoToken::TEXTO: case TipoToken::COMP:
            return "'" + t.lexema + "' (" + nombreLegible(t.tipo) + ")";
        default:
            return "'" + t.lexema + "'";   // simbolos y palabras reservadas
    }
}

// Construye el mensaje y aborta el analisis (modo panico).
void Parser::error(const std::string& esperado) {
    const Token& t = actual();
    std::string msg = "Error sintactico en linea " + std::to_string(t.linea) +
                      ", columna " + std::to_string(t.columna) +
                      ": se esperaba " + esperado +
                      " pero se encontro " + describir(t) + ".";

    // Si lo que falta es un cierre/terminador y el token encontrado ya esta
    // en una linea posterior, casi seguro el problema es la linea anterior
    // (el caso tipico: "int edad" sin ';').
    bool esCierre = esperado == "';'" || esperado == "')'" ||
                    esperado == "'}'" || esperado == "']'";
    if (esCierre && pos_ > 0 && t.linea > anterior().linea) {
        msg += "\n  Sugerencia: probablemente falta " + esperado +
               " al final de la linea " + std::to_string(anterior().linea) +
               ", despues de '" + anterior().lexema + "'.";
    }
    throw ErrorSintactico(msg);
}

bool Parser::analizar() {
    try {
        programa();
        if (!es(TipoToken::FIN_ARCHIVO)) {
            error("fin de archivo (el programa ya termino con main)");
        }
        return true;
    } catch (const ErrorSintactico& e) {
        mensajeError_ = e.what();
        return false;
    }
}

// ===========================================================================
//  Conjuntos PRIMERO usados para elegir producciones
// ===========================================================================

bool Parser::esTipoBase() const {
    return es(TipoToken::CHAR) || es(TipoToken::FLOAT) ||
           es(TipoToken::INT)  || es(TipoToken::BOOLEAN);
}

bool Parser::iniciaComando() const {
    return es(TipoToken::SCANF) || es(TipoToken::PRINTLN) || es(TipoToken::ID) ||
           es(TipoToken::FUNC)  || es(TipoToken::IF)      || es(TipoToken::WHILE) ||
           es(TipoToken::RETURN);
}

// PRIMERO(EXPRESION) = PRIMERO(FACTOR) = { + - id num_int num_dec texto ! ( }
bool Parser::iniciaExpresion() const {
    return es(TipoToken::SUMA)    || es(TipoToken::RESTA)   || es(TipoToken::ID) ||
           es(TipoToken::NUM_INT) || es(TipoToken::NUM_DEC) || es(TipoToken::TEXTO) ||
           es(TipoToken::NOT)     || es(TipoToken::PAR_IZQ);
}

// ===========================================================================
//  Estructura general del programa
// ===========================================================================

// PROGRAMA -> LISTA_FUNCIONES PRINCIPAL
void Parser::programa() {
    listaFunciones();
    principal();
}

// LISTA_FUNCIONES -> DECLARACION_FUNCION LISTA_FUNCIONES | e
//
// Decision de implementacion: los ejemplos del enunciado escriben
// "void main()", pero PRINCIPAL es "main ( ) BLOQUE". Como 'void' tambien
// inicia una DECLARACION_FUNCION, se mira un token mas (LL(2)): si despues
// de 'void' viene 'main', NO es una funcion sino el inicio de PRINCIPAL.
void Parser::listaFunciones() {
    while (esTipoBase() ||
           (es(TipoToken::VOID) && siguienteTok().tipo != TipoToken::MAIN)) {
        declaracionFuncion();
    }
    // e: si no empieza con un tipo, LISTA_FUNCIONES es vacia
}

// DECLARACION_FUNCION -> TIPO_RETORNO id ( PARAMETROS ) BLOQUE
void Parser::declaracionFuncion() {
    tipoRetorno();
    esperar(TipoToken::ID, "el nombre de la funcion (identificador)");
    if (!es(TipoToken::PAR_IZQ)) {
        // Fuera de main solo puede haber funciones: no existen variables globales.
        error("'(' (fuera de main solo se permiten declaraciones de funcion)");
    }
    avanzar();
    parametros();
    esperar(TipoToken::PAR_DER, "')'");
    bloque();
}

// TIPO_RETORNO -> TIPO | void
void Parser::tipoRetorno() {
    if (es(TipoToken::VOID)) avanzar();
    else                     tipo();
}

// TIPO -> TIPO_BASE DIMENSION
void Parser::tipo() {
    tipoBase();
    dimension();
}

// TIPO_BASE -> char | float | int | boolean
void Parser::tipoBase() {
    if (esTipoBase()) avanzar();
    else              error("un tipo de dato (int, float, char o boolean)");
}

// DIMENSION -> [ num_int ] DIMENSION | e
void Parser::dimension() {
    while (es(TipoToken::COR_IZQ)) {
        avanzar();
        esperar(TipoToken::NUM_INT, "el tamano del arreglo (numero entero)");
        esperar(TipoToken::COR_DER, "']'");
    }
}

// PARAMETROS -> TIPO id LISTA_PARAMETROS | e
void Parser::parametros() {
    if (esTipoBase()) {
        tipo();
        esperar(TipoToken::ID, "el nombre del parametro (identificador)");
        listaParametros();
    }
}

// LISTA_PARAMETROS -> , TIPO id LISTA_PARAMETROS | e
void Parser::listaParametros() {
    while (es(TipoToken::COMA)) {
        avanzar();
        tipo();
        esperar(TipoToken::ID, "el nombre del parametro (identificador)");
    }
}

// PRINCIPAL -> main ( ) BLOQUE
// (se acepta un 'void' opcional delante: ver listaFunciones)
void Parser::principal() {
    if (es(TipoToken::VOID)) avanzar();
    esperar(TipoToken::MAIN, "'main'");
    esperar(TipoToken::PAR_IZQ, "'('");
    esperar(TipoToken::PAR_DER, "')'");
    bloque();
}

// BLOQUE -> { LISTA_VARIABLES COMANDOS }
void Parser::bloque() {
    esperar(TipoToken::LLAVE_IZQ, "'{'");
    listaVariables();
    comandos();
    if (!es(TipoToken::LLAVE_DER)) {
        if (esTipoBase())
            error("'}' o un comando (las declaraciones deben ir antes de los comandos)");
        error("'}'");
    }
    avanzar();
}

// ===========================================================================
//  Declaraciones de variables
// ===========================================================================

// LISTA_VARIABLES -> TIPO id LISTA_ID ; LISTA_VARIABLES | e
void Parser::listaVariables() {
    while (esTipoBase()) {
        tipo();
        esperar(TipoToken::ID, "el nombre de la variable (identificador)");
        listaId();
        esperar(TipoToken::PUNTOYCOMA, "';'");
    }
}

// LISTA_ID -> , id LISTA_ID | e
void Parser::listaId() {
    while (es(TipoToken::COMA)) {
        avanzar();
        esperar(TipoToken::ID, "un identificador despues de ','");
    }
}

// ===========================================================================
//  Comandos
// ===========================================================================

// COMANDOS -> COMANDO COMANDOS | e
void Parser::comandos() {
    while (iniciaComando()) {
        comando();
    }
}

// COMANDO -> LECTURA | ESCRITURA | ASIGNACION | FUNCION | SELECCION | MIENTRAS | RETORNO
// Cada alternativa empieza con un terminal distinto, asi que basta mirar
// el token actual para elegir (gramatica LL(1) en este punto).
void Parser::comando() {
    switch (actual().tipo) {
        case TipoToken::SCANF:   lectura();    break;
        case TipoToken::PRINTLN: escritura();  break;
        case TipoToken::ID:      asignacion(); break;
        case TipoToken::IF:      seleccion();  break;
        case TipoToken::WHILE:   mientras();   break;
        case TipoToken::RETURN:  retorno();    break;
        case TipoToken::FUNC:
            funcion();
            // La gramatica no pone ';' despues de FUNCION como comando.
            // Se acepta uno opcional para no rechazar "func f(x);".
            if (es(TipoToken::PUNTOYCOMA)) avanzar();
            break;
        default:
            error("un comando");
    }
}

// LECTURA -> scanf ( TERMINO_LECTURA NUEVO_TERMINO_LECTURA ) ;
void Parser::lectura() {
    esperar(TipoToken::SCANF, "'scanf'");
    esperar(TipoToken::PAR_IZQ, "'('");
    terminoLectura();
    nuevoTerminoLectura();
    esperar(TipoToken::PAR_DER, "')'");
    esperar(TipoToken::PUNTOYCOMA, "';'");
}

// TERMINO_LECTURA -> id DIMENSION2
void Parser::terminoLectura() {
    esperar(TipoToken::ID, "una variable (identificador) para leer");
    dimension2();
}

// NUEVO_TERMINO_LECTURA -> , TERMINO_LECTURA NUEVO_TERMINO_LECTURA | e
void Parser::nuevoTerminoLectura() {
    while (es(TipoToken::COMA)) {
        avanzar();
        terminoLectura();
    }
}

// DIMENSION2 -> [ EXPRESION_ADITIVA ] DIMENSION2 | e
void Parser::dimension2() {
    while (es(TipoToken::COR_IZQ)) {
        avanzar();
        expresionAditiva();
        esperar(TipoToken::COR_DER, "']'");
    }
}

// ESCRITURA -> println ( TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA ) ;
void Parser::escritura() {
    esperar(TipoToken::PRINTLN, "'println'");
    esperar(TipoToken::PAR_IZQ, "'('");
    terminoEscritura();
    nuevoTerminoEscritura();
    esperar(TipoToken::PAR_DER, "')'");
    esperar(TipoToken::PUNTOYCOMA, "';'");
}

// TERMINO_ESCRITURA -> id DIMENSION2 | CONSTANTE | texto
void Parser::terminoEscritura() {
    if (es(TipoToken::ID)) {
        avanzar();
        dimension2();
    } else if (es(TipoToken::NUM_INT) || es(TipoToken::NUM_DEC)) {
        constante();
    } else if (es(TipoToken::TEXTO)) {
        avanzar();
    } else {
        error("un identificador, una constante o un texto para imprimir");
    }
}

// NUEVO_TERMINO_ESCRITURA -> , TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA | e
void Parser::nuevoTerminoEscritura() {
    while (es(TipoToken::COMA)) {
        avanzar();
        terminoEscritura();
    }
}

// SELECCION -> if ( EXPRESION ) BLOQUE SINO
void Parser::seleccion() {
    esperar(TipoToken::IF, "'if'");
    esperar(TipoToken::PAR_IZQ, "'('");
    expresion();
    esperar(TipoToken::PAR_DER, "')'");
    bloque();
    sino();
}

// SINO -> else BLOQUE | e
void Parser::sino() {
    if (es(TipoToken::ELSE)) {
        avanzar();
        bloque();
    }
}

// MIENTRAS -> while ( EXPRESION ) BLOQUE
void Parser::mientras() {
    esperar(TipoToken::WHILE, "'while'");
    esperar(TipoToken::PAR_IZQ, "'('");
    expresion();
    esperar(TipoToken::PAR_DER, "')'");
    bloque();
}

// ASIGNACION -> id = COMPLEMENTO ;
void Parser::asignacion() {
    esperar(TipoToken::ID, "un identificador");
    esperar(TipoToken::ASIGNACION, "'=' (asignacion)");
    complemento();
    esperar(TipoToken::PUNTOYCOMA, "';'");
}

// COMPLEMENTO -> EXPRESION | FUNCION
void Parser::complemento() {
    if (es(TipoToken::FUNC)) funcion();
    else                     expresion();
}

// FUNCION -> func id ( ARGUMENTOS )
void Parser::funcion() {
    esperar(TipoToken::FUNC, "'func'");
    esperar(TipoToken::ID, "el nombre de la funcion (identificador)");
    esperar(TipoToken::PAR_IZQ, "'('");
    argumentos();
    esperar(TipoToken::PAR_DER, "')'");
}

// ARGUMENTOS -> EXPRESION NUEVO_ARGUMENTO | e
void Parser::argumentos() {
    if (iniciaExpresion()) {
        expresion();
        nuevoArgumento();
    }
}

// NUEVO_ARGUMENTO -> , EXPRESION NUEVO_ARGUMENTO | e
void Parser::nuevoArgumento() {
    while (es(TipoToken::COMA)) {
        avanzar();
        expresion();
    }
}

// RETORNO -> return EXPRESION ;
void Parser::retorno() {
    esperar(TipoToken::RETURN, "'return'");
    expresion();          // la gramatica no admite "return;" sin expresion
    esperar(TipoToken::PUNTOYCOMA, "';'");
}

// ===========================================================================
//  Expresiones (de menor a mayor precedencia: || , && , comp , + - , * / %)
//  Las producciones X2 -> op Y X2 | e son recursion por la derecha; se
//  implementan con un while, que es equivalente y no usa la pila.
// ===========================================================================

// EXPRESION -> EXPRESION_O
void Parser::expresion() { expresionO(); }

// EXPRESION_O -> EXPRESION_Y EXPRESION_O2
void Parser::expresionO() {
    expresionY();
    expresionO2();
}

// EXPRESION_O2 -> || EXPRESION_Y EXPRESION_O2 | e
void Parser::expresionO2() {
    while (es(TipoToken::OR)) {
        avanzar();
        expresionY();
    }
}

// EXPRESION_Y -> EXPRESION_RELACIONAL EXPRESION_Y2
void Parser::expresionY() {
    expresionRelacional();
    expresionY2();
}

// EXPRESION_Y2 -> && EXPRESION_RELACIONAL EXPRESION_Y2 | e
void Parser::expresionY2() {
    while (es(TipoToken::AND)) {
        avanzar();
        expresionRelacional();
    }
}

// EXPRESION_RELACIONAL -> EXPRESION_ADITIVA EXPRESION_RELACIONAL2
void Parser::expresionRelacional() {
    expresionAditiva();
    expresionRelacional2();
}

// EXPRESION_RELACIONAL2 -> comp EXPRESION_ADITIVA | e
// (sin recursion: "a < b < c" NO es valido en LP)
void Parser::expresionRelacional2() {
    if (es(TipoToken::COMP)) {
        avanzar();
        expresionAditiva();
    }
}

// EXPRESION_ADITIVA -> EXPRESION_MULTIPLICATIVA EXPRESION_ADITIVA2
void Parser::expresionAditiva() {
    expresionMultiplicativa();
    expresionAditiva2();
}

// EXPRESION_ADITIVA2 -> OPERADOR_ADITIVO EXPRESION_MULTIPLICATIVA EXPRESION_ADITIVA2 | e
// OPERADOR_ADITIVO   -> + | -
void Parser::expresionAditiva2() {
    while (es(TipoToken::SUMA) || es(TipoToken::RESTA)) {
        avanzar();
        expresionMultiplicativa();
    }
}

// EXPRESION_MULTIPLICATIVA -> FACTOR EXPRESION_MULTIPLICATIVA2
void Parser::expresionMultiplicativa() {
    factor();
    expresionMultiplicativa2();
}

// EXPRESION_MULTIPLICATIVA2 -> OPERADOR_MULTIPLICATIVO FACTOR EXPRESION_MULTIPLICATIVA2 | e
// OPERADOR_MULTIPLICATIVO   -> * | / | %
void Parser::expresionMultiplicativa2() {
    while (es(TipoToken::MULT) || es(TipoToken::DIV) || es(TipoToken::MOD)) {
        avanzar();
        factor();
    }
}

// FACTOR -> SIGNO TERMINO | texto | ! FACTOR | ( EXPRESION )
void Parser::factor() {
    if (es(TipoToken::TEXTO)) {
        avanzar();
    } else if (es(TipoToken::NOT)) {
        avanzar();
        factor();
    } else if (es(TipoToken::PAR_IZQ)) {
        avanzar();
        expresion();
        esperar(TipoToken::PAR_DER, "')'");
    } else if (es(TipoToken::SUMA) || es(TipoToken::RESTA) ||
               es(TipoToken::ID)   || es(TipoToken::NUM_INT) || es(TipoToken::NUM_DEC)) {
        signo();
        termino();
    } else {
        error("una expresion (identificador, numero, texto, '!' o '(')");
    }
}

// TERMINO -> id DIMENSION2 | CONSTANTE
void Parser::termino() {
    if (es(TipoToken::ID)) {
        avanzar();
        dimension2();
    } else if (es(TipoToken::NUM_INT) || es(TipoToken::NUM_DEC)) {
        constante();
    } else {
        error("un identificador o una constante numerica despues del signo");
    }
}

// SIGNO -> + | - | e
void Parser::signo() {
    if (es(TipoToken::SUMA) || es(TipoToken::RESTA)) avanzar();
}

// CONSTANTE -> num_int | num_dec
void Parser::constante() {
    if (es(TipoToken::NUM_INT) || es(TipoToken::NUM_DEC)) avanzar();
    else error("una constante numerica");
}
