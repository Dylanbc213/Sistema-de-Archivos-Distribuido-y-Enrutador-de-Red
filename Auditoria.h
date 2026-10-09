#ifndef AUDITORIA_H
#define AUDITORIA_H

#include <string>
#include <vector>

// =====================================================================
// Auditoria: registro de eventos del Network OS.
// =====================================================================
// Almacena una traza cronológica de eventos (creación de servidores,
// conexiones, altas de usuarios, etc.). Cada evento guarda una marca de
// tiempo, un tipo/categoría y un mensaje descriptivo.
//
// De momento solo prepara la estructura en memoria; la persistencia a un
// archivo de texto se delega en el método guardar(), que puede invocarse
// cuando se desee volcar la traza.
struct EventoAuditoria {
    std::string marcaTiempo;   // fecha y hora formateadas
    std::string tipo;          // categoría del evento (SISTEMA, RED, ...)
    std::string mensaje;       // descripción del evento
};

class Auditoria {
private:
    std::vector<EventoAuditoria> eventos;

    // Nombre del archivo destino para guardar().
    std::string rutaArchivo;

    // Genera la marca de tiempo actual con formato legible.
    std::string marcaTiempoActual() const;

public:
    // Constructor: permite indicar la ruta del archivo de volcado.
    explicit Auditoria(const std::string& rutaArchivo = "auditoria.log");

    // Registra un nuevo evento con el tipo y mensaje dados; la marca de
    // tiempo se añade automáticamente.
    void registrar(const std::string& tipo, const std::string& mensaje);

    // Consultas.
    std::size_t cantidadEventos() const;
    const std::vector<EventoAuditoria>& getEventos() const;

    // Cambia la ruta del archivo de volcado.
    void setRutaArchivo(const std::string& ruta);

    // Escribe todos los eventos en el archivo de texto indicado. Devuelve
    // false si no se pudo abrir el archivo.
    bool guardar() const;
    bool guardar(const std::string& ruta) const;

    // Imprime la traza por consola.
    void mostrar() const;

    // Vacía el registro en memoria.
    void limpiar();
};

#endif
