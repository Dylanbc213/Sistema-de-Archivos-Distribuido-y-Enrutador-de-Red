#include "Sistema.h"

#include "Archivo.h"      // Fase 2: tipo Archivo en crearArchivo
#include "Auditoria.h"
#include "Carpeta.h"
#include "LoggerAuditoria.h"   // Fase 3: auditoría persistente a disco
#include "GrafoRed.h"
#include "NodoArchivo.h"  // Fase 2: tipo NodoArchivo en búsquedas
#include "Servidor.h"
#include "TablaHashUsuarios.h"
#include "Usuario.h"

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <set>
#include <functional>
#include <filesystem>
#include <windows.h>

using namespace std;

namespace {
string aHex(const string& bytes) {
    static const char digitos[] = "0123456789abcdef";
    string resultado;
    resultado.reserve(bytes.size() * 2);
    for (unsigned char byte : bytes) {
        resultado.push_back(digitos[byte >> 4]);
        resultado.push_back(digitos[byte & 15]);
    }
    return resultado;
}

bool deHex(const string& texto, string& bytes) {
    if (texto.size() % 2 != 0) return false;
    auto valor = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    bytes.clear();
    for (size_t i = 0; i < texto.size(); i += 2) {
        int alto = valor(texto[i]), bajo = valor(texto[i + 1]);
        if (alto < 0 || bajo < 0) return false;
        bytes.push_back(static_cast<char>((alto << 4) | bajo));
    }
    return true;
}

string aHexEstado(const string& bytes) {
    return bytes.empty() ? "-" : aHex(bytes);
}

bool deHexEstado(const string& texto, string& bytes) {
    if (texto == "-") { bytes.clear(); return true; }
    return deHex(texto, bytes);
}

string rutaUsuariosPredeterminada() {
    const char* local = getenv("LOCALAPPDATA");
    if (local == nullptr || local[0] == '\0') return string();
    return (filesystem::path(local) / "NetworkOS" / "usuarios.db").string();
}

string rutaEstadoPredeterminada() {
    const char* local = getenv("LOCALAPPDATA");
    if (local == nullptr || local[0] == '\0') return string();
    return (filesystem::path(local) / "NetworkOS" / "sistema.db").string();
}
}

// ---------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------

// Crea las cuatro estructuras y deja constancia del arranque en auditoría.
// Si se inyecta un logger, también se appendea al archivo persistente.
Sistema::Sistema(size_t bucketsUsuarios, const string& rutaAuditoria,
                 LoggerAuditoria* logger, const string& rutaArchivoUsuarios,
                 const string& rutaArchivoEstado)
    : usuarios(make_unique<TablaHashUsuarios>(bucketsUsuarios)),
      grafo(make_unique<GrafoRed>()),
      auditoria(make_unique<Auditoria>(rutaAuditoria)),
      loggerAuditoria(logger),
      usuarioEnSesion(),
      rutaUsuarios(rutaArchivoUsuarios.empty() ? rutaUsuariosPredeterminada()
                                               : rutaArchivoUsuarios),
      almacenamientoUsuariosDisponible(false),
      rutaEstado(rutaArchivoEstado.empty() ? rutaEstadoPredeterminada()
                                           : rutaArchivoEstado),
      almacenamientoEstadoDisponible(false) {
    almacenamientoUsuariosDisponible = cargarUsuarios();
    if (!almacenamientoUsuariosDisponible)
        cerr << "[ERROR] No se pudieron cargar usuarios; almacenamiento deshabilitado para proteger los datos. Ruta: "
             << (rutaUsuarios.empty() ? "LOCALAPPDATA no configurado" : rutaUsuarios) << "\n";
    almacenamientoEstadoDisponible = cargarEstado();
    if (!almacenamientoEstadoDisponible)
        cerr << "[ERROR] No se pudo cargar el estado de servidores/red/archivos; las operaciones no modificarán el almacenamiento. Ruta: "
             << (rutaEstado.empty() ? "LOCALAPPDATA no configurado" : rutaEstado) << "\n";
    auditar("SISTEMA", "Network OS iniciado.");
}

