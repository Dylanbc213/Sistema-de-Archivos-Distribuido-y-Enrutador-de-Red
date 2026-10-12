#include <iostream>
#include <memory>
#include <cstdlib>
#include <filesystem>

#include "LoggerAuditoria.h"
#include "Menu.h"
#include "Sistema.h"
#include "TablaHashUsuarios.h"

using namespace std;

// =====================================================================
// main: punto de entrada del Network OS.
// =====================================================================
// Compone los tres objetos principales (logger de auditoría, sistema y
// menú) y delega la interaccion por consola en la clase Menu. Toda la
// logica de navegacion vive en Menu; main solo orquesta el arranque, el
// registro de auditoria y el apagado.
int main() {
    const std::string directorioDatos = Sistema::directorioDatosLocal();
    if (directorioDatos.empty()) {
        std::cerr << "[ERROR] LOCALAPPDATA no está disponible; no se iniciará con almacenamiento local volátil.\n";
        return 1;
    }
    std::error_code errorDirectorio;
    std::filesystem::create_directories(directorioDatos, errorDirectorio);
    if (errorDirectorio) {
        std::cerr << "[ERROR] No se pudo preparar el directorio de datos '"
                  << directorioDatos << "': " << errorDirectorio.message() << "\n";
        return 1;
    }
    // 1) Logger de auditoria persistente: escribe en network_audit_log.txt
    //    usando modo append (no borra registros de ejecuciones previas).
    const std::filesystem::path rutaDatos(directorioDatos);
    LoggerAuditoria logger((rutaDatos / "network_audit_log.txt").string());

    // 2) Evento inicial: arranque del sistema.
    logger.append("SISTEMA", "Inicio del sistema Network OS.");

    // 3) Sistema gestiona servidores, usuarios, grafo y auditoria en
    //    memoria (arboles, tabla hash, grafo de red y traza volatil).
    Sistema so(16, (rutaDatos / "auditoria.log").string(), &logger);

    // El administrador inicial se configura explícitamente fuera del código.
    // Solo provisiona un ADMIN si el almacenamiento persistente aún no tiene uno.
    const char* adminNombre = std::getenv("NETWORK_OS_ADMIN_USER");
    const char* adminContrasena = std::getenv("NETWORK_OS_ADMIN_PASSWORD");
    if (so.getUsuarios().cantidadAdministradores() > 0) {
        cout << "Se reutilizaron las cuentas ADMIN guardadas.\n";
    } else if (adminNombre != nullptr && adminContrasena != nullptr &&
        adminNombre[0] != '\0' && adminContrasena[0] != '\0') {
        if (so.asegurarAdministradorInicial(0, adminNombre, adminContrasena)) {
            cout << "Administrador inicial persistido: " << adminNombre << " (ADMIN).\n";
        } else {
            cout << "[!] No se pudo provisionar el administrador inicial; revise la configuracion.\n";
        }
    } else {
        cout << "[!] Configure NETWORK_OS_ADMIN_USER y NETWORK_OS_ADMIN_PASSWORD "
                "para provisionar el administrador inicial.\n";
    }

    // 4) Evento: estructuras internas inicializadas.
    logger.append("SISTEMA",
                  "Inicializacion de las estructuras "
                  "(arboles de directorios, tabla hash de usuarios, "
                  "grafo de red y auditoria).");

    // 5) Menu ofrece el menu principal y los submenus de cada area.
    Menu menu(so, logger);
    menu.ejecutar();

    // 6) Al salir del menu, vuelca la auditoria en memoria a su archivo.
    if (so.guardarAuditoria()) {
        cout << "Auditoria guardada en disco correctamente.\n";
    }

    return 0;
}
