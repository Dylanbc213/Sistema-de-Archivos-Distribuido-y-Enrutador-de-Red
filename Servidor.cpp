#include "Servidor.h"

#include "Archivo.h"   // Necesario para crearArchivo()

#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Constructores
// ---------------------------------------------------------------------

// Constructor por defecto: servidor sin datos con su raíz creada.
// La raíz se representa con el nombre "/" (directorio principal).
Servidor::Servidor()
    : id(0), nombre("servidor"),
      raiz(make_shared<Carpeta>(0, "/")) {}

// Constructor principal: crea la carpeta raíz con nombre "/" (directorio
// principal). El nombre del servidor se guarda como metadato.
Servidor::Servidor(int id, const string& nombre)
    : id(id), nombre(nombre),
      raiz(make_shared<Carpeta>(id, "/")) {}

// Constructor completo: permite inyectar una raíz ya construida.
// Si la raíz inyectada es nula, se crea una con nombre "/" por defecto.
Servidor::Servidor(int id, const string& nombre,
                   const shared_ptr<Carpeta>& raiz)
    : id(id), nombre(nombre), raiz(raiz ? raiz : make_shared<Carpeta>(id, "/")) {}

// ---------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------

// Devuelve el ID del servidor.
int Servidor::getId() const { return id; }

// Devuelve el nombre del servidor.
string Servidor::getNombre() const { return nombre; }

// Devuelve la carpeta raíz del árbol de archivos del servidor.
shared_ptr<Carpeta> Servidor::getRaiz() const { return raiz; }

// ---------------------------------------------------------------------
// Setters (con validación básica)
// ---------------------------------------------------------------------

// Asigna el ID (rechaza valores negativos).
void Servidor::setId(int nuevoId) {
    if (nuevoId < 0) {
        cerr << "[Servidor] ID inválido (no puede ser negativo). No se aplicó.\n";
        return;
    }
    id = nuevoId;
}

// Asigna el nombre (rechaza cadenas vacías).
void Servidor::setNombre(const string& nuevoNombre) {
    if (nuevoNombre.empty()) {
        cerr << "[Servidor] El nombre no puede estar vacío. No se aplicó.\n";
        return;
    }
    nombre = nuevoNombre;
}

// Reemplaza la carpeta raíz (rechaza un puntero nulo).
void Servidor::setRaiz(const shared_ptr<Carpeta>& nuevaRaiz) {
    if (!nuevaRaiz) {
        cerr << "[Servidor] La raíz no puede ser nula. No se aplicó.\n";
        return;
    }
    raiz = nuevaRaiz;
}

// ---------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------

// Imprime los datos del servidor y, debajo, su árbol de archivos.
void Servidor::mostrar() const {
    cout << "Servidor #" << id << " \"" << nombre << "\"\n";
    if (raiz) {
        raiz->mostrar(1);
    } else {
        cout << "  (sin carpeta raíz)\n";
    }
}

// Igualdad por ID (identidad del servidor en la red).
bool Servidor::operator==(const Servidor& otro) const { return id == otro.id; }

// Negación de la igualdad por ID.
bool Servidor::operator!=(const Servidor& otro) const { return !(*this == otro); }

// =====================================================================
// Fase 2: Operaciones de sistema de archivos sobre la raíz
// =====================================================================
// Todos los métodos delegan en la carpeta raíz del servidor, manteniendo
// una interfaz limpia que oculta los detalles del árbol interno.

// Búsqueda por ruta desde la raíz. Si la ruta es "/" devuelve la raíz.
shared_ptr<NodoArchivo> Servidor::buscarPorRuta(const string& ruta) const {
    if (!raiz) {
        return nullptr;
    }
    if (ruta == "/" || ruta.empty()) {
        return raiz;  // La raíz representa "/"
    }
    return raiz->buscarPorRuta(ruta);
}

// Búsqueda recursiva (DFS) por nombre en todo el árbol.
shared_ptr<NodoArchivo> Servidor::buscarRecursivoPorNombre(
    const string& nombre) const {
    if (!raiz) {
        return nullptr;
    }
    return raiz->buscarRecursivoPorNombre(nombre);
}

// Crea una subcarpeta en la ruta indicada (delegado a la raíz).
shared_ptr<Carpeta> Servidor::crearSubdirectorio(const string& ruta) {
    if (!raiz) {
        return nullptr;
    }
    return raiz->crearSubdirectorio(ruta);
}

// Crea un archivo en la ruta indicada (delegado a la raíz).
shared_ptr<Archivo> Servidor::crearArchivo(const string& ruta,
                                           const string& contenido) {
    if (!raiz) {
        return nullptr;
    }
    return raiz->crearArchivo(ruta, contenido);
}

// Elimina recursivamente un nodo por ruta (cascada si es carpeta).
bool Servidor::eliminarRecursivo(const string& ruta) {
    if (!raiz) {
        return false;
    }
    return raiz->eliminarRecursivo(ruta);
}

// Muestra el árbol de directorios a partir de la raíz.
void Servidor::mostrarArbol() const {
    if (!raiz) {
                cout << "  (sin carpeta raíz)\n";
        return;
    }
    raiz->mostrar(0);
}