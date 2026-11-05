#include "donaciones/persistencia.hpp"
#include "donaciones/sistema.hpp"
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <set>
using namespace donaciones;
std::filesystem::path archivos(const std::filesystem::path &p) {
    std::ifstream f(p / "actual.csv");
    std::string h, n;
    std::getline(f, h);
    std::getline(f, n);
    if (!n.empty() && n.back() == '\r')
        n.pop_back();
    if (n.size() > 2 && n.front() == '"')
        n = n.substr(1, n.size() - 2);
    return p / n;
}
int total = 0, fallos = 0;
void check(bool ok) {
    if (!ok)
        throw std::runtime_error("condicion incumplida");
}
template <class F> void rechaza(F f) {
    bool error = false;
    try {
        f();
    } catch (const std::exception &) {
        error = true;
    }
    check(error);
}
void caso(const char *n, std::function<void()> f) {
    ++total;
    try {
        f();
        std::cout << "OK " << n << '\n';
    } catch (const std::exception &e) {
        ++fallos;
        std::cerr << "FAIL " << n << ": " << e.what() << '\n';
    }
}
Sistema base() {
    Sistema s(parsear_fecha("2026-11-01"));
    s.agregar(Centro{"A", "Medellin", true});
    s.agregar(Municipio{"B", "Caldas", 5});
    s.agregar(Donante{"D", "Ana", 0});
    s.red.agregar("A", "B", 60, true);
    return s;
}
Lote lote(std::string id = "L", int cantidad = 10) {
    return {id, "D", "A", "agua", parsear_fecha("2026-12-01"), cantidad, 2, 1, false};
}
int main() {
    caso("fechas estrictas", [] {
        check(texto_fecha(parsear_fecha("2024-02-29")) == "2024-02-29");
        rechaza([] { parsear_fecha("2026-02-29"); });
        rechaza([] { parsear_fecha("2026-11-01x"); });
    });
    caso("validaciones", [] {
        auto s = base();
        auto l = lote();
        l.cantidad = -1;
        rechaza([&] { s.registrar_lote(l); });
        l = lote();
        l.peso = std::numeric_limits<double>::quiet_NaN();
        rechaza([&] { s.registrar_lote(l); });
        l = lote();
        l.donante = "X";
        rechaza([&] { s.registrar_lote(l); });
        l = lote();
        l.caducidad = parsear_fecha("2026-10-31");
        rechaza([&] { s.registrar_lote(l); });
        s.registrar_lote(lote());
        rechaza([&] { s.registrar_lote(lote()); });
    });
    caso("AVL rotaciones y eliminacion", [] {
        for (auto seq : {std::vector<int>{3, 2, 1}, {1, 2, 3}, {3, 1, 2}, {1, 3, 2}}) {
            IndiceAVL a;
            for (int x : seq) {
                a.insertar({Fecha{std::chrono::days{x}}, std::to_string(x)});
                check(a.validar());
            }
            a.eliminar({Fecha{std::chrono::days{2}}, "2"});
            check(a.validar());
            check(a.rango(Fecha{}, Fecha{std::chrono::days{9}}).size() == 2);
        }
    });
    caso("AVL aleatorio contra set", [] {
        IndiceAVL a;
        std::set<ClaveLote> ref;
        std::mt19937 g(42);
        for (int i = 0; i < 2000; i++) {
            ClaveLote k{Fecha{std::chrono::days{g() % 100}}, std::to_string(g() % 300)};
            if (g() % 2) {
                a.insertar(k);
                ref.insert(k);
            } else {
                a.eliminar(k);
                ref.erase(k);
            }
            check(a.validar());
            auto got = a.rango(Fecha{}, Fecha{std::chrono::days{100}});
            check(std::vector<ClaveLote>(ref.begin(), ref.end()) == got);
        }
    });
    caso("Dijkstra y cierres", [] {
        Red r;
        r.agregar("A", "B", 10, true);
        r.agregar("B", "C", 5, true);
        r.agregar("A", "C", 20, false);
        auto p = r.ruta("A", "C", false);
        check(p && p->minutos == 15 && p->nodos.size() == 3);
        r.cerrar("B", "C", true);
        check(r.ruta("A", "C", false)->minutos == 20);
        check(!r.ruta("A", "C", true));
        check(!r.ruta("A", "Z", false));
        check(r.ruta("A", "A", false)->minutos == 0);
        rechaza([&] { r.agregar("A", "X", -1, true); });
    });
    caso("prioridad y FEFO parcial", [] {
        auto s = base();
        s.registrar_lote(lote("tarde"));
        auto l = lote("pronto");
        l.caducidad = parsear_fecha("2026-11-05");
        s.registrar_lote(l);
        s.solicitar({"S", "B", "agua", 15, 1});
        auto p = s.proponer_despacho("S", 100, 100, parsear_fecha("2026-11-02"));
        check(p.lote == "pronto" && p.cantidad == 10);
        s.confirmar_despacho(p, "E");
        check(s.buscar_lote("pronto").disponible == 0);
        check(s.solicitudes().at("S").pendiente == 5);
        check(s.informe().despachado == 10);
        check(s.buscar_categoria("A", "agua").size() == 2);
    });
    caso("reintento y propuesta obsoleta", [] {
        auto s = base();
        s.registrar_lote(lote());
        s.solicitar({"S", "B", "agua", 5, 1});
        auto p = s.proponer_despacho("S", 100, 100, parsear_fecha("2026-11-02"));
        s.confirmar_despacho(p, "E");
        rechaza([&] { s.confirmar_despacho(p, "E"); });
        rechaza([&] { s.confirmar_despacho(p, "E2"); });
        check(s.informe().disponible == 5);
    });
    caso("propuesta alterada rechazada", [] {
        auto s = base();
        s.registrar_lote(lote());
        s.solicitar({"S", "B", "agua", 5, 1});
        auto p = s.proponer_despacho("S", 100, 100, parsear_fecha("2026-11-02"));
        p.cantidad = 999;
        rechaza([&] { s.confirmar_despacho(p, "E"); });
        check(s.informe().disponible == 10);
    });
    caso("sin ruta no descuenta", [] {
        auto s = base();
        s.registrar_lote(lote());
        s.solicitar({"S", "B", "agua", 5, 1});
        s.red.cerrar("A", "B", true);
        rechaza([&] { s.proponer_despacho("S", 100, 100, parsear_fecha("2026-11-02")); });
        check(s.informe().disponible == 10);
    });
    caso("frio capacidad y vencimiento al llegar", [] {
        auto s = base();
        auto l = lote();
        l.frio = true;
        s.registrar_lote(l);
        s.solicitar({"S", "B", "agua", 5, 1});
        rechaza([&] { s.proponer_despacho("S", 0, 100, parsear_fecha("2026-11-02")); });
        s.red.cerrar("A", "B", true);
        rechaza([&] { s.proponer_despacho("S", 100, 100, parsear_fecha("2026-11-02")); });
        auto t = base();
        l = lote();
        l.caducidad = parsear_fecha("2026-11-02");
        t.registrar_lote(l);
        t.solicitar({"S", "B", "agua", 5, 1});
        rechaza([&] { t.proponer_despacho("S", 100, 100, parsear_fecha("2026-11-02")); });
    });
    caso("triaje desempate y pendiente", [] {
        auto s = base();
        s.agregar(Municipio{"C", "Otro", 4});
        s.solicitar({"z", "C", "agua", 2, 0});
        s.solicitar({"b", "B", "agua", 2, 2});
        s.solicitar({"a", "B", "agua", 2, 2});
        auto q = s.prioridades();
        check(q[0] == "a" && q[1] == "b" && q[2] == "z");
    });
    caso("CSV roundtrip comillas historial", [] {
        auto s = base();
        s.agregar(Donante{"D2", "ONG, \"Ayuda\"", 120050});
        s.registrar_lote(lote());
        s.solicitar({"S", "B", "agua", 3, 1});
        auto p = s.proponer_despacho("S", 100, 100, parsear_fecha("2026-11-02"));
        s.confirmar_despacho(p, "E");
        auto dir = std::filesystem::current_path() / "donaciones_test_roundtrip";
        guardar_datos(s, dir);
        auto t = cargar_datos(dir);
        check(t.donantes().at("D2").nombre == "ONG, \"Ayuda\"");
        check(t.informe().despachado == 3 && t.informe().disponible == 7);
        check(t.despachos().size() == 1);
    });
    caso("masivas secuenciales", [] {
        auto s = base();
        s.registrar_lote(lote("L", 1000));
        for (int i = 0; i < 100; i++) {
            auto id = std::to_string(i);
            s.solicitar({id, "B", "agua", 10, i});
            auto p = s.proponer_despacho(id, 100, 100, parsear_fecha("2026-11-02"));
            s.confirmar_despacho(p, "E" + id);
        }
        auto r = s.informe();
        check(r.recibido == 1000 && r.disponible == 0 && r.despachado == 1000);
    });
    caso("CSV malformado e importacion atomica", [] {
        auto s = base();
        s.registrar_lote(lote());
        auto dir = std::filesystem::current_path() / "csv_invalido";
        guardar_datos(s, dir);
        {
            std::ofstream f(archivos(dir) / "lotes.csv", std::ios::app);
            f << "\"sin cerrar\n";
        }
        rechaza([&] {
            auto t = cargar_datos(dir);
            s = std::move(t);
        });
        check(s.informe().disponible == 10);
    });
    caso("CSV campo nulo y cabecera incorrecta", [] {
        auto s = base();
        auto dir = std::filesystem::current_path() / "csv_nulo";
        guardar_datos(s, dir);
        {
            std::ofstream f(archivos(dir) / "municipios.csv");
            f << "id,nombre,urgencia\nB,Caldas,\n";
        }
        rechaza([&] { cargar_datos(dir); });
        {
            std::ofstream f(archivos(dir) / "municipios.csv");
            f << "id,nombre,otra\n";
        }
        rechaza([&] { cargar_datos(dir); });
    });
    caso("red modificada invalida propuesta", [] {
        auto s = base();
        s.registrar_lote(lote());
        s.solicitar({"S", "B", "agua", 5, 1});
        auto p = s.proponer_despacho("S", 100, 100, parsear_fecha("2026-11-02"));
        s.red.cerrar("A", "B", true);
        rechaza([&] { s.confirmar_despacho(p, "E"); });
        check(s.informe().despachado == 0);
    });
    caso("capacidad parcial y agotamiento", [] {
        auto s = base();
        s.registrar_lote(lote());
        s.solicitar({"S", "B", "agua", 2, 1});
        auto p = s.proponer_despacho("S", 2, 1, parsear_fecha("2026-11-02"));
        check(p.cantidad == 1);
        s.confirmar_despacho(p, "E1");
        p = s.proponer_despacho("S", 100, 100, parsear_fecha("2026-11-02"));
        s.confirmar_despacho(p, "E2");
        rechaza([&] { s.proponer_despacho("S", 100, 100, parsear_fecha("2026-11-02")); });
        check(s.prioridades().empty());
    });
    caso("historial inconsistente rechazado", [] {
        auto s = base();
        auto l = lote();
        l.disponible = 9;
        l.despachado = 1;
        s.registrar_lote(l);
        rechaza([&] { s.restaurar_historial({}); });
    });
    caso("fallo guardado conserva snapshot anterior", [] {
        auto s = base();
        s.registrar_lote(lote());
        auto dir = std::filesystem::current_path() / "snapshot_seguro";
        std::filesystem::remove(dir / "actual.csv.tmp");
        guardar_datos(s, dir);
        s.solicitar({"S", "B", "agua", 2, 1});
        auto p = s.proponer_despacho("S", 100, 100, parsear_fecha("2026-11-02"));
        s.confirmar_despacho(p, "E");
        std::filesystem::create_directories(dir / "actual.csv.tmp");
        rechaza([&] { guardar_datos(s, dir); });
        check(cargar_datos(dir).informe().despachado == 0);
        std::filesystem::remove(dir / "actual.csv.tmp");
    });
    caso("historial llegada y tramos invalidos", [] {
        auto armar = []() {
            auto s = base();
            auto l = lote();
            l.disponible = 9;
            l.despachado = 1;
            s.registrar_lote(l);
            s.solicitar({"S", "B", "agua", 1, 1, 0});
            return s;
        };
        auto s = armar();
        rechaza([&] {
            s.restaurar_historial(
                {{"E", "S", "L", 1, {{"A", "B"}, 1e8}, parsear_fecha("2026-11-02")}});
        });
        auto t = armar();
        rechaza([&] {
            t.restaurar_historial(
                {{"E", "S", "L", 1, {{"A", "X", "B"}, 60}, parsear_fecha("2026-11-02")}});
        });
    });
    caso("CSV negativos rechazados", [] {
        auto s = base();
        s.registrar_lote(lote());
        auto dir = std::filesystem::current_path() / "csv_negativo";
        guardar_datos(s, dir);
        {
            std::ofstream f(archivos(dir) / "lotes.csv");
            f << "id,donante,centro,categoria,caducidad,cantidad,peso_kg_unidad,volumen_m3_unidad,"
                 "frio,disponible,reservado,despachado\nL,D,A,agua,2026-12-01,10,2,1,0,-1,0,0\n";
        }
        rechaza([&] { cargar_datos(dir); });
    });
    caso("siguiente viable por urgencia", [] {
        auto s = base();
        s.agregar(Municipio{"C", "Otro", 4});
        s.red.agregar("A", "C", 20, true);
        s.registrar_lote(lote());
        s.solicitar({"sin_stock", "B", "medicamentos", 3, 0});
        s.solicitar({"alta", "B", "agua", 3, 2});
        s.solicitar({"baja", "C", "agua", 3, 1});
        check(s.proponer_siguiente(100, 100, parsear_fecha("2026-11-02")).solicitud == "alta");
    });
    std::cout << total - fallos << "/" << total << " pruebas correctas\n";
    return fallos ? 1 : 0;
}
