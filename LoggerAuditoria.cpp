#include "LoggerAuditoria.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

// ---------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------

LoggerAuditoria::LoggerAuditoria(const string& ruta) : rutaArchivo(ruta) {}

// ---------------------------------------------------------------------
// Marca de tiempo
// ---------------------------------------------------------------------

// Devuelve la fecha y hora locales con formato "AAAA-MM-DD HH:MM:SS".
string LoggerAuditoria::marcaTiempoActual() const {
    time_t ahora = time(nullptr);
    tm tiempoLocal{};
#if defined(_WIN32)
    localtime_s(&tiempoLocal, &ahora);
#else
    localtime_r(&ahora, &tiempoLocal);
#endif
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tiempoLocal);
    return string(buffer);
}

// ---------------------------------------------------------------------
// Escritura (append)
// ---------------------------------------------------------------------

// Abre el archivo en modo append (ios::app): el puntero de escritura se
// sitúa al final, por lo que NUNCA se sobreescriben los registros
// previos. Si el archivo no existe, el flujo lo crea. Al escribir una
// línea con endl se fuerza el vaciado a disco (flush) de inmediato, lo
// que mantiene la traza a salvo incluso si el programa se cierra de forma
// inesperada.
bool LoggerAuditoria::append(const string& tipo, const string& mensaje) {
    ofstream salida(rutaArchivo, ios::app);
    if (!salida) {
        // Error de apertura/escritura (p. ej. sin permisos o ruta no válida).
        cerr << "[LoggerAuditoria] Error: no se pudo abrir '"
             << rutaArchivo << "' para escribir el registro.\n";
        return false;
    }

    salida << "[" << marcaTiempoActual() << "] <" << tipo << "> "
           << mensaje << "\n";

    // salida se cierra al salir del ámbito; si ocurrió un fallo de
    // escritura se detecta aquí.
    if (!salida) {
        cerr << "[LoggerAuditoria] Error: fallo al escribir en '"
             << rutaArchivo << "'.\n";
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------
// Lectura / visualización
// ---------------------------------------------------------------------

// Lee el archivo línea a línea y lo muestra por consola. Si no existe o
// no puede abrirse, lo notifica y devuelve false (no es un error crítico:
// puede que todavía no se haya registrado ningún evento).
bool LoggerAuditoria::mostrar() const {
    ifstream entrada(rutaArchivo);
    if (!entrada) {
        cout << "\n[i] El archivo de auditoria '" << rutaArchivo
             << "' aun no existe o no puede leerse.\n";
        return false;
    }

    cout << "\n" << string(48, '=') << "\n";
    cout << "   REGISTRO DE AUDITORIA (" << rutaArchivo << ")\n";
    cout << string(48, '=') << "\n";

    string linea;
    bool hayContenido = false;
    while (getline(entrada, linea)) {
        cout << linea << "\n";
        hayContenido = true;
    }

    // Detecta errores de lectura distintos del simple fin de archivo.
    if (entrada.bad()) {
        cerr << "\n[LoggerAuditoria] Error de lectura en '"
             << rutaArchivo << "'.\n";
        return false;
    }

    if (!hayContenido) {
        cout << "(El registro esta vacio.)\n";
    }
    cout << string(48, '=') << "\n";
    return true;
}

// ---------------------------------------------------------------------
// Consultas
// ---------------------------------------------------------------------

const string& LoggerAuditoria::getRuta() const { return rutaArchivo; }
