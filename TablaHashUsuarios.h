#ifndef TABLAHASHUSUARIOS_H
#define TABLAHASHUSUARIOS_H

#include "Usuario.h"

#include <cstddef>
#include <string>
#include <vector>

// =====================================================================
// TablaHashUsuarios: tabla hash PROPIA para almacenar usuarios.
// =====================================================================
// Construida desde cero (NO std::unordered_map / std::map) con un ARREGLO
// DINAMICO de listas simples enlazadas: cada bucket es una lista simplemente
// enlazada de nodos (NodoLista) cuyo unico dueno es la tabla. Las colisiones
// se resuelven por encadenamiento separado (chaining).
//
// La clave de busqueda es el nombre del usuario (debe ser unico) y la
// funcion de dispersion es una funcion HASH PERSONALIZADA (polinomio
// generador modular), NO std::hash. La tabla inserta/busca/elimina en
// tiempo O(1) promedio; el peor caso (todas las claves colisionan en un
// mismo bucket) es O(n), aunque con una buena funcion hash y un factor de
// carga controlado se mantiene constante en la practica.
//
// La tabla administra la memoria de los nodos con new/delete: el
// destructor libera cada lista enlazada y el arreglo de buckets, sin fugas.
class TablaHashUsuarios {
private:
    // -----------------------------------------------------------------
    // Nodo de la lista enlazada de cada bucket (encadenamiento).
    // 'dato' almacena el Usuario POR VALOR; al destruir el nodo se liberan
    // los std::string internos del propio Usuario. 'siguiente' apunta al
    // nodo siguiente del mismo bucket (nullptr si es el ultimo).
    // -----------------------------------------------------------------
    struct NodoLista {
        Usuario dato;
        NodoLista* siguiente;
        NodoLista(const Usuario& u, NodoLista* sig)
            : dato(u), siguiente(sig) {}
    };

    // Arreglo dinamico de cabezas de lista (uno por bucket). Se reserva con
    // new[] y se libera con delete[] en el destructor.
    NodoLista** buckets;

    std::size_t numBuckets_;   // capacidad actual (tam. del arreglo de buckets)
    std::size_t cantidad;      // numero de usuarios almacenados

    // -----------------------------------------------------------------
    // Funcion hash PERSONALIZADA.
    // Deriva de un polinomio generador modular: h = (h*P + c) % MOD con
    // P = 131 (multiplicador) y MOD = 1000000007 (primo grande). No usa
    // std::hash ni ninguna libreria estandar de hashing. El resultado se
    // reduce modulo numBuckets_ en indiceDe().
    // -----------------------------------------------------------------
    std::size_t hashPersonal(const std::string& nombre) const;

    // Indice de bucket para un nombre: hashPersonal % numBuckets_.
    std::size_t indiceDe(const std::string& nombre) const;

    // Busca el NodoLista cuyo nombre coincide, devolviendo el nodo y su
    // nodo anterior (para poder desenlazar en O(1)). Si no lo encuentra
    // devuelve nullptr en ambos.
    NodoLista* buscarNodo(const std::string& nombre,
                          std::size_t& bucketIdx,
                          NodoLista*& anterior) const;

public:
    // Constructor: genera 'numBuckets' buckets vacios (>=1 siempre).
    explicit TablaHashUsuarios(std::size_t numBuckets = 16);

    // Destructor: libera TODOS los nodos de TODAS las listas y el arreglo.
    ~TablaHashUsuarios();

    // No se permiten copias/movimientos: poseen memoria con new[] que no se
    // copia trivialmente, y Sistema la administra con unique_ptr.
    TablaHashUsuarios(const TablaHashUsuarios&) = delete;
    TablaHashUsuarios& operator=(const TablaHashUsuarios&) = delete;
    TablaHashUsuarios(TablaHashUsuarios&&) = delete;
    TablaHashUsuarios& operator=(TablaHashUsuarios&&) = delete;

    // Inserta un usuario (en cabeza del bucket correspondiente, O(1)).
    // Devuelve false si ya existe uno con ese nombre (no hay duplicados).
    bool insertar(const Usuario& usuario);

    // Busca por nombre. Devuelve un puntero NO propietario al usuario o
    // nullptr si no existe. El puntero se invalida si luego se inserta,
    // elimina o se hace rehash.
    Usuario* buscar(const std::string& nombre);
    const Usuario* buscar(const std::string& nombre) const;

    // Autenticacion: localiza al usuario por nombre (O(1) promedio via hash)
    // y valida la contrasena con verificarCredenciales. Devuelve el puntero
    // al usuario autenticado o nullptr (nombre inexistente o contrasena
    // incorrecta). No realiza auditoria: eso la realiza la capa Sistema.
    Usuario* verificarSesion(const std::string& nombre,
                             const std::string& contrasena);

    // Elimina el usuario con ese nombre del bucket correspondiente.
    bool eliminar(const std::string& nombre);

    // Indica si existe un usuario con ese nombre.
    bool contiene(const std::string& nombre) const;
    // Indica si ya existe un usuario con este identificador.
    bool contieneId(int id) const;
    std::size_t cantidadAdministradores() const;
    std::vector<Usuario> obtenerTodos() const;

    // Consultas de estado.
    std::size_t tamano() const;                          // usuarios almacenados
    std::size_t numBuckets() const;                    // buckets totales
    double factorCarga() const;                        // tamano / numBuckets_
    std::size_t colisionesEn(std::size_t idx) const;   // nodos en un bucket

    // Reconstruye la tabla con otro numero de buckets y vuelve a insertar
    // todos los usuarios recalculando sus indices (rehashing manual).
    void rehash(std::size_t nuevoNumBuckets);

    // Imprime el contenido bucket por bucket; marca con '!' los buckets con
    // mas de un nodo (colisiones resueltas por encadenamiento).
    void mostrar() const;
};

#endif
