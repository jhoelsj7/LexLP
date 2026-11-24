#include "Parser.h"

// ===========================================================================
//  Utilidades sobre la secuencia de tokens
// ===========================================================================

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), pos_(0), ultimaPosError_(static_cast<size_t>(-1)) {
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

// Igual que esperar(), pero devuelve el token consumido (para el AST).
Token Parser::consumir(TipoToken tipo, const std::string& descripcion) {
    Token t = actual();
    esperar(tipo, descripcion);
    return t;
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

// Construye el mensaje y corta la produccion actual (modo panico).
// Quien lo atrapa (comandos, listaVariables, ...) lo registra y se recupera.
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
    throw ErrorSintactico(msg, pos_);
}

bool Parser::analizar() {
    errores_.clear();
    mensajeError_.clear();
    ultimaPosError_ = static_cast<size_t>(-1);
    try {
        ast_ = programa();
        if (!es(TipoToken::FIN_ARCHIVO)) {
            error("fin de archivo (el programa ya termino con main)");
        }
    } catch (const ErrorSintactico& e) {
        registrar(e);            // error del que no hay donde recuperarse
    } catch (const AnalisisDetenido&) {
        // se llego al fin de archivo durante la recuperacion: ya registrado
    }
    if (!errores_.empty()) mensajeError_ = errores_.front();
    return errores_.empty();
}

// ===========================================================================
//  Recuperacion de errores (Entregable 2)
//
//  Estrategia: modo panico con conjuntos de sincronizacion.
//   1. El metodo que detecta el error lanza ErrorSintactico (como antes).
//   2. Lo atrapan los puntos de recuperacion: cada COMANDO, cada
//      declaracion de LISTA_VARIABLES, el cierre de BLOQUE y las cabeceras
//      de funcion / main.
//   3. Se registra el mensaje y se descartan tokens hasta un token de
//      sincronizacion; luego el analisis sigue normalmente.
//  Un mismo token no genera dos errores (evita errores "en cascada").
// ===========================================================================

void Parser::registrar(const ErrorSintactico& e) {
    if (e.pos == ultimaPosError_) return;   // ya se reporto un error aqui
    errores_.push_back(e.what());
    ultimaPosError_ = e.pos;
}

// Descarta tokens hasta un punto seguro para reanudar:
//  - ';'   se consume (termina la instruccion rota)
//  - '}'   no se consume (lo cierra el BLOQUE que corresponde)
//  - '{'   se salta el bloque completo (pertenece a la instruccion rota)
//  - scanf, println, if, while, return: inician el siguiente comando
//  - un comando o una declaracion que empieza en una linea NUEVA
//    (caso tipico: falta ';' y el siguiente comando esta debajo)
void Parser::sincronizar() {
    const int lineaError = anterior().linea;
    while (!es(TipoToken::FIN_ARCHIVO)) {
        if (es(TipoToken::PUNTOYCOMA)) { avanzar(); return; }
        if (es(TipoToken::LLAVE_DER))  return;
        if (es(TipoToken::LLAVE_IZQ))  { saltarBloque(); return; }
        if (es(TipoToken::SCANF) || es(TipoToken::PRINTLN) || es(TipoToken::IF) ||
            es(TipoToken::WHILE) || es(TipoToken::RETURN))
            return;
        if (actual().linea > lineaError && (iniciaComando() || esTipoBase()))
            return;
        avanzar();
    }
}

// Consume un bloque { ... } completo, respetando las llaves anidadas.
void Parser::saltarBloque() {
    int profundidad = 0;
    while (!es(TipoToken::FIN_ARCHIVO)) {
        if (es(TipoToken::LLAVE_IZQ)) profundidad++;
        if (es(TipoToken::LLAVE_DER)) profundidad--;
        avanzar();
        if (profundidad == 0) return;
    }
}

// Tras un error en la cabecera de una funcion o de main: se avanza hasta
// el '{' de su cuerpo. Si antes aparece (en otra linea) el inicio de otra
// funcion o de main, se para ahi y se devuelve false (no hay cuerpo).
void Parser::sincronizarHastaBloque() {
    const int lineaError = anterior().linea;
    while (!es(TipoToken::LLAVE_IZQ)) {
        if (es(TipoToken::FIN_ARCHIVO)) throw AnalisisDetenido{};
        if (actual().linea > lineaError &&
            (esTipoBase() || es(TipoToken::VOID) || es(TipoToken::MAIN)))
            return;
        avanzar();
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
NodoPtr Parser::programa() {
    NodoPtr p = nuevoNodo("PROGRAMA", "", actual().linea);
    listaFunciones(p.get());
    p->agregar(principal());
    return p;
}

// LISTA_FUNCIONES -> DECLARACION_FUNCION LISTA_FUNCIONES | e
//
// Decision de implementacion: los ejemplos del enunciado escriben
// "void main()", pero PRINCIPAL es "main ( ) BLOQUE". Como 'void' tambien
// inicia una DECLARACION_FUNCION, se mira un token mas (LL(2)): si despues
// de 'void' viene 'main', NO es una funcion sino el inicio de PRINCIPAL.
void Parser::listaFunciones(NodoAST* p) {
    while (esTipoBase() ||
           (es(TipoToken::VOID) && siguienteTok().tipo != TipoToken::MAIN)) {
        p->agregar(declaracionFuncion());
    }
    // e: si no empieza con un tipo, LISTA_FUNCIONES es vacia
}

// DECLARACION_FUNCION -> TIPO_RETORNO id ( PARAMETROS ) BLOQUE
// AST: FUNCION <tipo> <nombre>  ->  PARAMETROS?, BLOQUE
NodoPtr Parser::declaracionFuncion() {
    const int linea = actual().linea;
    std::string tipoRet, nombre;
    NodoPtr params;
    try {
        tipoRet = tipoRetorno();
        nombre  = consumir(TipoToken::ID, "el nombre de la funcion (identificador)").lexema;
        if (!es(TipoToken::PAR_IZQ)) {
            // Fuera de main solo puede haber funciones: no existen variables globales.
            error("'(' (fuera de main solo se permiten declaraciones de funcion)");
        }
        avanzar();
        params = parametros();
        esperar(TipoToken::PAR_DER, "')'");
    } catch (const ErrorSintactico& e) {
        registrar(e);
        sincronizarHastaBloque();
        if (!es(TipoToken::LLAVE_IZQ)) return nullptr;   // no habia cuerpo
    }
    NodoPtr f = nuevoNodo("FUNCION", tipoRet + " " + nombre, linea);
    f->agregar(std::move(params));
    f->agregar(bloque());
    return f;
}

// TIPO_RETORNO -> TIPO | void
std::string Parser::tipoRetorno() {
    if (es(TipoToken::VOID)) { avanzar(); return "void"; }
    return tipo();
}

// TIPO -> TIPO_BASE DIMENSION      (ej. "int[3][4]")
std::string Parser::tipo() {
    std::string t = tipoBase();
    return t + dimension();
}

// TIPO_BASE -> char | float | int | boolean
std::string Parser::tipoBase() {
    if (!esTipoBase()) error("un tipo de dato (int, float, char o boolean)");
    std::string t = actual().lexema;
    avanzar();
    return t;
}

// DIMENSION -> [ num_int ] DIMENSION | e
std::string Parser::dimension() {
    std::string d;
    while (es(TipoToken::COR_IZQ)) {
        avanzar();
        d += "[" + consumir(TipoToken::NUM_INT, "el tamano del arreglo (numero entero)").lexema + "]";
        esperar(TipoToken::COR_DER, "']'");
    }
    return d;
}

// PARAMETROS -> TIPO id LISTA_PARAMETROS | e
// AST: PARAMETROS -> PARAMETRO <tipo> <nombre> ...   (nullptr si no hay)
NodoPtr Parser::parametros() {
    if (!esTipoBase()) return nullptr;
    NodoPtr ps = nuevoNodo("PARAMETROS", "", actual().linea);
    const int linea = actual().linea;
    std::string t = tipo();
    std::string n = consumir(TipoToken::ID, "el nombre del parametro (identificador)").lexema;
    ps->agregar(nuevoNodo("PARAMETRO", t + " " + n, linea));
    listaParametros(ps.get());
    return ps;
}

// LISTA_PARAMETROS -> , TIPO id LISTA_PARAMETROS | e
void Parser::listaParametros(NodoAST* ps) {
    while (es(TipoToken::COMA)) {
        avanzar();
        const int linea = actual().linea;
        std::string t = tipo();
        std::string n = consumir(TipoToken::ID, "el nombre del parametro (identificador)").lexema;
        ps->agregar(nuevoNodo("PARAMETRO", t + " " + n, linea));
    }
}

// PRINCIPAL -> main ( ) BLOQUE
// (se acepta un 'void' opcional delante: ver listaFunciones)
NodoPtr Parser::principal() {
    const int linea = actual().linea;
    try {
        if (es(TipoToken::VOID)) avanzar();
        esperar(TipoToken::MAIN, "'main'");
        esperar(TipoToken::PAR_IZQ, "'('");
        esperar(TipoToken::PAR_DER, "')'");
    } catch (const ErrorSintactico& e) {
        registrar(e);
        while (!es(TipoToken::LLAVE_IZQ)) {          // hasta el cuerpo de main
            if (es(TipoToken::FIN_ARCHIVO)) throw AnalisisDetenido{};
            avanzar();
        }
    }
    NodoPtr m = nuevoNodo("PRINCIPAL", "main", linea);
    m->agregar(bloque());
    return m;
}

// BLOQUE -> { LISTA_VARIABLES COMANDOS }
NodoPtr Parser::bloque() {
    const int linea = consumir(TipoToken::LLAVE_IZQ, "'{'").linea;
    NodoPtr b = nuevoNodo("BLOQUE", "", linea);
    listaVariables(b.get());
    comandos(b.get());

    // Recuperacion: mientras no aparezca '}', se reporta lo que sobra y se
    // sigue analizando el resto del bloque.
    while (!es(TipoToken::LLAVE_DER) && !es(TipoToken::FIN_ARCHIVO)) {
        bool declaracionTardia = esTipoBase();
        try {
            if (declaracionTardia)
                error("'}' o un comando (las declaraciones deben ir antes de los comandos)");
            error("'}'");
        } catch (const ErrorSintactico& e) {
            registrar(e);
        }
        if (declaracionTardia) listaVariables(b.get());   // se analiza igual
        else                   sincronizar();
        comandos(b.get());
    }
    esperar(TipoToken::LLAVE_DER, "'}'");
    return b;
}

// ===========================================================================
//  Declaraciones de variables
// ===========================================================================

// LISTA_VARIABLES -> TIPO id LISTA_ID ; LISTA_VARIABLES | e
// AST: DECLARACION <tipo> -> VARIABLE <nombre> ...
// Punto de recuperacion: un error en una declaracion no impide analizar
// las siguientes.
void Parser::listaVariables(NodoAST* b) {
    while (esTipoBase()) {
        try {
            const int linea = actual().linea;
            NodoPtr d = nuevoNodo("DECLARACION", tipo(), linea);
            Token id = consumir(TipoToken::ID, "el nombre de la variable (identificador)");
            d->agregar(nuevoNodo("VARIABLE", id.lexema, id.linea));
            listaId(d.get());
            esperar(TipoToken::PUNTOYCOMA, "';'");
            b->agregar(std::move(d));
        } catch (const ErrorSintactico& e) {
            registrar(e);
            sincronizar();
        }
    }
}

// LISTA_ID -> , id LISTA_ID | e
void Parser::listaId(NodoAST* d) {
    while (es(TipoToken::COMA)) {
        avanzar();
        Token id = consumir(TipoToken::ID, "un identificador despues de ','");
        d->agregar(nuevoNodo("VARIABLE", id.lexema, id.linea));
    }
}

// ===========================================================================
//  Comandos
// ===========================================================================

// COMANDOS -> COMANDO COMANDOS | e
// Punto de recuperacion principal: si un comando tiene un error, se
// registra, se sincroniza y se sigue con el siguiente comando.
void Parser::comandos(NodoAST* b) {
    while (iniciaComando()) {
        try {
            b->agregar(comando());
        } catch (const ErrorSintactico& e) {
            registrar(e);
            sincronizar();
        }
    }
}

// COMANDO -> LECTURA | ESCRITURA | ASIGNACION | FUNCION | SELECCION | MIENTRAS | RETORNO
// Cada alternativa empieza con un terminal distinto, asi que basta mirar
// el token actual para elegir (gramatica LL(1) en este punto).
NodoPtr Parser::comando() {
    switch (actual().tipo) {
        case TipoToken::SCANF:   return lectura();
        case TipoToken::PRINTLN: return escritura();
        case TipoToken::ID:      return asignacion();
        case TipoToken::IF:      return seleccion();
        case TipoToken::WHILE:   return mientras();
        case TipoToken::RETURN:  return retorno();
        case TipoToken::FUNC: {
            NodoPtr llamada = funcion();
            // La gramatica no pone ';' despues de FUNCION como comando.
            // Se acepta uno opcional para no rechazar "func f(x);".
            if (es(TipoToken::PUNTOYCOMA)) avanzar();
            return llamada;
        }
        default:
            error("un comando");
    }
}

// LECTURA -> scanf ( TERMINO_LECTURA NUEVO_TERMINO_LECTURA ) ;
// AST: LECTURA -> ID | ACCESO_ARREGLO ...
NodoPtr Parser::lectura() {
    const int linea = consumir(TipoToken::SCANF, "'scanf'").linea;
    NodoPtr l = nuevoNodo("LECTURA", "scanf", linea);
    esperar(TipoToken::PAR_IZQ, "'('");
    l->agregar(terminoLectura());
    nuevoTerminoLectura(l.get());
    esperar(TipoToken::PAR_DER, "')'");
    esperar(TipoToken::PUNTOYCOMA, "';'");
    return l;
}

// TERMINO_LECTURA -> id DIMENSION2
NodoPtr Parser::terminoLectura() {
    Token id = consumir(TipoToken::ID, "una variable (identificador) para leer");
    return dimension2(nuevoNodo("ID", id.lexema, id.linea));
}

// NUEVO_TERMINO_LECTURA -> , TERMINO_LECTURA NUEVO_TERMINO_LECTURA | e
void Parser::nuevoTerminoLectura(NodoAST* l) {
    while (es(TipoToken::COMA)) {
        avanzar();
        l->agregar(terminoLectura());
    }
}

// DIMENSION2 -> [ EXPRESION_ADITIVA ] DIMENSION2 | e
// AST: si hay indices, el ID se convierte en ACCESO_ARREGLO <nombre> -> indices
NodoPtr Parser::dimension2(NodoPtr base) {
    if (!es(TipoToken::COR_IZQ)) return base;
    NodoPtr acceso = nuevoNodo("ACCESO_ARREGLO", base->valor, base->linea);
    while (es(TipoToken::COR_IZQ)) {
        avanzar();
        acceso->agregar(expresionAditiva());
        esperar(TipoToken::COR_DER, "']'");
    }
    return acceso;
}

// ESCRITURA -> println ( TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA ) ;
// AST: ESCRITURA -> terminos a imprimir
NodoPtr Parser::escritura() {
    const int linea = consumir(TipoToken::PRINTLN, "'println'").linea;
    NodoPtr e = nuevoNodo("ESCRITURA", "println", linea);
    esperar(TipoToken::PAR_IZQ, "'('");
    e->agregar(terminoEscritura());
    nuevoTerminoEscritura(e.get());
    esperar(TipoToken::PAR_DER, "')'");
    esperar(TipoToken::PUNTOYCOMA, "';'");
    return e;
}

// TERMINO_ESCRITURA -> id DIMENSION2 | CONSTANTE | texto
NodoPtr Parser::terminoEscritura() {
    if (es(TipoToken::ID)) {
        Token id = actual();
        avanzar();
        return dimension2(nuevoNodo("ID", id.lexema, id.linea));
    } else if (es(TipoToken::NUM_INT) || es(TipoToken::NUM_DEC)) {
        return constante();
    } else if (es(TipoToken::TEXTO)) {
        Token t = actual();
        avanzar();
        return nuevoNodo("TEXTO", t.lexema, t.linea);
    }
    error("un identificador, una constante o un texto para imprimir");
}

// NUEVO_TERMINO_ESCRITURA -> , TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA | e
void Parser::nuevoTerminoEscritura(NodoAST* e) {
    while (es(TipoToken::COMA)) {
        avanzar();
        e->agregar(terminoEscritura());
    }
}

// SELECCION -> if ( EXPRESION ) BLOQUE SINO
// AST: SI -> CONDICION, BLOQUE, SINO?
NodoPtr Parser::seleccion() {
    const int linea = consumir(TipoToken::IF, "'if'").linea;
    NodoPtr s = nuevoNodo("SI", "if", linea);
    esperar(TipoToken::PAR_IZQ, "'('");
    NodoPtr cond = nuevoNodo("CONDICION", "", actual().linea);
    cond->agregar(expresion());
    esperar(TipoToken::PAR_DER, "')'");
    s->agregar(std::move(cond));
    s->agregar(bloque());
    s->agregar(sino());
    return s;
}

// SINO -> else BLOQUE | e
NodoPtr Parser::sino() {
    if (!es(TipoToken::ELSE)) return nullptr;
    NodoPtr n = nuevoNodo("SINO", "else", actual().linea);
    avanzar();
    n->agregar(bloque());
    return n;
}

// MIENTRAS -> while ( EXPRESION ) BLOQUE
// AST: MIENTRAS -> CONDICION, BLOQUE
NodoPtr Parser::mientras() {
    const int linea = consumir(TipoToken::WHILE, "'while'").linea;
    NodoPtr m = nuevoNodo("MIENTRAS", "while", linea);
    esperar(TipoToken::PAR_IZQ, "'('");
    NodoPtr cond = nuevoNodo("CONDICION", "", actual().linea);
    cond->agregar(expresion());
    esperar(TipoToken::PAR_DER, "')'");
    m->agregar(std::move(cond));
    m->agregar(bloque());
    return m;
}

// ASIGNACION -> id = COMPLEMENTO ;
// AST: ASIGNACION <variable> -> valor
NodoPtr Parser::asignacion() {
    Token id = consumir(TipoToken::ID, "un identificador");
    esperar(TipoToken::ASIGNACION, "'=' (asignacion)");
    NodoPtr a = nuevoNodo("ASIGNACION", id.lexema, id.linea);
    a->agregar(complemento());
    esperar(TipoToken::PUNTOYCOMA, "';'");
    return a;
}

// COMPLEMENTO -> EXPRESION | FUNCION
NodoPtr Parser::complemento() {
    if (es(TipoToken::FUNC)) return funcion();
    return expresion();
}

// FUNCION -> func id ( ARGUMENTOS )
// AST: LLAMADA <nombre> -> argumentos
NodoPtr Parser::funcion() {
    esperar(TipoToken::FUNC, "'func'");
    Token id = consumir(TipoToken::ID, "el nombre de la funcion (identificador)");
    NodoPtr f = nuevoNodo("LLAMADA", id.lexema, id.linea);
    esperar(TipoToken::PAR_IZQ, "'('");
    argumentos(f.get());
    esperar(TipoToken::PAR_DER, "')'");
    return f;
}

// ARGUMENTOS -> EXPRESION NUEVO_ARGUMENTO | e
void Parser::argumentos(NodoAST* f) {
    if (iniciaExpresion()) {
        f->agregar(expresion());
        nuevoArgumento(f);
    }
}

// NUEVO_ARGUMENTO -> , EXPRESION NUEVO_ARGUMENTO | e
void Parser::nuevoArgumento(NodoAST* f) {
    while (es(TipoToken::COMA)) {
        avanzar();
        f->agregar(expresion());
    }
}

// RETORNO -> return EXPRESION ;
// AST: RETORNO -> expresion
NodoPtr Parser::retorno() {
    const int linea = consumir(TipoToken::RETURN, "'return'").linea;
    NodoPtr r = nuevoNodo("RETORNO", "return", linea);
    r->agregar(expresion());   // la gramatica no admite "return;" sin expresion
    esperar(TipoToken::PUNTOYCOMA, "';'");
    return r;
}

// ===========================================================================
//  Expresiones (de menor a mayor precedencia: || , && , comp , + - , * / %)
//  Las producciones X2 -> op Y X2 | e son recursion por la derecha; se
//  implementan con un while, que es equivalente y no usa la pila.
//
//  AST: cada X2 recibe el operando izquierdo ya construido y lo va
//  envolviendo en OPERACION <op> (izq, der). Asi "a - b - c" queda
//  ((a - b) - c): asociatividad por la IZQUIERDA, como en matematica.
// ===========================================================================

NodoPtr Parser::binaria(const Token& op, NodoPtr izq, NodoPtr der) {
    NodoPtr n = nuevoNodo("OPERACION", op.lexema, op.linea);
    n->agregar(std::move(izq));
    n->agregar(std::move(der));
    return n;
}

// EXPRESION -> EXPRESION_O
NodoPtr Parser::expresion() { return expresionO(); }

// EXPRESION_O -> EXPRESION_Y EXPRESION_O2
NodoPtr Parser::expresionO() {
    return expresionO2(expresionY());
}

// EXPRESION_O2 -> || EXPRESION_Y EXPRESION_O2 | e
NodoPtr Parser::expresionO2(NodoPtr izq) {
    while (es(TipoToken::OR)) {
        Token op = actual();
        avanzar();
        izq = binaria(op, std::move(izq), expresionY());
    }
    return izq;
}

// EXPRESION_Y -> EXPRESION_RELACIONAL EXPRESION_Y2
NodoPtr Parser::expresionY() {
    return expresionY2(expresionRelacional());
}

// EXPRESION_Y2 -> && EXPRESION_RELACIONAL EXPRESION_Y2 | e
NodoPtr Parser::expresionY2(NodoPtr izq) {
    while (es(TipoToken::AND)) {
        Token op = actual();
        avanzar();
        izq = binaria(op, std::move(izq), expresionRelacional());
    }
    return izq;
}

// EXPRESION_RELACIONAL -> EXPRESION_ADITIVA EXPRESION_RELACIONAL2
NodoPtr Parser::expresionRelacional() {
    return expresionRelacional2(expresionAditiva());
}

// EXPRESION_RELACIONAL2 -> comp EXPRESION_ADITIVA | e
// (sin recursion: "a < b < c" NO es valido en LP)
NodoPtr Parser::expresionRelacional2(NodoPtr izq) {
    if (es(TipoToken::COMP)) {
        Token op = actual();
        avanzar();
        return binaria(op, std::move(izq), expresionAditiva());
    }
    return izq;
}

// EXPRESION_ADITIVA -> EXPRESION_MULTIPLICATIVA EXPRESION_ADITIVA2
NodoPtr Parser::expresionAditiva() {
    return expresionAditiva2(expresionMultiplicativa());
}

// EXPRESION_ADITIVA2 -> OPERADOR_ADITIVO EXPRESION_MULTIPLICATIVA EXPRESION_ADITIVA2 | e
// OPERADOR_ADITIVO   -> + | -
NodoPtr Parser::expresionAditiva2(NodoPtr izq) {
    while (es(TipoToken::SUMA) || es(TipoToken::RESTA)) {
        Token op = actual();
        avanzar();
        izq = binaria(op, std::move(izq), expresionMultiplicativa());
    }
    return izq;
}

// EXPRESION_MULTIPLICATIVA -> FACTOR EXPRESION_MULTIPLICATIVA2
NodoPtr Parser::expresionMultiplicativa() {
    return expresionMultiplicativa2(factor());
}

// EXPRESION_MULTIPLICATIVA2 -> OPERADOR_MULTIPLICATIVO FACTOR EXPRESION_MULTIPLICATIVA2 | e
// OPERADOR_MULTIPLICATIVO   -> * | / | %
NodoPtr Parser::expresionMultiplicativa2(NodoPtr izq) {
    while (es(TipoToken::MULT) || es(TipoToken::DIV) || es(TipoToken::MOD)) {
        Token op = actual();
        avanzar();
        izq = binaria(op, std::move(izq), factor());
    }
    return izq;
}

// FACTOR -> SIGNO TERMINO | texto | ! FACTOR | ( EXPRESION )
// AST: "( EXPRESION )" no genera nodo: los parentesis ya quedan
// representados por la forma del arbol.
NodoPtr Parser::factor() {
    if (es(TipoToken::TEXTO)) {
        Token t = actual();
        avanzar();
        return nuevoNodo("TEXTO", t.lexema, t.linea);
    } else if (es(TipoToken::NOT)) {
        const int linea = actual().linea;
        avanzar();
        NodoPtr n = nuevoNodo("NEGACION", "!", linea);
        n->agregar(factor());
        return n;
    } else if (es(TipoToken::PAR_IZQ)) {
        avanzar();
        NodoPtr e = expresion();
        esperar(TipoToken::PAR_DER, "')'");
        return e;
    } else if (es(TipoToken::SUMA) || es(TipoToken::RESTA) ||
               es(TipoToken::ID)   || es(TipoToken::NUM_INT) || es(TipoToken::NUM_DEC)) {
        const int linea = actual().linea;
        std::string s = signo();
        NodoPtr t = termino();
        if (s.empty()) return t;
        NodoPtr n = nuevoNodo("SIGNO", s, linea);   // -x  /  +3
        n->agregar(std::move(t));
        return n;
    }
    error("una expresion (identificador, numero, texto, '!' o '(')");
}

// TERMINO -> id DIMENSION2 | CONSTANTE
NodoPtr Parser::termino() {
    if (es(TipoToken::ID)) {
        Token id = actual();
        avanzar();
        return dimension2(nuevoNodo("ID", id.lexema, id.linea));
    } else if (es(TipoToken::NUM_INT) || es(TipoToken::NUM_DEC)) {
        return constante();
    }
    error("un identificador o una constante numerica despues del signo");
}

// SIGNO -> + | - | e
std::string Parser::signo() {
    if (es(TipoToken::SUMA) || es(TipoToken::RESTA)) {
        std::string s = actual().lexema;
        avanzar();
        return s;
    }
    return "";
}

// CONSTANTE -> num_int | num_dec
NodoPtr Parser::constante() {
    if (es(TipoToken::NUM_INT) || es(TipoToken::NUM_DEC)) {
        Token c = actual();
        avanzar();
        return nuevoNodo(c.tipo == TipoToken::NUM_INT ? "NUM_INT" : "NUM_DEC", c.lexema, c.linea);
    }
    error("una constante numerica");
}
