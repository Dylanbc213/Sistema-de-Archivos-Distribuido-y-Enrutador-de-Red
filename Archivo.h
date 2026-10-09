#ifndef ARCHIVO_H
#define ARCHIVO_H

#include "NodoArchivo.h"

#include <cstddef>
#include <string>

// =====================================================================
// Archivo: nodo hoja del sistema de archivos.
// =====================================================================
// Almacena el contenido como std::string. No puede contener otros nodos.
// En fases posteriores el contenido podrá fragmentarse entre servidores,
// por lo que se mantiene desacoplado del resto.
class Archivo : public NodoArchivo {
private:
    std::string contenido;

public:
    // Constructores / destructor
    Archivo();
    // El contenido es opcional: permite crear el archivo "vacío" y
    // llenarlo después.
    Archivo(int id, const std::string& nombre, const std::string& contenido = "");
    ~Archivo() override;

    // -------- Implementación de la interfaz abstracta --------
    TipoNodo getTipo() const override;
    bool esCarpeta() const override;
    long long obtenerTamano() const override;   // longitud del contenido
    void mostrar(int nivel = 0) const override;

    // -------- Contenido --------
    std::string getContenido() const;
    void setContenido(const std::string& nuevoContenido);
    void agregarContenido(const std::string& fragmento);
    void limpiar();

    std::size_t cantidadCaracteres() const;
};

#endif