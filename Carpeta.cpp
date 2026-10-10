#include "Carpeta.h"

#include "Archivo.h"   // Necesario para crearArchivo() — incluir solo en el .cpp

#include <algorithm>   // find_if
#include <iostream>
#include <sstream>     // dividirRuta: lectura de componentes separados por '/'

using namespace std;

// ---------------------------------------------------------------------
// Constructores / destructor
// ---------------------------------------------------------------------

// Constructor por defecto: carpeta anónima y vacía.
Carpeta::Carpeta() : NodoArchivo() {}

// Constructor principal: carpeta con identificador y nombre, sin hijos.
Carpeta::Carpeta(int id, const string& nombre) : NodoArchivo(id, nombre) {}

// Destructor: el vector libera automáticamente a los hijos (shared_ptr),
// por lo que no se gestiona memoria manualmente.
Carpeta::~Carpeta() {}

// ---------------------------------------------------------------------
// Interfaz abstracta
// ---------------------------------------------------------------------

// Tipo de nodo: siempre CARPETA.
TipoNodo Carpeta::getTipo() const { return TipoNodo::CARPETA; }

// Una carpeta siempre responde afirmativamente a esCarpeta().
bool Carpeta::esCarpeta() const { return true; }

// Tamaño de la carpeta: suma recursiva de los tamaños de sus hijos.
long long Carpeta::obtenerTamano() const {
    long long total = 0;
    for (const auto& hijo : hijos) {
        if (hijo) {
            total += hijo->obtenerTamano();
        }
    }
    return total;
}

// Imprime la carpeta y, recursivamente, todo su contenido indentado por
// nivel de profundidad.
void Carpeta::mostrar(int nivel) const {
    string sangria(static_cast<size_t>(nivel) * 2, ' ');
    cout << sangria << "[DIR] " << getNombre();
    // Evita el doble slash "/" cuando el nombre ya es la raíz "/".
    if (getNombre() != "/") {
        cout << "/";
    }
    cout << "\n";
    for (const auto& hijo : hijos) {
        if (hijo) {
            hijo->mostrar(nivel + 1);
        }
    }
}

// ---------------------------------------------------------------------
// Gestión de hijos
// ---------------------------------------------------------------------

// Indica si ya existe un hijo con ese nombre.
bool Carpeta::nombreOcupado(const string& nombre) const {
    for (const auto& hijo : hijos) {
        if (hijo && hijo->getNombre() == nombre) {
            return true;
        }
    }
    return false;
}

// Devuelve un puntero crudo const al hijo con el nombre dado (nullptr si no
// existe). Se usa internamente por la navegación recursiva para evitar
// crear shared_ptr temporales durante la búsqueda.
const NodoArchivo* Carpeta::obtenerHijoCrudo(const string& nombre) const {
    for (const auto& hijo : hijos) {
        if (hijo && hijo->getNombre() == nombre) {
            return hijo.get();
        }
    }
    return nullptr;
}

// Agrega un nodo como hijo y enlaza el padre. Devuelve false (y no
// modifica nada) si el nodo es nulo, es esta misma carpeta o ya hay un
// hijo con ese nombre.
bool Carpeta::agregarHijo(const shared_ptr<NodoArchivo>& nodo) {
    if (!nodo) {
        cerr << "[Carpeta] No se puede agregar un nodo nulo.\n";
        return false;
    }
    if (nodo.get() == this) {
        cerr << "[Carpeta] Una carpeta no puede contenerse a sí misma.\n";
        return false;
    }
    if (nombreOcupado(nodo->getNombre())) {
        cerr << "[Carpeta] Ya existe un nodo llamado '" << nodo->getNombre()
             << "' en '" << getNombre() << "'.\n";
        return false;
    }

    // Enlaza el padre solo si esta carpeta está gestionada por un
    // shared_ptr. Si se creó en la pila, shared_from_this() lanzaría
    // bad_weak_ptr; en tal caso se agrega el hijo sin enlace de padre en
    // lugar de abortar.
    try {
        nodo->setPadre(dynamic_pointer_cast<Carpeta>(shared_from_this()));
    } catch (const bad_weak_ptr&) {
        // Uso no gestionado por shared_ptr: se omite el enlace de padre.
    }

    hijos.push_back(nodo);
    return true;
}

