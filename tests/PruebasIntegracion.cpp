#include "Archivo.h"
#include "Auditoria.h"
#include "Carpeta.h"
#include "GrafoRed.h"
#include "LoggerAuditoria.h"
#include "Menu.h"
#include "Sistema.h"
#include "Servidor.h"
#include "TablaHashUsuarios.h"

#include <cstdio>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {
int comprobaciones = 0;
int fallos = 0;

void verificar(bool condicion, const std::string& descripcion) {
    ++comprobaciones;
    if (!condicion) {
        ++fallos;
        std::cerr << "FALLO: " << descripcion << "\n";
    }
}

std::string unirRuta(const std::string& directorio, const std::string& nombre) {
    if (directorio.empty()) return nombre;
    const char ultimo = directorio[directorio.size() - 1];
    return directorio + ((ultimo == '/' || ultimo == '\\') ? "" : "/") + nombre;
}

std::string leerArchivo(const std::string& ruta) {
    std::ifstream archivo(ruta.c_str());
    std::ostringstream contenido;
    contenido << archivo.rdbuf();
    return contenido.str();
}

void probarTablaHash() {
    TablaHashUsuarios tabla(0);  // límite: capacidad solicitada nula
    verificar(tabla.numBuckets() == 1, "la tabla hash conserva al menos un bucket");
    verificar(tabla.insertar(Usuario(1, "ana", "clave")), "inserción normal de usuario");
    verificar(tabla.insertar(Usuario(2, "bea", "clave2")), "segunda inserción en bucket compartido");
    verificar(!tabla.insertar(Usuario(3, "ana", "otra")), "rechazo de nombre duplicado");
    verificar(!tabla.insertar(Usuario(1, "otro_nombre", "otra")) &&
                  tabla.contieneId(1) && !tabla.contieneId(99),
              "unicidad de ID de usuario y búsqueda de ID");
    verificar(!tabla.insertar(Usuario(-1, "negativo", "clave")), "rechazo de ID de usuario negativo");
    verificar(!tabla.insertar(Usuario(4, "", "vacia")), "rechazo de nombre vacío");
    verificar(tabla.tamano() == 2 && tabla.colisionesEn(0) == 2,
              "cantidad y cadena de colisiones consistentes");
    verificar(tabla.buscar("ana") != nullptr && tabla.buscar("desconocido") == nullptr,
              "búsqueda existente e inexistente");
    verificar(tabla.verificarSesion("ana", "clave") != nullptr,
              "autenticación con credenciales correctas");
    verificar(tabla.verificarSesion("ana", "incorrecta") == nullptr,
              "rechazo de contraseña incorrecta");
    Usuario credencial(99, "hashcheck", "secreto-hash");
    std::ostringstream informacion;
    std::streambuf* salidaAnterior = std::cout.rdbuf(informacion.rdbuf());
    credencial.mostrarInformacion();
    std::cout.rdbuf(salidaAnterior);
    verificar(credencial.verificarCredenciales("hashcheck", "secreto-hash") &&
                  !credencial.verificarCredenciales("hashcheck", "incorrecta") &&
                  informacion.str().find("secreto-hash") == std::string::npos &&
                  informacion.str().find("[protegida]") != std::string::npos,
              "password derivation works and is not exposed in user display");
    tabla.rehash(17);
    verificar(tabla.numBuckets() == 17 && tabla.tamano() == 2 &&
                  tabla.contiene("ana") && tabla.contiene("bea"),
              "rehash conserva elementos y consistencia de conteo");
    tabla.rehash(0);
    verificar(tabla.numBuckets() == 1 && tabla.tamano() == 2,
              "rehash con cero normaliza capacidad sin perder usuarios");
    verificar(tabla.eliminar("ana") && !tabla.eliminar("ana") &&
                  tabla.tamano() == 1 && !tabla.contiene("ana"),
              "eliminación normal y repetida sin corromper el conteo");
}

