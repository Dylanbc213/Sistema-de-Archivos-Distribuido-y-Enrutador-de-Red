#include "Menu.h"

#include "Auditoria.h"

#include <cctype>
#include <iostream>
#include <string>

using namespace std;

// ---------------------------------------------------------------------
// Utilidades de consola
// ---------------------------------------------------------------------

// Líneas decorativas reutilizables para mantener los menús ordenados.
namespace {
    const string SEPARADOR = string(48, '=');
    const string GUIONES = string(48, '-');
}

// ---------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------

Menu::Menu(Sistema& s, LoggerAuditoria& log) : sistema(s), logger(log) {}

// ---------------------------------------------------------------------
// Entrada validada
// ---------------------------------------------------------------------

// Lee una línea completa y la intenta interpretar como entero. Cualquier
// entrada no numérica (letras, símbolos, vacía) produce -1, que el
// llamador considera inválida. Se usa getline en lugar de 'cin >>' para
// consumir la línea entera y no dejar residuos que ensucien la siguiente
// lectura.
int Menu::leerOpcion() const {
    string linea;
    getline(cin, linea);

    // Descarta espacios alrededor de la entrada.
    size_t inicio = linea.find_first_not_of(" \t");
    if (inicio == string::npos) {
        return -1;  // línea vacía o solo espacios.
    }
    size_t fin = linea.find_last_not_of(" \t");
    linea = linea.substr(inicio, fin - inicio + 1);

    // Verifica que todos los caracteres sean dígitos (admite signo).
    for (size_t i = 0; i < linea.size(); ++i) {
        char c = linea[i];
        bool signo = (c == '-' || c == '+') && (i == 0);
        if (!isdigit(static_cast<unsigned char>(c)) && !signo) {
            return -1;  // hay caracteres no numéricos.
        }
    }

    try {
        return stoi(linea);
    } catch (const exception&) {
        // Fuera de rango de int.
        return -1;
    }
}

// ---------------------------------------------------------------------
// Mensaje de funcionalidad pendiente
// ---------------------------------------------------------------------

void Menu::noImplementado(const string& funcionalidad) const {
    cout << "\n[!] La funcionalidad \"" << funcionalidad
         << "\" aun no esta implementada (fase posterior).\n";
    sistema.getAuditoria().registrar(
        "MENU", "Acceso a funcionalidad pendiente: " + funcionalidad + ".");
    logger.append("MENU", "Acceso a funcionalidad pendiente: " + funcionalidad + ".");
}

// ---------------------------------------------------------------------
// Dibujo de los menús
// ---------------------------------------------------------------------

void Menu::mostrarMenuPrincipal() const {
    cout << "\n" << SEPARADOR << "\n";
    cout << "           N E T W O R K   O S\n";
    cout << SEPARADOR << "\n";
    cout << "  1. Gestion de archivos\n";
    cout << "  2. Gestion de usuarios\n";
    cout << "  3. Red y enrutamiento\n";
    cout << "  4. Auditoria\n";
    cout << "  0. Salir\n";
    cout << GUIONES << "\n";
    cout << "Seleccione una opcion: ";
}

void Menu::mostrarMenuArchivos() const {
    cout << "\n" << SEPARADOR << "\n";
    cout << "         GESTION DE ARCHIVOS\n";
    cout << SEPARADOR << "\n";
    cout << "  0. Volver al menu principal\n";
    cout << GUIONES << "\n";
    cout << "Seleccione una opcion: ";
}

void Menu::mostrarMenuUsuarios() const {
    cout << "\n" << SEPARADOR << "\n";
    cout << "         GESTION DE USUARIOS\n";
    cout << SEPARADOR << "\n";
    cout << "  0. Volver al menu principal\n";
    cout << GUIONES << "\n";
    cout << "Seleccione una opcion: ";
}

void Menu::mostrarMenuRed() const {
    cout << "\n" << SEPARADOR << "\n";
    cout << "         RED Y ENRUTAMIENTO\n";
    cout << SEPARADOR << "\n";
    cout << "  0. Volver al menu principal\n";
    cout << GUIONES << "\n";
    cout << "Seleccione una opcion: ";
}

