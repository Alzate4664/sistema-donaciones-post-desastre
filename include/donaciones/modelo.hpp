#pragma once
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
namespace donaciones {
using Fecha = std::chrono::sys_days;
Fecha parsear_fecha(const std::string &);
std::string texto_fecha(Fecha);
void exigir(bool, const std::string &);
void identificador(const std::string &);
struct Donante {
    std::string id, nombre;
    std::int64_t centavos = 0;
};
struct Centro {
    std::string id, nombre;
    bool frio = false;
};
struct Municipio {
    std::string id, nombre;
    int urgencia = 1;
};
struct Lote {
    std::string id, donante, centro, categoria;
    Fecha caducidad;
    int cantidad = 0;
    double peso = 0, volumen = 0;
    bool frio = false;
    int disponible = -1, reservado = 0, despachado = 0;
};
struct Solicitud {
    std::string id, municipio, categoria;
    int cantidad = 0;
    std::int64_t llegada = 0;
    int pendiente = -1;
};
struct Ruta {
    std::vector<std::string> nodos;
    double minutos = 0;
};
struct Propuesta {
    std::string solicitud, lote;
    int cantidad = 0;
    Ruta ruta;
    std::uint64_t version = 0, version_red = 0;
    Fecha salida;
    double capacidad_peso = 0, capacidad_volumen = 0;
};
struct Despacho {
    std::string id, solicitud, lote;
    int cantidad = 0;
    Ruta ruta;
    Fecha salida;
};
struct Informe {
    std::int64_t recibido = 0, disponible = 0, reservado = 0, despachado = 0, centavos = 0;
    std::vector<std::string> alertas;
};
} // namespace donaciones
