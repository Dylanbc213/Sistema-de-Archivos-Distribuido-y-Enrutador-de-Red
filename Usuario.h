#ifndef USUARIO_H
#define USUARIO_H

#include <string>

// =====================================================================
// NivelPermiso: roles de acceso dentro del Network OS.
// =====================================================================
// NORMAL : usuario estandar (solo sus propios recursos).
// ADMIN  : privilegios elevados (alta/baja de usuarios, etc.).
// Se utiliza tanto por la tabla hash como por la autenticacion y la
// auditoria de la Fase 3.
enum class NivelPermiso {
    NORMAL,   // acceso estandar
    ADMIN     // privilegios administrativos
};

// =====================================================================
// Usuario: identidad y credenciales dentro del Network OS.
// =====================================================================
// Guarda ID, nombre, contrasena (texto plano segun el alcance actual) y
// metadatos de autorizacion: nivel de permiso y el ID del servidor al que
// esta asignado (-1 indica "sin servidor asignado").
//
// La clase esta pensada para almacenarse POR VALOR dentro de la tabla hash
// (se copia de forma segura gracias a std::string). La verificacion de
// credenciales se basa en la comparacion directa de nombre y contrasena;
// en fases posteriores la contrasena puede cifrarse o hashear sin cambiar
// la interfaz de verificarCredenciales.
//
// El constructor de 3 argumentos (id, nombre, contrasena) conserva la
// misma firma del proyecto original, por compatibilidad con codigo
// existente; los campos nuevos toman valores por defecto.
class Usuario {
private:
    int id;                       // identificador unico
    std::string nombre;           // clave de busqueda en la tabla hash
    std::string contrasena;       // texto plano (alcance actual)
    NivelPermiso permisos;        // nivel de acceso del usuario
    int servidorAsignado;         // ID del servidor asignado (-1 = ninguno)

public:
    // Constructores
    Usuario();
    Usuario(int id, const std::string& nombre, const std::string& contrasena,
            NivelPermiso permisos = NivelPermiso::NORMAL,
            int servidorAsignado = -1);

    // Getters
    int getId() const;
    std::string getNombre() const;
    std::string getContrasena() const;
    NivelPermiso getPermisos() const;
    std::string getPermisoStr() const;   // representacion legible del nivel
    int getServidorAsignado() const;

    // Setters (con validacion basica: no aplican valores invalidos)
    void setId(int id);
    void setNombre(const std::string& nombre);
    void setContrasena(const std::string& contrasena);
    void setPermisos(NivelPermiso permisos);
    void setServidorAsignado(int servidorAsignado);

    // atajo: permiso == ADMIN
    bool esAdmin() const;

    // Utilidades
    // Comprueba si el nombre y la contrasena dados coinciden con los
    // almacenados. Es la base sobre la que se construira la autenticacion:
    // la tabla hash localiza al usuario en O(1) promedio y este metodo
    // valida la credencial en tiempo constante respecto al almacenamiento.
    bool verificarCredenciales(const std::string& nombre,
                               const std::string& contrasena) const;

    // Imprime los datos del usuario. La contrasena se oculta con '*' para
    // no exponerla en pantalla.
    void mostrarInformacion() const;

    // Igualdad por ID (identidad del usuario).
    bool operator==(const Usuario& otro) const;
    bool operator!=(const Usuario& otro) const;
};

#endif
