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

## prompts 
## 1
Revisa primero la estructura existente del proyecto y crea las clases base:

- Servidor: ID, nombre y carpeta raíz.
- NodoArchivo: clase base abstracta.
- Carpeta: hereda de NodoArchivo y contiene otros nodos.
- Archivo: hereda de NodoArchivo y almacena contenido.
- Usuario: ID, nombre y contraseña.
- ConexionRed: servidor origen, destino y latencia.

Requisitos:
- Usar POO, encapsulamiento y herencia.
- Separar declaraciones en .h e implementaciones en .cpp.
- Utilizar std::string, std::vector y smart pointers cuando corresponda.
- Evitar punteros propietarios sin gestión automática.
- Preparar las clases para las próximas fases.

No implementar todavía autenticación, operaciones completas de archivos ni algoritmos de red.

Crear únicamente los archivos necesarios y verificar que el código compile.

Al finalizar, resumir los cambios.

## 2

Usando las clases existentes, inicializa estas estructuras:

1. Árbol de directorios:
   - Cada servidor debe tener su carpeta raíz.
   - Utilizar std::unique_ptr para manejar los nodos.

2. Tabla hash de usuarios:
   - Crear una estructura propia usando std::vector.
   - Preparar buckets para manejar colisiones.
   - No usar std::unordered_map para la tabla principal.

3. Grafo de red:
   - Utilizar listas de adyacencia.
   - Representar servidores y conexiones con latencia.

4. Archivo de auditoría:
   - Preparar la estructura para registrar eventos.

Crear una clase central que inicialice y administre estas estructuras.

No implementar todavía búsqueda recursiva, autenticación, Dijkstra ni BFS/DFS.

Mantener los archivos .h y .cpp organizados.
Verificar compilación sin modificar código innecesario.

## 3 

Implementa un menú principal por consola:

1. Gestión de archivos
2. Gestión de usuarios
3. Red y enrutamiento
4. Auditoría
0. Salir

Requisitos:
- Utilizar std::cout y std::cin.
- Mantener el menú activo mediante un ciclo.
- Validar entradas inválidas, incluyendo letras.
- Permitir regresar al menú principal.
- Crear funciones independientes para cada sección.
- Mostrar "Funcionalidad pendiente" donde todavía no exista implementación.

Separar el menú en archivos .h y .cpp.

Conectar el menú con la clase central del sistema.

No implementar funcionalidades de las siguientes fases ni agregar bibliotecas externas.

Verificar compilación y funcionamiento.

## 4

Implementa una clase Auditoria para registrar eventos.

Requisitos:
- Utilizar <fstream> para manejar archivos.
- Crear network_audit_log.txt automáticamente.
- Utilizar std::ios::app para conservar registros.
- Obtener fecha y hora con <chrono> y <ctime>.
- Registrar eventos mediante un método registrarEvento().
- Implementar un método mostrarLogs().

Registrar:
- Inicio del programa.
- Inicialización de estructuras.
- Acceso a opciones del menú.
- Entradas incorrectas.
- Cierre del programa.

Integrar mostrarLogs() con la opción de auditoría del menú.

Manejar errores de lectura y escritura.

No implementar todavía diagnósticos de red ni modificar otras funcionalidades.

Compilar y comprobar que los registros se guarden correctamente.

## 5 

Verifica:

1. Todas las clases .h y .cpp compilan.
2. El servidor inicializa su carpeta raíz.
3. El árbol utiliza correctamente smart pointers.
4. La tabla hash propia se inicializa.
5. El grafo de red se inicializa.
6. El menú funciona y permite regresar.
7. Se controlan entradas inválidas.
8. El archivo network_audit_log.txt se crea.
9. Los eventos se registran con fecha y hora.
10. Los registros anteriores se conservan.
11. El programa cierra correctamente.
12. No existen fugas evidentes de memoria.

Compilar usando C++17 con advertencias activadas.

Si está disponible, utilizar sanitizadores para detectar errores de memoria.

Corregir únicamente errores relacionados con la Fase 1.

No agregar funcionalidades de fases posteriores ni reestructurar código que funciona.

Al finalizar, indicar:
- Archivos modificados.
- Pruebas realizadas.
- Errores corregidos.
- Estado final de la Fase 1.

  ## Fase 5. Diagnóstico y auditoría

### Objetivo

Implementar y verificar las funcionalidades de diagnóstico de red y auditoría, reutilizando las estructuras y clases existentes.

### Verificaciones

