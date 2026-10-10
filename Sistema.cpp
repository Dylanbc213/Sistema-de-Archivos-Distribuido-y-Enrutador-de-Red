#include "Sistema.h"

#include "Archivo.h"      // Fase 2: tipo Archivo en crearArchivo
#include "Auditoria.h"
#include "LoggerAuditoria.h"   // Fase 3: auditoría persistente a disco
#include "GrafoRed.h"
#include "NodoArchivo.h"  // Fase 2: tipo NodoArchivo en búsquedas
#include "Servidor.h"
#include "TablaHashUsuarios.h"
#include "Usuario.h"

#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------

// Crea las cuatro estructuras y deja constancia del arranque en auditoría.
// Si se inyecta un logger, también se appendea al archivo persistente.
Sistema::Sistema(size_t bucketsUsuarios, const string& rutaAuditoria,
                 LoggerAuditoria* logger)
    : usuarios(make_unique<TablaHashUsuarios>(bucketsUsuarios)),
      grafo(make_unique<GrafoRed>()),
      auditoria(make_unique<Auditoria>(rutaAuditoria)),
      loggerAuditoria(logger) {
    auditar("SISTEMA", "Network OS iniciado.");
}

// ---------------------------------------------------------------------
// Auditoría transaccional
// ---------------------------------------------------------------------

// Helper: escribe la entrada en la traza volátil (Auditoria) y, si existe el
// logger, la appendea de inmediato a network_audit_log.txt.
void Sistema::auditar(const string& tipo, const string& mensaje) {
    auditoria->registrar(tipo, mensaje);
    if (loggerAuditoria != nullptr) {
        loggerAuditoria->append(tipo, mensaje);
    }
}

// Asocia dinámicamente un logger persistente (alternativa al constructor).
void Sistema::asignarLogger(LoggerAuditoria& log) {
    loggerAuditoria = &log;
}

// ---------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------

// Se define aquí (con los tipos ya completos) para que los destructores de
// los unique_ptr miembros se instancien correctamente. Al ser '= default'
// no requiere código adicional.
Sistema::~Sistema() = default;

// ---------------------------------------------------------------------
// Árbol de directorios
// ---------------------------------------------------------------------

// Busca un servidor por ID dentro del vector de unique_ptr.
Servidor* Sistema::buscarServidorPorId(int id) const {
    for (const auto& servidor : servidores) {
        if (servidor && servidor->getId() == id) {
            return servidor.get();
        }
    }
    return nullptr;
}

// Crea un servidor (con su carpeta raíz), lo registra como nodo del grafo
// y deja traza en auditoría. Rechaza IDs negativos o repetidos.
Servidor* Sistema::crearServidor(int id, const string& nombre) {
    if (id < 0) {
        auditoria->registrar("ERROR", "Intento de crear servidor con ID negativo.");
        return nullptr;
    }
    if (buscarServidorPorId(id) != nullptr) {
        auditoria->registrar("ERROR",
                             "Ya existe el servidor #" + to_string(id) + ".");
        return nullptr;
    }

    servidores.push_back(make_unique<Servidor>(id, nombre));
    grafo->agregarServidor(id, nombre);
    auditoria->registrar("ARBOL", "Servidor #" + to_string(id) +
                                       " \"" + nombre + "\" creado con su raíz.");

    return servidores.back().get();
}

// Cantidad de servidores registrados.
size_t Sistema::cantidadServidores() const { return servidores.size(); }

// Obtiene un puntero no propietario a un servidor por su ID.
Servidor* Sistema::getServidor(int id) const {
    return buscarServidorPorId(id);
}

// Imprime una lista de todos los servidores registrados.
void Sistema::listarServidores() const {
    if (servidores.empty()) {
        cout << "\n  (No hay servidores registrados.)\n";
        return;
    }
    cout << "\n  Servidores (" << servidores.size() << "):\n";
    for (const auto& srv : servidores) {
        if (srv) {
            cout << "    - #" << srv->getId() << " \""
                 << srv->getNombre() << "\"\n";
        }
    }
}

