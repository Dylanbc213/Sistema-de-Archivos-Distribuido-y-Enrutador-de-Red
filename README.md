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

### Persistencia de cuentas

Las cuentas se cargan antes de mostrar el menú y se guardan en
`%LOCALAPPDATA%\NetworkOS\usuarios.db` (por ejemplo,
`C:\Users\<usuario>\AppData\Local\NetworkOS\usuarios.db`). El archivo conserva
ID, nombre, rol, servidor asignado, sal aleatoria y derivación PBKDF2-HMAC-SHA256;
nunca contiene la contraseña en texto plano. El archivo se actualiza mediante
un temporal y reemplazo atómico. Si la lectura o escritura falla, el alta o
cambio se rechaza y la cuenta en memoria se revierte para no pisar datos
previos. Para pruebas o instalaciones aisladas, `Sistema` acepta una ruta de
almacenamiento explícita en su cuarto argumento.

Las versiones anteriores no persistían las cuentas y no hay contraseñas
legibles que puedan migrarse de forma segura. Deben registrarse de nuevo si no
existe un `usuarios.db` previo. El administrador inicial se crea solo cuando
no hay un ADMIN cargado y se configura con `NETWORK_OS_ADMIN_USER` y
`NETWORK_OS_ADMIN_PASSWORD`; si ya hay un administrador guardado, se reutilizan
las cuentas existentes y no se duplica ni reemplaza.

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

La búsqueda por nombre conserva O(1) promedio. La inserción también comprueba
unicidad del ID en los buckets existentes, por lo que su costo actual es O(n).

### Gestión de usuarios y credenciales

`Usuario` almacena: `id`, `nombre` (clave de la tabla hash), sal y derivación
PBKDF2-HMAC-SHA256 de la contraseña, `NivelPermiso` (`NORMAL`/`ADMIN`) y
`servidorAsignado` (ID del servidor al que pertenece; `-1` = sin servidor).
`Sistema::registrarUsuario(...)` requiere una sesión ADMIN y
`Sistema::iniciarSesion(...)` valida las credenciales en **O(1) promedio**
(`TablaHashUsuarios::verificarSesion` → lookup hash + `verificarCredenciales`).
La contraseña usa una sal aleatoria BCrypt y 600 000 iteraciones PBKDF2 mediante
Windows CNG; el hash se compara en tiempo constante. No se guarda la contraseña
original ni se muestra su longitud.

No hay credenciales predeterminadas. Antes de ejecutar, configura
`NETWORK_OS_ADMIN_USER` y `NETWORK_OS_ADMIN_PASSWORD`; `main` provisiona esa
cuenta con rol ADMIN. La provisión es idempotente dentro de una instancia.
Por ejemplo, en PowerShell:

```powershell
$env:NETWORK_OS_ADMIN_USER = 'admin'
$env:NETWORK_OS_ADMIN_PASSWORD = 'elige-una-clave'
g++ -std=c++17 -Wall -Wextra -O2 *.cpp -lbcrypt -o NetworkOS.exe
.\NetworkOS.exe
```

### Auditoría transaccional

Cada **registro de usuario** y cada **intento de inicio de sesión** (éxito o
fracaso) se escribe **automáticamente** en `network_audit_log.txt` (modo
*append*, inmediato) mediante `LoggerAuditoria`, además de quedar en la traza
volátil `Auditoria` (que `guardarAuditoria()` vierte a `auditoria.log` al salir).
Formato de la línea: `[AAAA-MM-DD HH:MM:SS] <USUARIO|AUT|ERROR> detalle`.

### Menú de gestión de usuarios

La opción 2 del menú principal solo permite entrar a un ADMIN autenticado.
Desde allí puede registrar usuarios, listar cuentas y eliminar usuarios; no
puede eliminarse la cuenta activa ni quedar el sistema sin administrador.
El inicio y cierre de sesión se realizan desde la opción 5 del menú principal.

---

En la interfaz, las funciones de archivos, red y auditoría requieren una sesión.
La opción 5 del menú principal inicia sesión cuando no hay usuario conectado y
cierra la sesión actual cuando ya existe; el nombre y el rol conectado se
muestran en el menú. Las funciones de archivos, red y auditoría requieren
cualquier sesión. Los roles no restringen esas tres áreas.

Los usuarios se conservan en `usuarios.db` según la ubicación indicada arriba.
Los servidores, sus árboles de directorios y el grafo dirigido se guardan en
`%LOCALAPPDATA%\NetworkOS\sistema.db`. Cada alta, modificación o eliminación
se persiste atómicamente; si falla la escritura, la operación se revierte y se
informa el error. El archivo se carga al inicio sin duplicar elementos.

