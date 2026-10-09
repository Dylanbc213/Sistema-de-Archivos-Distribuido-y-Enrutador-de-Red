#ifndef SERVIDOR_H
#define SERVIDOR_H

#include "Carpeta.h"

#include <memory>
#include <string>

// =====================================================================
// Servidor: nodo físico de la red.
// =====================================================================
// Cada servidor tiene un ID, un nombre y una carpeta raíz que es la
// entrada a su árbol de archivos. En fases posteriores los servidores se
// enlazarán entre sí mediante ConexionRed (grafo + Dijkstra/BFS/DFS).
//
// La raíz se guarda en std::shared_ptr<Carpeta>: el servidor es su
// dueño. Si no se entrega una raíz explícita, se crea una carpeta raíz
// por defecto con el nombre del servidor.
class Servidor {
private:
    int id;
    std::string nombre;
    std::shared_ptr<Carpeta> raiz;

public:
    // Constructores
    Servidor();
    Servidor(int id, const std::string& nombre);
    Servidor(int id, const std::string& nombre,
             const std::shared_ptr<Carpeta>& raiz);

    // Getters
    int getId() const;
    std::string getNombre() const;
    std::shared_ptr<Carpeta> getRaiz() const;

    // Setters (con validación básica: no aplican valores inválidos)
    void setId(int id);
    void setNombre(const std::string& nombre);
    void setRaiz(const std::shared_ptr<Carpeta>& nuevaRaiz);

    // Utilidades
    void mostrar() const;   // imprime el servidor y su árbol de archivos

    // Igualdad por ID (identidad del servidor en la red).
    bool operator==(const Servidor& otro) const;
    bool operator!=(const Servidor& otro) const;
};

#endif