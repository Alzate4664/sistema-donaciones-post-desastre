#pragma once
#include "modelo.hpp"
#include <optional>
#include <unordered_map>
namespace donaciones {
struct Via {
    std::string origen, destino;
    double minutos;
    bool frio, cerrada = false;
};
class Red {
    std::vector<Via> vias_;
    std::unordered_map<std::string, std::vector<size_t>> ady;
    std::uint64_t version_ = 0;

  public:
    void agregar(const std::string &, const std::string &, double, bool);
    void cerrar(const std::string &, const std::string &, bool);
    std::optional<Ruta> ruta(const std::string &, const std::string &, bool) const;
    const auto &vias() const {
        return vias_;
    }
    auto version() const {
        return version_;
    }
};
} // namespace donaciones
