# Sistema-de-Archivos-Distribuido-y-Enrutador-de-Red

Simulador de Network OS con arboles, tabla hash, grafos, Dijkstra, BFS/DFS y auditoria de operaciones.

---

## Descripcion
Simulador de Network OS con arboles, tabla hash, grafos, Dijkstra, BFS/DFS y auditoria de operaciones.

Network OS combina multiples estructuras de datos avanzadas:

- Arboles (carpetas/archivos) para el sistema de archivos distribuido.
- Tabla hash con encadenamiento para la gestion de usuarios.
- Grafos (listas de adyacencia) para la topologia de red y enrutamiento.
- Dijkstra / BFS / DFS para algoritmos de enrutamiento.
- Auditoria persistente de todas las operaciones del sistema.

---

## Arquitectura

```
┌────────────┐       ┌──────────┐
│    main    │──────▶│  Sistema │
└────────────┘       └──┬──┬───┘
                         │  │
          ┌──────────────┘  └──────────────┐
          ▼                                  ▼
   ┌────────────┐                   ┌──────────────┐
   │    Menu    │                   │   Auditoria  │
   └─────┬──────┘                   └──────┬───────┘
         │                                  │
         ▼                                  ▼
   ┌──────────┐   ┌─────────┐   ┌────────┐  ┌──────────┐
   │ Servidor │   │ TablaHash│  │ Grafo  │  │ Logger   │
   └────┬─────┘   └──────────┘  └────────┘  └──────────┘
        │
        ▼
   ┌────────┐
   │ Carpeta │ ◀── (árbol raíz)
   └────────┘
```

### Relaciones de propiedad

- **Sistema** es dueno unico de cada `Servidor` (nivel raiz).
- Cada **Servidor** posee su `Carpeta` raiz mediante `shared_ptr`.
- Cada **Carpeta** es duena de sus hijos (`NodoArchivo`) mediante `shared_ptr`.
- Los enlaces hijo -> padre usan `weak_ptr`, evitando ciclos de referencia.

---

## Fase 1: Base del arbol (completada)

| Clase | Rol |
|---|---|
| `NodoArchivo` | Clase base abstracta polimorfica. Hereda de `enable_shared_from_this`, almacena `weak_ptr<Carpeta> padre`, `id`, `nombre`. Metodos: `getId()`, `getNombre()`, `getTipo()`, `esCarpeta()`, `obtenerTamano()`, `obtenerRuta()`, `mostrar()`. |
| `Archivo` | Nodo hoja: archivo con contenido de texto. |
| `Carpeta` | Nodo contenedor: gestion de hijos y operaciones recursivas. |
| `Servidor` | Servidor con raiz `/`; delega operaciones al arbol. |

---

## Fase 2: Operaciones recursivas sobre directorios

### Operaciones implementadas

| Operacion | Descripcion |
|---|---|
| Crear carpeta | Crea subcarpeta y directorios intermedios recursivamente (`crearSubdirectorio`). |
| Crear archivo | Crea archivo y directorios intermedios recursivamente (`crearArchivo`). |
| Buscar por ruta | Navega jerarquicamente desde la raiz (`buscarporRuta`). |
| Buscar por nombre | Recorrido DFS completo del arbol (`buscarRecursivoPorNombre`). |
| Eliminar (cascada) | Borra un nodo y, si es carpeta, todo su subarbol en post-orden. |
| Ver arbol | Muestra el arbol con indentacion por nivel de profundidad. |

### Gestion de memoria (eliminacion en cascada)

La eliminacion de una carpeta y su subarbol se realiza en **post-orden**:
primero se recursivamente vacian las subcarpetas hijas (sus hojas se liberan
antes), y finalmente se limpia el vector `hijos` de la carpeta objetivo. Gracias
al uso de `weak_ptr` en los enlaces padre, **no hay ciclos de referencia** y toda
la memoria se libera automaticamente al destruirse los `shared_ptr`.

### Division de rutas

Las rutas siguen formato Unix: `/docs/reportes/archivo.txt`. El separador es
`/` y se ignoran componentes vacios (ej. `docs//archivos` -> `["docs", "archivos"]`).

---

## Fase 3: Autenticación y tabla hash

### Tabla hash (desde cero)

`TablaHashUsuarios` se reescribió desde cero: **NO** usa `std::unordered_map`,
`std::map` ni `std::hash`. La tabla principal es un **arreglo dinámico** de
**listas simples enlazadas** (`NodoLista`), y las colisiones se resuelven por
**encadenamiento separado** (chaining). La función de dispersión es matemática
y propia (polinomio generador modular `h = (h*131 + c) % 1000000007`), reducida
módulo el número de buckets. Operaciones `insertar`/`buscar`/`eliminar` en
**O(1) promedio**; el peor caso O(n) es teórico. El destructor libera todos los
nodos y el arreglo con `delete`/`delete[]`, **sin fugas**. El factor de carga se
controla con `rehash()`.

