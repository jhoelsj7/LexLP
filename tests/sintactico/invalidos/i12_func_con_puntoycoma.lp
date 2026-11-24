// COMANDO -> FUNCION  y  FUNCION -> func id ( ARGUMENTOS )
// Una llamada usada como comando NO lleva ';' segun la gramatica LP.
// (En una asignacion si: x = func f(x); porque ASIGNACION termina en ';')
void saludar(int n) {
    println("hola", n);
}

main() {
    int x;
    x = 1;
    func saludar(x);
    x = func saludar(x);
}
