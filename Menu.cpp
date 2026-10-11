#include "Menu.h"

#include "Auditoria.h"
#include "NodoArchivo.h"  // Fase 2: obtenerRuta(), getNombre(), esCarpeta()
#include "Servidor.h"     // Fase 2: getId(), getNombre() en listado
#include "TablaHashUsuarios.h"  // Fase 3: buscar()/mostrar() de usuarios
#include "Usuario.h"            // Fase 3: Usuario* y NivelPermiso

#include <cctype>
#include <climits>
#include <algorithm>
#include <iostream>
#include <string>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

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

Menu::Menu(Sistema& s, LoggerAuditoria& log)
    : sistema(s), logger(log) {}

bool Menu::autorizarAcceso(const string& recurso, bool requiereAdmin) {
    if (!sistema.tieneSesion()) {
        const string mensaje = "Acceso denegado a " + recurso + ": no hay una sesion iniciada.";
        cout << "\n[!] Inicie sesion para acceder a " << recurso << ".\n";
        sistema.getAuditoria().registrar("ACCESS_DENIED", mensaje);
        logger.append("ACCESS_DENIED", mensaje);
        return false;
    }
    if (requiereAdmin && !sistema.sesionEsAdmin()) {
        const string mensaje = "Acceso denegado a " + recurso + ": se requiere rol ADMIN.";
        cout << "\n[!] Se requiere una sesion ADMIN para acceder a " << recurso << ".\n";
        sistema.getAuditoria().registrar("ACCESS_DENIED", mensaje);
        logger.append("ACCESS_DENIED", mensaje);
        return false;
    }
    return true;
}

void Menu::cerrarSesion() {
    if (!sistema.tieneSesion()) {
        cout << "\n[i] No hay una sesion iniciada.\n";
        return;
    }
    const string nombre = sistema.nombreUsuarioEnSesion();
    sistema.cerrarSesion();
    cout << "\nSesion cerrada para \"" << nombre << "\".\n";
}

bool Menu::iniciarSesionInteractiva() {
    if (sistema.tieneSesion()) {
        cout << "\n[i] Ya hay una sesion activa para \"" << sistema.nombreUsuarioEnSesion()
             << "\". Cierrela antes de iniciar otra.\n";
        return false;
    }

    logger.append("MENU", "Opcion: Iniciar sesion.");
    cout << "\n  Nombre de usuario: ";
    const string nombre = leerLinea();
    if (nombre.empty()) {
        cout << "  [!] El nombre no puede estar vacio.\n";
        return false;
    }
    cout << "  Contrasena: ";
    const string contra = leerContrasena();
    if (contra.empty()) {
        cout << "  [!] La contrasena no puede estar vacia.\n";
        return false;
    }

    if (sistema.iniciarSesion(nombre, contra)) {
        cout << "  [+] Bienvenido, \"" << nombre << "\" ("
             << sistema.rolUsuarioEnSesion() << "). Sesion iniciada.\n";
        return true;
    }

    if (sistema.getUsuarios().buscar(nombre) == nullptr) {
        cout << "  [!] El usuario \"" << nombre << "\" no existe.\n";
    } else {
        cout << "  [!] Contrasena incorrecta para el usuario \""
             << nombre << "\".\n";
    }
    return false;
}

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

string Menu::leerContrasena() const {
    string linea;
    HANDLE entrada = GetStdHandle(STD_INPUT_HANDLE);
    DWORD modoOriginal = 0;
    const bool consola = GetConsoleMode(entrada, &modoOriginal) != 0;
    if (consola) SetConsoleMode(entrada, modoOriginal & ~ENABLE_ECHO_INPUT);
    getline(cin, linea);
    if (consola) {
        SetConsoleMode(entrada, modoOriginal);
        cout << "\n";
    }
    const size_t inicio = linea.find_first_not_of(" \t");
    if (inicio == string::npos) return "";
    const size_t fin = linea.find_last_not_of(" \t");
    return linea.substr(inicio, fin - inicio + 1);
}

// ---------------------------------------------------------------------
// Opción inválida en un submenú
// ---------------------------------------------------------------------

