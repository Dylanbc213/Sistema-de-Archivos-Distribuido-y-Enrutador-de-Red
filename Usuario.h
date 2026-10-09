#ifndef USUARIO_H
#define USUARIO_H

#include <string>

// =====================================================================
// Usuario: identidad dentro del Network OS.
// =====================================================================
// Guarda ID, nombre y contraseña. La autenticación y el control de
// roles se implementarán en una fase posterior; aquí solo se modela la
// entidad y su persistencia básica.
class Usuario {
private:
    int id;
    std::string nombre;
    std::string contrasena;

public:
    // Constructores
    Usuario();
    Usuario(int id, const std::string& nombre, const std::string& contrasena);

    // Getters
    int getId() const;
    std::string getNombre() const;
    std::string getContrasena() const;

    // Setters (con validación básica: no aplican valores inválidos)
    void setId(int id);
    void setNombre(const std::string& nombre);
    void setContrasena(const std::string& contrasena);

    // Utilidades
    // Comprueba si el nombre y la contraseña dados coinciden con los
    // almacenados. Es la base sobre la que se construirá la autenticación.
    bool verificarCredenciales(const std::string& nombre,
                               const std::string& contrasena) const;

    // Imprime los datos del usuario (la contraseña se oculta con '*').
    void mostrarInformacion() const;

    // Igualdad por ID (identidad del usuario).
    bool operator==(const Usuario& otro) const;
    bool operator!=(const Usuario& otro) const;
};

#endif