void Menu::mostrarMenuAuditoria() const {
    cout << "\n" << SEPARADOR << "\n";
    cout << "              AUDITORIA\n";
    cout << SEPARADOR << "\n";
    cout << "  1. Ver registro de auditoria\n";
    cout << "  0. Volver al menu principal\n";
    cout << GUIONES << "\n";
    cout << "Seleccione una opcion: ";
}


// ---------------------------------------------------------------------
// Submenús de cada área funcional
// ---------------------------------------------------------------------
// En esta fase cada submenú solo reconoce la opción de volver. Cualquier
// otra entrada se comunica como funcionalidad pendiente. Cuando se
// implementen las operaciones reales, aquí se añadirán las entradas
// correspondientes sin tocar el bucle principal.

void Menu::menuGestionArchivos() {
    bool volver = false;
    while (!volver) {
        mostrarMenuArchivos();
        int opcion = leerOpcion();
        switch (opcion) {
            case 0:
                volver = true;
                break;
            default:
                noImplementado("Gestion de archivos");
                break;
        }
    }
}

void Menu::menuGestionUsuarios() {
    bool volver = false;
    while (!volver) {
        mostrarMenuUsuarios();
        int opcion = leerOpcion();
        switch (opcion) {
            case 0:
                volver = true;
                break;
            default:
                noImplementado("Gestion de usuarios");
                break;
        }
    }
}

void Menu::menuRedEnrutamiento() {
    bool volver = false;
    while (!volver) {
        mostrarMenuRed();
        int opcion = leerOpcion();
        switch (opcion) {
            case 0:
                volver = true;
                break;
            default:
                noImplementado("Red y enrutamiento");
                break;
        }
    }
}

void Menu::menuAuditoria() {
    bool volver = false;
    while (!volver) {
        mostrarMenuAuditoria();
        int opcion = leerOpcion();
        switch (opcion) {
            case 1:
                logger.append("MENU", "Consulta del registro de auditoria.");
                logger.mostrar();
                break;
            case 0:
                volver = true;
                break;
            default:
                noImplementado("Auditoria");
                break;
        }
    }
}

// ---------------------------------------------------------------------
// Bucle principal
// ---------------------------------------------------------------------

void Menu::ejecutar() {
    bool salir = false;
    while (!salir) {
        mostrarMenuPrincipal();
        int opcion = leerOpcion();

        switch (opcion) {
            case 1:
                logger.append("MENU", "Acceso: Gestion de archivos.");
                sistema.getAuditoria().registrar("MENU", "Acceso: Gestion de archivos.");
                menuGestionArchivos();
                break;
            case 2:
                logger.append("MENU", "Acceso: Gestion de usuarios.");
                sistema.getAuditoria().registrar("MENU", "Acceso: Gestion de usuarios.");
                menuGestionUsuarios();
                break;
            case 3:
                logger.append("MENU", "Acceso: Red y enrutamiento.");
                sistema.getAuditoria().registrar("MENU", "Acceso: Red y enrutamiento.");
                menuRedEnrutamiento();
                break;
            case 4:
                logger.append("MENU", "Acceso: Auditoria.");
                sistema.getAuditoria().registrar("MENU", "Acceso: Auditoria.");
                menuAuditoria();
                break;
            case 0:
                salir = true;
                cout << "\nCerrando Network OS... Hasta pronto.\n";
                sistema.getAuditoria().registrar("SISTEMA",
                                                 "Network OS finalizado por el usuario.");
                logger.append("SISTEMA", "Cierre del programa solicitado por el usuario.");
                break;
            default:
                cout << "\n[!] Opcion invalida. Ingrese un numero del menu.\n";
                sistema.getAuditoria().registrar("ERROR", "Entrada invalida en el menu principal.");
                logger.append("ERROR", "Entrada invalida en el menu principal: " + to_string(opcion));
                break;
        }
    }
}
