#ifndef NODOARCHIVO_H
#define NODOARCHIVO_H

#include <memory>
#include <string>

// Se declara por adelantado: el padre de un nodo es siempre una Carpeta,
// pero Carpeta depende a su vez de NodoArchivo (relación circular de
// tipos). El aviso anticipado rompe el ciclo; el encabezado completo solo
// se incluye en NodoArchivo.cpp.
class Carpeta;

// Diferencia en tiempo de ejecución entre los dos tipos de nodo del
// sistema de archivos. Permite clasificar sin recurrir a dynamic_cast.
enum class TipoNodo {
    ARCHIVO,
    CARPETA
};

// =====================================================================
// Clase base ABSTRACTA de todo el sistema de archivos.
// =====================================================================
// Un NodoArchivo representa cualquier elemento que puede vivir dentro de
// una carpeta: otra carpeta (Carpeta) o un archivo (Archivo).
//
// Es abstracta porque declara operaciones puramente virtuales (tipo,
// tamaño y representación) que cada especialización concreta debe
// resolver; por eso no se puede instanciar directamente. El destructor
// virtual garantiza que se destruya correctamente a través de un
// puntero a la base.
//
// Hereda de std::enable_shared_from_this para poder obtener un
// std::shared_ptr a sí mismo (necesario al enlazar padre/hijo sin usar
// punteros propietarios crudos).
class NodoArchivo : public std::enable_shared_from_this<NodoArchivo> {
private:
    int id;
    std::string nombre;

    // Enlace NO propietario hacia la carpeta contenedora. Se usa weak_ptr
    // para romper el ciclo de referencias: la carpeta es dueña del hijo
    // mediante shared_ptr, y el hijo solo observa a su padre.
    std::weak_ptr<Carpeta> padre;

public:
    // Constructores / destructor
    NodoArchivo();
    NodoArchivo(int id, const std::string& nombre);
    virtual ~NodoArchivo();

    // Getters
    int getId() const;
    std::string getNombre() const;

    // Devuelve el padre bloqueando el weak_ptr. Puede ser nullptr si el
    // nodo no está enlazado a ninguna carpeta (por ejemplo, la raíz).
    std::shared_ptr<Carpeta> getPadre() const;

    // Setters (con validación básica: no aplican valores inválidos)
    void setId(int id);
    void setNombre(const std::string& nombre);
    void setPadre(const std::shared_ptr<Carpeta>& nuevaCarpeta);

    // Ruta del nodo reconstruida subiendo por la cadena de padres.
    std::string obtenerRuta() const;

    // -------- Interfaz abstracta (la implementa cada subclase) --------
    virtual TipoNodo getTipo() const = 0;           // ARCHIVO o CARPETA
    virtual bool esCarpeta() const = 0;             // atajo de clasificación
    virtual long long obtenerTamano() const = 0;    // tamaño en bytes
    virtual void mostrar(int nivel = 0) const = 0;  // volcado jerárquico

    // Igualdad por ID dentro del sistema de archivos.
    bool operator==(const NodoArchivo& otro) const;
    bool operator!=(const NodoArchivo& otro) const;
};

#endif