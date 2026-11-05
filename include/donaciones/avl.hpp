#pragma once
#include "modelo.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
namespace donaciones {
using ClaveLote = std::pair<Fecha, std::string>;
class IndiceAVL {
    struct Nodo {
        ClaveLote k;
        int h = 1;
        std::unique_ptr<Nodo> iz, de;
        explicit Nodo(ClaveLote v) : k(std::move(v)) {}
    };
    using Ptr = std::unique_ptr<Nodo>;
    Ptr raiz;
    static int h(const Ptr &n) {
        return n ? n->h : 0;
    }
    static void actualizar(Ptr &n) {
        n->h = 1 + std::max(h(n->iz), h(n->de));
    }
    static Ptr derecha(Ptr n) {
        auto p = std::move(n->iz);
        n->iz = std::move(p->de);
        actualizar(n);
        p->de = std::move(n);
        actualizar(p);
        return p;
    }
    static Ptr izquierda(Ptr n) {
        auto p = std::move(n->de);
        n->de = std::move(p->iz);
        actualizar(n);
        p->iz = std::move(n);
        actualizar(p);
        return p;
    }
    static Ptr balancear(Ptr n) {
        if (!n)
            return n;
        actualizar(n);
        int b = h(n->iz) - h(n->de);
        if (b > 1) {
            if (h(n->iz->iz) < h(n->iz->de))
                n->iz = izquierda(std::move(n->iz));
            return derecha(std::move(n));
        }
        if (b < -1) {
            if (h(n->de->de) < h(n->de->iz))
                n->de = derecha(std::move(n->de));
            return izquierda(std::move(n));
        }
        return n;
    }
    static Ptr ins(Ptr n, const ClaveLote &k) {
        if (!n)
            return std::make_unique<Nodo>(k);
        if (k < n->k)
            n->iz = ins(std::move(n->iz), k);
        else if (n->k < k)
            n->de = ins(std::move(n->de), k);
        return balancear(std::move(n));
    }
    static Ptr del(Ptr n, const ClaveLote &k) {
        if (!n)
            return n;
        if (k < n->k)
            n->iz = del(std::move(n->iz), k);
        else if (n->k < k)
            n->de = del(std::move(n->de), k);
        else {
            if (!n->iz)
                return std::move(n->de);
            if (!n->de)
                return std::move(n->iz);
            auto p = n->de.get();
            while (p->iz)
                p = p->iz.get();
            n->k = p->k;
            n->de = del(std::move(n->de), n->k);
        }
        return balancear(std::move(n));
    }
    static void rango_(const Ptr &n, Fecha a, Fecha b, std::vector<ClaveLote> &r) {
        if (!n)
            return;
        if (n->k.first >= a)
            rango_(n->iz, a, b, r);
        if (n->k.first >= a && n->k.first <= b)
            r.push_back(n->k);
        if (n->k.first <= b)
            rango_(n->de, a, b, r);
    }
    static int verificar(const Ptr &n, const ClaveLote *a, const ClaveLote *b) {
        if (!n)
            return 0;
        if ((a && !(*a < n->k)) || (b && !(n->k < *b)))
            return -1;
        int l = verificar(n->iz, a, &n->k), r = verificar(n->de, &n->k, b);
        if (l < 0 || r < 0 || std::abs(l - r) > 1 || n->h != 1 + std::max(l, r))
            return -1;
        return n->h;
    }

  public:
    void insertar(const ClaveLote &k) {
        raiz = ins(std::move(raiz), k);
    }
    void eliminar(const ClaveLote &k) {
        raiz = del(std::move(raiz), k);
    }
    std::vector<ClaveLote> rango(Fecha a, Fecha b) const {
        std::vector<ClaveLote> r;
        rango_(raiz, a, b, r);
        return r;
    }
    bool validar() const {
        return verificar(raiz, nullptr, nullptr) >= 0;
    }
};
} // namespace donaciones
