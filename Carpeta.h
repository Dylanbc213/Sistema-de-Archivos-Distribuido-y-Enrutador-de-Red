#ifndef CARPETA_H
#define CARPETA_H

#include "NodoArchivo.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// =====================================================================
// Carpeta: nodo contenedor del sistema de archivos.
// =====================================================================
// Forma el árbol de carpetas y archivos. Es dueña de sus hijos mediante
// std::shared_ptr<NodoArchivo> (el único dueño real de cada nodo). La
// relación inversa hijo -> padre se apoya en el weak_ptr que guarda cada
// NodoArchivo, de modo que no hay ciclos de propiedad.
class Carpeta : public NodoArchivo {
private:
    std::vector<std::shared_ptr<NodoArchivo>> hijos;

    // Comprueba si ya existe un hijo con ese nombre (evita duplicados,
    // igual que un sistema de archivos real).
    bool nombreOcupado(const std::string& nombre) const;

public:
    // Constructores / destructor
    Carpeta();
    Carpeta(int id, const std::string& nombre);
    ~Carpeta() override;

    // -------- Implementación de la interfaz abstracta --------
    TipoNodo getTipo() const override;
    bool esCarpeta() const override;
    long long obtenerTamano() const override;   // suma recursiva de hijos
    void mostrar(int nivel = 0) const override;

    // -------- Gestión de hijos --------
    // Agrega un nodo, que pasa a ser propiedad de esta carpeta, y enlaza
    // su padre. Rechaza nullptr, autorreferencias y nombres duplicados.
    bool agregarHijo(const std::shared_ptr<NodoArchivo>& nodo);

    bool eliminarHijoPorNombre(const std::string& nombre);
    bool eliminarHijoPorId(int id);

    std::shared_ptr<NodoArchivo> buscarHijoPorNombre(const std::string& nombre) const;
    std::shared_ptr<NodoArchivo> buscarHijoPorId(int id) const;

    // Acceso de solo lectura a la lista de hijos.
    const std::vector<std::shared_ptr<NodoArchivo>>& getHijos() const;

    std::size_t cantidadHijos() const;
    bool estaVacia() const;
    void vaciar();
};

#endif