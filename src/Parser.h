#ifndef PARSER_H
#define PARSER_H

#include <stdexcept>
#include <string>
#include <vector>
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
// ---------------------------------------------------------------------------

struct ErrorSintactico : public std::runtime_error {
    explicit ErrorSintactico(const std::string& msg) : std::runtime_error(msg) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    // Analiza todo el programa. true = ACEPTADO, false = error sintactico.
    bool analizar();

    // Mensaje del error encontrado (vacio si el programa fue aceptado).
    const std::string& mensajeError() const { return mensajeError_; }

private:
    std::vector<Token> tokens_;
    size_t             pos_;          // indice del token actual (lookahead)
    std::string        mensajeError_;

    // --- utilidades sobre la secuencia de tokens ---
    const Token& actual() const;               // token actual sin consumirlo
    const Token& siguienteTok() const;         // token despues del actual (LL(2))
    const Token& anterior() const;             // ultimo token consumido
    bool  es(TipoToken tipo) const;            // ¿el token actual es de este tipo?
    void  avanzar();                           // consume el token actual
    void  esperar(TipoToken tipo, const std::string& descripcion); // match()
    [[noreturn]] void error(const std::string& esperado);

    static std::string describir(const Token& t);  // "'edad' (ID)"
    static std::string nombreLegible(TipoToken t);  // "';'" , "identificador"

    // --- PRIMERO de algunos no terminales ---
    bool esTipoBase() const;          // char | float | int | boolean
    bool iniciaComando() const;       // PRIMERO(COMANDO)
    bool iniciaExpresion() const;     // PRIMERO(EXPRESION)

    // --- un metodo por cada no terminal de la gramatica LP ---
    void programa();                 // PROGRAMA        -> LISTA_FUNCIONES PRINCIPAL
    void listaFunciones();           // LISTA_FUNCIONES -> DECLARACION_FUNCION LISTA_FUNCIONES | e
    void declaracionFuncion();       // DECLARACION_FUNCION -> TIPO_RETORNO id ( PARAMETROS ) BLOQUE
    void tipoRetorno();              // TIPO_RETORNO    -> TIPO | void
    void tipo();                     // TIPO            -> TIPO_BASE DIMENSION
    void tipoBase();                 // TIPO_BASE       -> char | float | int | boolean
    void dimension();                // DIMENSION       -> [ num_int ] DIMENSION | e
    void parametros();               // PARAMETROS      -> TIPO id LISTA_PARAMETROS | e
    void listaParametros();          // LISTA_PARAMETROS -> , TIPO id LISTA_PARAMETROS | e
    void principal();                // PRINCIPAL       -> main ( ) BLOQUE
    void bloque();                   // BLOQUE          -> { LISTA_VARIABLES COMANDOS }
    void listaVariables();           // LISTA_VARIABLES -> TIPO id LISTA_ID ; LISTA_VARIABLES | e
    void listaId();                  // LISTA_ID        -> , id LISTA_ID | e
    void comandos();                 // COMANDOS        -> COMANDO COMANDOS | e
    void comando();                  // COMANDO         -> LECTURA | ESCRITURA | ASIGNACION | FUNCION | SELECCION | MIENTRAS | RETORNO
    void lectura();                  // LECTURA         -> scanf ( TERMINO_LECTURA NUEVO_TERMINO_LECTURA ) ;
    void terminoLectura();           // TERMINO_LECTURA -> id DIMENSION2
    void nuevoTerminoLectura();      // NUEVO_TERMINO_LECTURA -> , TERMINO_LECTURA NUEVO_TERMINO_LECTURA | e
    void dimension2();               // DIMENSION2      -> [ EXPRESION_ADITIVA ] DIMENSION2 | e
    void escritura();                // ESCRITURA       -> println ( TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA ) ;
    void terminoEscritura();         // TERMINO_ESCRITURA -> id DIMENSION2 | CONSTANTE | texto
    void nuevoTerminoEscritura();    // NUEVO_TERMINO_ESCRITURA -> , TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA | e
    void seleccion();                // SELECCION       -> if ( EXPRESION ) BLOQUE SINO
    void sino();                     // SINO            -> else BLOQUE | e
    void mientras();                 // MIENTRAS        -> while ( EXPRESION ) BLOQUE
    void asignacion();               // ASIGNACION      -> id = COMPLEMENTO ;
    void complemento();              // COMPLEMENTO     -> EXPRESION | FUNCION
    void funcion();                  // FUNCION         -> func id ( ARGUMENTOS )
    void argumentos();               // ARGUMENTOS      -> EXPRESION NUEVO_ARGUMENTO | e
    void nuevoArgumento();           // NUEVO_ARGUMENTO -> , EXPRESION NUEVO_ARGUMENTO | e
    void retorno();                  // RETORNO         -> return EXPRESION ;

    void expresion();                // EXPRESION       -> EXPRESION_O
    void expresionO();               // EXPRESION_O     -> EXPRESION_Y EXPRESION_O2
    void expresionO2();              // EXPRESION_O2    -> || EXPRESION_Y EXPRESION_O2 | e
    void expresionY();               // EXPRESION_Y     -> EXPRESION_RELACIONAL EXPRESION_Y2
    void expresionY2();              // EXPRESION_Y2    -> && EXPRESION_RELACIONAL EXPRESION_Y2 | e
    void expresionRelacional();      // EXPRESION_RELACIONAL -> EXPRESION_ADITIVA EXPRESION_RELACIONAL2
    void expresionRelacional2();     // EXPRESION_RELACIONAL2 -> comp EXPRESION_ADITIVA | e
    void expresionAditiva();         // EXPRESION_ADITIVA -> EXPRESION_MULTIPLICATIVA EXPRESION_ADITIVA2
    void expresionAditiva2();        // EXPRESION_ADITIVA2 -> OPERADOR_ADITIVO EXPRESION_MULTIPLICATIVA EXPRESION_ADITIVA2 | e
    void expresionMultiplicativa();  // EXPRESION_MULTIPLICATIVA -> FACTOR EXPRESION_MULTIPLICATIVA2
    void expresionMultiplicativa2(); // EXPRESION_MULTIPLICATIVA2 -> OPERADOR_MULTIPLICATIVO FACTOR EXPRESION_MULTIPLICATIVA2 | e
    void factor();                   // FACTOR          -> SIGNO TERMINO | texto | ! FACTOR | ( EXPRESION )
    void termino();                  // TERMINO         -> id DIMENSION2 | CONSTANTE
    void signo();                    // SIGNO           -> + | - | e
    void constante();                // CONSTANTE       -> num_int | num_dec
};

#endif // PARSER_H