1. El grafo de red permite recorrer los servidores mediante BFS y DFS.
2. Los recorridos se realizan sobre el grafo de red, no sobre el árbol de directorios.
3. Se identifican correctamente los servidores aislados y los problemas de conectividad.
4. La simulación de ping produce resultados coherentes con las conexiones existentes.
5. Se detectan correctamente los servidores inaccesibles y las rutas inexistentes.
6. Los eventos relevantes se registran en el sistema de auditoría.
7. Los registros incluyen fecha y hora.
8. Los registros anteriores se conservan al agregar nuevos eventos.
9. Las funcionalidades de diagnóstico y consulta de auditoría están disponibles desde los menús correspondientes.
10. Se controlan las entradas inválidas y los errores de ejecución.
11. Las funcionalidades existentes de archivos, usuarios y enrutamiento continúan funcionando.
12. No se duplican clases, estructuras de datos ni funcionalidades existentes.

### Restricciones

* Revisar primero las clases `GrafoRed`, `LoggerAuditoria`, los menús y los módulos relacionados.
* Reutilizar las implementaciones existentes siempre que sea posible.
* No modificar funcionalidades ajenas al alcance de la Fase 5.
* No implementar funciones vacías ni resultados simulados que no correspondan al estado real de la red.
* Compilar utilizando C++17 con advertencias activadas.
* Ejecutar pruebas de diagnóstico y auditoría cuando el entorno lo permita.

### Entrega

Al finalizar, indicar:

* Archivos modificados.
* Funcionalidades implementadas.
* Pruebas realizadas y sus resultados.
* Errores encontrados y corregidos.
* Estado final de la Fase 5.
* Funcionalidades pendientes de verificar.

---

## Fase 6. Integración y pruebas

### Objetivo

Verificar la integración de los módulos del sistema, comprobar su funcionamiento conjunto y corregir errores de integración sin alterar innecesariamente la arquitectura existente.

### Verificaciones

1. Todas las clases `.h` y `.cpp` compilan correctamente.
2. Los módulos se integran sin duplicación de responsabilidades ni definiciones incompatibles.
3. El menú principal y los submenús permiten acceder a las funcionalidades y regresar correctamente.
4. El sistema de archivos permite crear, buscar, visualizar y eliminar archivos y carpetas.
5. La autenticación, las sesiones y los permisos funcionan según las reglas establecidas.
6. La tabla hash de usuarios conserva su funcionamiento y maneja correctamente las colisiones.
7. El grafo permite agregar y eliminar servidores y conexiones.
8. Dijkstra calcula correctamente las rutas y sus costos.
9. BFS y DFS recorren correctamente el grafo de red.
10. La auditoría registra los eventos correspondientes con fecha y hora.
11. Se controlan las entradas inválidas, las operaciones no permitidas y los casos límite.
12. Las estructuras de datos permanecen consistentes después de las operaciones de creación y eliminación.
13. Se verifica la persistencia de los datos que el proyecto tenga implementada, incluyendo la recuperación de servidores, conexiones, archivos y carpetas cuando corresponda.
14. Se comprueba que cerrar sesión no elimine los datos persistentes.
15. No existen fallos evidentes de memoria, referencias inválidas ni errores graves de ejecución.
16. El programa se cierra correctamente.

### Pruebas

* Ejecutar pruebas unitarias de las clases y estructuras de datos cuando estén disponibles.
* Ejecutar pruebas de integración entre los módulos.
* Probar flujos completos desde el menú principal.
* Probar entradas inválidas, elementos inexistentes, servidores desconectados y rutas no disponibles.
* Probar operaciones consecutivas de creación, consulta y eliminación.
* Verificar la persistencia mediante el cierre de sesión y el reinicio completo del programa, según las funcionalidades implementadas.
* Compilar utilizando C++17 con advertencias activadas.
* Si el entorno lo permite, utilizar sanitizadores para detectar errores de memoria.

### Restricciones

* Inspeccionar el código existente antes de realizar modificaciones.
* Mantener la arquitectura, las clases y las estructuras de datos actuales.
* Corregir los errores de integración sin reescribir módulos completos cuando no sea necesario.
* No eliminar funcionalidades para conseguir que las pruebas pasen.
* No afirmar que una prueba fue exitosa si no se ejecutó.
* No considerar una funcionalidad terminada únicamente porque su código existe.

### Entrega

Al finalizar, indicar:

* Archivos modificados.
* Pruebas unitarias y de integración realizadas.
* Resultado de la compilación.
* Errores encontrados y corregidos.
* Funcionalidades verificadas y funcionalidades pendientes.
* Estado final de la Fase 6.

---

## Auditoría integral. Verificación de las fases 1 a 7

### Objetivo

Realizar una auditoría completa del proyecto Network OS para comprobar que las siete fases estén correctamente implementadas, que sus funcionalidades sean operativas y que los módulos estén integrados.