void Menu::opcionInvalida(const string& submenu) {
    cout << "\n[!] Opcion invalida en el submenu \"" << submenu << "\".\n";
    sistema.getAuditoria().registrar(
        "ERROR", "Opcion invalida en el submenu " + submenu + ".");
    logger.append("ERROR", "Opcion invalida en el submenu " + submenu + ".");
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
    if (!sistema.tieneSesion()) {
        cout << "  5. Iniciar sesion\n";
        cout << "  Sesion: no iniciada\n";
    } else {
        cout << "  5. Cerrar sesion (" << sistema.nombreUsuarioEnSesion() << ")\n";
        cout << "  Sesion activa: " << sistema.nombreUsuarioEnSesion()
             << " (" << sistema.rolUsuarioEnSesion() << ")\n";
    }
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
    cout << "  9. Modificar contenido de archivo\n";
    cout << "  0. Volver al menu principal\n";
    cout << GUIONES << "\n";
    cout << "Seleccione una opcion: ";
}

void Menu::mostrarMenuUsuarios() const {
    cout << "\n" << SEPARADOR << "\n";
    cout << "         GESTION DE USUARIOS\n";
    cout << SEPARADOR << "\n";
    cout << "  1. Registrar nuevo usuario\n";
    cout << "  2. Mostrar usuarios\n";
    cout << "  3. Eliminar usuario\n";
    cout << "  0. Volver al menu principal\n";
    cout << GUIONES << "\n";
    cout << "Seleccione una opcion: ";
}

void Menu::mostrarMenuRed() const {
    cout << "\n" << SEPARADOR << "\n";
    cout << "         RED Y ENRUTAMIENTO\n";
    cout << SEPARADOR << "\n";
    cout << "  1. Recorrido BFS desde un servidor\n";
    cout << "  2. Recorrido DFS desde un servidor\n";
    cout << "  3. Diagnosticar conectividad y enlaces\n";
    cout << "  4. Simular ping general\n";
    cout << "  5. Mostrar grafo de red\n";
    cout << "  6. Agregar conexion dirigida\n";
    cout << "  7. Eliminar conexion dirigida\n";
    cout << "  8. Calcular ruta optima (Dijkstra)\n";
    cout << "  9. Eliminar servidor de la red\n";
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
// Cada submenu ejecuta las operaciones disponibles para su area.

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

            case 9: {
                logger.append("MENU", "Opcion: Modificar contenido de archivo.");
                cout << "\n  ID del servidor: ";
                int id = leerOpcion();
                cout << "  Ruta del archivo: ";
                string ruta = leerLinea();
                cout << "  Nuevo contenido: ";
                string contenido = leerLinea();
                if (sistema.modificarContenidoArchivo(id, ruta, contenido))
                    cout << "  [+] Contenido actualizado y guardado.\n";
                else
                    cout << "  [!] No se pudo modificar el archivo; revise el servidor, la ruta y el almacenamiento.\n";
                break;
            }

            default:
                opcionInvalida("Gestion de archivos");
                break;
        }
    }
}

