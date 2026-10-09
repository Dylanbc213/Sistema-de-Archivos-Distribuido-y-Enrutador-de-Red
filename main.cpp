#include <iostream>
#include <memory>

#include "LoggerAuditoria.h"
#include "Menu.h"
#include "Sistema.h"

using namespace std;

// =====================================================================
// main: punto de entrada del Network OS.
// =====================================================================
// Compone los tres objetos principales (logger de auditoría, sistema y
// menú) y delega la interaccion por consola en la clase Menu. Toda la
// logica de navegacion vive en Menu; main solo orquesta el arranque, el
// registro de auditoria y el apagado.
int main() {
    // 1) Logger de auditoria persistente: escribe en network_audit_log.txt
    //    usando modo append (no borra registros de ejecuciones previas).
    LoggerAuditoria logger("network_audit_log.txt");

    // 2) Evento inicial: arranque del sistema.
    logger.append("SISTEMA", "Inicio del sistema Network OS.");

    // 3) Sistema gestiona servidores, usuarios, grafo y auditoria en
    //    memoria (arboles, tabla hash, grafo de red y traza volatil).
    Sistema so;

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
