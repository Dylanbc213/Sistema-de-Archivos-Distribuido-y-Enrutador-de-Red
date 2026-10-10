#include "Usuario.h"

#include <cctype>
#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Constructores
// ---------------------------------------------------------------------

// Constructor por defecto: usuario neutro sin nombre ni contrasena.
Usuario::Usuario()
    : id(0), nombre(""), contrasena(""),
      permisos(NivelPermiso::NORMAL), servidorAsignado(-1) {}

// Constructor principal: fija identificador, nombre y contrasena. El nivel
// de permiso y el servidor asignado son opcionales (por defecto: NORMAL y
// -1 = sin servidor). Los defaults conservan la compatibilidad con el
// codigo previo que construia Usuario(id, nombre, contrasena).
Usuario::Usuario(int id, const std::string& nombre, const std::string& contrasena,
                 NivelPermiso permisos, int servidorAsignado)
    : id(id), nombre(nombre), contrasena(contrasena),
      permisos(permisos), servidorAsignado(servidorAsignado) {}

// ---------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------

// Devuelve el ID del usuario.
int Usuario::getId() const { return id; }

// Devuelve el nombre del usuario.
string Usuario::getNombre() const { return nombre; }

// Devuelve la contrasena del usuario.
string Usuario::getContrasena() const { return contrasena; }

NivelPermiso Usuario::getPermisos() const { return permisos; }

// Representacion legible del nivel de permiso.
string Usuario::getPermisoStr() const {
    return (permisos == NivelPermiso::ADMIN) ? "ADMIN" : "NORMAL";
}

int Usuario::getServidorAsignado() const { return servidorAsignado; }

// ---------------------------------------------------------------------
// Setters (con validacion basica)
// ---------------------------------------------------------------------

// Asigna el ID (rechaza valores negativos).
void Usuario::setId(int nuevoId) {
    if (nuevoId < 0) {
        cerr << "[Usuario] ID invalido (no puede ser negativo). No se aplicó.\n";
        return;
    }
    id = nuevoId;
}

// Asigna el nombre (rechaza vacios o solo espacios).
void Usuario::setNombre(const string& nuevoNombre) {
    bool tieneContenido = false;
    for (char c : nuevoNombre) {
        if (!isspace(static_cast<unsigned char>(c))) {
            tieneContenido = true;
            break;
        }
    }
    if (!tieneContenido) {
        cerr << "[Usuario] El nombre no puede estar vacio. No se aplicó.\n";
        return;
    }
    nombre = nuevoNombre;
}

// Asigna la contrasena (rechaza cadenas vacias).
void Usuario::setContrasena(const string& nuevaContrasena) {
    if (nuevaContrasena.empty()) {
        cerr << "[Usuario] La contraseña no puede estar vacia. No se aplicó.\n";
        return;
    }
    contrasena = nuevaContrasena;
}

// Asigna el nivel de permiso del usuario.
void Usuario::setPermisos(NivelPermiso nuevoPermiso) {
    permisos = nuevoPermiso;
}

// Asigna el servidor al que pertenece el usuario (-1 = sin asignar).
void Usuario::setServidorAsignado(int servidorAsignado) {
    if (servidorAsignado < -1) {
        cerr << "[Usuario] ID de servidor invalido. No se aplicó.\n";
        return;
    }
    this->servidorAsignado = servidorAsignado;
}

// Indica si el usuario posee privilegios de administrador.
bool Usuario::esAdmin() const {
    return permisos == NivelPermiso::ADMIN;
}

// ---------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------

// Verifica si el nombre y la contrasena coinciden con los almacenados.
bool Usuario::verificarCredenciales(const string& nombreIngresado,
                                    const string& contrasenaIngresada) const {
    return nombre == nombreIngresado && contrasena == contrasenaIngresada;
}

// Imprime los datos del usuario; la contrasena se muestra oculta con '*'.
void Usuario::mostrarInformacion() const {
    cout << "Usuario #" << id << "\n"
         << "  Nombre     : " << nombre << "\n"
         << "  Contraseña : " << string(contrasena.size(), '*') << "\n"
         << "  Permiso    : " << getPermisoStr() << "\n"
         << "  Servidor   : " << (servidorAsignado >= 0
                                      ? to_string(servidorAsignado)
                                      : string("sin asignar")) << "\n";
}

// Igualdad por ID (identidad del usuario).
bool Usuario::operator==(const Usuario& otro) const { return id == otro.id; }

// Negacion de la igualdad por ID.
bool Usuario::operator!=(const Usuario& otro) const { return !(*this == otro); }