void probarArbolArchivos() {
    std::shared_ptr<Carpeta> raiz = std::make_shared<Carpeta>(0, "/");
    std::shared_ptr<Carpeta> docs = raiz->crearSubdirectorio("/docs//reportes/");
    verificar(docs != nullptr && docs->obtenerRuta() == "/docs/reportes",
              "creación recursiva y normalización de ruta");
    std::shared_ptr<Archivo> reporte = raiz->crearArchivo("docs/reportes/a.txt", "abc");
    verificar(reporte != nullptr && reporte->obtenerTamano() == 3 &&
                  reporte->obtenerRuta() == "/docs/reportes/a.txt",
              "archivo, tamaño y enlace padre correctos");
    verificar(raiz->buscarPorRuta("/docs/reportes/a.txt") == reporte &&
                  raiz->buscarRecursivoPorNombre("a.txt") == reporte,
              "búsquedas por ruta y por nombre encuentran el mismo nodo");
    verificar(!raiz->crearArchivo("docs/reportes/a.txt", "duplicado"),
              "rechazo de archivo duplicado");
    verificar(!raiz->crearArchivo("/", "sin_nombre") &&
                  raiz->buscarPorRuta("/no/existe") == nullptr,
              "rechazo de ruta sin nombre y búsqueda inexistente");
    verificar(reporte->getPadre() == docs && docs->cantidadHijos() == 1,
              "enlaces padre/hijo consistentes antes de borrar");
    verificar(raiz->eliminarRecursivo("/docs") &&
                  raiz->buscarPorRuta("/docs/reportes/a.txt") == nullptr &&
                  raiz->estaVacia(),
              "eliminación recursiva libera el subárbol completo");
    verificar(!raiz->eliminarRecursivo("/docs"), "eliminar una ruta inexistente falla limpiamente");
}

void probarGrafo() {
    GrafoRed grafo;
    verificar(!grafo.agregarServidor(-1, "invalido"), "rechazo de ID de servidor negativo");
    verificar(grafo.agregarServidor(1, "uno") && grafo.agregarServidor(2, "dos") &&
                  grafo.agregarServidor(3, "tres") && grafo.agregarServidor(4, "aislado"),
              "registro de nodos de red");
    verificar(!grafo.agregarServidor(1, "duplicado"), "rechazo de nodo duplicado");
    verificar(grafo.agregarArista(1, 2, 7) && grafo.agregarArista(2, 3, 8) &&
                  grafo.agregarArista(1, 3, 30), "creación de conexiones dirigidas");
    verificar(!grafo.agregarArista(1, 2, 1) && !grafo.agregarArista(1, 9, 1) &&
                  !grafo.agregarArista(2, 3, -1) && !grafo.agregarArista(4, 4, 0),
              "rechazo de duplicado, extremo faltante, latencia negativa y autoenlace");
    verificar(grafo.recorridoBFS(1) == std::vector<int>({1, 2, 3}) &&
                  grafo.recorridoDFS(1) == std::vector<int>({1, 2, 3}),
              "BFS y DFS visitan cada servidor alcanzable en orden estable");
    verificar(grafo.recorridoBFS(99).empty() && grafo.recorridoDFS(99).empty(),
              "recorridos sobre nodo inexistente devuelven vacío");
    verificar(grafo.servidoresAislados() == std::vector<int>({4}),
              "servidor aislado detectado contando enlaces entrantes y salientes");
    RutaOptima ruta = grafo.dijkstra(1, 3);
    verificar(ruta.existeCamino && ruta.camino == std::vector<int>({1, 2, 3}) &&
                  ruta.latenciaTotal == 15,
              "Dijkstra selecciona ruta de menor latencia");
    verificar(!grafo.dijkstra(3, 1).existeCamino &&
                  grafo.dijkstra(3, 3).existeCamino &&
                  !grafo.dijkstra(99, 1).existeCamino,
              "Dijkstra valida sentido, origen igual a destino y nodo ausente");
    GrafoRed limite;
    limite.agregarServidor(1, "origen");
    limite.agregarServidor(2, "destino");
    limite.agregarArista(1, 2, (std::numeric_limits<int>::max)());
    RutaOptima rutaMaxima = limite.dijkstra(1, 2);
    verificar(rutaMaxima.existeCamino &&
                  rutaMaxima.latenciaTotal == (std::numeric_limits<int>::max)(),
              "Dijkstra admite latencia máxima representable por una arista");
    limite.agregarServidor(3, "tercero");
    limite.agregarArista(2, 3, 1);
    RutaOptima sumaAmpliada = limite.dijkstra(1, 3);
    verificar(sumaAmpliada.existeCamino &&
                  sumaAmpliada.latenciaTotal == static_cast<long long>((std::numeric_limits<int>::max)()) + 1,
              "Dijkstra acumula rutas que exceden el rango de int sin overflow");
    verificar(grafo.validarConexiones().empty(), "diagnóstico no encuentra enlaces mal formados");
    verificar(grafo.eliminarServidor(2) &&
                  grafo.cantidadServidores() == 3 && grafo.cantidadAristas() == 1 &&
                  grafo.latenciaEntre(1, 2) == -1 && grafo.latenciaEntre(1, 3) == 30,
              "eliminaciones actualizan nodos y enlaces entrantes/salientes");
    verificar(!grafo.eliminarServidor(2) && !grafo.eliminarArista(1, 2),
              "eliminaciones repetidas no alteran el grafo");
}