// Elimina el primer hijo que coincida con el nombre indicado.
bool Carpeta::eliminarHijoPorNombre(const string& nombre) {
    for (auto it = hijos.begin(); it != hijos.end(); ++it) {
        if (*it && (*it)->getNombre() == nombre) {
            hijos.erase(it);
            return true;
        }
    }
    return false;
}

// Elimina el primer hijo que coincida con el ID indicado.
bool Carpeta::eliminarHijoPorId(int id) {
    for (auto it = hijos.begin(); it != hijos.end(); ++it) {
        if (*it && (*it)->getId() == id) {
            hijos.erase(it);
            return true;
        }
    }
    return false;
}

// Busca un hijo por nombre; devuelve nullptr si no existe.
shared_ptr<NodoArchivo> Carpeta::buscarHijoPorNombre(const string& nombre) const {
    for (const auto& hijo : hijos) {
        if (hijo && hijo->getNombre() == nombre) {
            return hijo;
        }
    }
    return nullptr;
}

// Busca un hijo por ID; devuelve nullptr si no existe.
shared_ptr<NodoArchivo> Carpeta::buscarHijoPorId(int id) const {
    for (const auto& hijo : hijos) {
        if (hijo && hijo->getId() == id) {
            return hijo;
        }
    }
    return nullptr;
}

// Acceso de solo lectura a la lista de hijos (no transfiere propiedad).
const vector<shared_ptr<NodoArchivo>>& Carpeta::getHijos() const {
    return hijos;
}

// Cantidad de hijos directos.
size_t Carpeta::cantidadHijos() const { return hijos.size(); }

// Indica si la carpeta no tiene hijos.
bool Carpeta::estaVacia() const { return hijos.empty(); }

// Elimina todos los hijos.
void Carpeta::vaciar() { hijos.clear(); }

// =====================================================================
// Fase 2: Operaciones recursivas sobre el árbol de directorios
// =====================================================================

// ---------------------------------------------------------------------
// Utilidades de ruta
// ---------------------------------------------------------------------

// Divide una ruta estilo Unix en componentes individuales.
// Usa stringstream y getline con '/' como delimitador; ignora
// componentes vacíos (equivalente a colapsar "docs//archivos").
vector<string> Carpeta::dividirRuta(const string& ruta) {
    vector<string> componentes;
    stringstream ss(ruta);
    string componente;
    while (getline(ss, componente, '/')) {
        if (!componente.empty()) {
            componentes.push_back(componente);
        }
    }
    return componentes;
}

// Navega hacia arriba por la cadena de padres hasta encontrar la raíz.
// Requiere shared_from_this(); protegido con try/catch por si la carpeta
// no está gestionada por un shared_ptr (caso de uso en pila).
shared_ptr<Carpeta> Carpeta::obtenerRaiz() {
    try {
        auto raiz = dynamic_pointer_cast<Carpeta>(shared_from_this());
        while (raiz && raiz->getPadre()) {
            raiz = raiz->getPadre();
                }
        return raiz;
    } catch (const bad_weak_ptr&) {
        // La carpeta no está gestionada por shared_ptr.
        return nullptr;
    }
}

// ---------------------------------------------------------------------
// Búsqueda recursiva
// ---------------------------------------------------------------------

// Búsqueda DFS (profundidad primero) por nombre: primero busca entre los
// hijos directos, luego desciende recursivamente en cada subcarpeta.
shared_ptr<NodoArchivo> Carpeta::buscarRecursivoPorNombre(
    const string& nombre) const {
    // 1) Buscar entre los hijos directos.
    shared_ptr<NodoArchivo> hijo = buscarHijoPorNombre(nombre);
    if (hijo) {
        return hijo;
    }

    // 2) Descendente recursiva en cada subcarpeta hija.
    for (const auto& h : hijos) {
        if (h && h->esCarpeta()) {
            auto subcarpeta = dynamic_pointer_cast<Carpeta>(h);
            if (subcarpeta) {
                shared_ptr<NodoArchivo> encontrado =
                    subcarpeta->buscarRecursivoPorNombre(nombre);
                if (encontrado) {
                    return encontrado;
                }
            }
        }
    }

    return nullptr;  // No encontrado en este subárbol.
}

