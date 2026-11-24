// Entregable 2: funciones con parametros, llamadas, lectura y escritura
float promedio(float a, float b, float c) {
    float p;
    p = (a + b + c) / 3;
    return p;
}

int maximo(int[10] v, int n) {
    int i, m;
    i = 1;
    m = v[0];
    while (i < n) {
        if (v[i] > m) {
            m = v[i];
        }
        i = i + 1;
    }
    return m;
}

void mostrar(float x) {
    println("Resultado:", x);
}

void main() {
    float n1, n2, n3, prom;
    int[10] datos;
    int mayor;
    scanf(n1, n2, n3);
    scanf(datos[0], datos[1], datos[2]);
    prom = func promedio(n1, n2, n3);
    mayor = func maximo(datos, 3);
    func mostrar(prom)
    println("Mayor:", mayor, "de", 3, "datos");
}
