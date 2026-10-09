#include "TablaHashUsuarios.h"

#include <iostream>
#include <limits>

using namespace std;

// Constante centinela para "no encontrado" al buscar dentro de un bucket.
static const size_t NO_ENCONTRADO = numeric_limits<size_t>::max();

// ---------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------

// Crea 'numBuckets' buckets vacíos. Se fuerza un mínimo de 1 para evitar
// un vector de tamaño cero (que haría fallar el módulo del hash).
TablaHashUsuarios::TablaHashUsuarios(size_t numBuckets)
    : buckets(numBuckets < 1 ? 1 : numBuckets), cantidad(0) {}

// ---------------------------------------------------------------------
// Métodos privados
// ---------------------------------------------------------------------

// Dispersa el nombre a un índice válido del vector de buckets.
size_t TablaHashUsuarios::indiceDe(const string& nombre) const {
    hash<string> hasher;
    return hasher(nombre) % buckets.size();
}

// Devuelve la posición del usuario dentro de su bucket, o NO_ENCONTRADO.
size_t TablaHashUsuarios::posicionEnBucket(const string& nombre,
                                           size_t bucketIdx) const {
    if (bucketIdx >= buckets.size()) {
        return NO_ENCONTRADO;
    }
    const vector<Usuario>& lista = buckets[bucketIdx];
    for (size_t i = 0; i < lista.size(); ++i) {
        if (lista[i].getNombre() == nombre) {
            return i;
        }
    }
    return NO_ENCONTRADO;
}

// ---------------------------------------------------------------------
// Inserción / búsqueda / eliminación
// ---------------------------------------------------------------------

// Agrega un usuario en el bucket que le corresponde. Rechaza nombres ya
// presentes para mantener la unicidad de la clave.
bool TablaHashUsuarios::insertar(const Usuario& usuario) {
    size_t idx = indiceDe(usuario.getNombre());
    if (posicionEnBucket(usuario.getNombre(), idx) != NO_ENCONTRADO) {
        cerr << "[TablaHash] Ya existe el usuario '" << usuario.getNombre()
             << "'. No se insertó.\n";
        return false;
    }
    buckets[idx].push_back(usuario);
    ++cantidad;
    return true;
}

// Devuelve un puntero al usuario con ese nombre, o nullptr.
Usuario* TablaHashUsuarios::buscar(const string& nombre) {
    size_t idx = indiceDe(nombre);
    size_t pos = posicionEnBucket(nombre, idx);
    if (pos == NO_ENCONTRADO) {
        return nullptr;
    }
    return &buckets[idx][pos];
}

// Versión constante de buscar.
const Usuario* TablaHashUsuarios::buscar(const string& nombre) const {
    size_t idx = indiceDe(nombre);
    size_t pos = posicionEnBucket(nombre, idx);
    if (pos == NO_ENCONTRADO) {
        return nullptr;
    }
    return &buckets[idx][pos];
}

// Elimina el usuario con ese nombre del bucket correspondiente.
bool TablaHashUsuarios::eliminar(const string& nombre) {
    size_t idx = indiceDe(nombre);
    size_t pos = posicionEnBucket(nombre, idx);
    if (pos == NO_ENCONTRADO) {
        return false;
    }
    buckets[idx].erase(buckets[idx].begin() + static_cast<long>(pos));
    --cantidad;
    return true;
}

// Comprueba existencia por nombre.
bool TablaHashUsuarios::contiene(const string& nombre) const {
    return posicionEnBucket(nombre, indiceDe(nombre)) != NO_ENCONTRADO;
}

// ---------------------------------------------------------------------
// Consultas de estado
// ---------------------------------------------------------------------

size_t TablaHashUsuarios::tamano() const { return cantidad; }

size_t TablaHashUsuarios::numBuckets() const { return buckets.size(); }

// Factor de carga = elementos / buckets. Indica cuántas colisiones cabría
// esperar en promedio por bucket.
double TablaHashUsuarios::factorCarga() const {
    return buckets.empty() ? 0.0
                           : static_cast<double>(cantidad) / buckets.size();
}

// Número de usuarios alojados en un bucket concreto.
size_t TablaHashUsuarios::colisionesEn(size_t idx) const {
    return idx < buckets.size() ? buckets[idx].size() : 0;
}

// ---------------------------------------------------------------------
// Rehash
// ---------------------------------------------------------------------

// Reconstruye la tabla con otro número de buckets y vuelve a insertar
// todos los usuarios recalculando sus índices.
void TablaHashUsuarios::rehash(size_t nuevoNumBuckets) {
    if (nuevoNumBuckets < 1) {
        nuevoNumBuckets = 1;
    }
    vector<vector<Usuario>> nuevos(nuevoNumBuckets);
    for (const auto& bucket : buckets) {
        for (const auto& usuario : bucket) {
            hash<string> hasher;
            size_t idx = hasher(usuario.getNombre()) % nuevos.size();
            nuevos[idx].push_back(usuario);
        }
    }
    buckets.swap(nuevos);
}

// ---------------------------------------------------------------------
// Visualización
// ---------------------------------------------------------------------

// Imprime cada bucket con sus usuarios; marca con '!' los buckets con más
// de un elemento (es decir, donde hubo colisiones resueltas por cadena).
void TablaHashUsuarios::mostrar() const {
    cout << "TablaHash de usuarios (" << cantidad << " elementos, "
         << buckets.size() << " buckets, carga "
         << factorCarga() << ")\n";
    for (size_t i = 0; i < buckets.size(); ++i) {
        if (buckets[i].empty()) {
            continue;
        }
        bool colision = buckets[i].size() > 1;
        cout << "  [" << i << "]" << (colision ? "! " : "  ");
        for (size_t j = 0; j < buckets[i].size(); ++j) {
            cout << (j ? " -> " : "") << buckets[i][j].getNombre();
        }
        cout << "\n";
    }
}