string Sistema::directorioDatosLocal() {
    const char* local = getenv("LOCALAPPDATA");
    if (local == nullptr || local[0] == '\0') return string();
    return (filesystem::path(local) / "NetworkOS").string();
}

string Sistema::serializarEstado() const {
    ostringstream salida;
    salida << "NETWORKOS_STATE_V1\n";
    function<void(const shared_ptr<Carpeta>&, int)> serializarCarpeta;
    serializarCarpeta = [&](const shared_ptr<Carpeta>& carpeta, int servidorId) {
        if (!carpeta) return;
        for (const auto& hijo : carpeta->getHijos()) {
            if (!hijo) continue;
            if (hijo->esCarpeta()) {
                auto subcarpeta = dynamic_pointer_cast<Carpeta>(hijo);
                if (!subcarpeta) continue;
                salida << "D|" << servidorId << '|' << hijo->getId() << '|'
                       << aHexEstado(carpeta->obtenerRuta()) << '|' << aHexEstado(hijo->getNombre()) << "\n";
                serializarCarpeta(subcarpeta, servidorId);
            } else {
                auto archivo = dynamic_pointer_cast<Archivo>(hijo);
                if (!archivo) continue;
                salida << "F|" << servidorId << '|' << hijo->getId() << '|'
                       << aHexEstado(carpeta->obtenerRuta()) << '|' << aHexEstado(hijo->getNombre()) << '|'
                       << aHexEstado(archivo->getContenido()) << "\n";
            }
        }
    };
    for (const auto& servidor : servidores) {
        if (!servidor) continue;
        salida << "S|" << servidor->getId() << '|' << aHexEstado(servidor->getNombre()) << "\n";
        serializarCarpeta(servidor->getRaiz(), servidor->getId());
    }
    for (int id : grafo->idsServidores())
        salida << "G|" << id << '|' << aHexEstado(grafo->nombreDe(id)) << "\n";
    for (int origen : grafo->idsServidores()) {
        const vector<Arista>* aristas = grafo->vecinos(origen);
        if (!aristas) continue;
        for (const Arista& arista : *aristas)
            salida << "E|" << origen << '|' << arista.destinoId << '|'
                   << arista.latencia << "\n";
    }
    return salida.str();
}

