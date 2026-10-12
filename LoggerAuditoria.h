#ifndef LOGGER_AUDITORIA_H
#define LOGGER_AUDITORIA_H

#include <string>

// =====================================================================
// LoggerAuditoria: módulo de auditoría persistente para Network OS.
// =====================================================================
// Escribe un registro cronológico de eventos en un archivo de texto
// (por defecto "network_audit_log.txt") usando SIEMPRE modo append: cada
// llamada añade una línea al final sin borrar los registros anteriores,
// de modo que la traza sobrevive entre ejecuciones del programa.
//
// Es un módulo autocontenido y reutilizable: no depende de ninguna otra
// clase del sistema, por lo que puede emplearse en fases posteriores
// (red, archivos, usuarios, etc.) para centralizar toda la auditoría.
//
// Formato de cada línea escrita:
//   [AAAA-MM-DD HH:MM:SS] <TIPO> mensaje descriptivo
//
// El archivo se crea automáticamente la primera vez que se escribe en él
// si aún no existe.
class LoggerAuditoria {
private:
    // Ruta del archivo de registro.
    std::string rutaArchivo;

    // Genera la fecha y hora locales con formato "AAAA-MM-DD HH:MM:SS".
    std::string marcaTiempoActual() const;

public:
    // Construye el logger apuntando a la ruta indicada. No abre el
    // archivo todavía: se crea/abre de forma perezosa en cada append.
    explicit LoggerAuditoria(const std::string& ruta = "network_audit_log.txt");

    // -----------------------------------------------------------------
    // Escritura
    // -----------------------------------------------------------------
    // Añade un evento al final del archivo (append). Si el archivo no
    // existe se crea. Devuelve true si la escritura fue correcta y false
    // si ocurrió un error de apertura/escritura (notificado por consola).
    bool append(const std::string& tipo, const std::string& mensaje);

    // -----------------------------------------------------------------
    // Lectura / visualización
    // -----------------------------------------------------------------
    // Lee el archivo de registro y vuelca su contenido por consola.
    // Devuelve true si pudo abrirlo (aunque esté vacío) y false si no
    // existe o hubo un error de lectura. Si el archivo no existe lo
    // informa en lugar de fallar en silencio.
    bool mostrar() const;

    // -----------------------------------------------------------------
    // Consultas
    // -----------------------------------------------------------------
    // Devuelve la ruta del archivo de registro configurada.
    const std::string& getRuta() const;
};

#endif
