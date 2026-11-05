#pragma once
#include "sistema.hpp"
#include <filesystem>
namespace donaciones {
Sistema cargar_datos(const std::filesystem::path &);
void guardar_datos(const Sistema &, const std::filesystem::path &);
} // namespace donaciones
