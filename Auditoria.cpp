#include "Auditoria.h"

#include <ctime>
#include <fstream>
#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------

Auditoria::Auditoria(const string& rutaArchivo) : rutaArchivo(rutaArchivo) {}

// ---------------------------------------------------------------------
// Métodos privados
// ---------------------------------------------------------------------

// Obtiene la fecha y hora locales con formato "AAAA-MM-DD HH:MM:SS".
string Auditoria::marcaTiempoActual() const {
    time_t ahora = time(nullptr);
    tm tiempoLocal{};
    // localtime_s es la variante segura en Windows/MSVC; se usa el
    // portable localtime_r cuando está disponible.
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
// Registro
// ---------------------------------------------------------------------

// Añade un evento con la marca de tiempo actual.
void Auditoria::registrar(const string& tipo, const string& mensaje) {
    eventos.push_back(EventoAuditoria{marcaTiempoActual(), tipo, mensaje});
}

// ---------------------------------------------------------------------
// Consultas
// ---------------------------------------------------------------------

size_t Auditoria::cantidadEventos() const { return eventos.size(); }

const vector<EventoAuditoria>& Auditoria::getEventos() const { return eventos; }

void Auditoria::setRutaArchivo(const string& ruta) { rutaArchivo = ruta; }

// ---------------------------------------------------------------------
// Persistencia
// ---------------------------------------------------------------------

// Vuelca la traza en la ruta configurada por defecto.
bool Auditoria::guardar() const { return guardar(rutaArchivo); }

// Vuelca la traza en el archivo indicado (lo sobreescribe).
bool Auditoria::guardar(const string& ruta) const {
    ofstream salida(ruta);
    if (!salida) {
        cerr << "[Auditoria] No se pudo abrir '" << ruta << "' para escribir.\n";
        return false;
    }
    for (const auto& evento : eventos) {
        salida << "[" << evento.marcaTiempo << "] "
               << "<" << evento.tipo << "> " << evento.mensaje << "\n";
    }
    return true;
}

// ---------------------------------------------------------------------
// Visualización / limpieza
// ---------------------------------------------------------------------

// Imprime todos los eventos por consola.
void Auditoria::mostrar() const {
    cout << "Auditoría (" << eventos.size() << " eventos)\n";
    for (const auto& evento : eventos) {
        cout << "  [" << evento.marcaTiempo << "] "
             << "<" << evento.tipo << "> " << evento.mensaje << "\n";
    }
}

// Vacía el registro.
void Auditoria::limpiar() { eventos.clear(); }
