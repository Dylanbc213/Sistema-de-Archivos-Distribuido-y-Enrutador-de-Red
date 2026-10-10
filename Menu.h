#ifndef MENU_H
#define MENU_H

#include "LoggerAuditoria.h"
#include "Sistema.h"

// =====================================================================
// Menu: capa de interacción por consola del Network OS.
// =====================================================================
// Separa la lógica de navegación (mostrar menús, leer y validar la
// entrada del usuario, mantener el programa en ejecución) de las clases
// del dominio (Sistema, Servidor, Usuario, GrafoRed, Auditoria...).
//
// Menu NO es dueña del Sistema ni del logger: recibe referencias a ambos
// (ya construidos por main) y se limita a ofrecer los puntos de entrada
// de cada área funcional. Registra en el archivo de auditoría el acceso
// a las opciones del menú, las entradas inválidas y el cierre del
// programa. Como las operaciones completas pertenecen a fases
// posteriores, de momento cada área se limita a mostrar que la
// funcionalidad aún no está implementada y a regresar al menú principal.
class Menu {
private:
    // Referencia al sistema sobre el que opera la interfaz. No es
    // propietaria; la vida del Sistema la gestiona main.
    Sistema& sistema;

    // Referencia al logger de auditoría persistente (archivo de texto).
    LoggerAuditoria& logger;

    // --- Renderizado de menús --------------------------------------
    void mostrarMenuPrincipal() const;
    void mostrarMenuArchivos() const;
    void mostrarMenuUsuarios() const;
    void mostrarMenuRed() const;
    void mostrarMenuAuditoria() const;

    // --- Entrada validada ------------------------------------------
    // Lee una línea de la consola y devuelve el número entero que
    // contiene. Si la entrada no es numérica, devuelve -1 para que el
    // llamador la trate como opción inválida sin romper el ciclo.
    int leerOpcion() const;

    // Lee una línea completa de la consola, la recorta de espacios en
    // blanco al inicio y al final, y la devuelve. Si la línea está vacía
    // devuelve una cadena vacía.
    std::string leerLinea() const;

    // --- Submenús (puntos de entrada de cada área) -----------------
    // Cada uno mantiene su propio ciclo hasta que el usuario elige
    // volver al menú principal.
    void menuGestionArchivos();
    void menuGestionUsuarios();
    void menuRedEnrutamiento();
    void menuAuditoria();

    // --- Utilidad --------------------------------------------------
    // Muestra un mensaje indicando que una funcionalidad concreta
    // todavía no está implementada y deja constancia en auditoría.
    void noImplementado(const std::string& funcionalidad) const;

public:
    // Construye el menú asociado a un Sistema y un logger ya inicializados.
    Menu(Sistema& s, LoggerAuditoria& log);

    // Bucle principal: dibuja el menú, lee la opción y despacha hasta
    // que el usuario elige salir (opción 0).
    void ejecutar();
};

#endif

