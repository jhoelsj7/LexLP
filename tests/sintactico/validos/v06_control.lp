// Entregable 2: estructuras de control anidadas (if / else / while)
void main() {
    int nota, i, aprobados;
    boolean seguir;
    i = 0;
    aprobados = 0;
    seguir = 1;
    while (i < 5 && seguir) {
        scanf(nota);
        if (nota >= 11) {
            aprobados = aprobados + 1;
            if (nota == 20) {
                println("Nota perfecta");
            } else {
                println("Aprobado con", nota);
            }
        } else {
            if (nota < 0 || nota > 20) {
                println("Nota invalida");
                seguir = 0;
            }
        }
        i = i + 1;
    }
    if (!(aprobados == 0)) {
        println("Aprobados:", aprobados);
    }
    return -aprobados * 2 % 3;
}
