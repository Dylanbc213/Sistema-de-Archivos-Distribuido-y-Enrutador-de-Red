#include "TablaHashUsuarios.h"

#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------

// Genera 'numBuckets' buckets vacios. Se fuerza un minimo de 1 para evitar
// un arreglo de tam. cero (que haria indefinido el modulo del hash) y para
// que delete[] nunca opere sobre un arreglo imposible de dimensionar.
TablaHashUsuarios::TablaHashUsuarios(size_t numBuckets)
    : buckets(nullptr), numBuckets_(numBuckets < 1 ? 1 : numBuckets), cantidad(0) {
    buckets = new NodoLista*[numBuckets_];
    for (size_t i = 0; i < numBuckets_; ++i) {
        buckets[i] = nullptr;  // bucket vacio
    }
}

// ---------------------------------------------------------------------
// Destructor
// ---------------------------------------------------------------------

// Libera la lista enlazada de cada bucket (delete nodo a nodo) y luego el
// arreglo de cabezas (delete[]). Al borrar cada NodoLista, su miembro 'dato'
// (Usuario, con std::string) se destruye y libera su propia memoria. No hay
// fugas ni nodos huellas.
TablaHashUsuarios::~TablaHashUsuarios() {
    for (size_t i = 0; i < numBuckets_; ++i) {
        NodoLista* nodo = buckets[i];
        while (nodo != nullptr) {
            NodoLista* siguiente = nodo->siguiente;
            delete nodo;            // destruye el Usuario contenido
            nodo = siguiente;
        }
        buckets[i] = nullptr;
    }
    delete[] buckets;
    buckets = nullptr;
    numBuckets_ = 0;
}

// ---------------------------------------------------------------------
// Funcion hash y utilidades privadas
// ---------------------------------------------------------------------

// Funcion hash matematica PERSONALIZADA: polinomio generador modular.
// Procesa cada caracter acumulando h = (h*P + c) % MOD. P=131 y MOD es un
// primo grande (~1e9) que evita desbordamientos de 64 bits para nombres de
// longitud razonable y distribuye bien los bits. El resultado final se reduce
// modulo numBuckets_ en indiceDe().
size_t TablaHashUsuarios::hashPersonal(const string& nombre) const {
    const size_t P = 131;            // multiplicador de generacion
    const size_t MOD = 1000000007;   // primo grande
    size_t h = 0;
    for (char c : nombre) {
        h = (h * P + static_cast<unsigned char>(c)) % MOD;
    }
    return h;
}

// Indice de bucket: hash % numero de buckets.
size_t TablaHashUsuarios::indiceDe(const string& nombre) const {
    return hashPersonal(nombre) % numBuckets_;
}

// Busca el nodo con el nombre dado dentro de su bucket. Rellena bucketIdx y
// 'anterior' para permitir un borrado eficiente en O(1) sobre la lista.
TablaHashUsuarios::NodoLista* TablaHashUsuarios::buscarNodo(
    const string& nombre, size_t& bucketIdx, NodoLista*& anterior) const {
    bucketIdx = indiceDe(nombre);
    NodoLista* prev = nullptr;
    for (NodoLista* n = buckets[bucketIdx]; n != nullptr; n = n->siguiente) {
        if (n->dato.getNombre() == nombre) {
            anterior = prev;
            return n;
        }
        prev = n;
    }
    anterior = nullptr;
    return nullptr;
}

// ---------------------------------------------------------------------
// Insercion / busqueda / eliminacion
// ---------------------------------------------------------------------

// Agrega un usuario al INICIO de la lista del bucket correspondiente
// (insercion en cabeza: O(1) sin recorrer la lista). Rechaza nombres ya
// presentes para mantener la unicidad de la clave.
bool TablaHashUsuarios::insertar(const Usuario& usuario) {
    const string& nombre = usuario.getNombre();
    if (nombre.empty()) {
        cerr << "[TablaHash] No se puede insertar un usuario sin nombre.\n";
        return false;
    }
    size_t idx = indiceDe(nombre);
    // Verifica unicidad recorriendo solo el bucket (colisiones reales).
    for (NodoLista* n = buckets[idx]; n != nullptr; n = n->siguiente) {
        if (n->dato.getNombre() == nombre) {
            cerr << "[TablaHash] Ya existe el usuario '" << nombre
                 << "'. No se inserto.\n";
            return false;
        }
    }
        // Inserta en cabeza de la lista del bucket (encadenamiento).
    buckets[idx] = new NodoLista(usuario, buckets[idx]);
    ++cantidad;
    return true;
}

// ---------------------------------------------------------------------
// Búsqueda por nombre
// ---------------------------------------------------------------------

// Devuelve un puntero NO propietario al usuario con ese nombre, o nullptr.
Usuario* TablaHashUsuarios::buscar(const string& nombre) {
    size_t idx = indiceDe(nombre);
    for (NodoLista* n = buckets[idx]; n != nullptr; n = n->siguiente) {
        if (n->dato.getNombre() == nombre) {
            return &(n->dato);
        }
    }
    return nullptr;
}