### Verificaciones por fase

**Fase 1. Base del sistema**

1. Todas las clases `.h` y `.cpp` compilan.
2. El servidor inicializa su carpeta raíz.
3. El árbol utiliza correctamente smart pointers.
4. La tabla hash propia se inicializa.
5. El grafo de red se inicializa.
6. El menú funciona y permite regresar.
7. Se controlan entradas inválidas.
8. El archivo `network_audit_log.txt` se crea.
9. Los eventos se registran con fecha y hora.
10. Los registros anteriores se conservan.
11. El programa cierra correctamente.
12. No existen fugas evidentes de memoria.

**Fase 2. Sistema de directorios**

1. Se crean archivos y carpetas correctamente.
2. Funcionan las búsquedas por ruta y por nombre.
3. El árbol de directorios se visualiza correctamente.
4. La eliminación de archivos y carpetas funciona, incluida la eliminación en cascada.
5. Las estructuras permanecen consistentes después de las operaciones.
6. Se controlan las rutas inválidas y los elementos inexistentes.

**Fase 3. Autenticación y tabla hash**

1. El registro y la autenticación funcionan correctamente.
2. La tabla hash maneja las colisiones.
3. Se validan las credenciales y los identificadores duplicados.
4. Las sesiones se inician y cierran correctamente.
5. Los permisos se aplican según el rol del usuario.
6. Las operaciones protegidas rechazan accesos no autorizados.

**Fase 4. Red y enrutamiento**

1. Se pueden agregar y eliminar servidores.
2. Se pueden agregar y eliminar conexiones.
3. El grafo mantiene su consistencia.
4. Dijkstra calcula rutas y costos correctamente.
5. Se manejan servidores aislados y rutas inexistentes.
6. Los resultados corresponden a la topología real de la red.

**Fase 5. Diagnóstico y auditoría**

1. BFS y DFS recorren correctamente el grafo de red.
2. Se detectan servidores aislados y problemas de conectividad.
3. La simulación de ping produce resultados coherentes.
4. Los eventos se registran con fecha y hora.
5. Los registros anteriores se conservan.
6. Los resultados del diagnóstico son accesibles desde los menús.

**Fase 6. Integración y pruebas**

1. Todos los módulos funcionan de manera conjunta.
2. Se ejecutan pruebas unitarias, de integración y de flujo completo cuando sea posible.
3. Se validan las entradas inválidas y los casos límite.
4. Se verifica la consistencia de las estructuras de datos.
5. Se comprueba la compilación y el funcionamiento general.
6. Se corrigen los errores reproducibles y se ejecutan nuevamente las pruebas afectadas.

### Prueba integral del sistema

Ejecutar un flujo completo que incluya:

1. Inicio del programa.
2. Autenticación y gestión de permisos.
3. Creación, búsqueda y eliminación de archivos y carpetas.
4. Creación y administración de servidores y conexiones.
5. Cálculo de rutas mediante Dijkstra.
6. Diagnóstico de conectividad mediante BFS y DFS.
7. Registro y consulta de auditoría.
8. Cierre de sesión y verificación de que los datos persistentes se conservan.
9. Reinicio completo del programa y recuperación de los datos que deban persistir.
10. Comprobación del funcionamiento general después de las operaciones.

### Restricciones generales

* Revisar primero la estructura completa del repositorio y el estado de Git.
* Mantener la arquitectura y las funcionalidades existentes.
* No duplicar clases ni estructuras de datos.
* No introducir funcionalidades fuera del alcance del proyecto.
* No modificar módulos que funcionen correctamente sin una razón comprobable.
* Corregir los errores identificados y ejecutar pruebas de regresión.
* Compilar utilizando C++17 con advertencias activadas.
* Si el entorno lo permite, utilizar sanitizadores para detectar errores de memoria.
* No declarar una fase terminada si sus funcionalidades no se han verificado.
* Diferenciar entre funcionalidades implementadas, pruebas ejecutadas y resultados comprobados.

### Entrega final

Al finalizar, presentar:

1. Estado de cada una de las siete fases.
2. Archivos modificados.
3. Errores encontrados y correcciones realizadas.
4. Pruebas ejecutadas y resultados reales.
5. Resultado de la compilación.
6. Funcionalidades pendientes y limitaciones.
7. Instrucciones para compilar, ejecutar y demostrar el sistema.

**Criterio de aceptación:** el proyecto debe compilar, ejecutar sus funcionalidades principales e integrar correctamente sus módulos. Toda funcionalidad declarada como correcta debe contar con evidencia de verificación; las que no pudieron probarse deben identificarse expresamente como no verificadas.


