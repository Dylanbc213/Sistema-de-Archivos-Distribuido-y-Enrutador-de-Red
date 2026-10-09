#include "GrafoRed.h"

#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Gestión de nodos (servidores)
// ---------------------------------------------------------------------

// Registra un servidor como nodo. Rechaza IDs negativos y duplicados.
bool GrafoRed::agregarServidor(int id, const string& nombre) {
    if (id < 0) {
        cerr << "[GrafoRed] ID de servidor inválido (negativo). No se agregó.\n";
        return false;
    }
    if (existeServidor(id)) {
        cerr << "[GrafoRed] El servidor #" << id << " ya existe.\n";
        return false;
    }
    adyacencia[id];                 // crea la lista de adyacencia vacía
    nombres[id] = nombre;
    return true;
}

// Elimina un servidor y todas las aristas que apuntan a él.
bool GrafoRed::eliminarServidor(int id) {
    if (!existeServidor(id)) {
        return false;
    }
    // Borra las aristas entrantes (en las listas de otros servidores).
    for (auto& par : adyacencia) {
        auto& lista = par.second;
        for (auto it = lista.begin(); it != lista.end();) {
            if (it->destinoId == id) {
                it = lista.erase(it);
            } else {
                ++it;
            }
        }
    }
    adyacencia.erase(id);
    nombres.erase(id);
    return true;
}

// ---------------------------------------------------------------------
// Gestión de aristas (conexiones)
// ---------------------------------------------------------------------

// Comprueba si ya hay un enlace origen -> destino.
bool GrafoRed::existeArista(int origenId, int destinoId) const {
    auto it = adyacencia.find(origenId);
    if (it == adyacencia.end()) {
        return false;
    }
    for (const auto& arista : it->second) {
        if (arista.destinoId == destinoId) {
            return true;
        }
    }
    return false;
}

// Añade un enlace dirigido con latencia. Valida existencia de extremos,
// ausencia de duplicado y latencia no negativa.
bool GrafoRed::agregarArista(int origenId, int destinoId, int latencia) {
    if (!existeServidor(origenId) || !existeServidor(destinoId)) {
        cerr << "[GrafoRed] Ambos extremos deben existir para conectar.\n";
        return false;
    }
    if (latencia < 0) {
        cerr << "[GrafoRed] La latencia no puede ser negativa. No se agregó.\n";
        return false;
    }
    if (existeArista(origenId, destinoId)) {
        cerr << "[GrafoRed] Ya existe el enlace " << origenId << " -> "
             << destinoId << ".\n";
        return false;
    }
    adyacencia[origenId].emplace_back(destinoId, latencia);
    return true;
}

// Elimina el enlace origen -> destino si existe.
bool GrafoRed::eliminarArista(int origenId, int destinoId) {
    auto it = adyacencia.find(origenId);
    if (it == adyacencia.end()) {
        return false;
    }
    auto& lista = it->second;
    for (auto arista = lista.begin(); arista != lista.end(); ++arista) {
        if (arista->destinoId == destinoId) {
            lista.erase(arista);
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------
// Consultas
// ---------------------------------------------------------------------

bool GrafoRed::existeServidor(int id) const {
    return adyacencia.find(id) != adyacencia.end();
}

std::size_t GrafoRed::cantidadServidores() const { return adyacencia.size(); }

// Cuenta el total de aristas sumando el tamaño de todas las listas.
std::size_t GrafoRed::cantidadAristas() const {
    std::size_t total = 0;
    for (const auto& par : adyacencia) {
        total += par.second.size();
    }
    return total;
}

// Devuelve la lista de vecinos de un servidor, o nullptr si no existe.
const std::vector<Arista>* GrafoRed::vecinos(int id) const {
    auto it = adyacencia.find(id);
    if (it == adyacencia.end()) {
        return nullptr;
    }
    return &it->second;
}

// Latencia de un enlace concreto, o -1 si no existe.
int GrafoRed::latenciaEntre(int origenId, int destinoId) const {
    auto it = adyacencia.find(origenId);
    if (it == adyacencia.end()) {
        return -1;
    }
    for (const auto& arista : it->second) {
        if (arista.destinoId == destinoId) {
            return arista.latencia;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------
// Visualización
// ---------------------------------------------------------------------

// Imprime cada servidor con sus conexiones salientes y sus latencias.
void GrafoRed::mostrar() const {
    cout << "Grafo de red (" << adyacencia.size() << " servidores, "
         << cantidadAristas() << " aristas)\n";
    for (const auto& par : adyacencia) {
        int id = par.first;
        string nombre = nombres.count(id) ? nombres.at(id) : "(sin nombre)";
        cout << "  #" << id << " \"" << nombre << "\" ->";
        if (par.second.empty()) {
            cout << " (sin conexiones)";
        }
        for (const auto& arista : par.second) {
            string destino = nombres.count(arista.destinoId)
                                 ? nombres.at(arista.destinoId)
                                 : "(?)";
            cout << " #" << arista.destinoId << " \"" << destino << "\"("
                 << arista.latencia << "ms)";
        }
        cout << "\n";
    }
}