void probarIntegracion(const std::string& directorioTemporal) {
    const std::string prefijo = "network_os_integration_" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const std::string rutaLog = unirRuta(directorioTemporal, prefijo + "_network.log");
    const std::string rutaAuditoria = unirRuta(directorioTemporal, prefijo + "_audit.log");
    const std::string rutaMenuLog = unirRuta(directorioTemporal, prefijo + "_menu.log");
    const std::string rutaMenuAuditoria = unirRuta(directorioTemporal, prefijo + "_menu_audit.log");

    {
        LoggerAuditoria logger(rutaLog);
        Sistema sistema(2, rutaAuditoria, &logger, unirRuta(directorioTemporal, prefijo + "_users.db"), unirRuta(directorioTemporal, prefijo + "_state.db"));
        const std::string carpetaInexistente = unirRuta(
            directorioTemporal, prefijo + "_missing_parent");
        LoggerAuditoria loggerNoDisponible(unirRuta(carpetaInexistente, "audit.log"));
        Auditoria auditoriaNoDisponible(unirRuta(carpetaInexistente, "memory.log"));
        auditoriaNoDisponible.registrar("PRUEBA", "fallo de persistencia esperado");
        verificar(!loggerNoDisponible.append("PRUEBA", "no debe escribirse") &&
                      !auditoriaNoDisponible.guardar(),
                  "logger y auditoría informan fallos de persistencia");
        verificar(sistema.crearServidor(-1, "negativo") == nullptr &&
                      sistema.crearServidor(99, "   ") == nullptr,
                  "Sistema rechaza IDs negativos y nombres de servidor vacíos");
        sistema.getGrafo().agregarServidor(98, "nodo externo");
        verificar(sistema.crearServidor(98, "duplicado externo") == nullptr &&
                      sistema.cantidadServidores() == 0,
                  "Sistema evita desincronizar árbol y grafo ante ID de red ocupado");
        sistema.getGrafo().eliminarServidor(98);
        verificar(sistema.crearServidor(10, "norte") != nullptr &&
                      sistema.crearServidor(20, "centro") != nullptr &&
                      sistema.crearServidor(30, "sur") != nullptr,
                  "Sistema integra servidores con sus nodos de grafo");
        verificar(sistema.cantidadServidores() == 3 && sistema.getGrafo().cantidadServidores() == 3,
                  "conteos de servidores y nodos de red consistentes tras creación");
        verificar(sistema.crearServidor(10, "repetido") == nullptr &&
                      sistema.crearServidor(-5, "negativo") == nullptr,
                  "Sistema rechaza IDs repetidos y negativos");
        verificar(sistema.crearSubdirectorio(10, "/datos") != nullptr &&
                      sistema.crearArchivo(10, "/datos/estado.txt", "ok") != nullptr &&
                      sistema.buscarPorRuta(10, "/datos/estado.txt") != nullptr,
                  "Sistema integra creación y consulta del árbol de archivos");
        verificar(sistema.crearArchivo(999, "/fallo", "x") == nullptr &&
                      !sistema.eliminarRecursivo(10, "/no-existe"),
                  "operaciones del Sistema manejan servidor y ruta inexistentes");
        verificar(!sistema.registrarUsuario(1, "sin-admin", "secreto", 1) &&
                      sistema.asegurarAdministradorInicial(1, "operador", "secreto") &&
                      sistema.asegurarAdministradorInicial(1, "operador", "secreto") &&
                      sistema.getUsuarios().tamano() == 1 &&
                      sistema.iniciarSesion("operador", "secreto") &&
                      sistema.sesionEsAdmin() &&
                      !sistema.registrarUsuario(2, "operador", "duplicado") &&
                      !sistema.registrarUsuario(1, "otro", "secreto") &&
                      !sistema.registrarUsuario(3, "permiso-invalido", "secreto", 2) &&
                      sistema.registrarUsuario(2, "normal", "normalpass", 0) &&
                      !sistema.registrarUsuario(4, "normal", "duplicado", 0),
                  "registro y autenticación integrados para éxito y errores");
        sistema.cerrarSesion();
        verificar(sistema.iniciarSesion("normal", "normalpass") &&
                      !sistema.sesionEsAdmin() &&
                      !sistema.registrarUsuario(3, "sin-permiso", "x") &&
                      !sistema.eliminarUsuario("operador"),
                  "usuario NORMAL se autentica pero no puede administrar cuentas");
        sistema.cerrarSesion();
        verificar(sistema.iniciarSesion("operador", "secreto") &&
                      !sistema.eliminarUsuario("operador") &&
                      leerArchivo(rutaLog).find("ultimo administrador") != std::string::npos,
                  "el administrador no puede eliminar la ultima cuenta ADMIN");
        sistema.cerrarSesion();
        verificar(sistema.agregarConexionRed(10, 20, 4) &&
                      sistema.agregarConexionRed(20, 30, 6) &&
                      !sistema.agregarConexionRed(10, 20, 1) &&
                      !sistema.agregarConexionRed(10, 999, 1) &&
                      !sistema.agregarConexionRed(10, 30, -1),
                  "Sistema valida conexiones duplicadas, inexistentes y latencias inválidas");
        verificar(leerArchivo(rutaLog).find("<ERROR> Intento fallido de agregar conexion #10 -> #20")
                      != std::string::npos,
                  "intentos de conexión inválidos quedan en auditoría persistente");
        RutaOptima ruta = sistema.calcularRutaOptima(10, 30);
        verificar(ruta.existeCamino && ruta.latenciaTotal == 10 &&
                      !sistema.calcularRutaOptima(30, 10).existeCamino,
                  "enrutamiento integrado en las dos direcciones");
        verificar(sistema.getGrafo().recorridoBFS(10) == std::vector<int>({10, 20, 30}) &&
                      sistema.getGrafo().servidoresAislados().empty(),
                  "diagnóstico del grafo coincide con las conexiones del Sistema");
        verificar(sistema.eliminarServidor(20) && sistema.getServidor(20) != nullptr &&
                      !sistema.getGrafo().existeServidor(20) &&
                      sistema.getGrafo().cantidadAristas() == 0 &&
                      sistema.cantidadServidores() == 3,
                  "baja de un nodo de red limpia sus enlaces sin borrar el servidor de archivos");
        verificar(sistema.agregarServidorRed(20, "centro") &&
                      sistema.getGrafo().existeServidor(20),
                  "servidor existente puede reincorporarse a la topología de red");
        verificar(sistema.getAuditoria().cantidadEventos() > 0 &&
                      sistema.getAuditoria().guardar(),
                  "eventos integrados tienen fecha y se persisten en auditoría");
        verificar(logger.append("PRUEBA", "evento persistente") &&
                      leerArchivo(rutaLog).find("<PRUEBA> evento persistente") != std::string::npos &&
                      leerArchivo(rutaLog).find("[") != std::string::npos,
                  "logger agrega eventos persistentes con marca de tiempo");
        verificar(leerArchivo(rutaAuditoria).find("<AUT>") != std::string::npos &&
                      leerArchivo(rutaAuditoria).find("<RED>") != std::string::npos,
                  "auditoría guardada refleja autenticación y cambios de red");
    }

    {
        LoggerAuditoria logger(rutaMenuLog);
        Sistema sistema(4, rutaMenuAuditoria, &logger, unirRuta(directorioTemporal, prefijo + "_menu_users.db"), unirRuta(directorioTemporal, prefijo + "_menu_state.db"));
        sistema.crearServidor(1, "A");
        sistema.crearServidor(2, "B");
        sistema.agregarConexionRed(1, 2, 5);
        sistema.asegurarAdministradorInicial(50, "tester", "clave");
        std::istringstream entrada("3\n2\n1\n5\ntester\nclave\n2\n1\n51\nnormal\nnormalpass\n0\n-1\n3\ntester\n0\n5\n5\nnormal\nnormalpass\n2\n1\n0\n3\n8\n1\n2\n1\n1\n2\n1\n3\n4\n0\n5\n1\n0\n");
        std::ostringstream salida;
        std::streambuf* entradaAnterior = std::cin.rdbuf(entrada.rdbuf());
        std::streambuf* salidaAnterior = std::cout.rdbuf(salida.rdbuf());
        Menu menu(sistema, logger);
        menu.ejecutar();
        std::cin.rdbuf(entradaAnterior);
        std::cout.rdbuf(salidaAnterior);
        verificar(salida.str().find("Sin ruta dirigida: #2 -> #1") != std::string::npos &&
                      salida.str().find("PING OK #1 -> #2") != std::string::npos &&
                      salida.str().find("PING FALLIDO #2 -> #1") != std::string::npos &&
                      salida.str().find("latencia total: 5 ms") != std::string::npos &&
                      salida.str().find("Recorrido BFS: #1 A -> #2 B") != std::string::npos &&
                      salida.str().find("Recorrido DFS: #1 A -> #2 B") != std::string::npos &&
                      salida.str().find("Inicie sesion para acceder a red y enrutamiento") != std::string::npos &&
                      salida.str().find("Se requiere una sesion ADMIN") != std::string::npos &&
                      salida.str().find("Bienvenido, \"normal\" (NORMAL)") != std::string::npos &&
                      salida.str().find("Sesion cerrada para \"tester\"") != std::string::npos,
                  "menú protege el acceso y cubre sesión, conexiones, BFS, DFS, Dijkstra y ping");
        verificar(salida.str().find("PING OK #1 -> #2") != std::string::npos &&
                      leerArchivo(rutaMenuLog).find("<PING> PING FALLIDO #2 -> #1") != std::string::npos &&
                      leerArchivo(rutaMenuLog).find("<ACCESS_DENIED>") != std::string::npos,
                  "auditoría del menú conserva pings y accesos denegados");
    }

    {
        const std::string loginLog = unirRuta(directorioTemporal, prefijo + "_login.log");
        const std::string loginAuditoria = unirRuta(directorioTemporal, prefijo + "_login_audit.log");
        LoggerAuditoria logger(loginLog);
        Sistema sistema(4, loginAuditoria, &logger, unirRuta(directorioTemporal, prefijo + "_login_users.db"), unirRuta(directorioTemporal, prefijo + "_login_state.db"));
        sistema.asegurarAdministradorInicial(77, "menuuser", "clave-menu");

        std::istringstream entrada(
            "texto-invalido\n1\n2\n5\nno-existe\nx\n5\nmenuuser\nincorrecta\n"
            "5\nmenuuser\nclave-menu\n1\n0\n5\n1\n0\n");
        std::ostringstream salida;
        std::streambuf* entradaAnterior = std::cin.rdbuf(entrada.rdbuf());
        std::streambuf* salidaAnterior = std::cout.rdbuf(salida.rdbuf());
        Menu menu(sistema, logger);
        menu.ejecutar();
        std::cin.rdbuf(entradaAnterior);
        std::cout.rdbuf(salidaAnterior);

        const std::string texto = salida.str();
        verificar(texto.find("5. Iniciar sesion") != std::string::npos &&
                      texto.find("El usuario \"no-existe\" no existe") != std::string::npos &&
                      texto.find("Contrasena incorrecta para el usuario \"menuuser\"") != std::string::npos &&
                      texto.find("Sesion iniciada") != std::string::npos &&
                      texto.find("5. Cerrar sesion (menuuser)") != std::string::npos &&
                      texto.find("Inicie sesion para acceder a gestion de archivos") != std::string::npos &&
                      texto.find("Opcion invalida. Ingrese un numero del menu") != std::string::npos,
                  "menu principal ofrece login/logout, informa errores y gestiona entrada invalida");

        const std::size_t inicioArchivos = texto.find("GESTION DE ARCHIVOS");
        const std::size_t inicioSesion = texto.find("Sesion iniciada");
        const std::size_t cierre = texto.find("Sesion cerrada para \"menuuser\"");
        const std::size_t denegadoTrasCierre = texto.find(
            "Inicie sesion para acceder a gestion de archivos", cierre);
        verificar(inicioArchivos != std::string::npos && inicioSesion < inicioArchivos &&
                      cierre != std::string::npos &&
                      denegadoTrasCierre != std::string::npos &&
                      texto.find("GESTION DE ARCHIVOS", cierre) == std::string::npos,
                  "archivo accesible con sesion y bloqueado de nuevo despues del logout");
        std::remove(loginLog.c_str());
        std::remove(loginAuditoria.c_str());
    }

    std::remove(rutaLog.c_str());
    std::remove(rutaAuditoria.c_str());
    std::remove(rutaMenuLog.c_str());
    std::remove(rutaMenuAuditoria.c_str());
}

