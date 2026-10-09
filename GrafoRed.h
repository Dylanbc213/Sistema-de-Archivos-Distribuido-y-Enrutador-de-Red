#ifndef GRAFODERED_H
#define GRAFODERED_H

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

// =====================================================================
// GrafoRed: grafo de la red modelado con listas de adyacencia.
// =====================================================================
// Los nodos son los servidores (identificados por su ID) y las aristas
// son las conexiones con su latencia en milisegundos. Se usa una lista de
// adyacencia: para cada servidor se guarda la lista de sus vecinos junto
// con el peso (latencia) del enlace.
//
// Para no obligar a compartir la propiedad de los servidores (que
// administra la clase central con unique_ptr), el grafo referencia a los
// servidores únicamente por su ID y guarda su nombre para poder
// imprimirse. Así convive con cualquier esquema de propiedad.
//
// Dijkstra, BFS y DFS se implementarán en fases posteriores; aquí solo se
// construye y consulta la topología.
struct Arista {
    int destinoId;   // ID del servidor vecino
    int latencia;    // latencia del enlace en milisegundos

    Arista(int destinoId, int latencia)
        : destinoId(destinoId), latencia(latencia < 0 ? 0 : latencia) {}
};

class GrafoRed {
private:
    // Lista de adyacencia: ID de servidor -> lista de (vecino, latencia).
    std::map<int, std::vector<Arista>> adyacencia;

    // Nombres de los servidores registrados (ID -> nombre), solo para
    // poder mostrar el grafo sin depender de la clase Servidor.
    std::map<int, std::string> nombres;

    // Indica si existe un enlace entre dos servidores (recorre la lista de
    // adyacencia del origen).
    bool existeArista(int origenId, int destinoId) const;

public:
    // Registra un servidor como nodo del grafo. Devuelve false si el ID es
    // negativo o si ya existía.
    bool agregarServidor(int id, const std::string& nombre);

    // Elimina un servidor y todas las aristas que lo referencian.
    bool eliminarServidor(int id);

    // Añade un enlace dirigido origen -> destino con su latencia. Devuelve
    // false si algún extremo no existe, si el enlace ya existe o si la
    // latencia es negativa.
    bool agregarArista(int origenId, int destinoId, int latencia);

    // Elimina el enlace origen -> destino.
    bool eliminarArista(int origenId, int destinoId);

    // Consultas.
    bool existeServidor(int id) const;
    std::size_t cantidadServidores() const;
    std::size_t cantidadAristas() const;

    // Devuelve la lista de adyacencia (vecinos + latencia) de un servidor.
    // El vector está vacío si el servidor no existe.
    const std::vector<Arista>* vecinos(int id) const;

    // Latencia del enlace origen -> destino, o -1 si no existe.
    int latenciaEntre(int origenId, int destinoId) const;

    // Imprime la lista de adyacencia del grafo completo.
    void mostrar() const;
};

#endif