// Version constante de buscar.
const Usuario* TablaHashUsuarios::buscar(const string& nombre) const {
    size_t idx = indiceDe(nombre);
    for (NodoLista* n = buckets[idx]; n != nullptr; n = n->siguiente) {
        if (n->dato.getNombre() == nombre) {
            return &(n->dato);
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------
// Autenticacion (Fase 3)
// ---------------------------------------------------------------------

// Localiza al usuario por nombre (O(1) promedio via hash) y valida la
// contrasena con verificarCredenciales. Devuelve el puntero al usuario
// autenticado o nullptr (nombre inexistente o contrasena incorrecta).
Usuario* TablaHashUsuarios::verificarSesion(const string& nombre,
                                            const string& contrasena) {
    Usuario* u = buscar(nombre);
    if (u == nullptr) {
        return nullptr;  // nombre no registrado
    }
    if (!u->verificarCredenciales(nombre, contrasena)) {
        return nullptr;  // contrasena incorrecta
    }
        return u;            // autenticacion exitosa
}

// ---------------------------------------------------------------------
// Eliminacion
// ---------------------------------------------------------------------

// Elimina el usuario con ese nombre del bucket correspondiente, reusando
// buscarNodo (que devuelve el nodo y su anterior para desenlazar en O(1)).
bool TablaHashUsuarios::eliminar(const string& nombre) {
    size_t idx = 0;
    NodoLista* anterior = nullptr;
    NodoLista* nodo = buscarNodo(nombre, idx, anterior);
    if (nodo == nullptr) {
        return false;  // no existe
    }
    if (anterior != nullptr) {
        anterior->siguiente = nodo->siguiente;   // desenlaza del medio/fondo
    } else {
        buckets[idx] = nodo->siguiente;          // era la cabeza
    }
    delete nodo;         // libera el nodo y destruye el Usuario contenido
    --cantidad;
    return true;
}

// ---------------------------------------------------------------------
// Consultas rapidas
// ---------------------------------------------------------------------

// Indica si existe un usuario con ese nombre.
bool TablaHashUsuarios::contiene(const string& nombre) const {
    size_t idx = indiceDe(nombre);
    for (NodoLista* n = buckets[idx]; n != nullptr; n = n->siguiente) {
        if (n->dato.getNombre() == nombre) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------
// Consultas de estado
// ---------------------------------------------------------------------

size_t TablaHashUsuarios::tamano() const { return cantidad; }
size_t TablaHashUsuarios::numBuckets() const { return numBuckets_; }

// Factor de carga = elementos / buckets.
double TablaHashUsuarios::factorCarga() const {
    return numBuckets_ == 0 ? 0.0
                            : static_cast<double>(cantidad) / numBuckets_;
}

// Numero de usuarios (nodos) alojados en un bucket concreto.
size_t TablaHashUsuarios::colisionesEn(size_t idx) const {
    if (idx >= numBuckets_) {
        return 0;
    }
    size_t count = 0;
    for (NodoLista* n = buckets[idx]; n != nullptr; n = n->siguiente) {
        ++count;
    }
    return count;
}

// ---------------------------------------------------------------------
// Rehash
// ---------------------------------------------------------------------

// Reconstruye la tabla con otro numero de buckets volviendo a insertar todos
// los usuarios recalculando sus indices. Se reutilizan los NODOS EXISTENTES
// (solo se re-enlazan), por lo que no hay crear/borrar — evita fugas y es
// mas eficiente que reconstruir desde cero.
void TablaHashUsuarios::rehash(size_t nuevoNumBuckets) {
    if (nuevoNumBuckets < 1) {
        nuevoNumBuckets = 1;
    }
    if (nuevoNumBuckets == numBuckets_) {
        return;  // nada que hacer
    }

    // Arreglo nuevo de cabezas (inicializado a nullptr con new[]()).
    NodoLista** nuevoBuckets = new NodoLista*[nuevoNumBuckets]();

    // Apunta a la tabla vieja y reubica cada nodo en el nuevo arreglo.
    NodoLista** viejoBuckets = buckets;
    size_t viejoSize = numBuckets_;
    buckets = nuevoBuckets;
    numBuckets_ = nuevoNumBuckets;
    cantidad = 0;

    for (size_t i = 0; i < viejoSize; ++i) {
        NodoLista* n = viejoBuckets[i];
        while (n != nullptr) {
            NodoLista* siguiente = n->siguiente;
            size_t idx = hashPersonal(n->dato.getNombre()) % numBuckets_;
            n->siguiente = buckets[idx];   // enlaza al frente del nuevo bucket
            buckets[idx] = n;
            ++cantidad;
            n = siguiente;
        }
    }

    // Libera SOLO el arreglo de cabezas viejo (los nodos migraron al nuevo).
    delete[] viejoBuckets;
}

// ---------------------------------------------------------------------
// Visualizacion
// ---------------------------------------------------------------------

// Imprime cada bucket con sus usuarios; marca con '!' los buckets con mas
// de un nodo (colisiones resueltas por encadenamiento).
void TablaHashUsuarios::mostrar() const {
    cout << "TablaHash de usuarios (" << cantidad << " elementos, "
         << numBuckets_ << " buckets, carga " << factorCarga() << ")\n";
    for (size_t i = 0; i < numBuckets_; ++i) {
        if (buckets[i] == nullptr) {
            continue;  // bucket vacio
        }
        // Cuenta los nodos del bucket para detectar colisiones.
        size_t count = 0;
        for (NodoLista* n = buckets[i]; n != nullptr; n = n->siguiente) {
            ++count;
        }
        bool colision = count > 1;
        cout << "  [" << i << "]" << (colision ? "! " : "  ");
        size_t j = 0;
        for (NodoLista* n = buckets[i]; n != nullptr; n = n->siguiente, ++j) {
            if (j > 0) {
                cout << " -> ";
            }
            cout << n->dato.getNombre();
        }
        cout << "  (tam. " << count << ")\n";
    }
}