### Gestión de usuarios y credenciales

`Usuario` almacena: `id`, `nombre` (clave de la tabla hash), `contrasena`
(texto plano, según el alcance actual), `NivelPermiso` (`NORMAL`/`ADMIN`) y
`servidorAsignado` (ID del servidor al que pertenece; `-1` = sin servidor).
`Sistema::registrarUsuario(...)` inserta en la tabla hash y
`Sistema::iniciarSesion(...)` valida las credenciales en **O(1) promedio**
(`TablaHashUsuarios::verificarSesion` → lookup hash + `verificarCredenciales`).

### Auditoría transaccional

Cada **registro de usuario** y cada **intento de inicio de sesión** (éxito o
fracaso) se escribe **automáticamente** en `network_audit_log.txt` (modo
*append*, inmediato) mediante `LoggerAuditoria`, además de quedar en la traza
volátil `Auditoria` (que `guardarAuditoria()` vierte a `auditoria.log` al salir).
Formato de la línea: `[AAAA-MM-DD HH:MM:SS] <USUARIO|AUT|ERROR> detalle`.

### Menú de gestión de usuarios

La opción 2 del menú principal abre `GESTION DE USUARIOS` con: registrar usuario
(ID, nombre, contraseña, nivel de permiso, servidor opcional), iniciar sesión
(nombre + contraseña) y mostrar usuarios (volcado bucket a bucket con marcas de
colisión).

---

## Compilacion

### Compilacion manual

```bash
g++ -std=c++17 -Wall -Wextra *.cpp -o NetworkOS.exe
```

### En VS Code

Presionar `Ctrl+Shift+B` para compilar usando la tarea definida en
`.vscode/tasks.json`. La salida es `NetworkOS.exe`.

---

## Uso

Ejecutar el binario y seguir el menu interactivo:

```
NetworkOS.exe
```

Menu principal:
```
================================================
           N E T W O R K   O S
================================================
  1. Gestion de archivos
  2. Gestion de usuarios
  3. Red y enrutamiento
  4. Auditoria
  0. Salir
```

Ejemplo de flujo de trabajo:

```
> 1  (Gestion de archivos)
> 1  (Crear servidor)
> ID: 1
> Nombre: servidor1
> [+] Servidor #1 creado

> 4  (Crear carpeta)
> ID: 1
> Ruta: /docs/reportes
> [+] Carpeta creada: /docs/reportes

> 5  (Crear archivo)
> ID: 1
> Ruta: /docs/reportes/nota.txt
> Contenido: Hola Mundo
> [+] Archivo creado

> 3  (Ver arbol)
> ID: 1
[DIR] /
  [DIR] docs/
    [DIR] reportes/
      [FILE] nota.txt (10 bytes)

> 8  (Eliminar cascada)
> ID: 1
> Ruta: /docs/reportes
> [-] Elemento eliminado: /docs/reportes

> 3  (Ver arbol)
[DIR] /
  [DIR] docs/
```

---

## Auditoria

Todas las operaciones de arbol se registran en `auditoria.log` con la etiqueta
`ARBOL`, incluyendo:

- Creacion de servidores
- Creacion de carpetas y archivos
- Busquedas (por nombre y por ruta)
- Eliminaciones en cascada

El archivo `network_audit_log.txt` contiene el registro del logger de auditoria
persistente (modo append) con eventos de nivel MENU, SISTEMA, USUARIO y AUT
(registro e inicio de sesión de usuarios).

---

## Archivos

| Archivo | Descripcion |
|---|---|
| `NodoArchivo.h/.cpp` | Clase base abstracta (polimorfica) con `shared_from_this`. |
| `Archivo.h/.cpp` | Nodo hoja: archivo con contenido de texto. |
| `Carpeta.h/.cpp` | Nodo contenedor: gestion de hijos y operaciones recursivas. |
| `Servidor.h/.cpp` | Servidor con raiz `/`; delega operaciones al arbol. |
| `Sistema.h/.cpp` | Orquestador central: servidores, usuarios, grafo, auditoria. |
| `Menu.h/.cpp` | Interfaz de consola interactiva. |
| `TablaHashUsuarios.h/.cpp` | Tabla hash desde cero (arreglo dinámico de listas enlazadas + hash matemática propia, O(1) promedio). |
| `GrafoRed.h/.cpp` | Grafo dirigido de topologia de red. |
| `Usuario.h/.cpp` | Modelo de usuario: credenciales, NivelPermiso y servidor asignado. |
| `Auditoria.h/.cpp` | Registro en memoria de eventos del sistema. |
| `LoggerAuditoria.h/.cpp` | Logger persistente a archivo de texto. |
| `ConexionRed.h/.cpp` | Modelo de conexion de red. |
| `main.cpp` | Punto de entrada: crea Logger, Sistema y Menu. |
| `.vscode/tasks.json` | Configuracion de compilacion para VS Code (Ctrl+Shift+B). |
