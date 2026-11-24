#ifndef PARSER_H
#define PARSER_H

#include <stdexcept>
#include <string>
#include <vector>
#include "Ast.h"
#include "Token.h"

// ---------------------------------------------------------------------------
// Analizador sintactico de LP por DESCENSO RECURSIVO.
//
// Recibe la secuencia de tokens que ya produjo el Lexer (terminada en
// FIN_ARCHIVO). No vuelve a leer caracteres: solo mira tokens.
//
// Cada no terminal de la gramatica LP es un metodo con el mismo nombre
// (PROGRAMA -> programa(), LISTA_VARIABLES -> listaVariables(), ...).
// Las producciones con epsilon se implementan como "si el token actual
// pertenece al PRIMERO de la alternativa, la sigo; si no, no consumo nada".
//
// Manejo de errores (Entregable 1): modo panico simple. Al primer error se
// lanza ErrorSintactico, se detiene el analisis y se reporta linea,
// columna, token encontrado y lo que se esperaba.
//
// Entregable 2:
//  - Cada metodo devuelve el nodo del AST que reconocio (ver Ast.h).
//  - Recuperacion de errores en modo panico CON SINCRONIZACION: el error se
//    registra, se descartan tokens hasta un punto seguro (';', '}', inicio
//    de un comando en una linea nueva...) y el analisis continua. Asi se
//    reportan varios errores en una sola ejecucion.
// ---------------------------------------------------------------------------

struct ErrorSintactico : public std::runtime_error {
    size_t pos;   // indice del token donde se detecto (evita repetir errores)
    ErrorSintactico(const std::string& msg, size_t p) : std::runtime_error(msg), pos(p) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    // Analiza todo el programa. true = ACEPTADO, false = hubo errores.
    bool analizar();

    // Primer error encontrado (vacio si el programa fue aceptado).
    const std::string& mensajeError() const { return mensajeError_; }

    // Todos los errores sintacticos, en el orden en que aparecen.
    const std::vector<std::string>& errores() const { return errores_; }

    // Raiz del AST (nullptr si el analisis no llego a construirlo).
    // Con errores el arbol es parcial: solo se debe mostrar si fue ACEPTADO.
    const NodoAST* ast() const { return ast_.get(); }

private:
    std::vector<Token>       tokens_;
    size_t                   pos_;          // indice del token actual (lookahead)
    std::string              mensajeError_;
    std::vector<std::string> errores_;
    size_t                   ultimaPosError_;
    NodoPtr                  ast_;

    // Se lanza cuando ya no hay forma de seguir (se llego al fin de archivo
    // durante la recuperacion). El error ya quedo registrado.
    struct AnalisisDetenido {};

    // --- utilidades sobre la secuencia de tokens ---
    const Token& actual() const;               // token actual sin consumirlo
    const Token& siguienteTok() const;         // token despues del actual (LL(2))
    const Token& anterior() const;             // ultimo token consumido
    bool  es(TipoToken tipo) const;            // ¿el token actual es de este tipo?
    void  avanzar();                           // consume el token actual
    void  esperar(TipoToken tipo, const std::string& descripcion); // match()
    Token consumir(TipoToken tipo, const std::string& descripcion); // match() que devuelve el token
    [[noreturn]] void error(const std::string& esperado);

    static std::string describir(const Token& t);  // "'edad' (ID)"
    static std::string nombreLegible(TipoToken t);  // "';'" , "identificador"

    // --- recuperacion de errores (Entregable 2) ---
    void registrar(const ErrorSintactico& e);  // guarda el error (sin duplicados)
    void sincronizar();                        // descarta tokens hasta un punto seguro
    void saltarBloque();                       // descarta un { ... } completo
    void sincronizarHastaBloque();             // tras un error en una cabecera: hasta '{'

    // --- PRIMERO de algunos no terminales ---
    bool esTipoBase() const;          // char | float | int | boolean
    bool iniciaComando() const;       // PRIMERO(COMANDO)
    bool iniciaExpresion() const;     // PRIMERO(EXPRESION)

