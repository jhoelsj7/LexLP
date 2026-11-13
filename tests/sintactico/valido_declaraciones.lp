// Varias declaraciones, listas de ids, arreglos y asignaciones con expresiones.
// Segun la gramatica, la dimension va pegada al tipo: TIPO -> TIPO_BASE DIMENSION
void main() {
    int a, b, c;
    float[10] notas;
    int[3][3] matriz;
    char letra;
    boolean activo;
    a = 5;
    b = a + 3 * 2;
    c = (a - b) / 2 % 3;
    a = -b + notas[a + 1];
    activo = !(a > b) || a == c;
}
