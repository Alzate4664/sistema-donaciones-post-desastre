#include "donaciones/avl.hpp"
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <unordered_map>
using namespace donaciones;
using Reloj = std::chrono::steady_clock;
template <class F> double medir(F fn) {
    auto t = Reloj::now();
    fn();
    return std::chrono::duration<double, std::milli>(Reloj::now() - t).count();
}
int main(int argc, char **argv) {
    std::ofstream out(argc > 1 ? argv[1] : "benchmark.csv");
    if (!out)
        return 1;
    out << "n,repeticion,operacion,ms,consultas\n";
    std::uint64_t checksum = 0;
    for (int n : {1000, 10000, 100000})
        for (int rep = 0; rep < 5; rep++) {
            std::vector<int> datos(n);
            std::iota(datos.begin(), datos.end(), 0);
            std::unordered_map<int, int> hash;
            for (int x : datos)
                hash[x] = x;
            std::mt19937 rng(42 + rep);
            std::vector<int> consultas(1000);
            for (auto &x : consultas)
                x = int(rng() % n);
            std::uint64_t a = 0, b = 0;
            double t = medir([&] {
                for (int k : consultas)
                    a += hash.at(k);
            });
            out << n << ',' << rep << ",hash_buscar," << t << ",1000\n";
            t = medir([&] {
                for (int k : consultas) {
                    auto p = std::find(datos.begin(), datos.end(), k);
                    b += *p;
                }
            });
            out << n << ',' << rep << ",lista_buscar," << t << ",1000\n";
            if (a != b)
                return 2;
            checksum += a;
            std::shuffle(datos.begin(), datos.end(), rng);
            auto lista = datos;
            std::priority_queue<int> heap(datos.begin(), datos.end());
            a = b = 0;
            int q = std::min(1000, n);
            t = medir([&] {
                for (int i = 0; i < q; i++) {
                    a += heap.top();
                    heap.pop();
                }
            });
            out << n << ',' << rep << ",heap_extraer," << t << ',' << q << '\n';
            t = medir([&] {
                for (int i = 0; i < q; i++) {
                    auto p = std::max_element(lista.begin(), lista.end());
                    b += *p;
                    lista.erase(p);
                }
            });
            out << n << ',' << rep << ",lista_extraer," << t << ',' << q << '\n';
            if (a != b)
                return 3;
            checksum += a;
            IndiceAVL avl;
            t = medir([&] {
                for (int i = 0; i < n; i++)
                    avl.insertar({Fecha{std::chrono::days{i}}, std::to_string(i)});
            });
            out << n << ',' << rep << ",avl_insertar_ordenado," << t << ',' << n << '\n';
            if (!avl.validar())
                return 4;
            // BST sin balance: nodos en vector y descenso explicito, sin recursion.
            struct Nodo {
                int clave, der = -1;
            };
            int nb = std::min(n, 10000);
            std::vector<Nodo> nodos;
            nodos.reserve(nb);
            t = medir([&] {
                for (int i = 0; i < nb; i++) {
                    if (nodos.empty()) {
                        nodos.push_back({i});
                        continue;
                    }
                    int j = 0;
                    while (nodos[j].der != -1)
                        j = nodos[j].der;
                    nodos[j].der = int(nodos.size());
                    nodos.push_back({i});
                }
            });
            out << nb << ',' << rep << ",bst_insertar_ordenado," << t << ',' << nb << '\n';
            checksum += nodos.back().clave;
        }
    std::cout << "Benchmark completado. checksum=" << checksum << "\n";
    return 0;
}
