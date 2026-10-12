#ifndef SERVIDOR_H
#define SERVIDOR_H

#include "Carpeta.h"

#include <memory>
#include <string>

// Forward declaration: Archivo se referencia en crearArchivo() sin
// necesidad de incluir su encabezado completo en este archivo.
class Archivo;

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

            // --- Utilidades ---
    // Imprime el servidor y, debajo, su árbol de archivos.
    void mostrar() const;

    // --- Fase 2: Operaciones de sistema de archivos sobre la raíz ---
    // Delegan en la carpeta raíz del servidor. Las rutas pueden
    // ser absolutas ('/docs/reportes/archivo.txt') o relativas.

    // Búsqueda por ruta (navegación jerárquica desde la raíz).
    std::shared_ptr<NodoArchivo> buscarPorRuta(const std::string& ruta) const;

    // Búsqueda recursiva (DFS) por nombre en todo el árbol.
    std::shared_ptr<NodoArchivo> buscarRecursivoPorNombre(
        const std::string& nombre) const;

    // Crea una subcarpeta en la ruta indicada (crea intermedias si falta).
    std::shared_ptr<Carpeta> crearSubdirectorio(const std::string& ruta);

    // Crea un archivo en la ruta indicada (crea intermedias si falta).
    std::shared_ptr<Archivo> crearArchivo(const std::string& ruta,
                                          const std::string& contenido = "");

    // Elimina recursivamente un nodo por ruta (cascada si es carpeta).
    bool eliminarRecursivo(const std::string& ruta);

    // Muestra el árbol de directorios a partir de la raíz.
    void mostrarArbol() const;

    // Igualdad por ID (identidad del servidor en la red).
    bool operator==(const Servidor& otro) const;
    bool operator!=(const Servidor& otro) const;
};

#endif