bool Sistema::cargarEstadoDesdeContenido(const string& contenido) {
    struct Registro { char tipo; vector<string> campos; };
    vector<Registro> registros;
    istringstream entrada(contenido);
    string linea, encabezado;
    if (!getline(entrada, encabezado) || encabezado != "NETWORKOS_STATE_V1") return false;
    while (getline(entrada, linea)) {
        if (linea.empty()) continue;
        vector<string> campos;
        string campo;
        istringstream fila(linea);
        while (getline(fila, campo, '|')) campos.push_back(campo);
        if (campos.empty() || campos[0].size() != 1) return false;
        registros.push_back(Registro{campos[0][0], move(campos)});
    }
    if (!entrada.eof()) return false;

    auto entero = [](const string& texto, int& valor) {
        try {
            size_t consumido = 0;
            valor = stoi(texto, &consumido);
            return consumido == texto.size();
        } catch (...) { return false; }
    };
    vector<unique_ptr<Servidor>> nuevosServidores;
    auto nuevoGrafo = make_unique<GrafoRed>();
    set<int> idsServidores, idsGrafo;
    for (const Registro& registro : registros) {
        if (registro.tipo == 'S') {
            int id; string nombre;
            if (registro.campos.size() != 3 || !entero(registro.campos[1], id) || id < 0 ||
                !deHexEstado(registro.campos[2], nombre) || nombre.empty() ||
                !idsServidores.insert(id).second) return false;
            auto raiz = make_shared<Carpeta>(id, "/");
            nuevosServidores.push_back(make_unique<Servidor>(id, nombre, raiz));
        } else if (registro.tipo == 'G') {
            int id; string nombre;
            if (registro.campos.size() != 3 || !entero(registro.campos[1], id) || id < 0 ||
                !deHexEstado(registro.campos[2], nombre) || nombre.empty() ||
                !idsGrafo.insert(id).second || !nuevoGrafo->agregarServidor(id, nombre)) return false;
        }
    }
    auto buscarServidorNuevo = [&](int id) -> Servidor* {
        for (const auto& servidor : nuevosServidores)
            if (servidor && servidor->getId() == id) return servidor.get();
        return nullptr;
    };
    for (const Registro& registro : registros) {
        if (registro.tipo == 'D' || registro.tipo == 'F') {
            const bool esDirectorio = registro.tipo == 'D';
            const size_t esperado = esDirectorio ? 5 : 6;
            int servidorId, id; string rutaPadre, nombre;
            if (registro.campos.size() != esperado ||
                !entero(registro.campos[1], servidorId) ||
                !entero(registro.campos[2], id) || id < 0 ||
                !deHexEstado(registro.campos[3], rutaPadre) ||
                !deHexEstado(registro.campos[4], nombre) || nombre.empty()) return false;
            Servidor* servidor = buscarServidorNuevo(servidorId);
            if (!servidor) return false;
            auto padreNodo = servidor->buscarPorRuta(rutaPadre);
            auto padre = dynamic_pointer_cast<Carpeta>(padreNodo);
            if (!padre) return false;
            shared_ptr<NodoArchivo> hijo;
            if (esDirectorio) hijo = make_shared<Carpeta>(id, nombre);
            else {
                string datos;
                if (!deHexEstado(registro.campos[5], datos)) return false;
                hijo = make_shared<Archivo>(id, nombre, datos);
            }
            if (!padre->agregarHijo(hijo)) return false;
        } else if (registro.tipo == 'E') {
            int origen, destino, costo;
            if (registro.campos.size() != 4 || !entero(registro.campos[1], origen) ||
                !entero(registro.campos[2], destino) || !entero(registro.campos[3], costo) ||
                costo < 0 || !nuevoGrafo->agregarArista(origen, destino, costo)) return false;
        } else if (registro.tipo != 'S' && registro.tipo != 'G') return false;
    }
    servidores.swap(nuevosServidores);
    grafo.swap(nuevoGrafo);
    return true;
}

bool Sistema::cargarEstado() {
    if (rutaEstado.empty()) return false;
    error_code ec;
    const filesystem::path ruta(rutaEstado);
    if (!filesystem::exists(ruta, ec)) return !ec;
    if (ec) return false;
    ifstream archivo(ruta, ios::binary);
    if (!archivo) return false;
    ostringstream contenido;
    contenido << archivo.rdbuf();
    if (archivo.bad()) return false;
    return cargarEstadoDesdeContenido(contenido.str());
}

