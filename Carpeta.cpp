#include "Carpeta.h"

#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Constructores / destructor
// ---------------------------------------------------------------------

// Constructor por defecto: carpeta anónima y vacía.
Carpeta::Carpeta() : NodoArchivo() {}

// Constructor principal: carpeta con identificador y nombre, sin hijos.
Carpeta::Carpeta(int id, const string& nombre) : NodoArchivo(id, nombre) {}

// Destructor: el vector libera automáticamente a los hijos (shared_ptr),
// por lo que no se gestiona memoria manualmente.
Carpeta::~Carpeta() {}

// ---------------------------------------------------------------------
// Interfaz abstracta
// ---------------------------------------------------------------------

// Tipo de nodo: siempre CARPETA.
TipoNodo Carpeta::getTipo() const { return TipoNodo::CARPETA; }

// Una carpeta siempre responde afirmativamente a esCarpeta().
bool Carpeta::esCarpeta() const { return true; }

// Tamaño de la carpeta: suma recursiva de los tamaños de sus hijos.
long long Carpeta::obtenerTamano() const {
    long long total = 0;
    for (const auto& hijo : hijos) {
        if (hijo) {
            total += hijo->obtenerTamano();
        }
    }
    return total;
}

// Imprime la carpeta y, recursivamente, todo su contenido indentado por
// nivel de profundidad.
void Carpeta::mostrar(int nivel) const {
    string sangria(static_cast<size_t>(nivel) * 2, ' ');
    cout << sangria << "[DIR] " << getNombre() << "/\n";
    for (const auto& hijo : hijos) {
        if (hijo) {
            hijo->mostrar(nivel + 1);
        }
    }
}

// ---------------------------------------------------------------------
// Gestión de hijos
// ---------------------------------------------------------------------

// Indica si ya existe un hijo con ese nombre.
bool Carpeta::nombreOcupado(const string& nombre) const {
    for (const auto& hijo : hijos) {
        if (hijo && hijo->getNombre() == nombre) {
            return true;
        }
    }
    return false;
}

// Agrega un nodo como hijo y enlaza el padre. Devuelve false (y no
// modifica nada) si el nodo es nulo, es esta misma carpeta o ya hay un
// hijo con ese nombre.
bool Carpeta::agregarHijo(const shared_ptr<NodoArchivo>& nodo) {
    if (!nodo) {
        cerr << "[Carpeta] No se puede agregar un nodo nulo.\n";
        return false;
    }
    if (nodo.get() == this) {
        cerr << "[Carpeta] Una carpeta no puede contenerse a sí misma.\n";
        return false;
    }
    if (nombreOcupado(nodo->getNombre())) {
        cerr << "[Carpeta] Ya existe un nodo llamado '" << nodo->getNombre()
             << "' en '" << getNombre() << "'.\n";
        return false;
    }

    // Enlaza el padre solo si esta carpeta está gestionada por un
    // shared_ptr. Si se creó en la pila, shared_from_this() lanzaría
    // bad_weak_ptr; en tal caso se agrega el hijo sin enlace de padre en
    // lugar de abortar.
    try {
        nodo->setPadre(dynamic_pointer_cast<Carpeta>(shared_from_this()));
    } catch (const bad_weak_ptr&) {
        // Uso no gestionado por shared_ptr: se omite el enlace de padre.
    }

    hijos.push_back(nodo);
    return true;
}

// Elimina el primer hijo que coincida con el nombre indicado.
bool Carpeta::eliminarHijoPorNombre(const string& nombre) {
    for (auto it = hijos.begin(); it != hijos.end(); ++it) {
        if (*it && (*it)->getNombre() == nombre) {
            hijos.erase(it);
            return true;
        }
    }
    return false;
}

// Elimina el primer hijo que coincida con el ID indicado.
bool Carpeta::eliminarHijoPorId(int id) {
    for (auto it = hijos.begin(); it != hijos.end(); ++it) {
        if (*it && (*it)->getId() == id) {
            hijos.erase(it);
            return true;
        }
    }
    return false;
}

// Busca un hijo por nombre; devuelve nullptr si no existe.
shared_ptr<NodoArchivo> Carpeta::buscarHijoPorNombre(const string& nombre) const {
    for (const auto& hijo : hijos) {
        if (hijo && hijo->getNombre() == nombre) {
            return hijo;
        }
    }
    return nullptr;
}

// Busca un hijo por ID; devuelve nullptr si no existe.
shared_ptr<NodoArchivo> Carpeta::buscarHijoPorId(int id) const {
    for (const auto& hijo : hijos) {
        if (hijo && hijo->getId() == id) {
            return hijo;
        }
    }
    return nullptr;
}

// Acceso de solo lectura a la lista de hijos (no transfiere propiedad).
const vector<shared_ptr<NodoArchivo>>& Carpeta::getHijos() const {
    return hijos;
}

// Cantidad de hijos directos.
size_t Carpeta::cantidadHijos() const { return hijos.size(); }

// Indica si la carpeta no tiene hijos.
bool Carpeta::estaVacia() const { return hijos.empty(); }

// Elimina todos los hijos.
void Carpeta::vaciar() { hijos.clear(); }