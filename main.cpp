#include <iostream>
#include <memory>

#include "Archivo.h"
#include "Auditoria.h"
#include "Carpeta.h"
#include "ConexionRed.h"
#include "Servidor.h"
#include "Sistema.h"
#include "Usuario.h"

using namespace std;

// =====================================================================
// main: prueba mínima de compilación y enlace de las clases base.
// =====================================================================
// Construye un par de servidores, un pequeño árbol de archivos, una
// conexión de red y un usuario, y muestra el resultado. NO implementa
// autenticación, operaciones completas de archivos ni algoritmos de red:
// solo verifica que las clases base cooperan correctamente.
int main() {
    // --- Servidores ---------------------------------------------------
    auto servidorA = make_shared<Servidor>(1, "Servidor-A");
    auto servidorB = make_shared<Servidor>(2, "Servidor-B");

    // --- Árbol de archivos del servidor A -----------------------------
    auto carpetaDocs = make_shared<Carpeta>(10, "documentos");
    carpetaDocs->agregarHijo(make_shared<Archivo>(11, "notas.txt", "hola red"));
    carpetaDocs->agregarHijo(make_shared<Archivo>(12, "informe.dat", "0123456789"));

    servidorA->getRaiz()->agregarHijo(carpetaDocs);
    servidorA->getRaiz()->agregarHijo(
        make_shared<Archivo>(13, "config.cfg", "debug=1"));

    // --- Conexión de red ---------------------------------------------
    ConexionRed enlace(servidorA, servidorB, 15);

    // --- Usuario ------------------------------------------------------
    Usuario admin(1, "admin", "secreta");

    // --- Salida -------------------------------------------------------
    cout << "===== Servidor A =====\n";
    servidorA->mostrar();

    cout << "\n===== Enlace de red =====\n";
    enlace.mostrar();

    cout << "\nTamaño total del arbol de A: "
         << servidorA->getRaiz()->obtenerTamano() << " bytes\n";

    // Ejemplo de uso polimórfico a través de la base abstracta.
    auto nodo = servidorA->getRaiz()->buscarHijoPorNombre("documentos");
    if (nodo) {
        cout << "\nNodo encontrado: " << nodo->getNombre()
             << " | es carpeta? " << (nodo->esCarpeta() ? "si" : "no")
             << " | ruta: " << nodo->obtenerRuta() << "\n";
    }

    cout << "\n===== Usuario =====\n";
    admin.mostrarInformacion();

    // ================================================================
    // Clase central Sistema: inicializa y administra las 4 estructuras.
    // ================================================================
    cout << "\n\n########## Clase central: Sistema ##########\n";
    Sistema so;

    // 1) Árbol de directorios: cada servidor recibe su carpeta raíz.
    so.crearServidor(1, "Servidor-A");
    so.crearServidor(2, "Servidor-B");
    so.crearServidor(3, "Servidor-C");
    so.crearServidor(1, "Duplicado");   // rechazado: ID repetido

    // 2) Tabla hash de usuarios (colisiones resueltas por encadenamiento).
    so.registrarUsuario(10, "admin", "secreta");
    so.registrarUsuario(11, "dylan", "pass123");
    so.registrarUsuario(12, "maria", "clave");
    so.registrarUsuario(13, "admin", "otra");   // rechazado: nombre duplicado

    // 3) Grafo de red (listas de adyacencia con latencia).
    so.conectarServidores(1, 2, 15);
    so.conectarServidores(2, 3, 20);
    so.conectarServidores(1, 3, 40);

    cout << "\n----- Resumen de estructuras -----\n";
    so.mostrarResumen();

    cout << "\n----- Volcado de auditoría -----\n";
    so.getAuditoria().mostrar();
    if (so.guardarAuditoria()) {
        cout << "Auditoría guardada en disco correctamente.\n";
    }

    return 0;
}