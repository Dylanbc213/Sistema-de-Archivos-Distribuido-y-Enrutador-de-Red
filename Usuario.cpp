#include "Usuario.h"

#include <cctype>
#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Constructores
// ---------------------------------------------------------------------

// Constructor por defecto: usuario neutro.
Usuario::Usuario() : id(0), nombre(""), contrasena("") {}

// Constructor principal: fija identificador, nombre y contraseña.
Usuario::Usuario(int id, const string& nombre, const string& contrasena)
    : id(id), nombre(nombre), contrasena(contrasena) {}

// ---------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------

// Devuelve el ID del usuario.
int Usuario::getId() const { return id; }

// Devuelve el nombre del usuario.
string Usuario::getNombre() const { return nombre; }

// Devuelve la contraseña del usuario.
string Usuario::getContrasena() const { return contrasena; }

// ---------------------------------------------------------------------
// Setters (con validación básica)
// ---------------------------------------------------------------------

// Asigna el ID (rechaza valores negativos).
void Usuario::setId(int nuevoId) {
    if (nuevoId < 0) {
        cerr << "[Usuario] ID inválido (no puede ser negativo). No se aplicó.\n";
        return;
    }
    id = nuevoId;
}

// Asigna el nombre (rechaza vacíos o solo espacios).
void Usuario::setNombre(const string& nuevoNombre) {
    bool tieneContenido = false;
    for (char c : nuevoNombre) {
        if (!isspace(static_cast<unsigned char>(c))) {
            tieneContenido = true;
            break;
        }
    }
    if (!tieneContenido) {
        cerr << "[Usuario] El nombre no puede estar vacío. No se aplicó.\n";
        return;
    }
    nombre = nuevoNombre;
}

// Asigna la contraseña (rechaza cadenas vacías).
void Usuario::setContrasena(const string& nuevaContrasena) {
    if (nuevaContrasena.empty()) {
        cerr << "[Usuario] La contraseña no puede estar vacía. No se aplicó.\n";
        return;
    }
    contrasena = nuevaContrasena;
}

// ---------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------

// Verifica si el nombre y la contraseña coinciden con los almacenados.
bool Usuario::verificarCredenciales(const string& nombreIngresado,
                                    const string& contrasenaIngresada) const {
    return nombre == nombreIngresado && contrasena == contrasenaIngresada;
}

// Imprime los datos del usuario; la contraseña se representa con '*' para
// no exponerla en pantalla.
void Usuario::mostrarInformacion() const {
    cout << "Usuario #" << id << "\n"
         << "  Nombre     : " << nombre << "\n"
         << "  Contraseña : " << string(contrasena.size(), '*') << "\n";
}

// Igualdad por ID (identidad del usuario).
bool Usuario::operator==(const Usuario& otro) const { return id == otro.id; }

// Negación de la igualdad por ID.
bool Usuario::operator!=(const Usuario& otro) const { return !(*this == otro); }