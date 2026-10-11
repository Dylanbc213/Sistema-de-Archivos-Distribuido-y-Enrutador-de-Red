#include "Usuario.h"

#include <windows.h>
#include <bcrypt.h>
#include <cctype>
#include <iostream>
#include <limits>

using namespace std;

namespace {
    const ULONG ITERACIONES_PASSWORD = 600000;

    bool derivarContrasena(const string& password, const string& sal,
                           unsigned char salida[32]) {
        if (password.size() > (numeric_limits<ULONG>::max)() || sal.empty()) return false;
        BCRYPT_ALG_HANDLE proveedor = nullptr;
        if (BCryptOpenAlgorithmProvider(&proveedor, BCRYPT_SHA256_ALGORITHM,
                                        nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0) return false;
        const NTSTATUS estado = BCryptDeriveKeyPBKDF2(
            proveedor,
            reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())),
            static_cast<ULONG>(password.size()),
            reinterpret_cast<PUCHAR>(const_cast<char*>(sal.data())),
            static_cast<ULONG>(sal.size()), ITERACIONES_PASSWORD,
            salida, 32, 0);
        BCryptCloseAlgorithmProvider(proveedor, 0);
        return estado >= 0;
    }
}

// ---------------------------------------------------------------------
// Constructores
// ---------------------------------------------------------------------

// Constructor por defecto: usuario neutro sin nombre ni contrasena.
Usuario::Usuario()
    : id(0), nombre(""), salContrasena(), hashContrasena(),
      permisos(NivelPermiso::NORMAL), servidorAsignado(-1) {}

// Constructor principal: fija identificador, nombre y contrasena. El nivel
// de permiso y el servidor asignado son opcionales (por defecto: NORMAL y
// -1 = sin servidor). Los defaults conservan la compatibilidad con el
// codigo previo que construia Usuario(id, nombre, contrasena).
Usuario::Usuario(int id, const std::string& nombre, const std::string& contrasena,
                 NivelPermiso permisos, int servidorAsignado)
    : id(id), nombre(nombre), salContrasena(), hashContrasena(),
      permisos(permisos), servidorAsignado(servidorAsignado) {
    setContrasena(contrasena);
}

Usuario Usuario::desdeDatosPersistidos(int id, const string& nombre,
        const string& sal, const string& hash, NivelPermiso permisos,
        int servidorAsignado) {
    Usuario usuario;
    usuario.id = id;
    usuario.nombre = nombre;
    usuario.salContrasena = sal;
    usuario.hashContrasena = hash;
    usuario.permisos = permisos;
    usuario.servidorAsignado = servidorAsignado;
    return usuario;
}

const string& Usuario::salPersistida() const { return salContrasena; }
const string& Usuario::hashPersistido() const { return hashContrasena; }

// ---------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------

// Devuelve el ID del usuario.
int Usuario::getId() const { return id; }

// Devuelve el nombre del usuario.
string Usuario::getNombre() const { return nombre; }

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
    salContrasena.clear();
    hashContrasena.clear();
    if (nuevaContrasena.empty()) {
        cerr << "[Usuario] La contraseña no puede estar vacia. No se aplicó.\n";
        return;
    }
    unsigned char sal[16];
    unsigned char derivacion[32];
    if (BCryptGenRandom(nullptr, sal, sizeof(sal), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) {
        cerr << "[Usuario] No se pudo obtener sal criptografica.\n";
        return;
    }
    string salBytes(reinterpret_cast<const char*>(sal), sizeof(sal));
    if (!derivarContrasena(nuevaContrasena, salBytes, derivacion)) {
        cerr << "[Usuario] No se pudo derivar la clave con BCrypt.\n";
        return;
    }
    salContrasena = salBytes;
    hashContrasena.assign(reinterpret_cast<const char*>(derivacion), sizeof(derivacion));
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

bool Usuario::tieneContrasenaValida() const {
    return salContrasena.size() == 16 && hashContrasena.size() == 32;
}

// ---------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------

// Verifica si el nombre y la contrasena coinciden con los almacenados.
bool Usuario::verificarCredenciales(const string& nombreIngresado,
                                    const string& contrasenaIngresada) const {
    if (nombre != nombreIngresado || salContrasena.size() != 16 ||
        hashContrasena.size() != 32) return false;
    unsigned char derivacion[32];
    if (!derivarContrasena(contrasenaIngresada, salContrasena, derivacion)) return false;
    unsigned char diferencia = 0;
    for (size_t i = 0; i < sizeof(derivacion); ++i)
        diferencia |= derivacion[i] ^ static_cast<unsigned char>(hashContrasena[i]);
    return diferencia == 0;
}

// Imprime los datos del usuario sin mostrar la clave ni su derivacion.
void Usuario::mostrarInformacion() const {
    cout << "Usuario #" << id << "\n"
         << "  Nombre     : " << nombre << "\n"
         << "  Contraseña : [protegida]\n"
         << "  Permiso    : " << getPermisoStr() << "\n"
         << "  Servidor   : " << (servidorAsignado >= 0
                                      ? to_string(servidorAsignado)
                                      : string("sin asignar")) << "\n";
}

// Igualdad por ID (identidad del usuario).
bool Usuario::operator==(const Usuario& otro) const { return id == otro.id; }

// Negacion de la igualdad por ID.
bool Usuario::operator!=(const Usuario& otro) const { return !(*this == otro); }
