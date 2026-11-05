#pragma once
#include "avl.hpp"
#include "red.hpp"
#include <unordered_set>
namespace donaciones {
class Sistema {
    Fecha evento_;
    std::uint64_t version_ = 0;
    std::unordered_map<std::string, Donante> donantes_;
    std::unordered_map<std::string, Centro> centros_;
    std::unordered_map<std::string, Municipio> municipios_;
    std::unordered_map<std::string, Lote> lotes_;
    std::unordered_map<std::string, Solicitud> solicitudes_;
    std::unordered_map<std::string, std::vector<std::string>> categorias_;
    std::vector<Despacho> despachos_;
    std::unordered_set<std::string> ids_despacho;
    IndiceAVL fechas_;

  public:
    Red red;
    explicit Sistema(Fecha evento) : evento_(evento) {}
    void agregar(Donante);
    void agregar(Centro);
    void agregar(Municipio);
    void registrar_lote(Lote);
    void solicitar(Solicitud);
    const Lote &buscar_lote(const std::string &) const;
    std::vector<std::string> buscar_categoria(const std::string &, const std::string &) const;
    auto caducidades(Fecha a, Fecha b) const {
        return fechas_.rango(a, b);
    }
    std::vector<std::string> prioridades() const;
    Propuesta proponer_despacho(const std::string &, double, double, Fecha) const;
    Propuesta proponer_siguiente(double, double, Fecha) const;
    Despacho confirmar_despacho(const Propuesta &, const std::string &);
    Informe informe() const;
    void restaurar_historial(std::vector<Despacho>);
    Fecha evento() const {
        return evento_;
    }
    const auto &donantes() const {
        return donantes_;
    }
    const auto &centros() const {
        return centros_;
    }
    const auto &municipios() const {
        return municipios_;
    }
    const auto &lotes() const {
        return lotes_;
    }
    const auto &solicitudes() const {
        return solicitudes_;
    }
    const auto &despachos() const {
        return despachos_;
    }
};
} // namespace donaciones
