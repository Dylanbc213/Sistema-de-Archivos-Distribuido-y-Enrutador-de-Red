#ifndef SISTEMA_H
#define SISTEMA_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// Declaraciones adelantadas: la cabecera de Sistema no necesita conocer el
// detalle de estas clases (solo las posee mediante punteros), lo que evita
// arrastrar todas las dependencias a quien incluya "Sistema.h".
class Servidor;
class NodoArchivo;
class Archivo;
class Carpeta;
class Usuario;
class TablaHashUsuarios;
class GrafoRed;
class Auditoria;
class LoggerAuditoria;   // auditoría persistente (append a network_audit_log.txt)


// =====================================================================
// Sistema: clase central del Network OS.
// =====================================================================
// Inicializa y administra las cuatro estructuras principales del sistema:
//
//   1. Árbol de directorios: cada Servidor posee su carpeta raíz. Los
//      servidores se guardan en un vector de std::unique_ptr, de modo que
//      Sistema es el único dueño de cada servidor (y, en cadena, de su
//      árbol de archivos).
//   2. Tabla hash de usuarios: tabla propia con arreglo dinámico de listas
//      enlazadas + hash matemática propia, sin usar std::unordered_map.
//   3. Grafo de red: listas de adyacencia que referencian a los servidores
//      por su ID y guardan la latencia de cada conexión.
//   4. Archivo de auditoría: traza de eventos del sistema.
//
// En esta fase solo se inicializan y administran; la búsqueda recursiva,
// la autenticación y los algoritmos de recorrido (Dijkstra, BFS, DFS) se
// implementarán más adelante.
class Sistema {
private:
    // 1) Árboles de directorios: un servidor (y su raíz) por elemento.
    std::vector<std::unique_ptr<Servidor>> servidores;

    // 2) Tabla hash de usuarios.
    std::unique_ptr<TablaHashUsuarios> usuarios;

    // 3) Grafo de red.
    std::unique_ptr<GrafoRed> grafo;

    // 4) Auditoría.
    std::unique_ptr<Auditoria> auditoria;

    // Logger de auditoría PERSISTENTE (append a network_audit_log.txt).
    // Es NO propietario: lo inyecta main. Sirve para que el registro y el
    // inicio de sesión de usuarios dejen constancia inmediata en disco.
    LoggerAuditoria* loggerAuditoria;

    // Busca un servidor por ID. Devuelve nullptr si no existe.
    Servidor* buscarServidorPorId(int id) const;

    // -----------------------------------------------------------------
    // Auditoría transaccional (volátil + persistente)
    // -----------------------------------------------------------------
    // Centraliza la escritura doble: la traza volátil (Auditoria) y, si
    // existe el logger, un append inmediato a network_audit_log.txt.
    void auditar(const std::string& tipo, const std::string& mensaje);

public:
    // Constructor: crea las estructuras con una configuración por defecto
    // (número de buckets de la tabla hash, ruta del archivo de auditoría y
    // logger persistente opcional). Registra el evento de arranque. El
    // logger permite que la auditoría de autenticación (Fase 3) se
    // appendee directamente a network_audit_log.txt.
    Sistema(std::size_t bucketsUsuarios = 16,
            const std::string& rutaAuditoria = "auditoria.log",
            LoggerAuditoria* logger = nullptr);

    // Asocia dinámicamente un logger persistente para la auditoría de
    // autenticación (alternativa al parámetro del constructor).
    void asignarLogger(LoggerAuditoria& log);

    // No se permite copiar ni mover (los unique_ptr no son copiables, y el
    // destructor se define en el .cpp). Esto permite declarar los miembros
    // como punteros a tipos incompletos en la cabecera, manteniéndola limpia
    // de dependencias.
    Sistema(const Sistema&) = delete;
    Sistema& operator=(const Sistema&) = delete;
    Sistema(Sistema&&) = delete;
    Sistema& operator=(Sistema&&) = delete;

    // Declarado aquí y definido como '= default' en Sistema.cpp: de este
    // modo los destructores de los unique_ptr miembros solo se instancian
    // en la unidad de traducción donde las clases ya son completas.
    ~Sistema();

    // --- Árbol de directorios ---------------------------------------
    // Crea un servidor con su carpeta raíz, lo añade al grafo como nodo y
    // registra el evento. Devuelve un puntero NO propietario al servidor
    // creado (nullptr si el ID ya existía o era inválido).
    Servidor* crearServidor(int id, const std::string& nombre);

    std::size_t cantidadServidores() const;

    // Imprime una lista de todos los servidores registrados
    // (ID, nombre y nombre de la carpeta raíz).
    void listarServidores() const;

    // Obtiene un puntero NO propietario a un servidor por su ID
    // (nullptr si no existe).
    Servidor* getServidor(int id) const;

    // --- Fase 2: Operaciones de sistema de archivos sobre un servidor ---
    // Todas delegan en la carpeta raíz del servidor identificado por
    // 'servidorId'. Registran eventos en auditoría con la etiqueta "ARBOL".

    // Búsqueda por ruta desde la raíz del servidor.
    std::shared_ptr<NodoArchivo> buscarPorRuta(int servidorId,
                                               const std::string& ruta);

        // Búsqueda recursiva (DFS) por nombre en todo el árbol del servidor.
    std::shared_ptr<NodoArchivo> buscarRecursivoPorNombre(
        int servidorId, const std::string& nombre);

    // Crea una subcarpeta en la ruta indicada del servidor
    // (crea directorios intermedios si no existen).
    std::shared_ptr<Carpeta> crearSubdirectorio(int servidorId,
                                                 const std::string& ruta);

    // Crea un archivo en la ruta indicada del servidor.
    std::shared_ptr<Archivo> crearArchivo(int servidorId,
                                           const std::string& ruta,
                                           const std::string& contenido);

    // Elimina recursivamente un nodo por ruta (cascada si es carpeta).
    bool eliminarRecursivo(int servidorId, const std::string& ruta);

    // Muestra el árbol de directorios de un servidor.
    void mostrarArbol(int servidorId) const;

    // --- Tabla hash de usuarios -------------------------------------
    // Da de alta un usuario en la tabla hash y lo registra en auditoría.
    // 'nivelPermiso' 0=Normal, 1=Admin (Normal por defecto). Devuelve false
    // si el nombre ya existía.
    bool registrarUsuario(int id, const std::string& nombre,
                          const std::string& contrasena,
                          int nivelPermiso = 0);

    // --- Autenticación (Fase 3) --------------------------------------
    // Inicia sesión validando credenciales en O(1) promedio (búsqueda hash
    // + verificación de contraseña). Registra automáticamente en la traza
    // volátil y en network_audit_log.txt cada intento (éxito o fracaso).
    // Devuelve true solo si las credenciales son válidas.
    bool iniciarSesion(const std::string& nombre,
                       const std::string& contrasena);

    // --- Grafo de red -----------------------------------------------
    // Conecta dos servidores existentes con una latencia dada.
    bool conectarServidores(int origenId, int destinoId, int latencia);

    // --- Acceso a las estructuras (para consultas / administración) --
    TablaHashUsuarios& getUsuarios();
    GrafoRed& getGrafo();
    Auditoria& getAuditoria();

    // --- Presentación -----------------------------------------------
    // Imprime un resumen con el estado de las cuatro estructuras.
    void mostrarResumen() const;

    // Vuelca la auditoría a su archivo configurado.
    bool guardarAuditoria() const;
};

#endif
