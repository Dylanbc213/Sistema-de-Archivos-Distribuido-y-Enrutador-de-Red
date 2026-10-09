#include "Servidor.h"

#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Constructores
// ---------------------------------------------------------------------

// Constructor por defecto: servidor sin datos con su raíz creada.
Servidor::Servidor()
    : id(0), nombre("servidor"),
      raiz(make_shared<Carpeta>(0, "servidor")) {}

// Constructor principal: crea la carpeta raíz con el nombre del servidor.
Servidor::Servidor(int id, const string& nombre)
    : id(id), nombre(nombre),
      raiz(make_shared<Carpeta>(id, nombre)) {}

// Constructor completo: permite inyectar una raíz ya construida.
Servidor::Servidor(int id, const string& nombre,
                   const shared_ptr<Carpeta>& raiz)
    : id(id), nombre(nombre), raiz(raiz ? raiz : make_shared<Carpeta>(id, nombre)) {}

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