// Busca un nodo siguiendo una ruta (absoluta o relativa).
// Para rutas absolutas (que empiezan con '/'), navega desde la raíz;
// para relativas, comienza desde esta carpeta.
// Usa punteros cruces (const Carpeta*) para la navegación: es seguro porque
// el vector 'hijos' mantiene vivas las memorias de los descendientes mientras
// esta carpeta vive, y solo se busca (no se modifica) el árbol.
shared_ptr<NodoArchivo> Carpeta::buscarPorRuta(const string& ruta) const {
    vector<string> componentes = dividirRuta(ruta);
    if (componentes.empty()) {
        // Ruta "/" o "" — representa la raíz misma. Desde un método const
        // no se puede devolver shared_from_this(); se devuelve nullptr y
        // el llamador (Servidor/Sistema) maneja el caso especial.
        return nullptr;
    }

    // Determinar el punto de partida.
    bool absoluta = (!ruta.empty() && ruta[0] == '/');
    const Carpeta* actual = this;
    if (absoluta) {
        // Navegar hacia la raíz subiendo por los padres.
        shared_ptr<Carpeta> padre = getPadre();
        while (padre) {
            actual = padre.get();
            padre = padre->getPadre();
        }
    }

    // Recorrer cada componente de la ruta.
    for (size_t i = 0; i < componentes.size(); ++i) {
        const string& comp = componentes[i];

        // Buscar el hijo con ese nombre en la carpeta actual.
        const NodoArchivo* hijo = actual->obtenerHijoCrudo(comp);
        if (!hijo) {
            return nullptr;  // Componente no encontrado.
        }

        if (i == componentes.size() - 1) {
            // Último componente: devolver el shared_ptr usando buscarHijoPorNombre.
            return actual->buscarHijoPorNombre(comp);
        }

        // Componente intermedio: debe ser una carpeta para poder navegar.
        if (!hijo->esCarpeta()) {
            return nullptr;  // Encontramos un archivo, no se puede seguir.
        }
            actual = static_cast<const Carpeta*>(hijo);
    }

    return nullptr;
}

// ---------------------------------------------------------------------
// Creación recursiva
// ---------------------------------------------------------------------

// Crea una subcarpeta en la ruta indicada, creando directorios intermedios
// si no existen. Se parte de la raíz para rutas absolutas y de 'this' para
// rutas relativas.
shared_ptr<Carpeta> Carpeta::crearSubdirectorio(const string& ruta) {
    vector<string> componentes = dividirRuta(ruta);
    if (componentes.empty()) {
        // No se puede crear la raíz (ya existe).
        cerr << "[Carpeta] No se puede crear la raiz. Use '/'.\n";
        return nullptr;
    }

    // Punto de partida: raíz (absoluta) o esta carpeta (relativa).
    bool absoluta = (!ruta.empty() && ruta[0] == '/');
    shared_ptr<Carpeta> actual;
    if (absoluta) {
        actual = obtenerRaiz();
    } else {
        try {
            actual = dynamic_pointer_cast<Carpeta>(shared_from_this());
        } catch (const bad_weak_ptr&) {
            actual = nullptr;
        }
    }
    if (!actual) {
        cerr << "[Carpeta] No se pudo determinar el punto de partida.\n";
        return nullptr;
    }

    // Navegar o crear los directorios intermedios (todos excepto el último).
    for (size_t i = 0; i + 1 < componentes.size(); ++i) {
        const string& comp = componentes[i];
        shared_ptr<NodoArchivo> hijo = actual->buscarHijoPorNombre(comp);

        if (!hijo) {
            // No existe: crear directorio intermedio.
            auto nueva = make_shared<Carpeta>(0, comp);
            if (!actual->agregarHijo(nueva)) {
                return nullptr;  // agregarHijo ya informó el error.
            }
            hijo = nueva;
        } else if (!hijo->esCarpeta()) {
            // Existe pero es un archivo: no se puede navegar.
            cerr << "[Carpeta] El componente '" << comp
                 << "' es un archivo, no una carpeta.\n";
            return nullptr;
        }

        actual = dynamic_pointer_cast<Carpeta>(hijo);
    }

    // Crear el directorio final.
    const string& nombreFinal = componentes.back();
    if (actual->nombreOcupado(nombreFinal)) {
        // Ya existe un nodo con ese nombre.
        shared_ptr<NodoArchivo> existente =
            actual->buscarHijoPorNombre(nombreFinal);
        if (existente && existente->esCarpeta()) {
            cerr << "[Carpeta] La subcarpeta '" << nombreFinal
                 << "' ya existe en '" << actual->getNombre() << "'.\n";
            return dynamic_pointer_cast<Carpeta>(existente);
        }
        cerr << "[Carpeta] Ya existe un archivo llamado '" << nombreFinal
             << "' en '" << actual->getNombre() << "'.\n";
        return nullptr;
    }

    auto nuevaCarpeta = make_shared<Carpeta>(0, nombreFinal);
        if (actual->agregarHijo(nuevaCarpeta)) {
        return nuevaCarpeta;
    }
    return nullptr;
}

