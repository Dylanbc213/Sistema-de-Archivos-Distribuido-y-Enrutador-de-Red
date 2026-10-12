#ifndef CONEXIONRED_H
#define CONEXIONRED_H

#include "Servidor.h"

#include <memory>

// =====================================================================
// ConexionRed: arista del grafo de la red.
// =====================================================================
// Representa un enlace dirigido entre un servidor origen y uno destino,
// con una latencia asociada (milisegundos). El conjunto de conexiones
// formará el grafo sobre el que en fases posteriores se aplicarán
// Dijkstra, BFS y DFS.
//
// Los extremos se guardan en std::shared_ptr<Servidor>: la conexión NO
// es dueña de los servidores (estos viven en el conjunto de la red), solo
// los referencia, de modo que varios enlaces pueden compartir extremos
// sin duplicarlos.
class ConexionRed {
private:
    std::shared_ptr<Servidor> origen;
    std::shared_ptr<Servidor> destino;
    int latencia;   // milisegundos

public:
    // Constructores
    ConexionRed();
    ConexionRed(const std::shared_ptr<Servidor>& origen,
                const std::shared_ptr<Servidor>& destino,
                int latencia = 0);

    // Getters
    std::shared_ptr<Servidor> getOrigen() const;
    std::shared_ptr<Servidor> getDestino() const;
    int getLatencia() const;

    // Setters (con validación básica)
    void setOrigen(const std::shared_ptr<Servidor>& origen);
    void setDestino(const std::shared_ptr<Servidor>& destino);
    void setLatencia(int latencia);

    // Utilidades
    // Indica si esta conexión involucra al servidor indicado (como origen
    // o como destino).
    bool involucra(const std::shared_ptr<Servidor>& servidor) const;

    // Imprime la conexión en el formato "origen -> destino (latencia ms)".
    void mostrar() const;

    // Igualdad por par origen/destino (dos enlaces son el mismo si unen
    // los mismos servidores, sin importar la latencia).
    bool operator==(const ConexionRed& otra) const;
    bool operator!=(const ConexionRed& otra) const;
};

#endif