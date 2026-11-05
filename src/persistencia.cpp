#include "donaciones/persistencia.hpp"
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
namespace donaciones {
namespace {
using Fila = std::vector<std::string>;
Fila csv(const std::string &linea) {
    Fila r;
    std::string campo;
    bool comillas = false, cerrado = false;
    for (size_t i = 0; i < linea.size(); i++) {
        char c = linea[i];
        if (comillas) {
            if (c == '"') {
                if (i + 1 < linea.size() && linea[i + 1] == '"') {
                    campo += '"';
                    i++;
                } else {
                    comillas = false;
                    cerrado = true;
                }
            } else
                campo += c;
        } else if (c == ',') {
            r.push_back(campo);
            campo.clear();
            cerrado = false;
        } else if (c == '"') {
            exigir(campo.empty() && !cerrado, "Comillas CSV mal ubicadas");
            comillas = true;
        } else {
            exigir(!cerrado, "Texto despues de comillas CSV");
            campo += c;
        }
    }
    exigir(!comillas, "Comillas CSV sin cerrar");
    r.push_back(campo);
    return r;
}
std::vector<Fila> leer(const std::filesystem::path &p, const Fila &cab) {
    std::ifstream f(p);
    exigir(bool(f), "No se pudo abrir " + p.string());
    std::string l;
    exigir(bool(std::getline(f, l)), "CSV vacio: " + p.string());
    if (!l.empty() && l.back() == '\r')
        l.pop_back();
    exigir(csv(l) == cab, "Cabecera incorrecta: " + p.string());
    std::vector<Fila> r;
    size_t num = 1;
    while (std::getline(f, l)) {
        ++num;
        if (!l.empty() && l.back() == '\r')
            l.pop_back();
        try {
            auto v = csv(l);
            exigir(v.size() == cab.size(), "Numero de campos incorrecto");
            r.push_back(std::move(v));
        } catch (const std::exception &e) {
            throw std::runtime_error(p.string() + " fila " + std::to_string(num) + ": " + e.what());
        }
    }
    exigir(!f.bad(), "Error leyendo " + p.string());
    return r;
}
template <class T> T entero(const std::string &s) {
    T n{};
    auto [p, e] = std::from_chars(s.data(), s.data() + s.size(), n);
    exigir(e == std::errc{} && p == s.data() + s.size(), "Entero invalido: " + s);
    return n;
}
double real(const std::string &s) {
    double v{};
    auto [p, e] = std::from_chars(s.data(), s.data() + s.size(), v);
    exigir(e == std::errc{} && p == s.data() + s.size() && std::isfinite(v),
           "Numero real invalido: " + s);
    return v;
}
bool booleano(const std::string &s) {
    exigir(s == "0" || s == "1", "Booleano debe ser 0 o 1");
    return s == "1";
}
std::string numero(double x) {
    std::ostringstream s;
    s << std::setprecision(17) << x;
    return s.str();
}
void fila(std::ostream &f, const Fila &v) {
    for (size_t i = 0; i < v.size(); i++) {
        if (i)
            f << ',';
        f << '"';
        for (char c : v[i]) {
            exigir(c != '\r' && c != '\n', "CSV no admite campos multilinea");
            if (c == '"')
                f << '"';
            f << c;
        }
        f << '"';
    }
    f << '\n';
}
void escribir(const std::filesystem::path &p, const Fila &cab, const std::vector<Fila> &rs) {
    auto tmp = p;
    tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::trunc);
        exigir(bool(f), "No se pudo escribir " + tmp.string());
        fila(f, cab);
        for (const auto &r : rs)
            fila(f, r);
        f.flush();
        exigir(bool(f), "Error guardando " + tmp.string());
    }
    std::error_code ec;
    std::filesystem::remove(p, ec);
    std::filesystem::rename(tmp, p);
}
template <class F> void procesar(const std::filesystem::path &p, const Fila &cab, F fn) {
    auto rs = leer(p, cab);
    for (size_t i = 0; i < rs.size(); i++)
        try {
            fn(rs[i]);
        } catch (const std::exception &e) {
            throw std::runtime_error(p.string() + " fila " + std::to_string(i + 2) + ": " +
                                     e.what());
        }
}
const Fila hc{"id", "nombre", "frio"}, hm{"id", "nombre", "urgencia"},
    hd{"id", "nombre", "centavos_COP"},
    hl{"id",        "donante",    "centro",         "categoria",
       "caducidad", "cantidad",   "peso_kg_unidad", "volumen_m3_unidad",
       "frio",      "disponible", "reservado",      "despachado"},
    hs{"id", "municipio", "categoria", "cantidad", "llegada", "pendiente"},
    hv{"origen", "destino", "minutos", "frio", "cerrada"},
    he{"id", "solicitud", "lote", "cantidad", "salida", "minutos", "ruta"};
} // namespace
Sistema cargar_datos(const std::filesystem::path &raiz) {
    auto dir = raiz;
    if (std::filesystem::exists(raiz / "actual.csv")) {
        auto filas = leer(raiz / "actual.csv", {"snapshot"});
        exigir(filas.size() == 1, "Puntero de snapshot invalido");
        auto n = filas[0][0];
        exigir(n.starts_with("version-") &&
                   n.find_first_not_of("version-0123456789") == std::string::npos,
               "Nombre de snapshot invalido");
        dir = raiz / n;
    }
    auto meta = leer(dir / "evento.csv", {"fecha"});
    exigir(meta.size() == 1, "Debe haber un solo evento");
    Sistema s(parsear_fecha(meta[0][0]));
    procesar(dir / "centros.csv", hc,
             [&](const Fila &v) { s.agregar(Centro{v[0], v[1], booleano(v[2])}); });
    procesar(dir / "municipios.csv", hm,
             [&](const Fila &v) { s.agregar(Municipio{v[0], v[1], entero<int>(v[2])}); });
    procesar(dir / "donantes.csv", hd,
             [&](const Fila &v) { s.agregar(Donante{v[0], v[1], entero<std::int64_t>(v[2])}); });
    procesar(dir / "lotes.csv", hl, [&](const Fila &v) {
        exigir(entero<int>(v[9]) >= 0 && entero<int>(v[10]) >= 0 && entero<int>(v[11]) >= 0,
               "Saldos CSV no pueden ser negativos");
        s.registrar_lote({v[0], v[1], v[2], v[3], parsear_fecha(v[4]), entero<int>(v[5]),
                          real(v[6]), real(v[7]), booleano(v[8]), entero<int>(v[9]),
                          entero<int>(v[10]), entero<int>(v[11])});
    });
    procesar(dir / "solicitudes.csv", hs, [&](const Fila &v) {
        exigir(entero<int>(v[5]) >= 0, "Pendiente CSV no puede ser negativo");
        s.solicitar(
            {v[0], v[1], v[2], entero<int>(v[3]), entero<std::int64_t>(v[4]), entero<int>(v[5])});
    });
    procesar(dir / "vias.csv", hv, [&](const Fila &v) {
        exigir((s.centros().contains(v[0]) || s.municipios().contains(v[0])) &&
                   (s.centros().contains(v[1]) || s.municipios().contains(v[1])),
               "Extremo de via inexistente");
        s.red.agregar(v[0], v[1], real(v[2]), booleano(v[3]));
        if (booleano(v[4]))
            s.red.cerrar(v[0], v[1], true);
    });
    std::vector<Despacho> ds;
    procesar(dir / "despachos.csv", he, [&](const Fila &v) {
        Ruta r;
        std::istringstream partes(v[6]);
        std::string n;
        while (std::getline(partes, n, '>')) {
            identificador(n);
            r.nodos.push_back(n);
        }
        r.minutos = real(v[5]);
        ds.push_back({v[0], v[1], v[2], entero<int>(v[3]), r, parsear_fecha(v[4])});
    });
    s.restaurar_historial(std::move(ds));
    return s;
}
static void guardar_archivos(const Sistema &s, const std::filesystem::path &dir) {
    std::filesystem::create_directories(dir);
    std::vector<Fila> rs;
    for (const auto &[id, c] : s.centros())
        rs.push_back({id, c.nombre, std::to_string(c.frio)});
    escribir(dir / "centros.csv", hc, rs);
    rs.clear();
    for (const auto &[id, m] : s.municipios())
        rs.push_back({id, m.nombre, std::to_string(m.urgencia)});
    escribir(dir / "municipios.csv", hm, rs);
    rs.clear();
    for (const auto &[id, d] : s.donantes())
        rs.push_back({id, d.nombre, std::to_string(d.centavos)});
    escribir(dir / "donantes.csv", hd, rs);
    rs.clear();
    for (const auto &[id, l] : s.lotes())
        rs.push_back({id, l.donante, l.centro, l.categoria, texto_fecha(l.caducidad),
                      std::to_string(l.cantidad), numero(l.peso), numero(l.volumen),
                      std::to_string(l.frio), std::to_string(l.disponible),
                      std::to_string(l.reservado), std::to_string(l.despachado)});
    escribir(dir / "lotes.csv", hl, rs);
    rs.clear();
    for (const auto &[id, x] : s.solicitudes())
        rs.push_back({id, x.municipio, x.categoria, std::to_string(x.cantidad),
                      std::to_string(x.llegada), std::to_string(x.pendiente)});
    escribir(dir / "solicitudes.csv", hs, rs);
    rs.clear();
    for (const auto &v : s.red.vias())
        rs.push_back({v.origen, v.destino, numero(v.minutos), std::to_string(v.frio),
                      std::to_string(v.cerrada)});
    escribir(dir / "vias.csv", hv, rs);
    rs.clear();
    for (const auto &d : s.despachos()) {
        std::string ruta;
        for (const auto &n : d.ruta.nodos) {
            if (!ruta.empty())
                ruta += '>';
            ruta += n;
        }
        rs.push_back({d.id, d.solicitud, d.lote, std::to_string(d.cantidad), texto_fecha(d.salida),
                      numero(d.ruta.minutos), ruta});
    }
    escribir(dir / "despachos.csv", he, rs);
    escribir(dir / "evento.csv", {"fecha"}, {{texto_fecha(s.evento())}});
}
void guardar_datos(const Sistema &s, const std::filesystem::path &raiz) {
    std::filesystem::create_directories(raiz);
    auto nombre = std::string("version-") +
                  std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    auto destino = raiz / nombre;
    exigir(std::filesystem::create_directory(destino), "No se pudo crear version unica");
    guardar_archivos(s, destino);
    // Solo publicamos un snapshot que tambien ha pasado la validacion de carga.
    auto verificado = cargar_datos(destino);
    (void)verificado;
    auto temporal = raiz / "actual.csv.tmp", actual = raiz / "actual.csv";
    {
        std::ofstream f(temporal, std::ios::trunc);
        exigir(bool(f), "No se pudo preparar puntero de version");
        fila(f, {"snapshot"});
        fila(f, {nombre});
        f.flush();
        exigir(bool(f), "Error escribiendo puntero de version");
    }
#ifdef _WIN32
    exigir(MoveFileExW(temporal.c_str(), actual.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0,
           "No se pudo publicar snapshot; se conserva la version anterior");
#else
    std::filesystem::rename(temporal, actual);
#endif
}
} // namespace donaciones
