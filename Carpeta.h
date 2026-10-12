#ifndef CARPETA_H
#define CARPETA_H

#include "NodoArchivo.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// Forward declaration: Archivo se incluye solo en el .cpp para evitar
// dependencia innecesaria entre encabezados (Carpeta no necesita conocer
// el detalle de Archivo, solo referenciarlo en la firma de crearArchivo).
class Archivo;

// =====================================================================
// Carpeta: nodo contenedor del sistema de archivos.
// =====================================================================
// Forma el árbol de carpetas y archivos. Es dueña de sus hijos mediante
// std::shared_ptr<NodoArchivo> (el único dueño real de cada nodo). La
// relación inversa hijo -> padre se apoya en el weak_ptr que guarda cada
// NodoArchivo, de modo que no hay ciclos de propiedad.
class Carpeta : public NodoArchivo {
private:
            std::vector<std::shared_ptr<NodoArchivo>> hijos;

    // Comprueba si ya existe un hijo con ese nombre (evita duplicados,
    // igual que un sistema de archivos real).
    bool nombreOcupado(const std::string& nombre) const;

    // -----------------------------------------------------------------
    // Navegación interna (utilizada por los métodos recursivos).
    // Busca un hijo directo por nombre usando un puntero crudo const para
    // evitar compartir shared_ptr innecesariamente durante la navegación.
    // El puntero es seguro porque el vector 'hijos' mantiene viva la memoria.
    // -----------------------------------------------------------------
    const NodoArchivo* obtenerHijoCrudo(const std::string& nombre) const;

public:
    // Constructores / destructor
    Carpeta();
    Carpeta(int id, const std::string& nombre);
    ~Carpeta() override;

    // -------- Implementación de la interfaz abstracta --------
    TipoNodo getTipo() const override;
    bool esCarpeta() const override;
    long long obtenerTamano() const override;   // suma recursiva de hijos
    void mostrar(int nivel = 0) const override;

    // -------- Gestión de hijos --------
    // Agrega un nodo, que pasa a ser propiedad de esta carpeta, y enlaza
    // su padre. Rechaza nullptr, autorreferencias y nombres duplicados.
    bool agregarHijo(const std::shared_ptr<NodoArchivo>& nodo);

    bool eliminarHijoPorNombre(const std::string& nombre);
    bool eliminarHijoPorId(int id);

    std::shared_ptr<NodoArchivo> buscarHijoPorNombre(const std::string& nombre) const;
    std::shared_ptr<NodoArchivo> buscarHijoPorId(int id) const;

    // Acceso de solo lectura a la lista de hijos.
    const std::vector<std::shared_ptr<NodoArchivo>>& getHijos() const;

        std::size_t cantidadHijos() const;
    bool estaVacia() const;
    void vaciar();

    // =================================================================
    // Fase 2: Operaciones recursivas sobre el árbol de directorios
    // =================================================================

    // -------- Utilidades de ruta --------

    // Divide una ruta estilo Unix en sus componentes individuales.
    // Ej.: "/docs/reportes/archivo.txt"  ->  ["docs", "reportes", "archivo.txt"]
    //       "config"                     ->  ["config"]
    //       "/"                          ->  []  (la raíz misma)
    //       ""                           ->  []
    // El separador es '/' y se ignoran componentes vacíos intermedios
    // (equivalente a "docs//archivos" -> ["docs", "archivos"]).
    static std::vector<std::string> dividirRuta(const std::string& ruta);

    // -------- Búsqueda recursiva --------

    // Obtiene la carpeta raíz navegando hacia arriba a través de la cadena
    // de padres (hasta que getPadre() devuelva nullptr). Útil para
    // resolver rutas absolutas desde cualquier nodo del árbol.
    // Requiere que el nodo esté gestionado por un shared_ptr (heredado de
    // enable_shared_from_this); lanza bad_weak_ptr si no lo está.
    std::shared_ptr<Carpeta> obtenerRaiz();

    // Busca un nodo por su nombre completo mediante recorrido DFS (profundidad
    // primero) sobre todo el subárbol. Devuelve el primer nodo encontrado o
    // nullptr si no existe. No descansa esta carpeta misma, solo sus hijos.
    std::shared_ptr<NodoArchivo> buscarRecursivoPorNombre(
        const std::string& nombre) const;

    // Busca un nodo siguiendo una ruta. Soporta rutas absolutas (inician
    // con '/', resueltas desde la raíz) y relativas (desde esta carpeta).
    // Devuelve el nodo encontrado o nullptr si algún componente no existe
    // o no es una carpeta intermedia.
    std::shared_ptr<NodoArchivo> buscarPorRuta(const std::string& ruta) const;

    // -------- Creación recursiva --------

    // Crea una subcarpeta en la ruta indicada. Si faltan directorios
    // intermedios, se crean automáticamente (comportamiento recursivo).
    // Devuelve el shared_ptr a la nueva carpeta o nullptr si falló
    // (nombre en uso, ruta inválida o nodo intermedio no es carpeta).
    std::shared_ptr<Carpeta> crearSubdirectorio(const std::string& ruta);

    // Crea un archivo en la ruta indicada, creando intermedias si falta.
    // El contenido es opcional (por defecto cadena vacía). Devuelve el
    // shared_ptr al nuevo Archivo o nullptr si falló.
    std::shared_ptr<Archivo> crearArchivo(const std::string& ruta,
                                          const std::string& contenido = "");

    // -------- Eliminación en cascada --------

    // Elimina recursivamente el nodo ubicado en 'ruta' (absoluta o relativa).
    // Si el nodo es una carpeta, primero se borran todos sus descendientes
    // (hojas primero, subiendo hasta la carpeta objetivo) y luego se la
    // quita de la lista de hijos. Garantiza: sin fugas ni huérfanos.
    bool eliminarRecursivo(const std::string& ruta);

    // Vacía TODO el contenido de esta carpeta y sus subcarpetas en orden
    // post-orden (DFS): recursivamente se vacían las subcarpetas hijas
    // primero (sus hojas se liberan antes), y finalmente se limpia el
    // vector 'hijos' de esta carpeta, liberando cada shared_ptr.
    // Con weak_ptr en el padre no hay ciclos, así que no hay fugas.
    void eliminarTodoRecursivo();
};

#endif