// Crea un archivo en la ruta indicada, creando dirs intermedios si falta.
shared_ptr<Archivo> Carpeta::crearArchivo(const string& ruta,
                                          const string& contenido) {
    vector<string> componentes = dividirRuta(ruta);
    if (componentes.empty()) {
        cerr << "[Carpeta] La ruta no especifica un nombre de archivo.\n";
        return nullptr;
    }

    // Punto de partida: raíz (absoluta) o esta carpeta (relativa).
    bool absoluta = (!ruta.empty() && ruta[0] == '/');
    shared_ptr<Carpeta> actual;
    if (absoluta) {
        actual = obtenerRaiz();
    } else {
        try {
            actual = dynamic_pointer_cast<Carpeta>(shared_from_this());
        } catch (const bad_weak_ptr&) {
            actual = nullptr;
        }
    }
    if (!actual) {
        cerr << "[Carpeta] No se pudo determinar el punto de partida.\n";
        return nullptr;
    }

    // Navegar o crear los directorios intermedios (todos excepto el último).
    for (size_t i = 0; i + 1 < componentes.size(); ++i) {
        const string& comp = componentes[i];
        shared_ptr<NodoArchivo> hijo = actual->buscarHijoPorNombre(comp);

        if (!hijo) {
            // No existe: crear directorio intermedio.
            auto nueva = make_shared<Carpeta>(0, comp);
            if (!actual->agregarHijo(nueva)) {
                return nullptr;
            }
            hijo = nueva;
        } else if (!hijo->esCarpeta()) {
            cerr << "[Carpeta] El componente '" << comp
                 << "' es un archivo, no una carpeta.\n";
            return nullptr;
        }

        actual = dynamic_pointer_cast<Carpeta>(hijo);
    }

    // Crear el archivo final.
    const string& nombreFinal = componentes.back();
    if (actual->nombreOcupado(nombreFinal)) {
        cerr << "[Carpeta] Ya existe un nodo llamado '" << nombreFinal
             << "' en '" << actual->getNombre() << "'.\n";
        return nullptr;
    }

    auto nuevoArchivo = make_shared<Archivo>(0, nombreFinal, contenido);
    if (actual->agregarHijo(nuevoArchivo)) {
                return nuevoArchivo;
    }
    return nullptr;
}

// ---------------------------------------------------------------------
// Eliminación en cascada (post-order: hojas → raíces del subárbol)
// ---------------------------------------------------------------------