// ---------------------------------------------------------------------
// Fase 2: Operaciones de sistema de archivos sobre un servidor
// ---------------------------------------------------------------------
// Cada método localiza el servidor por ID y delega en su carpeta raíz.
// Si el servidor no existe, registra un error en auditoría y devuelve
// nullptr/false según corresponda.

// Búsqueda por ruta desde la raíz del servidor.
shared_ptr<NodoArchivo> Sistema::buscarPorRuta(int servidorId,
                                               const string& ruta) const {
    Servidor* srv = buscarServidorPorId(servidorId);
    if (!srv) {
        auditoria->registrar("ERROR",
            "Busqueda por ruta: servidor #" + to_string(servidorId)
            + " no existe.");
        return nullptr;
    }
    auto resultado = srv->buscarPorRuta(ruta);
    auditoria->registrar("ARBOL", "Busqueda por ruta '" + ruta
        + "' en servidor #" + to_string(servidorId) + ".");
    return resultado;
}

// Búsqueda recursiva (DFS) por nombre en todo el árbol del servidor.
shared_ptr<NodoArchivo> Sistema::buscarRecursivoPorNombre(
    int servidorId, const string& nombre) const {
    Servidor* srv = buscarServidorPorId(servidorId);
    if (!srv) {
        auditoria->registrar("ERROR",
            "Busqueda recursiva: servidor #" + to_string(servidorId)
            + " no existe.");
        return nullptr;
    }
    auto resultado = srv->buscarRecursivoPorNombre(nombre);
    auditoria->registrar("ARBOL", "Busqueda recursiva '" + nombre
        + "' en servidor #" + to_string(servidorId) + ".");
    return resultado;
}

// Crea una subcarpeta (y directorios intermedios) en un servidor.
shared_ptr<Carpeta> Sistema::crearSubdirectorio(int servidorId,
                                                 const string& ruta) {
    Servidor* srv = buscarServidorPorId(servidorId);
    if (!srv) {
        auditoria->registrar("ERROR",
            "Crear subdirectorio: servidor #" + to_string(servidorId)
            + " no existe.");
        return nullptr;
    }
    auto resultado = srv->crearSubdirectorio(ruta);
    if (resultado) {
        auditoria->registrar("ARBOL", "Carpeta creada: '" + ruta
            + "' en servidor #" + to_string(servidorId) + ".");
    }
    return resultado;
}

// Crea un archivo (y directorios intermedios) en un servidor.
shared_ptr<Archivo> Sistema::crearArchivo(int servidorId,
                                           const string& ruta,
                                           const string& contenido) {
    Servidor* srv = buscarServidorPorId(servidorId);
    if (!srv) {
        auditoria->registrar("ERROR",
            "Crear archivo: servidor #" + to_string(servidorId)
            + " no existe.");
        return nullptr;
    }
    auto resultado = srv->crearArchivo(ruta, contenido);
    if (resultado) {
        auditoria->registrar("ARBOL", "Archivo creado: '" + ruta
            + "' en servidor #" + to_string(servidorId) + ".");
    }
    return resultado;
}

// Elimina recursivamente un nodo por ruta (cascada si es carpeta).
bool Sistema::eliminarRecursivo(int servidorId, const string& ruta) {
    Servidor* srv = buscarServidorPorId(servidorId);
    if (!srv) {
        auditoria->registrar("ERROR",
            "Eliminar: servidor #" + to_string(servidorId)
            + " no existe.");
        return false;
    }
    bool ok = srv->eliminarRecursivo(ruta);
    if (ok) {
        auditoria->registrar("ARBOL", "Elemento eliminado: '" + ruta
            + "' en servidor #" + to_string(servidorId) + ".");
    }
    return ok;
}

// Muestra el árbol de directorios de un servidor.
void Sistema::mostrarArbol(int servidorId) const {
    const Servidor* srv = buscarServidorPorId(servidorId);
    if (!srv) {
        cout << "Servidor #" << servidorId << " no existe.\n";
        return;
    }
    srv->mostrarArbol();
}

