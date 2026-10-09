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
class Usuario;
class TablaHashUsuarios;
class GrafoRed;
class Auditoria;

// =====================================================================
// Sistema: clase central del Network OS.
// =====================================================================
// Inicializa y administra las cuatro estructuras principales del sistema:
//
//   1. Árbol de directorios: cada Servidor posee su carpeta raíz. Los
//      servidores se guardan en un vector de std::unique_ptr, de modo que
//      Sistema es el único dueño de cada servidor (y, en cadena, de su
//      árbol de archivos).
//   2. Tabla hash de usuarios: tabla propia con vector + encadenamiento,
//      sin usar std::unordered_map.
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

    // Busca un servidor por ID. Devuelve nullptr si no existe.
    Servidor* buscarServidorPorId(int id) const;

public:
    // Constructor: crea las estructuras con una configuración por defecto
    // (número de buckets de la tabla hash y ruta del archivo de auditoría)
    // y registra el evento de arranque.
    Sistema(std::size_t bucketsUsuarios = 16,
            const std::string& rutaAuditoria = "auditoria.log");

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

    // --- Tabla hash de usuarios -------------------------------------
    // Da de alta un usuario en la tabla hash y lo registra en auditoría.
    // Devuelve false si el nombre ya existía.
    bool registrarUsuario(int id, const std::string& nombre,
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