// Elimina TODO el contenido de esta carpeta en orden post-orden.
// Para cada hijo que sea Carpeta, se llama recursivamente a
// eliminarTodoRecursivo() (profundidad primero), de modo que las hojas
// se liberan antes que sus contenedoras. Finalmente se limpia el vector
// 'hijos', liberando los shared_ptr de los archivos hoja y de las
// subcarpetas ya vaciadas.
// Con weak_ptr en el padre no hay ciclos, así que no hay fugas.
void Carpeta::eliminarTodoRecursivo() {
    // Fase 1: recursivamente vaciar cada subcarpeta hija.
    // Las hojas (archivos y carpetas sin hijos) se llegan al final de
    // esta recursión y se liberan al limpiarse sus vectores.
    for (auto it = hijos.begin(); it != hijos.end();) {
        if (*it && (*it)->esCarpeta()) {
            auto subcarpeta = dynamic_pointer_cast<Carpeta>(*it);
            if (subcarpeta) {
                subcarpeta->eliminarTodoRecursivo();
            }
            // La subcarpeta está ahora vacía; liberarla del vector.
            it = hijos.erase(it);
        } else {
            ++it;
        }
    }

    // Fase 2: limpiar los archivos hoja restantes.
    // Al borrar el vector, cada shared_ptr se libera; como el padre usa
    // weak_ptr, no hay ciclos y la memoria se libera correctamente.
    hijos.clear();
}

// Elimina recursivamente el nodo ubicado en 'ruta' (absoluta o relativa).
// Si el nodo es una carpeta, se borra todo su subárbol en cascada
// (hojas primero, subiendo hasta la carpeta objetivo) y luego se la
// quita de la lista de hijos del padre.
bool Carpeta::eliminarRecursivo(const string& ruta) {
    vector<string> componentes = dividirRuta(ruta);
    if (componentes.empty()) {
        // No se puede eliminar la raíz.
        cerr << "[Carpeta] No se puede eliminar la raiz del sistema.\n";
        return false;
    }

    // Punto de partida: raíz (absoluta) o esta carpeta (relativa).
    bool absoluta = (!ruta.empty() && ruta[0] == '/');
    shared_ptr<Carpeta> actual;
    if (absoluta) {
        actual = obtenerRaiz();
    } else {
        try {
            actual = dynamic_pointer_cast<Carpeta>(shared_from_this());
        } catch (const bad_weak_ptr&) {
            actual = nullptr;
        }
    }
    if (!actual) {
        cerr << "[Carpeta] No se pudo determinar el punto de partida.\n";
        return false;
    }

    // Navegar hasta el PADRE del nodo objetivo (todos menos el último).
    for (size_t i = 0; i + 1 < componentes.size(); ++i) {
        const string& comp = componentes[i];
        shared_ptr<NodoArchivo> hijo = actual->buscarHijoPorNombre(comp);

        if (!hijo) {
            cerr << "[Carpeta] No existe el componente '" << comp << "'.\n";
            return false;
        }
        if (!hijo->esCarpeta()) {
            cerr << "[Carpeta] El componente '" << comp
                 << "' es un archivo, no una carpeta.\n";
            return false;
        }
        actual = dynamic_pointer_cast<Carpeta>(hijo);
    }

    // Localizar el nodo objetivo dentro de 'actual'.
    const string& nombreObjetivo = componentes.back();
    auto it = std::find_if(actual->hijos.begin(), actual->hijos.end(),
                           [&](const shared_ptr<NodoArchivo>& h) {
                               return h && h->getNombre() == nombreObjetivo;
                           });

    if (it == actual->hijos.end()) {
        cerr << "[Carpeta] No se encontro '" << nombreObjetivo << "'.\n";
        return false;
    }

    // Si es una carpeta, vaciar su subárbol COMPLETO primero (post-order:
    // hojas → subcarpetas → esta carpeta). Esto libera toda la memoria
    // de los descendientes antes de destruir el nodo objetivo.
    shared_ptr<NodoArchivo> nodoObjetivo = *it;
    if (nodoObjetivo && nodoObjetivo->esCarpeta()) {
        auto carpetaObjetivo = dynamic_pointer_cast<Carpeta>(nodoObjetivo);
        if (carpetaObjetivo) {
            carpetaObjetivo->eliminarTodoRecursivo();
        }
    }

    // Finalmente: quitar el nodo del vector del padre. Al borrar el
    // shared_ptr, la memoria se libera si nadie más lo referencia.
    actual->hijos.erase(it);

    return true;
}