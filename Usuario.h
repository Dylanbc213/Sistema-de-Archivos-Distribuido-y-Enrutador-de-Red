#ifndef USUARIO_H
#define USUARIO_H

#include <string>

// =====================================================================
// NivelPermiso: roles de acceso dentro del Network OS.
// =====================================================================
// NORMAL : puede usar los módulos que requieren sesión.
// ADMIN  : puede administrar las cuentas de usuario.
// Se utiliza tanto por la tabla hash como por la autenticacion y la
// auditoria de la Fase 3.
enum class NivelPermiso {
    NORMAL,   // acceso estandar
    ADMIN     // privilegios administrativos
};

// =====================================================================
// Usuario: identidad y credenciales dentro del Network OS.
// =====================================================================
// Guarda ID, nombre y la derivacion PBKDF2 de la contrasena con sal aleatoria;
// metadatos de autorizacion: nivel de permiso y el ID del servidor al que
// esta asignado (-1 indica "sin servidor asignado").
//
// La clase esta pensada para almacenarse POR VALOR dentro de la tabla hash
// (se copia de forma segura gracias a std::string). La verificacion de
// credenciales se validan con PBKDF2-HMAC-SHA256 y una sal aleatoria CNG.
//
// El constructor de 3 argumentos (id, nombre, contrasena) conserva la
// misma firma del proyecto original, por compatibilidad con codigo
// existente; los campos nuevos toman valores por defecto.
class Usuario {
private:
    int id;                       // identificador unico
    std::string nombre;           // clave de busqueda en la tabla hash
    std::string salContrasena;    // bytes aleatorios generados por BCrypt
    std::string hashContrasena;   // PBKDF2-HMAC-SHA256, nunca la clave original
    NivelPermiso permisos;        // nivel de acceso del usuario
    int servidorAsignado;         // ID del servidor asignado (-1 = ninguno)

public:
    static Usuario desdeDatosPersistidos(int id, const std::string& nombre,
        const std::string& sal, const std::string& hash,
        NivelPermiso permisos, int servidorAsignado);
    const std::string& salPersistida() const;
    const std::string& hashPersistido() const;
    // Constructores
    Usuario();
    Usuario(int id, const std::string& nombre, const std::string& contrasena,
            NivelPermiso permisos = NivelPermiso::NORMAL,
            int servidorAsignado = -1);

    // Getters
    int getId() const;
    std::string getNombre() const;
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
    bool tieneContrasenaValida() const;

    // Utilidades
    // Comprueba si el nombre y la contrasena dados coinciden con los
    // almacenados. Es la base sobre la que se construira la autenticacion:
    // la tabla hash localiza al usuario en O(1) promedio y este metodo
    // valida una derivacion de clave independiente del tamano de la tabla.
    bool verificarCredenciales(const std::string& nombre,
                               const std::string& contrasena) const;

    // Imprime los datos sin revelar la contrasena ni la derivacion almacenada.
    void mostrarInformacion() const;

    // Igualdad por ID (identidad del usuario).
    bool operator==(const Usuario& otro) const;
    bool operator!=(const Usuario& otro) const;
};

#endif
