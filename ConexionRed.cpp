#include "ConexionRed.h"

#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Constructores
// ---------------------------------------------------------------------

// Constructor por defecto: enlace nulo sin latencia.
ConexionRed::ConexionRed() : origen(nullptr), destino(nullptr), latencia(0) {}

// Constructor principal: fija extremos y latencia (no negativa).
ConexionRed::ConexionRed(const shared_ptr<Servidor>& origen,
                         const shared_ptr<Servidor>& destino,
                         int latencia)
    : origen(origen), destino(destino), latencia(latencia < 0 ? 0 : latencia) {}

// ---------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------

// Devuelve el servidor origen del enlace.
shared_ptr<Servidor> ConexionRed::getOrigen() const { return origen; }

// Devuelve el servidor destino del enlace.
shared_ptr<Servidor> ConexionRed::getDestino() const { return destino; }

// Devuelve la latencia del enlace en milisegundos.
int ConexionRed::getLatencia() const { return latencia; }

// ---------------------------------------------------------------------
// Setters (con validación básica)
// ---------------------------------------------------------------------

// Asigna el servidor origen (rechaza puntero nulo).
void ConexionRed::setOrigen(const shared_ptr<Servidor>& nuevoOrigen) {
    if (!nuevoOrigen) {
        cerr << "[ConexionRed] El origen no puede ser nulo. No se aplicó.\n";
        return;
    }
    origen = nuevoOrigen;
}

// Asigna el servidor destino (rechaza puntero nulo).
void ConexionRed::setDestino(const shared_ptr<Servidor>& nuevoDestino) {
    if (!nuevoDestino) {
        cerr << "[ConexionRed] El destino no puede ser nulo. No se aplicó.\n";
        return;
    }
    destino = nuevoDestino;
}

// Asigna la latencia (rechaza valores negativos).
void ConexionRed::setLatencia(int nuevaLatencia) {
    if (nuevaLatencia < 0) {
        cerr << "[ConexionRed] La latencia no puede ser negativa. No se aplicó.\n";
        return;
    }
    latencia = nuevaLatencia;
}

// ---------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------

// Indica si el servidor dado es el origen o el destino de esta conexión.
bool ConexionRed::involucra(const shared_ptr<Servidor>& servidor) const {
    if (!servidor) {
        return false;
    }
    return (origen && *origen == *servidor) || (destino && *destino == *servidor);
}

// Imprime la conexión con sus extremos y su latencia.
void ConexionRed::mostrar() const {
    cout << "Conexion: "
         << (origen ? origen->getNombre() : string("(nulo)"))
         << " -> "
         << (destino ? destino->getNombre() : string("(nulo)"))
         << " (" << latencia << " ms)\n";
}

// Igualdad por par origen/destino, comparando por ID de servidor.
bool ConexionRed::operator==(const ConexionRed& otra) const {
    const bool mismoOrigen =
        (origen && otra.origen) ? (*origen == *otra.origen) : (origen == otra.origen);
    const bool mismoDestino =
        (destino && otra.destino) ? (*destino == *otra.destino) : (destino == otra.destino);
    return mismoOrigen && mismoDestino;
}

// Negación de la igualdad por par origen/destino.
bool ConexionRed::operator!=(const ConexionRed& otra) const {
    return !(*this == otra);
}