// ---------------------------------------------------------------------
// Tabla hash de usuarios
// ---------------------------------------------------------------------

// Inserta un usuario en la tabla hash y audita la operación.
// 'nivelPermiso' 0=Normal, 1=Admin.
bool Sistema::registrarUsuario(int id, const string& nombre,
                               const string& contrasena,
                               int nivelPermiso) {
    NivelPermiso permiso = (nivelPermiso == 0)
                               ? NivelPermiso::NORMAL
                               : NivelPermiso::ADMIN;
    const string nivelStr = (permiso == NivelPermiso::ADMIN) ? "ADMIN" : "NORMAL";
    bool ok = usuarios->insertar(
        Usuario(id, nombre, contrasena, permiso));
    if (ok) {
        auditar("USUARIO",
            "Usuario \"" + nombre + "\" registrado (id " + to_string(id)
            + ", permiso " + nivelStr + ").");
    } else {
        auditar("ERROR",
            "Registro fallido: el nombre \"" + nombre
            + "\" ya esta en uso.");
    }
    return ok;
}

// ---------------------------------------------------------------------
// Autenticación (Fase 3)
// ---------------------------------------------------------------------

// La tabla hash localiza al usuario en O(1) promedio (verificarSesion) y
// compara la contraseña. Cada intento (éxito o fracaso) se audita en la
// traza volátil y en network_audit_log.txt de forma automática.
bool Sistema::iniciarSesion(const string& nombre, const string& contrasena) {
    Usuario* u = usuarios->verificarSesion(nombre, contrasena);
    if (u != nullptr) {
        auditar("AUT",
            "Inicio de sesion EXITOSO: \"" + nombre
            + "\" (permiso " + u->getPermisoStr() + ").");
        return true;
    }
    auditar("AUT",
        "Inicio de sesion FALLIDO para: \"" + nombre + "\".");
    return false;
}

// ---------------------------------------------------------------------
// Grafo de red
// ---------------------------------------------------------------------

// Conecta dos servidores existentes con la latencia indicada.
bool Sistema::conectarServidores(int origenId, int destinoId, int latencia) {
    bool ok = grafo->agregarArista(origenId, destinoId, latencia);
    if (ok) {
        auditoria->registrar("RED", "Conexión #" + to_string(origenId) + " -> #" +
                                        to_string(destinoId) + " (" +
                                        to_string(latencia) + " ms).");
    }
    return ok;
}

// ---------------------------------------------------------------------
// Acceso a las estructuras
// ---------------------------------------------------------------------

TablaHashUsuarios& Sistema::getUsuarios() { return *usuarios; }

GrafoRed& Sistema::getGrafo() { return *grafo; }

Auditoria& Sistema::getAuditoria() { return *auditoria; }

// ---------------------------------------------------------------------
// Presentación
// ---------------------------------------------------------------------

// Muestra un resumen del estado de cada estructura.
void Sistema::mostrarResumen() const {
    cout << "===== Resumen del Network OS =====\n";
    cout << "Servidores (árboles de directorios): " << servidores.size() << "\n";
    for (const auto& servidor : servidores) {
        if (servidor) {
            cout << "  - #" << servidor->getId() << " \"" << servidor->getNombre()
                 << "\" (raíz: \"" << servidor->getRaiz()->getNombre() << "\")\n";
        }
    }

    cout << "Usuarios en tabla hash: " << usuarios->tamano() << "\n";
    usuarios->mostrar();

    cout << "\n";
    grafo->mostrar();

    cout << "\nEventos de auditoría: " << auditoria->cantidadEventos() << "\n";
    cout << "=================================\n";
}

// ---------------------------------------------------------------------
// Grafo de red (Fase 4: enrutamiento inteligente)
// ---------------------------------------------------------------------

