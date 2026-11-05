#include "donaciones/red.hpp"
#include <algorithm>
#include <cmath>
#include <queue>
namespace donaciones {
void Red::agregar(const std::string &a, const std::string &b, double minutos, bool frio) {
    identificador(a);
    identificador(b);
    exigir(a != b && std::isfinite(minutos) && minutos >= 0,
           "Via invalida: extremos distintos y minutos no negativos");
    for (const auto &v : vias_)
        exigir(v.origen != a || v.destino != b, "Via dirigida duplicada");
    ady[a].push_back(vias_.size());
    ady.try_emplace(b);
    vias_.push_back({a, b, minutos, frio});
    ++version_;
}
void Red::cerrar(const std::string &a, const std::string &b, bool c) {
    for (auto &v : vias_)
        if (v.origen == a && v.destino == b) {
            v.cerrada = c;
            ++version_;
            return;
        }
    throw std::runtime_error("Via no encontrada");
}
// Adaptacion de T4_Red_Donantes/Solucion_medicos/06_logistica_traslado.cpp,
// Prof. Edison Valencia Diaz: distancias acumuladas, heap y relajacion.
// Extensiones: filtros de via, predecesores y resultado opcional.
std::optional<Ruta> Red::ruta(const std::string &a, const std::string &b, bool frio) const {
    if (!ady.contains(a) || !ady.contains(b))
        return {};
    using Item = std::pair<double, std::string>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> q;
    std::unordered_map<std::string, double> d;
    std::unordered_map<std::string, std::string> ant;
    d[a] = 0;
    q.push({0, a});
    while (!q.empty()) {
        auto [c, u] = q.top();
        q.pop();
        if (c != d.at(u))
            continue;
        if (u == b)
            break;
        for (auto i : ady.at(u)) {
            const auto &v = vias_[i];
            if (v.cerrada || (frio && !v.frio))
                continue;
            double nd = c + v.minutos;
            if (std::isfinite(nd) && (!d.contains(v.destino) || nd < d.at(v.destino))) {
                d[v.destino] = nd;
                ant[v.destino] = u;
                q.push({nd, v.destino});
            }
        }
    }
    if (!d.contains(b))
        return {};
    Ruta r{{b}, d.at(b)};
    while (r.nodos.back() != a)
        r.nodos.push_back(ant.at(r.nodos.back()));
    std::reverse(r.nodos.begin(), r.nodos.end());
    return r;
}
} // namespace donaciones
