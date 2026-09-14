// ---------------------------------------------------------------------------
// LexLP - Analizador Lexico para el lenguaje LP
// Curso: Compiladores
//
// AVANCE SEMANA 1 (15/09): lectura del archivo fuente + reconocimiento de
//                          NUM_INT y NUM_DEC + reporte de errores lexicos
//                          con linea y columna.
//
// Uso:  LexLP.exe  <archivo.lp>
//       Si no se indica archivo, usa tests/prueba_semana1.lp
// ---------------------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#include "Lexer.h"
#include "Token.h"

// Lee TODO el archivo fuente y lo devuelve en un solo string.
// Se lee completo (y no linea por linea) porque el analizador necesita poder
// mirar el caracter siguiente aunque este al final de una linea.
static bool leerArchivo(const std::string& ruta, std::string& contenido) {
    std::ifstream archivo(ruta);
    if (!archivo.is_open()) {
        return false;
    }
    std::stringstream buffer;
    buffer << archivo.rdbuf();
    contenido = buffer.str();
    return true;
}

// Imprime la lista de tokens respetando el orden del programa fuente
// y agrupandolos por linea, tal como los muestra la especificacion.
static void imprimirListaTokens(std::ostream& salida, const std::vector<Token>& tokens) {
    int lineaActual = -1;
    for (const Token& t : tokens) {
        if (t.linea != lineaActual) {
            if (lineaActual != -1) salida << "\n";
            lineaActual = t.linea;
        } else {
            salida << " ";
        }
        salida << t.aCadena();
    }
    salida << "\n";
}

// Detalle de cada token con su ubicacion: sirve de apoyo para depurar
// y para sustentar el reconocimiento en la presentacion.
static void imprimirDetalleTokens(std::ostream& salida, const std::vector<Token>& tokens) {
    salida << "LINEA  COLUMNA  TOKEN         LEXEMA\n";
    salida << "-----  -------  ------------  ------------\n";
    for (const Token& t : tokens) {
        salida << "  " << t.linea
               << "\t   " << t.columna
               << "\t   " << nombreTipoToken(t.tipo)
               << "\t " << t.lexema << "\n";
    }
}

static void imprimirErrores(std::ostream& salida, const std::vector<Token>& errores) {
    if (errores.empty()) {
        salida << "No se encontraron errores lexicos.\n";
        return;
    }
    salida << "LINEA  COLUMNA  LEXEMA  RESULTADO\n";
    salida << "-----  -------  ------  -------------\n";
    for (const Token& e : errores) {
        salida << "  " << e.linea
               << "\t   " << e.columna
               << "\t   " << e.lexema
               << "\t " << nombreTipoToken(e.tipo) << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::string rutaFuente = (argc > 1) ? argv[1] : "tests/prueba_semana1.lp";

    std::string fuente;
    if (!leerArchivo(rutaFuente, fuente)) {
        std::cerr << "ERROR: no se pudo abrir el archivo '" << rutaFuente << "'\n";
        return 1;
    }

    std::cout << "=== LexLP - Analizador Lexico ===\n";
    std::cout << "Archivo fuente: " << rutaFuente << "\n\n";

    // 1. Analisis lexico
    Lexer lexer(fuente);
    std::vector<Token> tokens = lexer.analizarTodo();

    // 2. Salida por consola
    std::cout << "--- LISTA DE TOKENS ---\n";
    imprimirListaTokens(std::cout, tokens);

    std::cout << "\n--- DETALLE (linea / columna) ---\n";
    imprimirDetalleTokens(std::cout, tokens);

    std::cout << "\n--- ERRORES LEXICOS ---\n";
    imprimirErrores(std::cout, lexer.errores());

    std::cout << "\nTotal de tokens reconocidos: " << tokens.size() << "\n";
    std::cout << "Total de errores lexicos:    " << lexer.errores().size() << "\n";

    // 3. Exportacion de resultados a la carpeta output/
    std::filesystem::create_directories("output");

    std::ofstream fTokens("output/tokens.txt");
    imprimirListaTokens(fTokens, tokens);
    fTokens << "\n";
    imprimirDetalleTokens(fTokens, tokens);

    std::ofstream fErrores("output/errores.txt");
    imprimirErrores(fErrores, lexer.errores());

    std::cout << "\nResultados exportados a output/tokens.txt y output/errores.txt\n";
    return 0;
}