// Registra un servidor existente como nodo del grafo de red.
bool Sistema::agregarServidorRed(int id, const string& nombre) {
    if (!buscarServidorPorId(id)) {
        auditoria->registrar("ERROR",
            "agregarServidorRed: servidor #" + to_string(id)
            + " no existe en el sistema.");
        return false;
    }
    bool ok = grafo->agregarServidor(id, nombre);
    if (ok) {
        auditar("RED", "Servidor #" + to_string(id)
            + " \"" + nombre + "\" registrado en el grafo de red.");
    }
    return ok;
}

// Añade un enlace dirigido origen -> destino con la latencia dada.
bool Sistema::agregarConexionRed(int origenId, int destinoId, int latencia) {
    bool ok = grafo->agregarArista(origenId, destinoId, latencia);
    if (ok) {
        auditar("RED", "Conexión #" + to_string(origenId) + " -> #"
            + to_string(destinoId) + " (" + to_string(latencia) + " ms) agregada.");
    }
    return ok;
}

// Elimina un servidor del grafo de red (y todas las aristas que referencian).
bool Sistema::eliminarServidor(int id) {
    bool ok = grafo->eliminarServidor(id);
    if (ok) {
        auditar("RED", "Servidor #" + to_string(id) + " eliminado del grafo de red.");
    }
    return ok;
}

// Elimina un enlace dirigido origen -> destino del grafo.
bool Sistema::eliminarConexion(int origenId, int destinoId) {
    bool ok = grafo->eliminarArista(origenId, destinoId);
    if (ok) {
        auditar("RED", "Conexión #" + to_string(origenId) + " -> #"
            + to_string(destinoId) + " eliminada.");
    }
    return ok;
}

// Calcula la ruta óptima (Dijkstra) entre dos servidores y la registra.
RutaOptima Sistema::calcularRutaOptima(int origenId, int destinoId) {
    RutaOptima ruta = grafo->dijkstra(origenId, destinoId);
    if (ruta.existeCamino) {
        auditar("RED", "Ruta optima #" + to_string(origenId) + " -> #"
            + to_string(destinoId) + " calculada (latencia total: "
            + to_string(ruta.latenciaTotal) + " ms).");
    } else {
        auditar("RED", "No se encontro ruta entre #" + to_string(origenId)
            + " y #" + to_string(destinoId) + ".");
    }
    return ruta;
}

// Ejecuta Dijkstra y devuelve una descripción legible del camino.
string Sistema::rutearDijkstra(int origenId, int destinoId) {
    RutaOptima ruta = calcularRutaOptima(origenId, destinoId);
    if (!ruta.existeCamino) {
        return "No hay ruta entre #" + to_string(origenId)
            + " y #" + to_string(destinoId) + ".";
    }
    string descripcion;
    for (size_t i = 0; i < ruta.camino.size(); ++i) {
        if (i > 0) {
            descripcion += " -> ";
        }
        descripcion += "[" + to_string(ruta.camino[i]) + "] "
            + grafo->nombreDe(ruta.camino[i]);
    }
    descripcion += " (latencia total: " + to_string(ruta.latenciaTotal) + " ms)";
    return descripcion;
}

// Muestra el grafo de red completo.
void Sistema::mostrarGrafo() const {
    grafo->mostrar();
}

// ---------------------------------------------------------------------
// Acceso a las estructuras (const)
// ---------------------------------------------------------------------

const GrafoRed& Sistema::getGrafo() const { return *grafo; }

// Devuelve el nombre de un servidor por su ID.
string Sistema::nombreDeServidor(int id) const {
    Servidor* srv = buscarServidorPorId(id);
    if (srv) {
        return srv->getNombre();
    }
    // Si no está en el árbol de servidores, intenta el grafo.
    if (grafo && grafo->existeServidor(id)) {
        return grafo->nombreDe(id);
    }
    return "(sin nombre)";
}

// Devuelve la cantidad total de aristas en el grafo de red.
size_t Sistema::cantidadAristas() const {
    return grafo->cantidadAristas();
}

// ---------------------------------------------------------------------
// Presentación
// ---------------------------------------------------------------------

// Vuelca la auditoría al archivo configurado.
bool Sistema::guardarAuditoria() const { return auditoria->guardar(); }