bool Sistema::guardarEstado() const {
    if (!almacenamientoEstadoDisponible || rutaEstado.empty()) return false;
    const filesystem::path ruta(rutaEstado);
    error_code ec;
    if (!ruta.parent_path().empty()) {
        filesystem::create_directories(ruta.parent_path(), ec);
        if (ec) return false;
    }
    const string temporal = rutaEstado + ".tmp";
    {
        ofstream salida(temporal, ios::binary | ios::trunc);
        if (!salida) return false;
        salida << serializarEstado();
        salida.flush();
        if (!salida) { salida.close(); filesystem::remove(temporal, ec); return false; }
    }
    if (!MoveFileExA(temporal.c_str(), rutaEstado.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        filesystem::remove(temporal, ec);
        return false;
    }
    return true;
}

bool Sistema::confirmarCambioEstado(const string& estadoAnterior) {
    if (guardarEstado()) return true;
    const bool restaurado = cargarEstadoDesdeContenido(estadoAnterior);
    cerr << "[ERROR] No se pudo guardar el estado persistente en '" << rutaEstado
         << "'; el cambio fue revertido" << (restaurado ? ".\n" : " y falló la restauración en memoria.\n");
    auditar("ERROR", "Operacion de servidores/archivos/red revertida por fallo de persistencia.");
    return false;
}

bool Sistema::cargarUsuarios() {
    if (rutaUsuarios.empty()) return false;
    error_code ec;
    const filesystem::path ruta(rutaUsuarios);
    if (!filesystem::exists(ruta, ec)) return !ec;
    if (ec) return false;
    ifstream entrada(ruta, ios::binary);
    if (!entrada) return false;
    string encabezado;
    if (!getline(entrada, encabezado) || encabezado != "NETWORKOS_USERS_V1") return false;
    vector<Usuario> cargados;
    set<string> nombres;
    set<int> ids;
    string linea;
    while (getline(entrada, linea)) {
        if (linea.empty()) continue;
        vector<string> campos;
        string campo;
        istringstream parser(linea);
        while (getline(parser, campo, '|')) campos.push_back(campo);
        if (campos.size() != 7) return false;
        try {
            size_t consumidoId = 0, consumidoRol = 0, consumidoServidor = 0;
            int id = stoi(campos[0], &consumidoId);
            int rol = stoi(campos[1], &consumidoRol);
            int servidor = stoi(campos[2], &consumidoServidor);
            string nombre, sal, hash;
            if (consumidoId != campos[0].size() || consumidoRol != campos[1].size() ||
                consumidoServidor != campos[2].size() || id < 0 || servidor < -1 ||
                (rol != 0 && rol != 1) || !deHex(campos[3], nombre) || nombre.empty() ||
                !deHex(campos[4], sal) || sal.size() != 16 ||
                !deHex(campos[5], hash) || hash.size() != 32 || campos[6] != "END" ||
                !nombres.insert(nombre).second || !ids.insert(id).second) return false;
            cargados.push_back(Usuario::desdeDatosPersistidos(id, nombre, sal, hash,
                rol == 1 ? NivelPermiso::ADMIN : NivelPermiso::NORMAL, servidor));
        } catch (...) { return false; }
    }
    if (!entrada.eof()) return false;
    for (const Usuario& usuario : cargados)
        if (!usuarios->insertar(usuario)) return false;
    return true;
}

bool Sistema::guardarUsuarios() const {
    if (!almacenamientoUsuariosDisponible || rutaUsuarios.empty()) return false;
    filesystem::path ruta(rutaUsuarios);
    error_code ec;
    if (!ruta.parent_path().empty()) {
        filesystem::create_directories(ruta.parent_path(), ec);
        if (ec) return false;
    }
    const string temporal = rutaUsuarios + ".tmp";
    {
        ofstream salida(temporal, ios::binary | ios::trunc);
        if (!salida) return false;
        salida << "NETWORKOS_USERS_V1\n";
        for (const Usuario& usuario : usuarios->obtenerTodos()) {
            salida << usuario.getId() << '|' << (usuario.esAdmin() ? 1 : 0) << '|'
                   << usuario.getServidorAsignado() << '|' << aHex(usuario.getNombre()) << '|'
                   << aHex(usuario.salPersistida()) << '|' << aHex(usuario.hashPersistido())
                   << "|END\n";
        }
        salida.flush();
        if (!salida) { salida.close(); filesystem::remove(temporal, ec); return false; }
    }
    if (!MoveFileExA(temporal.c_str(), rutaUsuarios.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        filesystem::remove(temporal, ec);
        return false;
    }
    return true;
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
    if (id < 0 || nombre.find_first_not_of(" \t\r\n") == string::npos) {
        auditar("ERROR", "Intento de crear servidor con ID o nombre invalido.");
        return nullptr;
    }
    if (buscarServidorPorId(id) != nullptr || grafo->existeServidor(id)) {
        auditar("ERROR", "El servidor #" + to_string(id) + " ya existe en el sistema o en la red.");
        return nullptr;
    }

    const string estadoAnterior = serializarEstado();
    servidores.push_back(make_unique<Servidor>(id, nombre));
    if (!grafo->agregarServidor(id, nombre)) {
        servidores.pop_back();
        auditar("ERROR", "No se pudo sincronizar el servidor #" + to_string(id) + " con la red.");
        return nullptr;
    }
    auditoria->registrar("ARBOL", "Servidor #" + to_string(id) +
                                       " \"" + nombre + "\" creado con su raíz.");

    if (!confirmarCambioEstado(estadoAnterior)) return nullptr;
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
    const string estadoAnterior = serializarEstado();
    auto resultado = srv->crearSubdirectorio(ruta);
    if (resultado) {
        if (!confirmarCambioEstado(estadoAnterior)) return nullptr;
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
    const string estadoAnterior = serializarEstado();
    auto resultado = srv->crearArchivo(ruta, contenido);
    if (resultado) {
        if (!confirmarCambioEstado(estadoAnterior)) return nullptr;
        auditoria->registrar("ARBOL", "Archivo creado: '" + ruta
            + "' en servidor #" + to_string(servidorId) + ".");
    }
    return resultado;
}

bool Sistema::modificarContenidoArchivo(int servidorId, const string& ruta,
                                       const string& contenido) {
    Servidor* servidor = buscarServidorPorId(servidorId);
    if (!servidor) {
        auditar("ERROR", "Modificar archivo: servidor #" + to_string(servidorId) + " no existe.");
        return false;
    }
    auto archivo = dynamic_pointer_cast<Archivo>(servidor->buscarPorRuta(ruta));
    if (!archivo) {
        auditar("ERROR", "Modificar archivo: no existe un archivo en la ruta '" + ruta + "'.");
        return false;
    }
    const string estadoAnterior = serializarEstado();
    archivo->setContenido(contenido);
    if (!confirmarCambioEstado(estadoAnterior)) return false;
    auditar("ARBOL", "Contenido de archivo modificado: '" + ruta + "' en servidor #" +
            to_string(servidorId) + ".");
    return true;
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
    const string estadoAnterior = serializarEstado();
    bool ok = srv->eliminarRecursivo(ruta);
    if (ok) {
        if (!confirmarCambioEstado(estadoAnterior)) return false;
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
    if (!sesionEsAdmin()) {
        auditar("ACCESS_DENIED", "Creacion de usuario denegada: se requiere ADMIN autenticado.");
        return false;
    }
    if (id < 0 || nombre.find_first_not_of(" \t\r\n") == string::npos ||
        contrasena.empty() ||
        (nivelPermiso != 0 && nivelPermiso != 1)) {
        auditar("ERROR", "Registro rechazado por ID, nombre, contrasena o permiso invalido.");
        return false;
    }
    NivelPermiso permiso = (nivelPermiso == 0)
                               ? NivelPermiso::NORMAL
                               : NivelPermiso::ADMIN;
    const string nivelStr = (permiso == NivelPermiso::ADMIN) ? "ADMIN" : "NORMAL";
    Usuario nuevo(id, nombre, contrasena, permiso);
    if (!nuevo.tieneContrasenaValida()) {
        auditar("ERROR", "Registro rechazado: fallo la derivacion de la contrasena.");
        return false;
    }
    bool ok = usuarios->insertar(nuevo);
    if (ok) {
        if (!guardarUsuarios()) {
            usuarios->eliminar(nombre);
            auditar("ERROR", "No se pudo persistir el usuario; alta revertida para proteger el archivo.");
            return false;
        }
        auditar("USUARIO",
            "Usuario \"" + nombre + "\" registrado (id " + to_string(id)
            + ", permiso " + nivelStr + ").");
    } else {
        auditar("ERROR", "Registro fallido para usuario \"" + nombre
            + "\": ID o nombre duplicado.");
    }
    return ok;
}

bool Sistema::asegurarAdministradorInicial(int id, const string& nombre,
                                           const string& contrasena) {
    if (usuarios->cantidadAdministradores() != 0) {
        return true;
    }
    if (id < 0 || nombre.find_first_not_of(" \t\r\n") == string::npos ||
        contrasena.empty()) {
        auditar("ERROR", "Configuracion del administrador inicial invalida.");
        return false;
    }
    if (usuarios->buscar(nombre) != nullptr || usuarios->contieneId(id)) {
        auditar("ERROR", "El administrador inicial colisiona con una cuenta existente.");
        return false;
    }
    Usuario admin(id, nombre, contrasena, NivelPermiso::ADMIN);
    if (!admin.tieneContrasenaValida() || !usuarios->insertar(admin)) {
        auditar("ERROR", "No se pudo crear el administrador inicial.");
        return false;
    }
    if (!guardarUsuarios()) {
        usuarios->eliminar(nombre);
        auditar("ERROR", "No se pudo persistir el administrador inicial; provision revertida.");
        return false;
    }
    auditar("USUARIO", "Administrador inicial provisionado: \"" + nombre + "\".");
    return true;
}

bool Sistema::eliminarUsuario(const string& nombre) {
    if (!sesionEsAdmin()) {
        auditar("ACCESS_DENIED", "Eliminacion de usuario denegada: se requiere ADMIN autenticado.");
        return false;
    }
    const Usuario* objetivo = usuarios->buscar(nombre);
    if (objetivo == nullptr) {
        auditar("ERROR", "No se puede eliminar un usuario inexistente.");
        return false;
    }
    if (objetivo->esAdmin() && usuarios->cantidadAdministradores() <= 1) {
        auditar("ERROR", "Eliminacion rechazada: no se puede eliminar el ultimo administrador.");
        return false;
    }
    if (nombre == usuarioEnSesion) {
        auditar("ERROR", "No se puede eliminar la cuenta activa.");
        return false;
    }
    Usuario copia = *objetivo;
    if (!usuarios->eliminar(nombre)) return false;
    if (!guardarUsuarios()) {
        usuarios->insertar(copia);
        auditar("ERROR", "No se pudo persistir la eliminacion; cuenta restaurada.");
        return false;
    }
    auditar("USUARIO", "Usuario eliminado: \"" + nombre + "\".");
    return true;
}

bool Sistema::asignarServidorUsuario(const string& nombre, int servidorId) {
    if (!sesionEsAdmin()) {
        auditar("ACCESS_DENIED", "Asignacion de servidor denegada: se requiere ADMIN autenticado.");
        return false;
    }
    if (servidorId < 0 || buscarServidorPorId(servidorId) == nullptr) return false;
    Usuario* usuario = usuarios->buscar(nombre);
    if (usuario == nullptr) return false;
    int servidorAnterior = usuario->getServidorAsignado();
    usuario->setServidorAsignado(servidorId);
    if (!guardarUsuarios()) {
        usuario->setServidorAsignado(servidorAnterior);
        auditar("ERROR", "No se pudo persistir el cambio; asignacion restaurada.");
        return false;
    }
    auditar("USUARIO", "Usuario \"" + nombre + "\" asignado a servidor #" +
            to_string(servidorId) + ".");
    return true;
}

// ---------------------------------------------------------------------
// Autenticación (Fase 3)
// ---------------------------------------------------------------------

// La tabla hash localiza al usuario en O(1) promedio (verificarSesion) y
// compara la contraseña. Cada intento (éxito o fracaso) se audita en la
// traza volátil y en network_audit_log.txt de forma automática.
bool Sistema::iniciarSesion(const string& nombre, const string& contrasena) {
    if (tieneSesion()) {
        auditar("ACCESS_DENIED", "Inicio de sesion rechazado: ya existe una sesion activa.");
        return false;
    }
    Usuario* u = usuarios->verificarSesion(nombre, contrasena);
    if (u != nullptr) {
        usuarioEnSesion = nombre;
        auditar("AUT",
            "Inicio de sesion EXITOSO: \"" + nombre
            + "\" (permiso " + u->getPermisoStr() + ").");
        return true;
    }
    auditar("AUT",
        "Inicio de sesion FALLIDO para: \"" + nombre + "\".");
    return false;
}

void Sistema::cerrarSesion() {
    if (usuarioEnSesion.empty()) return;
    auditar("AUT", "Sesion cerrada para usuario: \"" + usuarioEnSesion + "\".");
    usuarioEnSesion.clear();
}

bool Sistema::tieneSesion() const { return !usuarioEnSesion.empty(); }

bool Sistema::sesionEsAdmin() const {
    const Usuario* usuario = usuarios->buscar(usuarioEnSesion);
    return usuario != nullptr && usuario->esAdmin();
}

string Sistema::nombreUsuarioEnSesion() const { return usuarioEnSesion; }

string Sistema::rolUsuarioEnSesion() const {
    const Usuario* usuario = usuarios->buscar(usuarioEnSesion);
    return usuario == nullptr ? string() : usuario->getPermisoStr();
}

// ---------------------------------------------------------------------
// Grafo de red
// ---------------------------------------------------------------------

// Conecta dos servidores existentes con la latencia indicada.
bool Sistema::conectarServidores(int origenId, int destinoId, int latencia) {
    const string estadoAnterior = serializarEstado();
    bool ok = grafo->agregarArista(origenId, destinoId, latencia);
    if (ok && !confirmarCambioEstado(estadoAnterior)) return false;
    if (!ok) {
        auditar("ERROR", "Intento fallido de conexion #" + to_string(origenId) +
                        " -> #" + to_string(destinoId) + " (" + to_string(latencia) + " ms).");
    }
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

const TablaHashUsuarios& Sistema::getUsuarios() const { return *usuarios; }

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
    const string estadoAnterior = serializarEstado();
    bool ok = grafo->agregarServidor(id, nombre);
    if (ok && !confirmarCambioEstado(estadoAnterior)) return false;
    if (!ok) {
        auditar("ERROR", "Intento fallido de registrar servidor #" + to_string(id)
            + " en el grafo de red.");
    }
    if (ok) {
        auditar("RED", "Servidor #" + to_string(id)
            + " \"" + nombre + "\" registrado en el grafo de red.");
    }
    return ok;
}

// Añade un enlace dirigido origen -> destino con la latencia dada.
bool Sistema::agregarConexionRed(int origenId, int destinoId, int latencia) {
    const string estadoAnterior = serializarEstado();
    bool ok = grafo->agregarArista(origenId, destinoId, latencia);
    if (ok && !confirmarCambioEstado(estadoAnterior)) return false;
    if (!ok) {
        auditar("ERROR", "Intento fallido de agregar conexion #" + to_string(origenId)
            + " -> #" + to_string(destinoId) + " (" + to_string(latencia) + " ms).");
    }
    if (ok) {
        auditar("RED", "Conexión #" + to_string(origenId) + " -> #"
            + to_string(destinoId) + " (" + to_string(latencia) + " ms) agregada.");
    }
    return ok;
}

// Elimina un servidor del grafo de red (y todas las aristas que referencian).
bool Sistema::eliminarServidor(int id) {
    const string estadoAnterior = serializarEstado();
    bool ok = grafo->eliminarServidor(id);
    if (ok && !confirmarCambioEstado(estadoAnterior)) return false;
    if (!ok) {
        auditar("ERROR", "Intento fallido de eliminar servidor de red #" + to_string(id) + ".");
    }
    if (ok) {
        auditar("RED", "Servidor #" + to_string(id) + " eliminado del grafo de red.");
    }
    return ok;
}

// Elimina un enlace dirigido origen -> destino del grafo.
bool Sistema::eliminarConexion(int origenId, int destinoId) {
    const string estadoAnterior = serializarEstado();
    bool ok = grafo->eliminarArista(origenId, destinoId);
    if (ok && !confirmarCambioEstado(estadoAnterior)) return false;
    if (!ok) {
        auditar("ERROR", "Intento fallido de eliminar conexion #" + to_string(origenId)
            + " -> #" + to_string(destinoId) + ".");
    }
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