El registro persistente `network_audit_log.txt` y la traza `auditoria.log`
también se guardan en `%LOCALAPPDATA%\NetworkOS`; ambos conservan eventos entre
ejecuciones. Estas rutas no dependen del directorio de trabajo. Cerrar sesión
solo cambia la sesión activa y no limpia los datos del sistema.

### Persistencia y ubicación de los datos

En Windows, `Sistema::directorioDatosLocal()` resuelve el directorio
`%LOCALAPPDATA%\NetworkOS` (normalmente
`C:\Users\<usuario>\AppData\Local\NetworkOS`). El archivo `sistema.db`
contiene el estado de servidores, directorios, archivos, contenido, nodos y
conexiones de red. `usuarios.db` contiene las cuentas en un formato separado.
Los registros `auditoria.log` y `network_audit_log.txt` también están en ese
directorio. Para abrirlo desde el Explorador, escribe `%LOCALAPPDATA%\NetworkOS`
en la barra de direcciones. No edites los archivos de datos manualmente.

El estado del sistema se carga al construir `Sistema` y las operaciones de
alta, modificación y eliminación lo guardan. El cierre de sesión solo cambia
la sesión activa; el estado compartido se conserva al cambiar de usuario y al
reiniciar. Los cambios del estado se escriben mediante un archivo temporal y
reemplazo; si guardar falla, se informa el problema y se intenta revertir la
operación en memoria.

### Fase 5: diagnóstico de red y auditoría

Los recorridos BFS y DFS se ejecutan sobre el grafo de servidores, no sobre el
árbol de carpetas. El submenú **Red y enrutamiento** permite recorrer desde un
servidor, listar servidores aislados, validar conexiones y simular ping
dirigido entre pares. Dijkstra calcula rutas según las latencias de los
enlaces. La auditoría registra eventos con fecha y hora en archivos de texto;
el logger persistente usa modo append para conservar ejecuciones anteriores.

### Fase 6: integración y pruebas

`Sistema` coordina los módulos de servidores, árbol de archivos, usuarios,
grafo y auditoría; `Menu` expone sus operaciones. Las pruebas de integración
cubren estructuras, operaciones normales y errores, persistencia, diagnóstico
de red y auditoría. El script E2E compila y conduce el menú en un directorio
temporal aislado, reinicia el proceso y verifica que los datos y eventos se
recuperen. Las instrucciones siguientes permiten reproducir esas
comprobaciones; la presencia de una prueba en el repositorio no implica por sí
sola que se haya ejecutado en cada entorno.

## Compilacion

### Compilacion manual

```bash
g++ -std=c++17 -Wall -Wextra *.cpp -lbcrypt -o NetworkOS.exe
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

## Pruebas de integración

La suite `tests/PruebasIntegracion.cpp` cubre operaciones normales, errores y
límites de los árboles de archivos, la tabla hash, el grafo, la auditoría y el
flujo del menú de diagnóstico. Compílala y ejecútala desde PowerShell:

```powershell
$testExe = Join-Path $env:TEMP ('NetworkOSIntegrationTests_' + [guid]::NewGuid().ToString('N') + '.exe')
if (Test-Path -LiteralPath $testExe) { throw 'La ruta temporal ya existe; elija otra' }
g++ -std=c++17 -Wall -Wextra -O2 -I. tests/PruebasIntegracion.cpp Archivo.cpp Auditoria.cpp Carpeta.cpp ConexionRed.cpp GrafoRed.cpp LoggerAuditoria.cpp Menu.cpp NodoArchivo.cpp Servidor.cpp Sistema.cpp TablaHashUsuarios.cpp Usuario.cpp -lbcrypt -o $testExe
if ($LASTEXITCODE -ne 0) { throw 'Falló la compilación de las pruebas' }
& $testExe $env:TEMP
if ($LASTEXITCODE -ne 0) { throw 'Fallaron las pruebas de integración' }
Remove-Item -LiteralPath $testExe
```

Los archivos de auditoría de prueba se crean en el directorio temporal y se
eliminan al terminar la suite.

Para compilar y ejecutar el flujo real del menú en una carpeta temporal aislada,
incluyendo un segundo arranque para verificar persistencia:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tests/FlujoE2E.ps1
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
  5. Iniciar sesion
  Sesion: no iniciada
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
