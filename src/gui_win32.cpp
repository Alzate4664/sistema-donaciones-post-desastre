#include "donaciones/persistencia.hpp"
#include <windows.h>
#include <commctrl.h>
#include <future>
#include <memory>
#include <set>
#include <sstream>
using namespace donaciones;
namespace {
HWND win, tabla, cola, resumen, estado, ruta, operacion, buscar, carpeta, botonProponer,
    botonConfirmar;
HWND campos[10]{}, etiquetas[10]{};
HFONT fuente, titulo;
std::vector<HWND> acciones;
std::unique_ptr<Sistema> sistema;
std::optional<Propuesta> propuesta;
std::future<Sistema> carga;
std::future<Propuesta> calculo;
std::future<std::string> guardado;
std::wstring mensaje = L"Cargando datos...";
bool ocupado = false, modificado = false;
std::wstring w(const std::string &s) {
    if (s.empty())
        return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
    std::wstring r(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), r.data(), n);
    return r;
}
std::string u(const std::wstring &s) {
    if (s.empty())
        return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
    std::string r(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), r.data(), n, nullptr, nullptr);
    return r;
}
std::wstring texto(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring s(n + 1, 0);
    GetWindowTextW(h, s.data(), n + 1);
    s.resize(n);
    return s;
}
HWND control(const wchar_t *cls, const wchar_t *txt, DWORD estilo, int x, int y, int ancho,
             int alto, int id) {
    HWND h = CreateWindowExW(0, cls, txt, WS_CHILD | WS_VISIBLE | estilo, x, y, ancho, alto, win,
                             reinterpret_cast<HMENU>(INT_PTR(id)), nullptr, nullptr);
    SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(fuente), TRUE);
    return h;
}
void decir(const std::wstring &s) {
    mensaje = s;
    SetWindowTextW(estado, s.c_str());
}
void busy(bool b) {
    ocupado = b;
    for (auto h : acciones)
        EnableWindow(h, !b);
    EnableWindow(botonConfirmar, !b && propuesta.has_value());
}
void invalidar() {
    propuesta.reset();
    EnableWindow(botonConfirmar, FALSE);
}
void columna(HWND h, int i, const wchar_t *t, int ancho) {
    LVCOLUMNW c{};
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.pszText = const_cast<wchar_t *>(t);
    c.cx = ancho;
    ListView_InsertColumn(h, i, &c);
}
void fila(HWND h, const std::vector<std::wstring> &vs) {
    int i = ListView_GetItemCount(h);
    LVITEMW v{};
    v.mask = LVIF_TEXT;
    v.iItem = i;
    v.pszText = const_cast<wchar_t *>(vs[0].c_str());
    ListView_InsertItem(h, &v);
    for (size_t j = 1; j < vs.size(); j++)
        ListView_SetItemText(h, i, int(j), const_cast<wchar_t *>(vs[j].c_str()));
}
void refrescar() {
    if (!sistema)
        return;
    auto r = sistema->informe();
    std::wstring t = L"RECIBIDO  " + std::to_wstring(r.recibido) + L"     DISPONIBLE  " +
                     std::to_wstring(r.disponible) + L"     DESPACHADO  " +
                     std::to_wstring(r.despachado) + L"     APORTES COP  " +
                     std::to_wstring(r.centavos / 100) + L"." +
                     (r.centavos % 100 < 10 ? L"0" : L"") + std::to_wstring(r.centavos % 100);
    SetWindowTextW(resumen, t.c_str());
    ListView_DeleteAllItems(tabla);
    auto filtro = u(texto(buscar));
    std::set<std::string> fechas;
    if (filtro.size() == 10 && filtro[4] == '-') {
        try {
            for (const auto &k : sistema->caducidades(sistema->evento(), parsear_fecha(filtro)))
                fechas.insert(k.second);
        } catch (...) {
        }
    }
    std::vector<std::string> ids;
    for (const auto &[id, l] : sistema->lotes())
        ids.push_back(id);
    std::sort(ids.begin(), ids.end());
    for (const auto &id : ids) {
        const auto &l = sistema->buscar_lote(id);
        if (!filtro.empty() && id.find(filtro) == std::string::npos &&
            l.categoria.find(filtro) == std::string::npos && !fechas.contains(id))
            continue;
        fila(tabla, {w(id), w(l.categoria), w(l.centro), w(texto_fecha(l.caducidad)),
                     std::to_wstring(l.disponible), std::to_wstring(l.despachado),
                     l.frio ? L"Si" : L"No"});
    }
    ListView_DeleteAllItems(cola);
    for (const auto &id : sistema->prioridades()) {
        const auto &s = sistema->solicitudes().at(id);
        const auto &m = sistema->municipios().at(s.municipio);
        fila(cola, {w(id), w(m.nombre), std::to_wstring(m.urgencia), w(s.categoria),
                    std::to_wstring(s.pendiente)});
    }
    std::wstring rr = L"ALERTAS / SOLICITUDES PENDIENTES\r\n";
    for (const auto &a : r.alertas)
        rr += w(a) + L"\r\n";
    rr += L"\r\nHISTORIAL DE DESPACHOS\r\n";
    for (const auto &d : sistema->despachos()) {
        rr += w(d.id + ": " + d.lote + " -> " + d.solicitud) + L" (" + std::to_wstring(d.cantidad) +
              L")\r\n";
    }
    SetWindowTextW(ruta, rr.c_str());
}
struct Form {
    std::vector<std::wstring> labels, defaults;
};
Form formulario(int n) {
    switch (n) {
    case 0:
        return {{L"ID lote", L"ID donante", L"ID centro", L"Categoria", L"Caducidad AAAA-MM-DD",
                 L"Unidades", L"Kg por unidad", L"m3 por unidad", L"Requiere frio (0/1)"},
                {L"", L"D1", L"AC1", L"agua", L"2026-12-30", L"100", L"1", L"0.001", L"0"}};
    case 1:
        return {{L"ID solicitud", L"ID municipio", L"Categoria", L"Unidades", L"Orden de llegada"},
                {L"", L"M1", L"agua", L"20", L"10"}};
    case 2:
        return {{L"ID donante", L"Nombre", L"Aporte en centavos COP"}, {L"", L"", L"0"}};
    case 3:
        return {{L"ID centro", L"Nombre", L"Refrigeracion (0/1)"}, {L"", L"", L"0"}};
    case 4:
        return {{L"ID municipio", L"Nombre", L"Urgencia (1 a 5)"}, {L"", L"", L"5"}};
    case 5:
        return {{L"Origen", L"Destino", L"Minutos", L"Permite frio (0/1)"},
                {L"AC1", L"M1", L"60", L"1"}};
    default:
        return {{L"Origen", L"Destino", L"Cerrada (0/1)"}, {L"AC1", L"M1", L"1"}};
    }
}
void cambiarForm() {
    auto f = formulario(int(SendMessageW(operacion, CB_GETCURSEL, 0, 0)));
    for (size_t i = 0; i < 10; i++) {
        bool visible = i < f.labels.size();
        ShowWindow(campos[i], visible ? SW_SHOW : SW_HIDE);
        ShowWindow(etiquetas[i], visible ? SW_SHOW : SW_HIDE);
        if (visible) {
            SetWindowTextW(etiquetas[i], f.labels[i].c_str());
            SetWindowTextW(campos[i], f.defaults[i].c_str());
        }
    }
}
std::string valor(int n) {
    return u(texto(campos[n]));
}
std::int64_t entero(const std::string &s) {
    size_t p;
    auto n = std::stoll(s, &p);
    exigir(p == s.size(), "Entero invalido");
    return n;
}
int cantidad(int n) {
    auto v = entero(valor(n));
    exigir(v >= 0 && v <= INT_MAX, "Cantidad fuera de rango");
    return int(v);
}
double numero(const std::string &s) {
    size_t p;
    double v = std::stod(s, &p);
    exigir(p == s.size(), "Numero invalido: use punto decimal");
    return v;
}
bool booleano(int n) {
    auto s = valor(n);
    exigir(s == "0" || s == "1", "Use 0 o 1");
    return s == "1";
}
void registrar() {
    switch (SendMessageW(operacion, CB_GETCURSEL, 0, 0)) {
    case 0:
        sistema->registrar_lote({valor(0), valor(1), valor(2), valor(3), parsear_fecha(valor(4)),
                                 cantidad(5), numero(valor(6)), numero(valor(7)), booleano(8)});
        break;
    case 1:
        sistema->solicitar({valor(0), valor(1), valor(2), cantidad(3), entero(valor(4))});
        break;
    case 2:
        sistema->agregar(Donante{valor(0), valor(1), entero(valor(2))});
        break;
    case 3:
        sistema->agregar(Centro{valor(0), valor(1), booleano(2)});
        break;
    case 4:
        sistema->agregar(Municipio{valor(0), valor(1), cantidad(2)});
        break;
    case 5:
        exigir(
            (sistema->centros().contains(valor(0)) || sistema->municipios().contains(valor(0))) &&
                (sistema->centros().contains(valor(1)) || sistema->municipios().contains(valor(1))),
            "Ubicacion inexistente");
        sistema->red.agregar(valor(0), valor(1), numero(valor(2)), booleano(3));
        break;
    case 6:
        sistema->red.cerrar(valor(0), valor(1), booleano(2));
        break;
    }
    invalidar();
    modificado = true;
    refrescar();
    decir(L"Registro completado. Guarde los cambios antes de salir.");
}
void iniciarCarga() {
    exigir(!texto(carpeta).empty(), "Indique carpeta de datos");
    auto p = std::filesystem::path(texto(carpeta));
    invalidar();
    busy(true);
    decir(L"Leyendo y validando archivos...");
    carga = std::async(std::launch::async, [p] { return cargar_datos(p); });
}
void proponer(bool automatico = false) {
    int idx = ListView_GetNextItem(cola, -1, LVNI_SELECTED);
    exigir(automatico || idx >= 0, "Seleccione una solicitud en la tabla");
    wchar_t id[128]{};
    if (idx >= 0)
        ListView_GetItemText(cola, idx, 0, id, 128);
    auto sid = u(id);
    auto f = GetDlgItem(win, 501);
    auto fecha = parsear_fecha(u(texto(f)));
    auto cp = numero(u(texto(GetDlgItem(win, 502))));
    auto cv = numero(u(texto(GetDlgItem(win, 503))));
    invalidar();
    busy(true);
    decir(L"Calculando lote FEFO y ruta viable...");
    calculo = std::async(std::launch::async, [sid, fecha, cp, cv, automatico] {
        return automatico ? sistema->proponer_siguiente(cp, cv, fecha)
                          : sistema->proponer_despacho(sid, cp, cv, fecha);
    });
}
void temporizador() {
    using namespace std::chrono_literals;
    try {
        if (carga.valid() && carga.wait_for(0ms) == std::future_status::ready) {
            sistema = std::make_unique<Sistema>(carga.get());
            modificado = false;
            busy(false);
            refrescar();
            decir(L"Datos cargados y validados. Seleccione una solicitud para planificar.");
        }
        if (calculo.valid() && calculo.wait_for(0ms) == std::future_status::ready) {
            propuesta = calculo.get();
            busy(false);
            auto &p = *propuesta;
            std::wstring t = L"PROPUESTA DE DESPACHO\r\n\r\nSolicitud: " + w(p.solicitud) +
                             L"\r\nLote: " + w(p.lote) + L"\r\nUnidades: " +
                             std::to_wstring(p.cantidad) + L"\r\nTiempo: " +
                             std::to_wstring(p.ruta.minutos) + L" min\r\n\r\n";
            for (const auto &n : p.ruta.nodos) {
                if (n != p.ruta.nodos.front())
                    t += L"  →  ";
                t += w(n);
            }
            t += L"\r\n\r\nRevise y confirme para descontar inventario.";
            SetWindowTextW(ruta, t.c_str());
            decir(L"Propuesta lista. Aun no se ha descontado stock.");
        }
        if (guardado.valid() && guardado.wait_for(0ms) == std::future_status::ready) {
            auto m = guardado.get();
            busy(false);
            modificado = false;
            decir(w(m));
        }
    } catch (const std::exception &e) {
        busy(false);
        decir(L"Error: " + w(e.what()));
    }
}
LRESULT CALLBACK proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    try {
        switch (msg) {
        case WM_CREATE: {
            win = h;
            fuente = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0,
                                 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
            titulo = CreateFontW(-26, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0,
                                 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
            auto cab =
                control(L"STATIC", L"AYUDA  /  Coordinacion de donaciones", 0, 24, 18, 1050, 38, 0);
            SendMessageW(cab, WM_SETFONT, reinterpret_cast<WPARAM>(titulo), TRUE);
            control(L"STATIC", L"Simulacion academica · Colombia · Datos y tiempos sinteticos", 0,
                    26, 58, 1020, 26, 0);
            resumen = control(L"STATIC", L"", SS_CENTERIMAGE, 24, 92, 1090, 42, 0);
            control(L"STATIC", L"Carpeta de datos", 0, 24, 146, 125, 24, 0);
            carpeta = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 152, 141, 630,
                              28, 100);
            auto cargar = control(L"BUTTON", L"Cargar", WS_TABSTOP, 796, 140, 105, 30, 101);
            auto guardar = control(L"BUTTON", L"Guardar", WS_TABSTOP, 912, 140, 105, 30, 102);
            acciones = {cargar, guardar, carpeta};
            operacion = control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 24,
                                190, 300, 240, 200);
            for (auto s : {L"Registrar lote", L"Nueva solicitud", L"Nuevo donante", L"Nuevo centro",
                           L"Nuevo municipio", L"Nueva via dirigida", L"Cerrar / abrir via"})
                SendMessageW(operacion, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
            SendMessageW(operacion, CB_SETCURSEL, 0, 0);
            acciones.push_back(operacion);
            for (int i = 0; i < 10; i++) {
                etiquetas[i] = control(L"STATIC", L"", 0, 24, 230 + i * 43, 300, 18, 0);
                campos[i] = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 24,
                                    249 + i * 43, 300, 24, 210 + i);
                SendMessageW(campos[i], EM_SETLIMITTEXT, 200, 0);
                acciones.push_back(campos[i]);
            }
            auto registrarB =
                control(L"BUTTON", L"Registrar / aplicar", WS_TABSTOP, 24, 666, 300, 34, 220);
            acciones.push_back(registrarB);
            cambiarForm();
            control(L"STATIC", L"INVENTARIO  ·  Buscar ID, categoria o caducidad hasta fecha", 0,
                    350, 192, 770, 24, 0);
            buscar = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 350, 222, 750,
                             27, 300);
            acciones.push_back(buscar);
            tabla = control(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | WS_BORDER | WS_TABSTOP,
                            350, 258, 750, 192, 301);
            ListView_SetExtendedListViewStyle(tabla, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            int i = 0;
            for (auto s : {L"Lote", L"Categoria", L"Centro", L"Caducidad", L"Disponible",
                           L"Enviado", L"Frio"})
                columna(tabla, i++, s, 103);
            control(L"STATIC", L"SOLICITUDES  ·  Urgencia mayor primero", 0, 350, 462, 750, 24, 0);
            cola = control(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | WS_BORDER | WS_TABSTOP,
                           350, 491, 750, 140, 400);
            ListView_SetExtendedListViewStyle(cola, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            i = 0;
            for (auto s : {L"Solicitud", L"Municipio", L"Urgencia", L"Categoria", L"Pendiente"})
                columna(cola, i++, s, 140);
            control(L"STATIC", L"Salida (AAAA-MM-DD)    Capacidad kg       Capacidad m3", 0, 350,
                    641, 650, 20, 0);
            control(L"EDIT", L"2026-11-05", WS_BORDER | WS_TABSTOP, 350, 666, 170, 28, 501);
            control(L"EDIT", L"1000", WS_BORDER | WS_TABSTOP, 535, 666, 140, 28, 502);
            control(L"EDIT", L"10", WS_BORDER | WS_TABSTOP, 690, 666, 100, 28, 503);
            botonProponer =
                control(L"BUTTON", L"Proponer ruta", WS_TABSTOP, 350, 706, 200, 34, 504);
            botonConfirmar =
                control(L"BUTTON", L"Confirmar despacho", WS_TABSTOP, 565, 706, 225, 34, 505);
            acciones.push_back(botonProponer);
            acciones.push_back(
                control(L"BUTTON", L"Siguiente por urgencia", WS_TABSTOP, 805, 706, 295, 34, 506));
            acciones.push_back(GetDlgItem(h, 501));
            acciones.push_back(GetDlgItem(h, 502));
            acciones.push_back(GetDlgItem(h, 503));
            ruta = control(L"EDIT", L"", ES_MULTILINE | ES_READONLY | WS_VSCROLL | WS_BORDER, 24,
                           755, 1076, 145, 600);
            estado = control(L"STATIC", L"", 0, 24, 912, 1080, 45, 601);
            wchar_t path[MAX_PATH];
            GetModuleFileNameW(nullptr, path, MAX_PATH);
            auto dir = std::filesystem::path(path).parent_path();
            if (dir.filename() == L"bin")
                dir = dir.parent_path();
            dir /= L"datos";
            SetWindowTextW(carpeta, dir.c_str());
            SetTimer(h, 1, 100, nullptr);
            iniciarCarga();
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wp);
            if (id == 300 && HIWORD(wp) == EN_CHANGE && !ocupado) {
                refrescar();
                return 0;
            }
            if (ocupado)
                return 0;
            if (id == 200 && HIWORD(wp) == CBN_SELCHANGE) {
                cambiarForm();
                return 0;
            }
            if (id == 101) {
                if (modificado &&
                    MessageBoxW(h, L"Hay cambios sin guardar. ¿Cargar otra vez y descartarlos?",
                                L"Cambios pendientes", MB_YESNO | MB_ICONQUESTION) != IDYES)
                    return 0;
                iniciarCarga();
            }
            if (id == 102) {
                exigir(bool(sistema), "Cargue datos primero");
                auto p = std::filesystem::path(texto(carpeta));
                busy(true);
                guardado = std::async(std::launch::async, [p] {
                    guardar_datos(*sistema, p);
                    return std::string("Datos e historial guardados.");
                });
            }
            if (id == 220) {
                exigir(bool(sistema), "Cargue datos primero");
                registrar();
            }
            if (id == 504) {
                exigir(bool(sistema), "Cargue datos primero");
                proponer();
            }
            if (id == 506) {
                exigir(bool(sistema), "Cargue datos primero");
                proponer(true);
            }
            if (id == 505 && propuesta) {
                int n = 1;
                std::string did;
                do {
                    did = "ENV" + std::to_string(n++);
                } while (std::any_of(sistema->despachos().begin(), sistema->despachos().end(),
                                     [&](const auto &d) { return d.id == did; }));
                sistema->confirmar_despacho(*propuesta, did);
                invalidar();
                modificado = true;
                refrescar();
                decir(L"Despacho confirmado. Inventario actualizado; guarde los cambios.");
            }
            return 0;
        }
        case WM_TIMER:
            temporizador();
            return 0;
        case WM_CLOSE:
            if (ocupado) {
                decir(L"Espere a que termine la operacion antes de cerrar.");
                return 0;
            }
            if (modificado &&
                MessageBoxW(h, L"Hay cambios sin guardar. ¿Salir de todos modos?",
                            L"Cambios pendientes", MB_YESNO | MB_ICONQUESTION) != IDYES)
                return 0;
            DestroyWindow(h);
            return 0;
        case WM_DESTROY:
            KillTimer(h, 1);
            DeleteObject(fuente);
            DeleteObject(titulo);
            PostQuitMessage(0);
            return 0;
        }
    } catch (const std::exception &e) {
        decir(L"Error: " + w(e.what()));
    }
    return DefWindowProcW(h, msg, wp, lp);
}
} // namespace
int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int show) {
    SetProcessDPIAware();
    INITCOMMONCONTROLSEX ic{sizeof ic, ICC_LISTVIEW_CLASSES};
    InitCommonControlsEx(&ic);
    WNDCLASSW wc{};
    wc.lpfnWndProc = proc;
    wc.hInstance = inst;
    wc.lpszClassName = L"DonacionesEDA";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&wc);
    HWND h = CreateWindowW(wc.lpszClassName, L"AYUDA - Sistema de Donaciones EAFIT",
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 40, 10, 1145,
                           1010, nullptr, nullptr, inst, nullptr);
    ShowWindow(h, show);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(h, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return int(msg.wParam);
}
