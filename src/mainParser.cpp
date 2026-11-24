// ---------------------------------------------------------------------------
// ParserLP - Analizador sintactico para el lenguaje LP
// Curso: Compiladores - Proyecto integrador (analisis sintactico)
//
// Flujo:   archivo .lp  ->  Lexer  ->  tokens  ->  Parser  ->  resultado
//
// Uso:  ParserLP.exe <archivo.lp> [--ast]
//       --ast  imprime tambien el Arbol de Sintaxis Abstracta (Entregable 2).
//              El AST de un programa aceptado siempre se guarda en output/ast.txt
//
// Codigo de salida:  0 = programa ACEPTADO
//                    1 = programa RECHAZADO (error lexico o sintactico)
//                    2 = no se pudo abrir el archivo / uso incorrecto
// ---------------------------------------------------------------------------
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Lexer.h"
#include "Parser.h"
#include "Token.h"

static bool leerArchivo(const std::string& ruta, std::string& contenido) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) return false;
    std::stringstream buffer;
    buffer << archivo.rdbuf();
    contenido = buffer.str();
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: ParserLP <archivo.lp> [--ast]\n";
        return 2;
    }
    const std::string ruta = argv[1];
    const bool mostrarAst = argc >= 3 && std::string(argv[2]) == "--ast";

    std::string fuente;
    if (!leerArchivo(ruta, fuente)) {
        std::cerr << "ERROR: no se pudo abrir el archivo '" << ruta << "'\n";
        return 2;
    }

    std::ostringstream informe;   // lo mismo que se imprime se guarda en output/
    informe << "=== ParserLP - Analizador Sintactico ===\n";
    informe << "Archivo fuente: " << ruta << "\n\n";

    // 1. Analisis lexico (el parser NO lee caracteres, solo tokens)
    Lexer lexer(fuente);
    std::vector<Token> tokens = lexer.analizarConFin();
    informe << "Fase lexica:      " << (tokens.size() - 1) << " tokens, "
            << lexer.errores().size() << " errores lexicos\n";

    int codigo = 0;

    if (!lexer.errores().empty()) {
        // Con caracteres invalidos los tokens no son confiables: no se
        // intenta el analisis sintactico.
        informe << "\nErrores lexicos (corrigelos antes del analisis sintactico):\n";
        for (const Token& e : lexer.errores()) {
            informe << "  linea " << e.linea << ", columna " << e.columna
                    << ": '" << e.lexema << "' no pertenece al lenguaje LP\n";
        }
        informe << "\nResultado: PROGRAMA RECHAZADO (error lexico)\n";
        codigo = 1;
    } else {
        // 2. Analisis sintactico
        // Entregable 2: el parser se recupera de los errores, asi que puede
        // reportar varios en una sola ejecucion.
        Parser parser(tokens);
        bool ok = parser.analizar();
        const size_t nErrores = parser.errores().size();
        informe << "Fase sintactica:  ";
        if (ok)                 informe << "sin errores";
        else if (nErrores == 1) informe << "1 error";
        else                    informe << nErrores << " errores";
        informe << "\n\n";
        if (ok) {
            informe << "Resultado: PROGRAMA ACEPTADO (sintacticamente correcto)\n";

            // AST: se imprime en consola con --ast y siempre se guarda en output/ast.txt
            std::ostringstream arbol;
            parser.ast()->imprimir(arbol);
            if (mostrarAst) {
                informe << "\n=== Arbol de Sintaxis Abstracta (AST) ===\n" << arbol.str();
            }
            std::filesystem::create_directories("output");
            std::ofstream salidaAst("output/ast.txt");
            salidaAst << arbol.str();
        } else {
            for (const std::string& e : parser.errores()) {
                informe << e << "\n\n";
            }
            informe << "Resultado: PROGRAMA RECHAZADO (error sintactico)\n";
            codigo = 1;
        }
    }

    std::cout << informe.str();

    std::filesystem::create_directories("output");
    std::ofstream salida("output/sintactico.txt");
    salida << informe.str();

    return codigo;
}