    // --- un metodo por cada no terminal de la gramatica LP ---
    NodoPtr     programa();                 // PROGRAMA        -> LISTA_FUNCIONES PRINCIPAL
    void        listaFunciones(NodoAST* p); // LISTA_FUNCIONES -> DECLARACION_FUNCION LISTA_FUNCIONES | e
    NodoPtr     declaracionFuncion();       // DECLARACION_FUNCION -> TIPO_RETORNO id ( PARAMETROS ) BLOQUE
    std::string tipoRetorno();              // TIPO_RETORNO    -> TIPO | void
    std::string tipo();                     // TIPO            -> TIPO_BASE DIMENSION
    std::string tipoBase();                 // TIPO_BASE       -> char | float | int | boolean
    std::string dimension();                // DIMENSION       -> [ num_int ] DIMENSION | e
    NodoPtr     parametros();               // PARAMETROS      -> TIPO id LISTA_PARAMETROS | e
    void        listaParametros(NodoAST* p);// LISTA_PARAMETROS -> , TIPO id LISTA_PARAMETROS | e
    NodoPtr     principal();                // PRINCIPAL       -> main ( ) BLOQUE
    NodoPtr     bloque();                   // BLOQUE          -> { LISTA_VARIABLES COMANDOS }
    void        listaVariables(NodoAST* b); // LISTA_VARIABLES -> TIPO id LISTA_ID ; LISTA_VARIABLES | e
    void        listaId(NodoAST* decl);     // LISTA_ID        -> , id LISTA_ID | e
    void        comandos(NodoAST* b);       // COMANDOS        -> COMANDO COMANDOS | e
    NodoPtr     comando();                  // COMANDO         -> LECTURA | ESCRITURA | ASIGNACION | FUNCION | SELECCION | MIENTRAS | RETORNO
    NodoPtr     lectura();                  // LECTURA         -> scanf ( TERMINO_LECTURA NUEVO_TERMINO_LECTURA ) ;
    NodoPtr     terminoLectura();           // TERMINO_LECTURA -> id DIMENSION2
    void        nuevoTerminoLectura(NodoAST* l);   // NUEVO_TERMINO_LECTURA -> , TERMINO_LECTURA NUEVO_TERMINO_LECTURA | e
    NodoPtr     dimension2(NodoPtr base);   // DIMENSION2      -> [ EXPRESION_ADITIVA ] DIMENSION2 | e
    NodoPtr     escritura();                // ESCRITURA       -> println ( TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA ) ;
    NodoPtr     terminoEscritura();         // TERMINO_ESCRITURA -> id DIMENSION2 | CONSTANTE | texto
    void        nuevoTerminoEscritura(NodoAST* e); // NUEVO_TERMINO_ESCRITURA -> , TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA | e
    NodoPtr     seleccion();                // SELECCION       -> if ( EXPRESION ) BLOQUE SINO
    NodoPtr     sino();                     // SINO            -> else BLOQUE | e
    NodoPtr     mientras();                 // MIENTRAS        -> while ( EXPRESION ) BLOQUE
    NodoPtr     asignacion();               // ASIGNACION      -> id = COMPLEMENTO ;
    NodoPtr     complemento();              // COMPLEMENTO     -> EXPRESION | FUNCION
    NodoPtr     funcion();                  // FUNCION         -> func id ( ARGUMENTOS )
    void        argumentos(NodoAST* f);     // ARGUMENTOS      -> EXPRESION NUEVO_ARGUMENTO | e
    void        nuevoArgumento(NodoAST* f); // NUEVO_ARGUMENTO -> , EXPRESION NUEVO_ARGUMENTO | e
    NodoPtr     retorno();                  // RETORNO         -> return EXPRESION ;

    NodoPtr expresion();                         // EXPRESION       -> EXPRESION_O
    NodoPtr expresionO();                        // EXPRESION_O     -> EXPRESION_Y EXPRESION_O2
    NodoPtr expresionO2(NodoPtr izq);            // EXPRESION_O2    -> || EXPRESION_Y EXPRESION_O2 | e
    NodoPtr expresionY();                        // EXPRESION_Y     -> EXPRESION_RELACIONAL EXPRESION_Y2
    NodoPtr expresionY2(NodoPtr izq);            // EXPRESION_Y2    -> && EXPRESION_RELACIONAL EXPRESION_Y2 | e
    NodoPtr expresionRelacional();               // EXPRESION_RELACIONAL -> EXPRESION_ADITIVA EXPRESION_RELACIONAL2
    NodoPtr expresionRelacional2(NodoPtr izq);   // EXPRESION_RELACIONAL2 -> comp EXPRESION_ADITIVA | e
    NodoPtr expresionAditiva();                  // EXPRESION_ADITIVA -> EXPRESION_MULTIPLICATIVA EXPRESION_ADITIVA2
    NodoPtr expresionAditiva2(NodoPtr izq);      // EXPRESION_ADITIVA2 -> OPERADOR_ADITIVO EXPRESION_MULTIPLICATIVA EXPRESION_ADITIVA2 | e
    NodoPtr expresionMultiplicativa();           // EXPRESION_MULTIPLICATIVA -> FACTOR EXPRESION_MULTIPLICATIVA2
    NodoPtr expresionMultiplicativa2(NodoPtr izq); // EXPRESION_MULTIPLICATIVA2 -> OPERADOR_MULTIPLICATIVO FACTOR EXPRESION_MULTIPLICATIVA2 | e
    NodoPtr factor();                            // FACTOR          -> SIGNO TERMINO | texto | ! FACTOR | ( EXPRESION )
    NodoPtr termino();                           // TERMINO         -> id DIMENSION2 | CONSTANTE
    std::string signo();                         // SIGNO           -> + | - | e
    NodoPtr constante();                         // CONSTANTE       -> num_int | num_dec

    // Une dos operandos con un operador binario: OPERACION op (izq, der).
    static NodoPtr binaria(const Token& op, NodoPtr izq, NodoPtr der);
};

#endif // PARSER_H