void probarPersistenciaUsuarios(const std::string& directorioTemporal) {
    const std::string prefijo = "network_os_users_" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const std::string ruta = unirRuta(directorioTemporal, prefijo + ".db");
    const std::string audit = unirRuta(directorioTemporal, prefijo + "_audit.log");
    const std::string rutaEstado = unirRuta(directorioTemporal, prefijo + "_state.db");
    {
        Sistema sistema(4, audit, nullptr, ruta, rutaEstado);
        verificar(sistema.asegurarAdministradorInicial(1, "admin_persist", "admin-secret") &&
                  sistema.iniciarSesion("admin_persist", "admin-secret") &&
                  sistema.registrarUsuario(2, "normal_a", "pass-a", 0) &&
                  sistema.registrarUsuario(3, "normal_b", "pass-b", 0),
                  "admin y dos cuentas normales quedan almacenados con credenciales derivadas");
        sistema.cerrarSesion();
        verificar(!sistema.tieneSesion() && sistema.getUsuarios().tamano() == 3 &&
                  sistema.iniciarSesion("normal_a", "pass-a"),
                  "cerrar sesion solo limpia la identidad activa y conserva las cuentas");
        sistema.cerrarSesion();
    }
    {
        Sistema reiniciado(4, audit, nullptr, ruta, rutaEstado);
        verificar(reiniciado.getUsuarios().tamano() == 3 &&
                  reiniciado.iniciarSesion("normal_a", "pass-a") &&
                  !reiniciado.iniciarSesion("admin_persist", "incorrecta"),
                  "usuarios y hashes sobreviven al reinicio y rechazan clave incorrecta");
        reiniciado.cerrarSesion();
        verificar(reiniciado.iniciarSesion("admin_persist", "admin-secret") &&
                  reiniciado.asegurarAdministradorInicial(99, "otro_admin", "otra-clave") &&
                  reiniciado.getUsuarios().tamano() == 3,
                  "admin persistente no se duplica en el reinicio");
        reiniciado.cerrarSesion();
        verificar(reiniciado.iniciarSesion("normal_a", "pass-a") &&
                  !reiniciado.registrarUsuario(4, "no_admin", "x", 1),
                  "usuario NORMAL persistido no puede crear cuentas ADMIN");
    }
    const std::string contenidoValido = leerArchivo(ruta);
    const std::string temporalBloqueado = ruta + ".tmp";
    std::filesystem::create_directory(temporalBloqueado);
    {
        Sistema escrituraFallidaConCuentas(4, audit, nullptr, ruta, rutaEstado);
        verificar(escrituraFallidaConCuentas.iniciarSesion("admin_persist", "admin-secret") &&
                  !escrituraFallidaConCuentas.registrarUsuario(5, "no_guardado", "secreto", 0) &&
                  escrituraFallidaConCuentas.getUsuarios().tamano() == 3 &&
                  leerArchivo(ruta) == contenidoValido,
                  "fallo al escribir no elimina cuentas ya persistidas ni confirma el alta");
    }
    std::filesystem::remove_all(temporalBloqueado);
    {
        std::ofstream corrupto(ruta.c_str(), std::ios::binary | std::ios::trunc);
        corrupto << "datos incompletos";
    }
    const std::string contenidoCorrupto = leerArchivo(ruta);
    {
        Sistema cargaFallida(4, audit, nullptr, ruta, rutaEstado);
        verificar(cargaFallida.getUsuarios().tamano() == 0 &&
                  !cargaFallida.asegurarAdministradorInicial(1, "admin_persist", "admin-secret") &&
                  leerArchivo(ruta) == contenidoCorrupto,
                  "archivo ilegible no es reemplazado por una tabla vacia");
    }
    verificar(!contenidoValido.empty(), "formato persistente se escribio en el archivo configurado");

    const std::string bloqueo = unirRuta(directorioTemporal, prefijo + "_bloqueo");
    { std::ofstream archivo(bloqueo.c_str()); archivo << "conservar"; }
    {
        Sistema escrituraFallida(4, audit, nullptr, unirRuta(bloqueo, "usuarios.db"), rutaEstado);
        verificar(!escrituraFallida.asegurarAdministradorInicial(1, "admin_fallo", "clave") &&
                  leerArchivo(bloqueo) == "conservar",
                  "fallo de escritura revierte el alta y conserva datos ajenos existentes");
    }
    std::remove(ruta.c_str());
    std::remove(rutaEstado.c_str());
    std::filesystem::remove_all(temporalBloqueado);
    std::remove(audit.c_str());
    std::remove(bloqueo.c_str());
}

