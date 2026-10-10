#include "Sistema.h"

#include "Archivo.h"      // Fase 2: tipo Archivo en crearArchivo
#include "Auditoria.h"
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
Sistema::Sistema(size_t bucketsUsuarios, const string& rutaAuditoria)
    : usuarios(make_unique<TablaHashUsuarios>(bucketsUsuarios)),
      grafo(make_unique<GrafoRed>()),
      auditoria(make_unique<Auditoria>(rutaAuditoria)) {
    auditoria->registrar("SISTEMA", "Network OS iniciado.");
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
                                               const string& ruta) {
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
    int servidorId, const string& nombre) {
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
bool Sistema::registrarUsuario(int id, const string& nombre,
                               const string& contrasena) {
    bool ok = usuarios->insertar(Usuario(id, nombre, contrasena));
    if (ok) {
        auditoria->registrar("USUARIO", "Usuario \"" + nombre + "\" registrado.");
    } else {
        auditoria->registrar("ERROR", "Usuario \"" + nombre + "\" ya existe.");
    }
    return ok;
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

// Vuelca la auditoría al archivo configurado.
bool Sistema::guardarAuditoria() const { return auditoria->guardar(); }
