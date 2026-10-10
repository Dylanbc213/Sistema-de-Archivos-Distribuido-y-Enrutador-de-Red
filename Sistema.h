#ifndef SISTEMA_H
#define SISTEMA_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "GrafoRed.h"  // RutaOptima (usado por valor en calcularRutaOptima)

// Declaraciones adelantadas: la cabecera de Sistema no necesita conocer el
// detalle de estas clases (solo las posee mediante punteros), lo que evita
// arrastrar todas las dependencias a quien incluya "Sistema.h".
class Servidor;
class NodoArchivo;
class Archivo;
class Carpeta;
class Usuario;
class TablaHashUsuarios;
class Auditoria;
class LoggerAuditoria;   // auditoría persistente (append a network_audit_log.txt)


// =====================================================================
// Sistema: clase central do Network OS.
// =====================================================================
class Sistema {
private:
    // 1) Arbores de directorios: un servidor (e sua raíz) por elemento.
    std::vector<std::unique_ptr<Servidor>> servidores;

    // 2) Tabela hash de usuarios.
    std::unique_ptr<TablaHashUsuarios> usuarios;

    // 3) Grafo de rede.
    std::unique_ptr<GrafoRed> grafo;

    // 4) Auditoría.
    std::unique_ptr<Auditoria> auditoria;

    // Logger de auditoría PERSISTENTE (append a network_audit_log.txt).
    // É NON propietario: inxéntoo main.
    LoggerAuditoria* loggerAuditoria;

    // Busca un servidor polo seu ID.
    Servidor* buscarServidorPorId(int id) const;

    // Auditoría transaccional (volátil + persistente).
    void auditar(const std::string& tipo, const std::string& mensaje);

public:
    Sistema(std::size_t bucketsUsuarios = 16,
            const std::string& rutaAuditoria = "auditoria.log",
            LoggerAuditoria* logger = nullptr);

    void asignarLogger(LoggerAuditoria& log);

    Sistema(const Sistema&) = delete;
    Sistema& operator=(const Sistema&) = delete;
    Sistema(Sistema&&) = delete;
    Sistema& operator=(Sistema&&) = delete;

    ~Sistema();

    Servidor* crearServidor(int id, const std::string& nombre);
    std::size_t cantidadServidores() const;
    void listarServidores() const;
    Servidor* getServidor(int id) const;


    // -----------------------------------------------------------------
    // Fase 2: Operacións de sistema de ficheiros sobre un servidor
    // -----------------------------------------------------------------
    // Delegan na carpeta raíz do servidor. As rutas pode ser absolutas
    // ('/docs/reportes/ficheiro.txt') ou relativas.

    std::shared_ptr<NodoArchivo> buscarPorRuta(int servidorId,
                                               const std::string& ruta) const;

    std::shared_ptr<NodoArchivo> buscarRecursivoPorNombre(
        int servidorId, const std::string& nombre) const;

    std::shared_ptr<Carpeta> crearSubdirectorio(int servidorId,
                                                 const std::string& ruta);

    std::shared_ptr<Archivo> crearArchivo(int servidorId,
                                            const std::string& ruta,
                                            const std::string& contenido);

    bool eliminarRecursivo(int servidorId, const std::string& ruta);

    void mostrarArbol(int servidorId) const;

    // -----------------------------------------------------------------
    // Fase 3: Tabela hash de usuarios e autenticación
    // -----------------------------------------------------------------
    bool registrarUsuario(int id, const std::string& nombre,
                          const std::string& contrasena,
                          int nivelPermiso = 0);

    bool iniciarSesion(const std::string& nombre,
                       const std::string& contrasena);


    // -----------------------------------------------------------------
    // Grafo de rede
    // -----------------------------------------------------------------
    // Conecta dous servidores existentes ca unha latencia dada.
    bool conectarServidores(int origenId, int destinoId, int latencia);

    // Rexistra un servidor no grafo de rede.
    bool agregarServidorRed(int id, const std::string& nombre);

    // Engade unha conexión dirixida origen -> destino ca latencia.
    bool agregarConexionRed(int origenId, int destinoId, int latencia);

    // Elimina un servidor do grafo (e as súas conexións).
    bool eliminarServidor(int id);

    // Elimina unha conexión do grafo.
    bool eliminarConexion(int origenId, int destinoId);

    // Calcula a ruta óptima (Dijkstra) entre dous servidores e a rexista
    // en auditoría. Devolve o RutaOptima resultante.
    RutaOptima calcularRutaOptima(int origenId, int destinoId);

    // Executa Dijkstra e devolve unha descrición lexible do camiño.
    std::string rutearDijkstra(int origenId, int destinoId);

    // Mostra o grafo de rede completo.
    void mostrarGrafo() const;

    // -----------------------------------------------------------------
    // Acceso ás estruturas (para consultas / administración)
    // -----------------------------------------------------------------
        TablaHashUsuarios& getUsuarios();
    GrafoRed& getGrafo();
    const GrafoRed& getGrafo() const;
        Auditoria& getAuditoria();
    std::string nombreDeServidor(int id) const;
    std::size_t cantidadAristas() const;

    // -----------------------------------------------------------------
    // Presentación
    // -----------------------------------------------------------------
    // Imprime un resumo coa situación das catro estruturas.
    void mostrarResumen() const;

    // Volve a auditoría ao seu ficheiro configurado.
    bool guardarAuditoria() const;
};

#endif



