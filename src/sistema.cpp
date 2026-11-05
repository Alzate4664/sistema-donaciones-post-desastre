#include "donaciones/sistema.hpp"
#include <cmath>
#include <limits>
#include <queue>
#include <tuple>
namespace donaciones {
namespace {
void nombre(const std::string &n) {
    exigir(!n.empty() && n.size() <= 200 && n.find_first_of("\r\n") == std::string::npos,
           "Nombre o categoria vacio, largo o multilinea");
}
std::string clave(const std::string &a, const std::string &b) {
    return a + '\n' + b;
}
} // namespace
void Sistema::agregar(Donante x) {
    identificador(x.id);
    nombre(x.nombre);
    exigir(x.centavos >= 0 && !donantes_.contains(x.id), "Donante duplicado o aporte negativo");
    donantes_.emplace(x.id, x);
    ++version_;
}
void Sistema::agregar(Centro x) {
    identificador(x.id);
    nombre(x.nombre);
    exigir(!municipios_.contains(x.id) && !centros_.contains(x.id), "ID de ubicacion duplicado");
    centros_.emplace(x.id, x);
    ++version_;
}
void Sistema::agregar(Municipio x) {
    identificador(x.id);
    nombre(x.nombre);
    exigir(x.urgencia >= 1 && x.urgencia <= 5, "Urgencia entre 1 y 5");
    exigir(!municipios_.contains(x.id) && !centros_.contains(x.id), "ID de ubicacion duplicado");
    municipios_.emplace(x.id, x);
    ++version_;
}
void Sistema::registrar_lote(Lote l) {
    identificador(l.id);
    nombre(l.categoria);
    exigir(!lotes_.contains(l.id), "Lote duplicado");
    exigir(donantes_.contains(l.donante) && centros_.contains(l.centro),
           "Donante o centro inexistente");
    exigir(l.caducidad > evento_, "Caducidad debe ser posterior al evento");
    exigir(l.cantidad > 0 && std::isfinite(l.peso) && std::isfinite(l.volumen) && l.peso > 0 &&
               l.volumen > 0,
           "Cantidad, peso y volumen deben ser positivos y finitos");
    exigir(!l.frio || centros_.at(l.centro).frio, "Centro sin refrigeracion");
    if (l.disponible == -1)
        l.disponible = l.cantidad;
    exigir(l.disponible >= 0 && l.reservado == 0 && l.despachado >= 0 &&
               std::int64_t(l.disponible) + l.despachado == l.cantidad,
           "Balance de lote invalido");
    fechas_.insertar({l.caducidad, l.id});
    categorias_[clave(l.centro, l.categoria)].push_back(l.id);
    lotes_.emplace(l.id, l);
    ++version_;
}
void Sistema::solicitar(Solicitud s) {
    identificador(s.id);
    nombre(s.categoria);
    exigir(!solicitudes_.contains(s.id) && municipios_.contains(s.municipio),
           "Solicitud duplicada o municipio inexistente");
    exigir(s.cantidad > 0 && s.llegada >= 0, "Cantidad positiva y llegada no negativa requeridas");
    if (s.pendiente == -1)
        s.pendiente = s.cantidad;
    exigir(s.pendiente >= 0 && s.pendiente <= s.cantidad, "Pendiente invalido");
    solicitudes_.emplace(s.id, s);
    ++version_;
}
const Lote &Sistema::buscar_lote(const std::string &id) const {
    auto p = lotes_.find(id);
    exigir(p != lotes_.end(), "Lote no encontrado");
    return p->second;
}
std::vector<std::string> Sistema::buscar_categoria(const std::string &c,
                                                   const std::string &t) const {
    auto p = categorias_.find(clave(c, t));
    return p == categorias_.end() ? std::vector<std::string>{} : p->second;
}
std::vector<std::string> Sistema::prioridades() const {
    using K = std::tuple<int, std::int64_t, std::string>;
    std::priority_queue<K, std::vector<K>, std::greater<K>> q;
    for (const auto &[id, s] : solicitudes_)
        if (s.pendiente > 0)
            q.emplace(-municipios_.at(s.municipio).urgencia, s.llegada, id);
    std::vector<std::string> r;
    while (!q.empty()) {
        r.push_back(std::get<2>(q.top()));
        q.pop();
    }
    return r;
}
Propuesta Sistema::proponer_siguiente(double cp, double cv, Fecha salida) const {
    exigir(std::isfinite(cp) && std::isfinite(cv) && cp > 0 && cv > 0 && salida >= evento_,
           "Fecha o capacidad invalida");
    for (const auto &id : prioridades()) {
        try {
            return proponer_despacho(id, cp, cv, salida);
        } catch (const std::runtime_error &) { /* Sin suministro viable: mantener pendiente. */
        }
    }
    throw std::runtime_error("No hay solicitudes pendientes con suministro viable");
}
Propuesta Sistema::proponer_despacho(const std::string &id, double cp, double cv,
                                     Fecha salida) const {
    exigir(std::isfinite(cp) && std::isfinite(cv) && cp > 0 && cv > 0,
           "Capacidades positivas y finitas requeridas");
    exigir(salida >= evento_, "Salida anterior al evento");
    auto it = solicitudes_.find(id);
    exigir(it != solicitudes_.end() && it->second.pendiente > 0,
           "Solicitud inexistente o ya atendida");
    const auto &s = it->second;
    for (const auto &[f, lid] : fechas_.rango(salida, parsear_fecha("9999-12-31"))) {
        const auto &l = lotes_.at(lid);
        if (l.categoria != s.categoria || l.disponible == 0)
            continue;
        auto r = red.ruta(l.centro, s.municipio, l.frio);
        if (!r)
            continue;
        // Caducidad es el inicio del dia indicado; la llegada debe ser estrictamente anterior.
        double ventana = double((f - salida).count()) * 1440;
        if (r->minutos >= ventana)
            continue;
        double capacidad = std::min(cp / l.peso, cv / l.volumen);
        int n = int(std::min({double(l.disponible), double(s.pendiente), std::floor(capacidad)}));
        if (n <= 0)
            continue;
        return {id, lid, n, *r, version_, red.version(), salida, cp, cv};
    }
    throw std::runtime_error(
        "Sin lote viable: revise stock, categoria, ruta, frio, capacidad y caducidad");
}
Despacho Sistema::confirmar_despacho(const Propuesta &p, const std::string &id) {
    identificador(id);
    exigir(!ids_despacho.contains(id), "ID de despacho ya utilizado");
    exigir(p.version == version_ && p.version_red == red.version(),
           "Propuesta obsoleta: vuelva a calcular");
    auto v = proponer_despacho(p.solicitud, p.capacidad_peso, p.capacidad_volumen, p.salida);
    exigir(v.lote == p.lote && v.cantidad == p.cantidad && v.ruta.nodos == p.ruta.nodos &&
               v.ruta.minutos == p.ruta.minutos,
           "Propuesta alterada");
    Despacho d{id, p.solicitud, p.lote, p.cantidad, v.ruta, p.salida};
    despachos_.push_back(d);
    ids_despacho.insert(id);
    auto &l = lotes_.at(p.lote);
    l.disponible -= p.cantidad;
    l.despachado += p.cantidad;
    solicitudes_.at(p.solicitud).pendiente -= p.cantidad;
    ++version_;
    return d;
}
Informe Sistema::informe() const {
    Informe r;
    for (const auto &[id, l] : lotes_) {
        r.recibido += l.cantidad;
        r.disponible += l.disponible;
        r.reservado += l.reservado;
        r.despachado += l.despachado;
    }
    for (const auto &[id, d] : donantes_) {
        exigir(d.centavos <= std::numeric_limits<std::int64_t>::max() - r.centavos,
               "Total monetario fuera de rango");
        r.centavos += d.centavos;
    }
    for (const auto &id : prioridades()) {
        const auto &s = solicitudes_.at(id);
        r.alertas.push_back(id + ": pendiente " + std::to_string(s.pendiente) + " " + s.categoria);
    }
    return r;
}
void Sistema::restaurar_historial(std::vector<Despacho> ds) {
    exigir(despachos_.empty(), "Historial ya cargado");
    std::unordered_set<std::string> ids;
    std::unordered_map<std::string, std::int64_t> por_lote, por_sol;
    for (const auto &d : ds) {
        identificador(d.id);
        exigir(ids.insert(d.id).second && lotes_.contains(d.lote) &&
                   solicitudes_.contains(d.solicitud) && d.cantidad > 0,
               "Historial invalido");
        const auto &l = lotes_.at(d.lote);
        const auto &s = solicitudes_.at(d.solicitud);
        exigir(l.categoria == s.categoria && d.salida >= evento_ && d.salida < l.caducidad &&
                   std::isfinite(d.ruta.minutos) && d.ruta.minutos >= 0 && !d.ruta.nodos.empty() &&
                   d.ruta.nodos.front() == l.centro && d.ruta.nodos.back() == s.municipio,
               "Historial incoherente");
        exigir(d.ruta.minutos < double((l.caducidad - d.salida).count()) * 1440,
               "Historial: llegada posterior a caducidad");
        double suma = 0;
        for (size_t i = 1; i < d.ruta.nodos.size(); i++) {
            auto via = std::find_if(red.vias().begin(), red.vias().end(), [&](const Via &v) {
                return v.origen == d.ruta.nodos[i - 1] && v.destino == d.ruta.nodos[i];
            });
            exigir(via != red.vias().end() && (!l.frio || via->frio),
                   "Historial: tramo inexistente o sin frio");
            suma += via->minutos;
        }
        exigir(std::abs(suma - d.ruta.minutos) < 1e-6,
               "Historial: duracion no coincide con tramos");
        por_lote[d.lote] += d.cantidad;
        por_sol[d.solicitud] += d.cantidad;
    }
    for (const auto &[id, l] : lotes_)
        exigir(por_lote[id] == l.despachado, "Historial no concuerda con lote");
    for (const auto &[id, s] : solicitudes_)
        exigir(por_sol[id] == s.cantidad - s.pendiente, "Historial no concuerda con solicitud");
    despachos_ = std::move(ds);
    ids_despacho = std::move(ids);
    ++version_;
}
} // namespace donaciones
