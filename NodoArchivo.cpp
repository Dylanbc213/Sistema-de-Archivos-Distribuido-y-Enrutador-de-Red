#include "NodoArchivo.h"

#include "Carpeta.h"

#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Constructores / destructor
// ---------------------------------------------------------------------

// Constructor por defecto: nodo neutro sin nombre ni padre.
NodoArchivo::NodoArchivo() : id(0), nombre("") {}

// Constructor principal: fija el identificador y el nombre del nodo.
NodoArchivo::NodoArchivo(int id, const string& nombre)
    : id(id), nombre(nombre) {}

// Destructor virtual: permite destruir el objeto a través de un puntero
// a la clase base sin provocar comportamiento indefinido.
NodoArchivo::~NodoArchivo() {}

// ---------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------

// Devuelve el identificador único del nodo.
int NodoArchivo::getId() const { return id; }

// Devuelve el nombre del nodo.
string NodoArchivo::getNombre() const { return nombre; }

// Devuelve el padre si todavía existe. lock() eleva el weak_ptr a
// shared_ptr; si la carpeta ya fue destruida devuelve nullptr.
shared_ptr<Carpeta> NodoArchivo::getPadre() const { return padre.lock(); }

// ---------------------------------------------------------------------
// Setters (con validación básica)
// ---------------------------------------------------------------------

// Asigna el ID (rechaza valores negativos).
void NodoArchivo::setId(int nuevoId) {
    if (nuevoId < 0) {
        cerr << "[NodoArchivo] ID inválido (no puede ser negativo). No se aplicó.\n";
        return;
    }
    id = nuevoId;
}

// Asigna el nombre (rechaza cadenas vacías).
void NodoArchivo::setNombre(const string& nuevoNombre) {
    if (nuevoNombre.empty()) {
        cerr << "[NodoArchivo] El nombre no puede estar vacío. No se aplicó.\n";
        return;
    }
    nombre = nuevoNombre;
}

// Registra la carpeta contenedora (enlace no propietario).
void NodoArchivo::setPadre(const shared_ptr<Carpeta>& nuevaCarpeta) {
    padre = nuevaCarpeta;
}

// ---------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------

// Reconstruye la ruta subiendo por los padres; los nodos raíz sin padre
// devuelven solo su propio nombre.
string NodoArchivo::obtenerRuta() const {
    string ruta = nombre.empty() ? "/" : nombre;
    shared_ptr<Carpeta> p = padre.lock();
    while (p) {
                string nombrePadre = p->getNombre();
        // Si el padre es la raíz (nombre "/" o vacío), ya incluye el
        // separador "/", así que no se añade otro para evitar "//".
        if (nombrePadre == "/" || nombrePadre.empty()) {
            ruta = "/" + ruta;
        } else {
            ruta = nombrePadre + "/" + ruta;
        }
        p = p->getPadre();
    }
    return ruta;
}

// Igualdad por ID (identidad dentro del sistema de archivos).
bool NodoArchivo::operator==(const NodoArchivo& otro) const {
    return id == otro.id;
}

// Negación de la igualdad por ID.
bool NodoArchivo::operator!=(const NodoArchivo& otro) const {
    return !(*this == otro);
}