void Menu::menuGestionUsuarios() {
    if (!autorizarAcceso("gestion de usuarios", true)) return;
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
                if (!autorizarAcceso("registro de usuarios", true)) break;
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
                string contra = leerContrasena();
                if (contra.empty()) {
                    cout << "  [!] La contrasena no puede estar vacia.\n";
                    break;
                }
                cout << "  Nivel de permiso (0=Normal, 1=Admin) [0]: ";
                int permIn = leerOpcion();
                if (permIn != 0 && permIn != 1) {
                    const string mensaje = "Registro rechazado: nivel de permiso invalido.";
                    cout << "  [!] El permiso debe ser 0 (Normal) o 1 (Admin).\n";
                    sistema.getAuditoria().registrar("ERROR", mensaje);
                    logger.append("ERROR", mensaje);
                    break;
                }
                int permiso = permIn;
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
                    if (srvId >= 0) sistema.asignarServidorUsuario(nombre, srvId);
                } else {
                    cout << "  [!] No se pudo registrar el usuario "
                            "(ID/nombre duplicado o datos invalidos).\n";
                }
                break;
            }

            case 2: {  // Mostrar usuarios
                if (!autorizarAcceso("la lista de usuarios", true)) break;
                logger.append("MENU", "Opcion: Mostrar usuarios.");
                sistema.getUsuarios().mostrar();
                break;
            }
            case 3: {
                if (!autorizarAcceso("eliminacion de usuarios", true)) break;
                cout << "\n  Nombre de usuario a eliminar: ";
                const string nombre = leerLinea();
                if (sistema.eliminarUsuario(nombre))
                    cout << "  [-] Usuario eliminado.\n";
                else
                    cout << "  [!] No se pudo eliminar el usuario; revise el nombre y que no sea el ultimo ADMIN.\n";
                break;
            }

            default:
                opcionInvalida("Gestion de usuarios");
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
            case 1:
            case 2: {
                const string tipo = opcion == 1 ? "BFS" : "DFS";
                cout << "\n  ID del servidor inicial: ";
                int inicio = leerOpcion();
                const GrafoRed& grafo = sistema.getGrafo();
                vector<int> recorrido = opcion == 1
                    ? grafo.recorridoBFS(inicio) : grafo.recorridoDFS(inicio);
                if (recorrido.empty()) {
                    cout << "  [!] El servidor #" << inicio << " no existe en el grafo.\n";
                    sistema.getAuditoria().registrar("ERROR", "Intento de recorrido " + tipo +
                        " desde servidor inexistente #" + to_string(inicio) + ".");
                    logger.append("ERROR", "Intento de recorrido " + tipo +
                        " desde servidor inexistente #" + to_string(inicio) + ".");
                    break;
                }
                cout << "  Recorrido " << tipo << ": ";
                for (size_t i = 0; i < recorrido.size(); ++i) {
                    if (i) cout << " -> ";
                    cout << "#" << recorrido[i] << " " << grafo.nombreDe(recorrido[i]);
                }
                cout << "\n";
                const string mensaje = "Recorrido " + tipo + " iniciado en servidor #" +
                    to_string(inicio) + "; se visitaron " + to_string(recorrido.size()) + " servidores.";
                sistema.getAuditoria().registrar("DIAGNOSTICO", mensaje);
                logger.append("DIAGNOSTICO", mensaje);
                break;
            }
            case 3: {
                const GrafoRed& grafo = sistema.getGrafo();
                vector<int> ids = grafo.idsServidores();
                vector<int> aislados = grafo.servidoresAislados();
                vector<string> problemas = grafo.validarConexiones();
                cout << "\n  Diagnostico de red (" << ids.size() << " servidores):\n";
                if (ids.empty()) cout << "  No hay servidores registrados en el grafo.\n";
                cout << "  Servidores aislados: ";
                if (aislados.empty()) cout << "ninguno\n";
                else {
                    for (int id : aislados) cout << "#" << id << " " << grafo.nombreDe(id) << "  ";
                    cout << "\n";
                }
                size_t rutasFaltantes = 0;
                for (int origen : ids) {
                    vector<int> alcanzables = grafo.recorridoBFS(origen);
                    for (int destino : ids) {
                        if (origen != destino &&
                            find(alcanzables.begin(), alcanzables.end(), destino) == alcanzables.end()) {
                            cout << "  Sin ruta dirigida: #" << origen << " -> #" << destino << "\n";
                            ++rutasFaltantes;
                        }
                    }
                }
                if (!rutasFaltantes) cout << "  Todos los servidores son alcanzables entre si en direccion de los enlaces.\n";
                if (problemas.empty()) cout << "  Enlaces invalidos: ninguno\n";
                else {
                    cout << "  Enlaces invalidos:\n";
                    for (const string& problema : problemas) cout << "    - " << problema << "\n";
                }
                const string mensaje = "Diagnostico de conectividad: " + to_string(aislados.size()) +
                    " servidores aislados, " + to_string(rutasFaltantes) + " rutas dirigidas inexistentes y " +
                    to_string(problemas.size()) + " problemas de enlaces.";
                sistema.getAuditoria().registrar("DIAGNOSTICO", mensaje);
                logger.append("DIAGNOSTICO", mensaje);
                break;
            }
            case 4: {
                const GrafoRed& grafo = sistema.getGrafo();
                vector<int> ids = grafo.idsServidores();
                size_t exitos = 0, fallidos = 0;
                cout << "\n  Simulacion de ping entre todos los pares dirigidos:\n";
                if (ids.size() < 2) cout << "  Se requieren al menos dos servidores.\n";
                for (int origen : ids) {
                    for (int destino : ids) {
                        if (origen == destino) continue;
                        RutaOptima ruta = grafo.dijkstra(origen, destino);
                        string mensaje;
                        if (ruta.existeCamino) {
                            mensaje = "PING OK #" + to_string(origen) + " -> #" + to_string(destino) +
                                " (latencia " + to_string(ruta.latenciaTotal) + " ms).";
                            cout << "  " << mensaje << "\n";
                            ++exitos;
                        } else {
                            mensaje = "PING FALLIDO #" + to_string(origen) + " -> #" + to_string(destino) +
                                " (sin ruta).";
                            cout << "  " << mensaje << "\n";
                            ++fallidos;
                        }
                        sistema.getAuditoria().registrar("PING", mensaje);
                        logger.append("PING", mensaje);
                    }
                }
                const string resumen = "Ping general finalizado: " + to_string(exitos) +
                    " exitosos y " + to_string(fallidos) + " fallidos.";
                sistema.getAuditoria().registrar("DIAGNOSTICO", resumen);
                logger.append("DIAGNOSTICO", resumen);
                break;
            }
            case 5:
                sistema.mostrarGrafo();
                sistema.getAuditoria().registrar("MENU", "Consulta del grafo de red.");
                logger.append("MENU", "Consulta del grafo de red.");
                break;
            case 6: {
                cout << "\n  ID del servidor origen: ";
                int origen = leerOpcion();
                cout << "  ID del servidor destino: ";
                int destino = leerOpcion();
                cout << "  Latencia en milisegundos: ";
                int latencia = leerOpcion();
                if (origen < 0 || destino < 0 || latencia < 0) {
                    const string mensaje = "Intento de conexion desde el menu con valores invalidos.";
                    cout << "  [!] Los IDs y la latencia deben ser enteros no negativos.\n";
                    sistema.getAuditoria().registrar("ERROR", mensaje);
                    logger.append("ERROR", mensaje);
                } else if (sistema.agregarConexionRed(origen, destino, latencia)) {
                    cout << "  [+] Conexion dirigida agregada.\n";
                } else {
                    cout << "  [!] No se agrego la conexion; revise los servidores y si el enlace ya existe.\n";
                }
                break;
            }
            case 7: {
                cout << "\n  ID del servidor origen: ";
                int origen = leerOpcion();
                cout << "  ID del servidor destino: ";
                int destino = leerOpcion();
                if (origen < 0 || destino < 0) {
                    const string mensaje = "Intento de eliminar conexion desde el menu con IDs invalidos.";
                    cout << "  [!] Los IDs deben ser enteros no negativos.\n";
                    sistema.getAuditoria().registrar("ERROR", mensaje);
                    logger.append("ERROR", mensaje);
                } else if (sistema.eliminarConexion(origen, destino)) {
                    cout << "  [-] Conexion eliminada.\n";
                } else {
                    cout << "  [!] No existe esa conexion.\n";
                }
                break;
            }
            case 8: {
                cout << "\n  ID del servidor origen: ";
                int origen = leerOpcion();
                cout << "  ID del servidor destino: ";
                int destino = leerOpcion();
                if (origen < 0 || destino < 0) {
                    const string mensaje = "Intento de calcular ruta desde el menu con IDs invalidos.";
                    cout << "  [!] Los IDs deben ser enteros no negativos.\n";
                    sistema.getAuditoria().registrar("ERROR", mensaje);
                    logger.append("ERROR", mensaje);
                } else {
                    cout << "  " << sistema.rutearDijkstra(origen, destino) << "\n";
                }
                break;
            }
            case 9: {
                cout << "\n  ID del servidor de red a eliminar: ";
                int id = leerOpcion();
                if (id < 0) {
                    const string mensaje = "Intento de eliminar servidor de red con ID invalido.";
                    cout << "  [!] El ID debe ser un entero no negativo.\n";
                    sistema.getAuditoria().registrar("ERROR", mensaje);
                    logger.append("ERROR", mensaje);
                } else if (sistema.eliminarServidor(id)) {
                    cout << "  [-] Servidor eliminado de la topologia de red; sus enlaces fueron retirados.\n";
                } else {
                    cout << "  [!] El servidor no existe en la topologia de red.\n";
                }
                break;
            }
            default:
                opcionInvalida("Red y enrutamiento");
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
                opcionInvalida("Auditoria");
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
                if (!autorizarAcceso("gestion de archivos")) break;
                logger.append("MENU", "Acceso: Gestion de archivos.");
                sistema.getAuditoria().registrar("MENU", "Acceso: Gestion de archivos.");
                menuGestionArchivos();
                break;
            case 2:
                if (!autorizarAcceso("gestion de usuarios", true)) break;
                logger.append("MENU", "Acceso: Gestion de usuarios.");
                sistema.getAuditoria().registrar("MENU", "Acceso: Gestion de usuarios.");
                menuGestionUsuarios();
                break;
            case 3:
                if (!autorizarAcceso("red y enrutamiento")) break;
                logger.append("MENU", "Acceso: Red y enrutamiento.");
                sistema.getAuditoria().registrar("MENU", "Acceso: Red y enrutamiento.");
                menuRedEnrutamiento();
                break;
            case 4:
                if (!autorizarAcceso("auditoria")) break;
                logger.append("MENU", "Acceso: Auditoria.");
                sistema.getAuditoria().registrar("MENU", "Acceso: Auditoria.");
                menuAuditoria();
                break;
            case 5:
                if (!sistema.tieneSesion()) {
                    iniciarSesionInteractiva();
                } else {
                    cerrarSesion();
                }
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
