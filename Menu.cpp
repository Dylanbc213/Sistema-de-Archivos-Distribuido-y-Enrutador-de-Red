#include "Menu.h"

#include "Auditoria.h"
#include "NodoArchivo.h"  // Fase 2: obtenerRuta(), getNombre(), esCarpeta()
#include "Servidor.h"     // Fase 2: getId(), getNombre() en listado
#include "TablaHashUsuarios.h"  // Fase 3: buscar()/mostrar() de usuarios
#include "Usuario.h"            // Fase 3: Usuario* y NivelPermiso

#include <cctype>
#include <climits>
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

    // Detecta EOF: cuando no hay más entrada disponible en stdin (p. ej.,
    // entrada redirigida agotada). Evita bucles infinitos devolviendo
    // INT_MIN como sentinela para que los llamadores cierren el programa.
    if (cin.eof() && linea.empty()) {
        return INT_MIN;
    }

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
// Entrada de texto libre
// ---------------------------------------------------------------------

// Lee una línea completa de cin, la recorta de espacios en blanco al
// inicio y al final, y la devuelve. Si la línea está vacía o solo tiene
// espacios, devuelve una cadena vacía.
string Menu::leerLinea() const {
    string linea;
    getline(cin, linea);

    // Recortar espacios y tabulaciones al inicio.
    size_t inicio = linea.find_first_not_of(" \t");
    if (inicio == string::npos) {
        return "";  // Línea vacía o solo espacios.
    }
    // Recortar espacios y tabulaciones al final.
    size_t fin = linea.find_last_not_of(" \t");
    return linea.substr(inicio, fin - inicio + 1);
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
    cout << "  1. Crear servidor\n";
    cout << "  2. Listar servidores\n";
    cout << "  3. Ver arbol de directorios\n";
    cout << "  4. Crear carpeta\n";
    cout << "  5. Crear archivo\n";
    cout << "  6. Buscar por nombre\n";
    cout << "  7. Buscar por ruta\n";
    cout << "  8. Eliminar (cascada)\n";
    cout << "  0. Volver al menu principal\n";
    cout << GUIONES << "\n";
    cout << "Seleccione una opcion: ";
}

