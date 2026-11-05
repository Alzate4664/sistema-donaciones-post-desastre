# AYUDA — Sistema de donaciones post-desastre

Proyecto académico C++20 basado en el parcial 3 de Estructuras de Datos y Algoritmos, EAFIT. Los municipios, acopios, cantidades y tiempos son **datos sintéticos**: no son información operativa de emergencia.

**Estudiante:** Miguel Angel Alzate Osorno. **Modalidad:** trabajo individual.

La [memoria del proyecto](docs/memoria_proyecto.md) reúne la presentación de la entrega. La [guía de sustentación](docs/guia_sustentacion.md) propone un recorrido de estudio y demostración.

## Abrir

En Windows, abra `bin/donaciones_gui.exe`. Mantenga las carpetas `bin` y `datos` juntas dentro del proyecto. Se incluyen los ejecutables comprobados en Windows x64; si otro equipo requiere el runtime de Visual C++, puede recompilar con Visual Studio.

La aplicación carga el ejemplo automáticamente: 940 unidades, dos centros, tres municipios, un donante y cuatro lotes. El evento simulado es 2026-11-01 y la salida predeterminada 2026-11-05.

1. Pulse **Siguiente por urgencia** para buscar la primera solicitud viable, con mayor urgencia primero. Las que no tienen recursos viables permanecen pendientes. También puede seleccionar una solicitud y pulsar **Proponer ruta** para examinarla de forma manual.
2. Revise lote, cantidad, tiempo y recorrido. Proponer no consume inventario.
3. Pulse **Confirmar despacho**: actualiza existencias, pendiente e historial. No permite confirmar dos veces la misma propuesta.
4. Pulse **Guardar** para persistir la versión completa. Cargar descarta cambios solo después de confirmación si existen cambios pendientes.
5. Use el formulario izquierdo para registrar lotes, solicitudes, donantes, centros, municipios y vías o cerrar/abrir vías. Peso y volumen son **por unidad**. Las vías son dirigidas; para permitir regreso registre la dirección inversa.

Buscar admite fragmentos de ID o categoría; una fecha `AAAA-MM-DD` muestra también lotes cuya caducidad está entre el evento y esa fecha. Las categorías deben coincidir exactamente entre lotes y solicitudes. IDs: letras ASCII, números, guion o guion bajo. Decimales: punto. Valores de frío: 0/1. Aportes monetarios: centavos de COP, separados de las unidades físicas.

La caducidad es el inicio del día indicado; la llegada debe ocurrir estrictamente antes. El modelo usa salida a medianoche y tiempos en minutos. Una operación produce un despacho de un lote; para completar una solicitud con varios lotes, repita el ciclo.

## Compilar y probar

Requisitos: compilador C++20 y CMake 3.20 o posterior. En Windows instale la carga de trabajo de desarrollo de escritorio C++ de Visual Studio, con Windows SDK.

Desde Developer Command Prompt:

```bat
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_DEBUG="/Od /Ob0"
cmake --build build
ctest --test-dir build --output-on-failure
build\benchmark.exe benchmark.csv
```

También puede ejecutar `compilar.cmd`, que intenta localizar Visual Studio 2022 Community. Con otras ediciones, utilice la consola de desarrollador correspondiente. Para probar la GUI recién compilada, copie `datos` junto a `build/donaciones_gui.exe` o indique su ruta en el campo de carpeta.

En Linux/macOS se compilan el núcleo, las pruebas y el benchmark con CMake; la GUI es exclusiva de Windows. Esas plataformas no se probaron durante esta sesión.

## Código y documentación

- `include/donaciones/avl.hpp`: AVL propio con inserción, borrado, consultas por fecha y verificación de invariantes.
- `src/red.cpp`: Dijkstra adaptado del ejemplo del profesor, con heap, restricciones y reconstrucción.
- `src/sistema.cpp`: tablas hash, prioridad, FEFO y transacciones de inventario.
- `src/persistencia.cpp`: CSV estricto y guardado por snapshots completos.
- `src/gui_win32.cpp`: interfaz nativa con carga, guardado y planificación en segundo plano.
- `tests/pruebas.cpp`: pruebas deterministas de reglas y casos límite.
- `docs/diseno.md`: decisiones, complejidad y guía de sustentación.
- `docs/pruebas.md`: resultados y límites de validación.
- `docs/transparencia_ia.md`: uso de IA y relación con el código de clase.

## Datos guardados

La carpeta de ejemplo contiene CSV editables de importación inicial. Al guardar, el sistema escribe una carpeta `version-...`, la vuelve a cargar para validarla y publica `actual.csv`, que apunta a esa versión. Las cargas posteriores leen esa versión; para editar archivos a mano, edite el snapshot señalado o use otra carpeta inicial. No modifique `actual.csv` mientras el programa trabaja.

Un fallo antes de publicar conserva el snapshot anterior. Las versiones anteriores se conservan; no hay limpieza automática. No se soportan varias instancias escribiendo en la misma carpeta, ni se ofrece una garantía frente a fallo físico del almacenamiento. Las operaciones del inventario son secuenciales y el modelo no contiene reservas pendientes: reservado se mantiene en cero y la confirmación es inmediata.

## Antes de entregar

Miguel Angel Alzate Osorno presenta el proyecto de forma individual. Repositorio personal proporcionado por el estudiante: [https://github.com/Alzate4664/sistema-donaciones-post-desastre](https://github.com/Alzate4664/sistema-donaciones-post-desastre). El repositorio de entrega es público: el código y la documentación pueden consultarse con el enlace. La visibilidad pública no concede permiso para modificar el repositorio. No se ha enviado la entrega a EAFIT. Revise con el docente si se exige implementar hash y heap desde cero: aquí se usan `std::unordered_map` y `std::priority_queue`, y el AVL sí es propio. No hay una exigencia de implementación manual explícita en el PDF leído.
