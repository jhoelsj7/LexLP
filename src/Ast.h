#ifndef AST_H
#define AST_H

#include <memory>
#include <ostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Arbol de Sintaxis Abstracta (AST) de LP  -  Entregable 2.
//
// Todos los nodos tienen la misma forma: un tipo (PROGRAMA, BLOQUE, SI,
// OPERACION, ...), un valor opcional (el nombre de la variable, el operador,
// la constante...), la linea donde empieza y la lista de hijos.
//
// A diferencia del arbol de derivacion, el AST NO guarda los simbolos que
// solo sirven para la sintaxis ( '(' ')' '{' '}' ';' ',' ) ni los no
// terminales auxiliares (EXPRESION_O2, LISTA_ID, ...). Solo queda la
// estructura que necesita una fase posterior (semantica / generacion).
//
// Se implementa solo en este header para no cambiar los comandos de
// compilacion del proyecto.
// ---------------------------------------------------------------------------

struct NodoAST;
using NodoPtr = std::unique_ptr<NodoAST>;

struct NodoAST {
    std::string          tipo;    // PROGRAMA, FUNCION, BLOQUE, ASIGNACION, OPERACION, ...
    std::string          valor;   // nombre, operador o constante ("" si no aplica)
    int                  linea;   // linea del fuente donde empieza
    std::vector<NodoPtr> hijos;

    NodoAST(std::string t, std::string v, int l)
        : tipo(std::move(t)), valor(std::move(v)), linea(l) {}

    // Agrega un hijo (ignora punteros nulos) y devuelve el nodo agregado.
    NodoAST* agregar(NodoPtr hijo) {
        if (!hijo) return nullptr;
        hijos.push_back(std::move(hijo));
        return hijos.back().get();
    }

    // Imprime el arbol con lineas ASCII:
    //   PROGRAMA
    //   `-- PRINCIPAL main  (linea 1)
    //       `-- BLOQUE
    void imprimir(std::ostream& os) const { imprimirRec(os, "", true, true); }

private:
    void imprimirRec(std::ostream& os, const std::string& prefijo,
                     bool ultimo, bool raiz) const {
        os << prefijo;
        if (!raiz) os << (ultimo ? "`-- " : "|-- ");
        os << tipo;
        if (!valor.empty()) os << " " << valor;
        os << "  (linea " << linea << ")\n";

        std::string sig = raiz ? "" : prefijo + (ultimo ? "    " : "|   ");
        for (size_t i = 0; i < hijos.size(); i++) {
            hijos[i]->imprimirRec(os, sig, i + 1 == hijos.size(), false);
        }
    }
};

inline NodoPtr nuevoNodo(const std::string& tipo, const std::string& valor, int linea) {
    return std::make_unique<NodoAST>(tipo, valor, linea);
}

#endif // AST_H
