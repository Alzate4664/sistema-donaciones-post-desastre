#include "donaciones/modelo.hpp"
#include <cctype>
#include <cstdio>
namespace donaciones {
void exigir(bool ok, const std::string &m) {
    if (!ok)
        throw std::runtime_error(m);
}
void identificador(const std::string &s) {
    exigir(!s.empty() && s.size() <= 80, "ID vacio o demasiado largo");
    for (unsigned char c : s)
        exigir(std::isalnum(c) || c == '_' || c == '-', "ID: use letras ASCII, numeros, _ o -");
}
Fecha parsear_fecha(const std::string &s) {
    exigir(s.size() == 10 && s[4] == '-' && s[7] == '-', "Fecha: use AAAA-MM-DD");
    for (size_t i = 0; i < s.size(); i++)
        if (i != 4 && i != 7)
            exigir(s[i] >= '0' && s[i] <= '9', "Fecha invalida");
    auto y = std::stoi(s.substr(0, 4));
    auto m = std::stoi(s.substr(5, 2));
    auto d = std::stoi(s.substr(8, 2));
    std::chrono::year_month_day v{std::chrono::year{y}, std::chrono::month{unsigned(m)},
                                  std::chrono::day{unsigned(d)}};
    exigir(y >= 1900 && y <= 9999 && v.ok(), "Fecha fuera de rango o inexistente");
    return Fecha{v};
}
std::string texto_fecha(Fecha f) {
    auto v = std::chrono::year_month_day{f};
    char b[16];
    std::snprintf(b, sizeof b, "%04d-%02u-%02u", int(v.year()), unsigned(v.month()),
                  unsigned(v.day()));
    return b;
}
} // namespace donaciones