void probarPersistenciaEstado(const std::string& directorioTemporal) {
    const std::string prefijo = "network_os_state_" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const std::string rutaEstado = unirRuta(directorioTemporal, prefijo + ".db");
    const std::string rutaUsuarios = unirRuta(directorioTemporal, prefijo + "_users.db");
    const std::string rutaAuditoria = unirRuta(directorioTemporal, prefijo + "_audit.log");
    {
        Sistema sistema(4, rutaAuditoria, nullptr, rutaUsuarios, rutaEstado);
        verificar(sistema.crearServidor(1, "uno") && sistema.crearServidor(2, "dos") &&
                  sistema.crearServidor(3, "tres") &&
                  sistema.crearSubdirectorio(1, "/docs/reportes") &&
                  sistema.crearArchivo(1, "/docs/reportes/datos.txt", "linea uno\ntexto|hex") &&
                  sistema.modificarContenidoArchivo(1, "/docs/reportes/datos.txt", "contenido actualizado\ncon separador|"),
                  "creacion de servidores y arbol con contenido persistible");
        verificar(sistema.agregarConexionRed(1, 2, 4) &&
                  sistema.agregarConexionRed(2, 3, 6) &&
                  sistema.agregarConexionRed(1, 3, 20),
                  "conexiones y costos quedan registrados en el grafo persistente");
        sistema.cerrarSesion();
        verificar(sistema.getServidor(1) != nullptr && sistema.cantidadAristas() == 3,
                  "cerrar sesion no limpia servidores ni topologia");
    }
    {
        Sistema reiniciado(4, rutaAuditoria, nullptr, rutaUsuarios, rutaEstado);
        auto servidor = reiniciado.getServidor(1);
        auto archivo = std::dynamic_pointer_cast<Archivo>(
            reiniciado.buscarPorRuta(1, "/docs/reportes/datos.txt"));
        RutaOptima ruta = reiniciado.calcularRutaOptima(1, 3);
        verificar(reiniciado.cantidadServidores() == 3 && servidor &&
                  servidor->getNombre() == "uno" && archivo &&
                  archivo->getContenido() == "contenido actualizado\ncon separador|" &&
                  archivo->obtenerRuta() == "/docs/reportes/datos.txt",
                  "reinicio restaura servidores, jerarquia, ruta y contenido exactos");
        verificar(reiniciado.cantidadAristas() == 3 &&
                  reiniciado.getGrafo().recorridoBFS(1) == std::vector<int>({1, 2, 3}) &&
                  reiniciado.getGrafo().recorridoDFS(1).size() == 3 &&
                  ruta.existeCamino && ruta.latenciaTotal == 10,
                  "reinicio restaura topologia y BFS, DFS y Dijkstra usan el grafo guardado");
        verificar(reiniciado.eliminarConexion(2, 3),
                  "eliminacion intencional de enlace se persiste");
    }
    {
        Sistema trasBaja(4, rutaAuditoria, nullptr, rutaUsuarios, rutaEstado);
        verificar(trasBaja.cantidadAristas() == 2 &&
                  trasBaja.getGrafo().dijkstra(1, 3).existeCamino &&
                  trasBaja.getGrafo().dijkstra(1, 3).latenciaTotal == 20 &&
                  trasBaja.buscarPorRuta(1, "/docs/reportes/datos.txt") != nullptr,
                  "reinicio conserva eliminacion del enlace y mantiene el arbol de archivos");
        verificar(trasBaja.eliminarRecursivo(1, "/docs") &&
                  trasBaja.eliminarServidor(3),
                  "baja de directorio y nodo de red se acepta");
    }
    {
        Sistema trasSegundaBaja(4, rutaAuditoria, nullptr, rutaUsuarios, rutaEstado);
        verificar(trasSegundaBaja.buscarPorRuta(1, "/docs") == nullptr &&
                  !trasSegundaBaja.getGrafo().existeServidor(3) &&
                  trasSegundaBaja.getServidor(3) != nullptr,
                  "reinicio conserva eliminacion de archivos y diferencia red de servidor de archivos");
    }
    const std::string contenidoPrevio = leerArchivo(rutaEstado);
    std::filesystem::create_directory(rutaEstado + ".tmp");
    {
        Sistema falloEscritura(4, rutaAuditoria, nullptr, rutaUsuarios, rutaEstado);
        verificar(falloEscritura.crearServidor(99, "no-confirmado") == nullptr &&
                  falloEscritura.getServidor(99) == nullptr &&
                  leerArchivo(rutaEstado) == contenidoPrevio,
                  "fallo de escritura revierte cambios y preserva el estado previo en disco");
    }
    const std::string rutaCorrupta = unirRuta(directorioTemporal, prefijo + "_corrupt_state.db");
    { std::ofstream corrupto(rutaCorrupta.c_str(), std::ios::binary); corrupto << "corrupto"; }
    const std::string corruptoAntes = leerArchivo(rutaCorrupta);
    {
        Sistema cargaCorrupta(4, rutaAuditoria, nullptr, rutaUsuarios, rutaCorrupta);
        verificar(cargaCorrupta.cantidadServidores() == 0 &&
                  cargaCorrupta.crearServidor(77, "no-sobrescribir") == nullptr &&
                  leerArchivo(rutaCorrupta) == corruptoAntes,
                  "estado corrupto no se interpreta como vacio ni se sobrescribe");
    }
    std::remove(rutaEstado.c_str());
    std::remove(rutaUsuarios.c_str());
    std::remove(rutaAuditoria.c_str());
    std::remove(rutaCorrupta.c_str());
    std::filesystem::remove_all(rutaEstado + ".tmp");
}
}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Uso: PruebasIntegracion <directorio-temporal>\n";
        return 2;
    }

    probarTablaHash();
    probarArbolArchivos();
    probarGrafo();
    probarIntegracion(argv[1]);
    probarPersistenciaUsuarios(argv[1]);
    probarPersistenciaEstado(argv[1]);

    std::cout << "Comprobaciones: " << comprobaciones << "\n";
    std::cout << "Fallos: " << fallos << "\n";
    return fallos == 0 ? 0 : 1;
}
