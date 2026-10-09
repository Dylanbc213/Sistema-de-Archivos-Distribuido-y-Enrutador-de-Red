#include "Archivo.h"

#include <iostream>

using namespace std;

// ---------------------------------------------------------------------
// Constructores / destructor
// ---------------------------------------------------------------------

// Constructor por defecto: archivo anónimo y vacío.
Archivo::Archivo() : NodoArchivo(), contenido("") {}

// Constructor principal: fija identificador, nombre y contenido inicial.
Archivo::Archivo(int id, const string& nombre, const string& contenido)
    : NodoArchivo(id, nombre), contenido(contenido) {}

// Destructor: no hay memoria manual que liberar (todo es std::string).
Archivo::~Archivo() {}

// ---------------------------------------------------------------------
// Interfaz abstracta
// ---------------------------------------------------------------------

// Tipo de nodo: siempre ARCHIVO.
TipoNodo Archivo::getTipo() const { return TipoNodo::ARCHIVO; }

// Un archivo nunca es una carpeta.
bool Archivo::esCarpeta() const { return false; }

// Tamaño del archivo medido en caracteres del contenido.
long long Archivo::obtenerTamano() const {
    return static_cast<long long>(contenido.size());
}

// Imprime el archivo con su nombre y tamaño entre corchetes.
void Archivo::mostrar(int nivel) const {
    string sangria(static_cast<size_t>(nivel) * 2, ' ');
    cout << sangria << "[FILE] " << getNombre()
         << " (" << obtenerTamano() << " bytes)\n";
}

// ---------------------------------------------------------------------
// Contenido
// ---------------------------------------------------------------------

// Devuelve una copia del contenido del archivo.
string Archivo::getContenido() const { return contenido; }

// Reemplaza por completo el contenido del archivo.
void Archivo::setContenido(const string& nuevoContenido) {
    contenido = nuevoContenido;
}

// Anexa un fragmento al final del contenido (útil para escrituras
// posteriores por bloques).
void Archivo::agregarContenido(const string& fragmento) {
    contenido += fragmento;
}

// Vacía el contenido del archivo.
void Archivo::limpiar() { contenido.clear(); }

// Cantidad de caracteres almacenados.
size_t Archivo::cantidadCaracteres() const { return contenido.size(); }