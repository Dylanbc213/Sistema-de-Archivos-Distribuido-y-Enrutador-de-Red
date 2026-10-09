#ifndef TABLAHASHUSUARIOS_H
#define TABLAHASHUSUARIOS_H

#include "Usuario.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

// =====================================================================
// TablaHashUsuarios: tabla hash propia para almacenar usuarios.
// =====================================================================
// Reemplaza el uso de std::unordered_map en la tabla principal: la
// estructura se construye manualmente con un std::vector de buckets y se
// resuelven las colisiones mediante encadenamiento separado (cada bucket
// es, a su vez, un vector de usuarios que comparten el mismo índice).
//
// La clave de búsqueda es el nombre del usuario (debe ser único), y la
// función de dispersión se apoya en std::hash<std::string>. En fases
// posteriores se añadirá la autenticación sobre esta tabla.
class TablaHashUsuarios {
private:
    // Buckets: cada posición guarda la lista de usuarios que colisionaron
    // en ese índice (encadenamiento separado). Es la "tabla principal".
    std::vector<std::vector<Usuario>> buckets;

    // Cantidad de usuarios almacenados (para calcular el factor de carga
    // sin recorrer todos los buckets).
    std::size_t cantidad;

    // Calcula el índice de bucket para un nombre usando la función hash.
    std::size_t indiceDe(const std::string& nombre) const;

    // Busca la posición de un usuario por nombre dentro de su bucket.
    // Devuelve el índice en el bucket o -1 (como std::size_t máximo) si no
    // existe. Se usa internamente por insertar/buscar/eliminar.
    std::size_t posicionEnBucket(const std::string& nombre,
                                 std::size_t bucketIdx) const;

public:
    // Constructor: prepara 'numBuckets' buckets vacíos.
    explicit TablaHashUsuarios(std::size_t numBuckets = 16);

    // Inserta un usuario. Devuelve false si ya existe uno con ese nombre
    // (no se permiten duplicados por nombre).
    bool insertar(const Usuario& usuario);

    // Busca por nombre. Devuelve un puntero NO propietario al usuario o
    // nullptr si no existe. El puntero se invalida si después se inserta,
    // elimina o se hace rehash.
    Usuario* buscar(const std::string& nombre);
    const Usuario* buscar(const std::string& nombre) const;

    // Elimina el usuario con ese nombre. Devuelve true si se eliminó.
    bool eliminar(const std::string& nombre);

    // Indica si existe un usuario con ese nombre.
    bool contiene(const std::string& nombre) const;

    // Consultas de estado.
    std::size_t tamano() const;                 // usuarios almacenados
    std::size_t numBuckets() const;             // buckets totales
    double factorCarga() const;                 // tamano / numBuckets
    std::size_t colisionesEn(std::size_t idx) const;  // tamaño de un bucket

    // Redistribuye todos los usuarios en un nuevo número de buckets.
    void rehash(std::size_t nuevoNumBuckets);

    // Imprime el contenido bucket por bucket (con marcas de colisión).
    void mostrar() const;
};

#endif
