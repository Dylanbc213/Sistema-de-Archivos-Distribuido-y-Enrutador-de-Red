#include "GrafoRed.h"

#include <algorithm>  // reverse
#include <climits>    // INT_MAX
#include <iostream>
#include <queue>
#include <set>

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
    if (origenId == destinoId) {
        cerr << "[GrafoRed] No se permiten conexiones de un servidor consigo mismo.\n";
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

vector<int> GrafoRed::idsServidores() const {
    vector<int> ids;
    for (const auto& nodo : adyacencia) ids.push_back(nodo.first);
    return ids;
}

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

// Devuelve el nombre de un servidor por su ID, o "(sin nombre)" si no existe.
string GrafoRed::nombreDe(int id) const {
    auto it = nombres.find(id);
    if (it != nombres.end()) {
        return it->second;
    }
    return "(sin nombre)";
}

// ---------------------------------------------------------------------
// Fase 4: Enrutamiento inteligente (Dijkstra)
// ---------------------------------------------------------------------

// Calcula la ruta de mínima latencia entre origen y destino usando
// el algoritmo de Dijkstra sobre listas de adyacencia dirigidas.
RutaOptima GrafoRed::dijkstra(int origenId, int destinoId) const {
    RutaOptima vacia;
    if (!existeServidor(origenId) || !existeServidor(destinoId)) {
        return vacia;
    }
    if (origenId == destinoId) {
        vacia.camino.push_back(origenId);
        vacia.latenciaTotal = 0;
        vacia.existeCamino = true;
        return vacia;
    }

    // Distancias mínimas conocidas (inicial: infinito).
    map<int, long long> distancia;
    // Nodo predecesor en el camino óptimo.
    map<int, int> predecesor;
    // Conjunto de nodos cuya distancia definitiva ya se conoció.
    map<int, bool> visitado;

    for (const auto& par : adyacencia) {
        distancia[par.first] = LLONG_MAX;
        visitado[par.first] = false;
    }
    distancia[origenId] = 0;

    // Dijkstra clásico: en cada paso elige el nodo no visitado con menor
    // distancia conocida y relaja sus aristas salientes.
    // Complejidad O(V^2) con búsqueda lineal (suficiente para este simulador).
    int nodoActual = origenId;
    while (true) {
        // Marcar nodo actual como visitado.
        visitado[nodoActual] = true;

        // Relajar aristas salientes del nodo actual.
        const vector<Arista>* vecinosActual = vecinos(nodoActual);
        if (vecinosActual != nullptr) {
            for (const Arista& arista : *vecinosActual) {
                int v = arista.destinoId;
                int peso = arista.latencia;
                if (visitado[v]) {
                    continue;
                }
                // LLONG_MAX representa infinito; evita desbordar al sumar latencias.
                if (distancia[nodoActual] != LLONG_MAX &&
                    distancia[nodoActual] <= LLONG_MAX - peso) {
                    long long nuevaDistancia = distancia[nodoActual] + peso;
                    if (nuevaDistancia < distancia[v]) {
                        distancia[v] = nuevaDistancia;
                        predecesor[v] = nodoActual;
                    }
                }
            }
        }

        // Elegir el siguiente nodo no visitado con la distancia mínima.
        long long mejorDistancia = LLONG_MAX;
        int siguienteNodo = -1;
        for (const auto& par : distancia) {
            int id = par.first;
            if (!visitado[id] && par.second < mejorDistancia) {
                mejorDistancia = par.second;
                siguienteNodo = id;
            }
        }

        // Si no quedan nodos alcanzables por explorar, terminar.
        if (siguienteNodo == -1 || distancia[nodoActual] == LLONG_MAX) {
            break;
        }
        nodoActual = siguienteNodo;

        // Optimización: si llegamos al destino, podemos detenernos.
        // La siguiente iteracion marca el destino como visitado antes de
        // reconstruir la ruta.
    }

    // Reconstruir el camino desde destino hasta origen.
    if (!visitado[destinoId] || distancia[destinoId] == LLONG_MAX) {
        return vacia;  // No hay ruta.
    }

    vector<int> camino;
    int actual = destinoId;
    while (actual != origenId) {
        camino.push_back(actual);
        auto it = predecesor.find(actual);
        if (it == predecesor.end()) {
            return vacia;  // Camino roto (no debería ocurrir).
        }
        actual = it->second;
    }
    camino.push_back(origenId);
    reverse(camino.begin(), camino.end());

    return RutaOptima(camino, distancia[destinoId], true);
}

// Recorrido en anchura desde un servidor existente.
vector<int> GrafoRed::recorridoBFS(int inicioId) const {
    vector<int> recorrido;
    if (!existeServidor(inicioId)) return recorrido;

    map<int, bool> visitado;
    queue<int> pendientes;
    visitado[inicioId] = true;
    pendientes.push(inicioId);
    while (!pendientes.empty()) {
        int actual = pendientes.front();
        pendientes.pop();
        recorrido.push_back(actual);
        const vector<Arista>* lista = vecinos(actual);
        if (!lista) continue;
        for (const Arista& arista : *lista) {
            if (existeServidor(arista.destinoId) && !visitado[arista.destinoId]) {
                visitado[arista.destinoId] = true;
                pendientes.push(arista.destinoId);
            }
        }
    }
    return recorrido;
}

// Recorrido en profundidad iterativo desde un servidor existente.
vector<int> GrafoRed::recorridoDFS(int inicioId) const {
    vector<int> recorrido;
    if (!existeServidor(inicioId)) return recorrido;

    map<int, bool> visitado;
    vector<int> pendientes(1, inicioId);
    while (!pendientes.empty()) {
        int actual = pendientes.back();
        pendientes.pop_back();
        if (visitado[actual]) continue;
        visitado[actual] = true;
        recorrido.push_back(actual);
        const vector<Arista>* lista = vecinos(actual);
        if (!lista) continue;
        // Se apilan en orden inverso para respetar el orden de adyacencia.
        for (auto it = lista->rbegin(); it != lista->rend(); ++it) {
            if (existeServidor(it->destinoId) && !visitado[it->destinoId])
                pendientes.push_back(it->destinoId);
        }
    }
    return recorrido;
}

// Aislado significa sin enlaces entrantes ni salientes.
vector<int> GrafoRed::servidoresAislados() const {
    map<int, bool> conectado;
    for (const auto& nodo : adyacencia) conectado[nodo.first] = false;
    for (const auto& nodo : adyacencia) {
        if (!nodo.second.empty()) conectado[nodo.first] = true;
        for (const Arista& arista : nodo.second) {
            auto destino = conectado.find(arista.destinoId);
            if (destino != conectado.end()) destino->second = true;
        }
    }
    vector<int> aislados;
    for (const auto& nodo : conectado)
        if (!nodo.second) aislados.push_back(nodo.first);
    return aislados;
}

// Revisa enlaces mal formados aun cuando la API normal los rechaza.
vector<string> GrafoRed::validarConexiones() const {
    vector<string> problemas;
    for (const auto& nodo : adyacencia) {
        set<int> destinos;
        for (const Arista& arista : nodo.second) {
            if (!existeServidor(arista.destinoId))
                problemas.push_back("Enlace #" + to_string(nodo.first) +
                    " -> #" + to_string(arista.destinoId) + " apunta a un servidor inexistente.");
            if (arista.latencia < 0)
                problemas.push_back("Enlace #" + to_string(nodo.first) +
                    " -> #" + to_string(arista.destinoId) + " tiene latencia negativa.");
            if (!destinos.insert(arista.destinoId).second)
                problemas.push_back("Enlace duplicado desde el servidor #" + to_string(nodo.first) +
                    " hacia #" + to_string(arista.destinoId) + ".");
            if (arista.destinoId == nodo.first)
                problemas.push_back("El servidor #" + to_string(nodo.first) + " tiene un enlace a si mismo.");
        }
    }
    return problemas;
}

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