void Menu::mostrarMenuUsuarios() const {
    cout << "\n" << SEPARADOR << "\n";
    cout << "         GESTION DE USUARIOS\n";
    cout << SEPARADOR << "\n";
    cout << "  1. Registrar nuevo usuario\n";
    cout << "  2. Iniciar sesion\n";
    cout << "  3. Mostrar usuarios\n";
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
        if (opcion == INT_MIN) {
            volver = true;
            break;
        }
        switch (opcion) {
            case 0:
                volver = true;
                break;

            case 1: {  // Crear servidor
                logger.append("MENU", "Opcion: Crear servidor.");
                cout << "\n  ID del servidor: ";
                int id = leerOpcion();
                if (id < 0) {
                    cout << "  [!] ID invalido.\n";
                    break;
                }
                cout << "  Nombre del servidor: ";
                string nombre = leerLinea();
                if (nombre.empty()) {
                    cout << "  [!] El nombre no puede estar vacio.\n";
                    break;
                }
                Servidor* srv = sistema.crearServidor(id, nombre);
                if (srv) {
                    cout << "  [+] Servidor #" << id << " \""
                         << nombre << "\" creado.\n";
                }
                break;
            }

                        case 2: {  // Listar servidores
                logger.append("MENU", "Opcion: Listar servidores.");
                sistema.listarServidores();
                break;
            }

            case 3: {  // Ver arbol de directorios
                logger.append("MENU", "Opcion: Ver arbol de directorios.");
                cout << "\n  ID del servidor: ";
                int id = leerOpcion();
                if (sistema.getServidor(id)) {
                    cout << "\n";
                    sistema.mostrarArbol(id);
                } else {
                    cout << "  [!] Servidor #" << id << " no existe.\n";
                }
                break;
                        }

            case 4: {  // Crear carpeta
                logger.append("MENU", "Opcion: Crear carpeta.");
                cout << "\n  ID del servidor: ";
                int id = leerOpcion();
                cout << "  Ruta (ej: /docs/reportes): ";
                string ruta = leerLinea();
                if (ruta.empty()) {
                    cout << "  [!] La ruta no puede estar vacia.\n";
                    break;
                }
                if (sistema.crearSubdirectorio(id, ruta)) {
                    cout << "  [+] Carpeta creada: " << ruta << "\n";
                } else {
                    cout << "  [!] No se pudo crear la carpeta.\n";
                }
                break;
            }

            case 5: {  // Crear archivo
                logger.append("MENU", "Opcion: Crear archivo.");
                cout << "\n  ID del servidor: ";
                int id = leerOpcion();
                cout << "  Ruta (ej: /docs/nota.txt): ";
                string ruta = leerLinea();
                cout << "  Contenido (Enter para vacio): ";
                string contenido = leerLinea();
                if (sistema.crearArchivo(id, ruta, contenido)) {
                    cout << "  [+] Archivo creado: " << ruta << "\n";
                } else {
                    cout << "  [!] No se pudo crear el archivo.\n";
                }
                break;
            }

            case 6: {  // Buscar por nombre (DFS)
                logger.append("MENU", "Opcion: Buscar por nombre.");
                cout << "\n  ID del servidor: ";
                int id = leerOpcion();
                cout << "  Nombre: ";
                string nombre = leerLinea();
                if (nombre.empty()) {
                    cout << "  [!] El nombre no puede estar vacio.\n";
                    break;
                }
                auto result = sistema.buscarRecursivoPorNombre(id, nombre);
                if (result) {
                    cout << "  [+] Encontrado: " << result->obtenerRuta()
                         << " (" << (result->esCarpeta()
                                         ? "DIR" : "FILE") << ")\n";
                } else {
                    cout << "  [-] No se encontro \"" << nombre << "\".\n";
                }
                break;
            }

            case 7: {  // Buscar por ruta
                logger.append("MENU", "Opcion: Buscar por ruta.");
                cout << "\n  ID del servidor: ";
                int id = leerOpcion();
                cout << "  Ruta (ej: /docs): ";
                string ruta = leerLinea();
                if (ruta.empty()) {
                    cout << "  [!] La ruta no puede estar vacia.\n";
                    break;
                }
                auto result = sistema.buscarPorRuta(id, ruta);
                if (result) {
                    cout << "  [+] Encontrado: " << result->getNombre()
                         << " (" << (result->esCarpeta() ? "DIR" : "FILE")
                         << ", " << result->obtenerTamano() << " bytes)\n";
                } else {
                    cout << "  [-] No se encontro \"" << ruta << "\".\n";
                }
                break;
            }

            case 8: {  // Eliminar (cascada)
                logger.append("MENU", "Opcion: Eliminar (cascada).");
                cout << "\n  ID del servidor: ";
                int id = leerOpcion();
                cout << "  Ruta a eliminar (ej: /docs/reportes): ";
                string ruta = leerLinea();
                if (ruta.empty() || ruta == "/") {
                    cout << "  [!] No se puede eliminar la raiz.\n";
                    break;
                }
                if (sistema.eliminarRecursivo(id, ruta)) {
                    cout << "  [-] Elemento eliminado: " << ruta << "\n";
                } else {
                    cout << "  [!] No se pudo eliminar.\n";
                }
                break;
            }

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
        if (opcion == INT_MIN) {
            volver = true;
            break;
        }
        switch (opcion) {
            case 0:
                volver = true;
                break;

            case 1: {  // Registrar nuevo usuario
                logger.append("MENU", "Opcion: Registrar usuario.");
                cout << "\n  ID del usuario: ";
                int id = leerOpcion();
                if (id < 0) {
                    cout << "  [!] El ID no puede ser negativo.\n";
                    break;
                }
                cout << "  Nombre de usuario: ";
                string nombre = leerLinea();
                if (nombre.empty()) {
                    cout << "  [!] El nombre no puede estar vacio.\n";
                    break;
                }
                cout << "  Contrasena: ";
                string contra = leerLinea();
                if (contra.empty()) {
                    cout << "  [!] La contrasena no puede estar vacia.\n";
                    break;
                }
                cout << "  Nivel de permiso (0=Normal, 1=Admin) [0]: ";
                int permIn = leerOpcion();
                int permiso = (permIn == 1) ? 1 : 0;
                cout << "  ID del servidor asignado (Enter para omitir): ";
                int srvId = leerOpcion();
                if (srvId >= 0 && sistema.getServidor(srvId) == nullptr) {
                    cout << "  [!] El servidor #" << srvId
                         << " no existe; se omite la asignacion.\n";
                    srvId = -1;
                }
                if (sistema.registrarUsuario(id, nombre, contra, permiso)) {
                    cout << "  [+] Usuario \"" << nombre << "\" registrado.\n";
                    // Asignacion opcional a un servidor existente.
                    if (srvId >= 0) {
                        Usuario* u = sistema.getUsuarios().buscar(nombre);
                        if (u != nullptr) {
                            u->setServidorAsignado(srvId);
                            sistema.getAuditoria().registrar(
                                "USUARIO", "Usuario \"" + nombre
                                + "\" asignado a servidor #"
                                + to_string(srvId) + ".");
                            logger.append("USUARIO", "Usuario \"" + nombre
                                + "\" asignado a servidor #"
                                + to_string(srvId) + ".");
                        }
                    }
                } else {
                    cout << "  [!] No se pudo registrar el usuario "
                            "(nombre duplicado).\n";
                }
                break;
            }

            case 2: {  // Iniciar sesion
                logger.append("MENU", "Opcion: Iniciar sesion.");
                cout << "\n  Nombre de usuario: ";
                string nombre = leerLinea();
                if (nombre.empty()) {
                    cout << "  [!] El nombre no puede estar vacio.\n";
                    break;
                }
                cout << "  Contrasena: ";
                string contra = leerLinea();
                if (contra.empty()) {
                    cout << "  [!] La contrasena no puede estar vacia.\n";
                    break;
                }
                if (sistema.iniciarSesion(nombre, contra)) {
                    cout << "  [+] Bienvenido, \"" << nombre
                         << "\". Sesion iniciada.\n";
                } else {
                    cout << "  [!] Credenciales invalidas o usuario "
                            "inexistente.\n";
                }
                break;
            }

            case 3: {  // Mostrar usuarios
                logger.append("MENU", "Opcion: Mostrar usuarios.");
                sistema.getUsuarios().mostrar();
                break;
            }

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
        if (opcion == INT_MIN) {
            volver = true;
            break;
        }
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
        if (opcion == INT_MIN) {
            volver = true;
            break;
        }
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

        if (opcion == INT_MIN) {
            salir = true;
            cout << "\nEntrada agotada. Cerrando Network OS...\n";
            logger.append("SISTEMA", "Cierre: entrada EOF detectada.");
            break;
        }

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
