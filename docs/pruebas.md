# Informe de pruebas y rendimiento

Fecha: 5 de noviembre de 2026. Entorno: Windows x64; Visual Studio 2022 Community; MSVC 19.44.35226; CMake 3.31.6; compilación Release, C++20. No se validaron otros sistemas operativos.

## Suite automatizada

El ejecutable `pruebas` contiene 22 casos; CTest lo registra como un único test que ejecuta todos ellos. Resultado observado: **22/22 correctos**, CTest sin fallos. La salida reproducible se conserva en `evidencia/pruebas.txt`.

| Área | Casos |
|---|---|
| Datos | Fechas reales/estrictas, cantidades negativas, NaN, duplicados, referencias y caducidad anterior al evento |
| AVL | Rotaciones LL/RR/LR/RL, eliminación, 2.000 operaciones aleatorias con semilla 42 contrastadas con `std::set` |
| Red | Dijkstra con ruta conocida, alternativa, cierre, desconexión, frío, origen=destino y pesos negativos |
| Inventario | FEFO, capacidad parcial, agotamiento, solicitudes pendientes, triaje y selección automática viable |
| Integridad | Reintento, propuesta alterada, cambio de red, fallo sin descuento y 100 despachos secuenciales |
| CSV | Comas y comillas, roundtrip con historial, cabecera incorrecta, campo nulo, malformación e importación sin cambios parciales |
| Recuperación | Guardado bloqueado deliberadamente antes de publicar conserva el snapshot anterior |
| Historial | Saldos incoherentes, llegada tras caducidad y tramo inexistente rechazados |

Se observaron fallos antes de las correcciones de guardado e historial: 18/20. Tras corregirlos: 20/20. Se reprodujo también la aceptación indebida de saldo CSV -1: 20/21 antes de endurecer el parser. La prueba de selección automática falló inicialmente por ausencia de la operación; posteriormente pasó.

## Prueba de interfaz con control de escritorio

Sobre una copia de los CSV en la carpeta de compilación se comprobó:

1. Carga inicial: recibido 940, disponible 940, despachado 0.
2. Registrar sin ID: mensaje «Error: ID vacio o demasiado largo»; cantidades sin cambios.
3. Seleccionar S1 y proponer: L1, 200 unidades, 45 minutos, recorrido AC1 → M1; inventario todavía intacto.
4. Confirmar: disponible 740, despachado 200, L1 disponible 300; S1 sale de pendientes, aparece ENV1 y confirmar queda deshabilitado.
5. Guardar: mensaje de éxito y CSV con ENV1. Se reabrió la versión siguiente y conservó 740 disponibles y 200 despachados.
6. En la versión final, «Siguiente por urgencia» propuso S2, lote L2, 40 unidades y AC1 → M1 → M2 en 135 minutos, sin descontar stock. El guardado por snapshots también terminó con éxito desde la GUI.

Estas acciones se ejecutaron mediante la herramienta de control de escritorio; no constituyen una suite autónoma de GUI incluida en el proyecto. La apariencia inicial y final se inspeccionó visualmente. No se midió latencia de interacción, no se forzó cierre durante una operación prolongada ni se automatizaron todos los formularios. El código bloquea cierre y mutaciones durante trabajo de fondo; esa rama necesita una prueba prolongada adicional si se desea certificarla.

## Benchmark

Archivo: `../benchmark.csv`, 90 filas de mediciones, cinco repeticiones y semillas 42–46. Las comparaciones hash/lista y heap/lista comprueban sumas equivalentes para impedir medir operaciones distintas sin detectarlo. Las consultas/extracciones excluyen construcción inicial; las pruebas de árboles miden inserción. Tiempos de pared en milisegundos, sin umbral de aprobación.

Medianas observadas:

| Tamaño | Hash, 1.000 búsquedas | Lista, 1.000 búsquedas | Heap, 1.000 extracciones | Lista, 1.000 extracciones |
|---|---:|---:|---:|---:|
| 1.000 | 0,0104 | 0,0973 | 0,0486 | 0,2013 |
| 10.000 | 0,0141 | 0,3213 | 0,0407 | 1,1363 |
| 100.000 | 0,0165 | 3,1583 | 0,0551 | 15,1953 |

| Entradas ordenadas | AVL, inserción completa | BST sin balance, inserción completa |
|---|---:|---:|
| 1.000 | 0,3424 | 1,8996 |
| 10.000 | 1,9713 | 111,3860 |
| 100.000 | 24,8591 | No ejecutado a este tamaño |

Para evitar el costo cuadrático del BST se limitó a 10.000 nodos; el CSV contiene esa serie de 10.000 también en las rondas del tamaño máximo. No comparar esa fila con 100.000 entradas AVL como si sus tamaños fueran iguales. El benchmark usa IDs enteros y no simula todas las validaciones o los despachos del dominio. Las diferencias respaldan la elección de estructura, pero no garantizan tiempos en otros equipos ni demuestran por sí solas complejidad asintótica.

Interfaz real del benchmark: `benchmark.exe ruta_salida.csv` (argumento posicional). Difiere de la bandera `--salida` prevista inicialmente; no use esa bandera.

## Revisión y límites

Una revisión independiente estática señaló los dos defectos de guardado/historial, posteriormente corregidos con regresiones. Se reclasificó el saldo negativo importado como un problema de validación y se corrigió. La diferencia del argumento del benchmark se conserva documentada como detalle menor.

No se afirma cobertura porcentual de líneas, seguridad frente a fallos físicos, concurrencia multiusuario ni cumplimiento certificado del nivel «Excelente» de la rúbrica. El panel usa controles nativos y recorrido textual con flechas; no contiene mapa geográfico, tablas virtualizadas ni una batería autónoma de UI. El núcleo sí permite pruebas y compilación independientes de la GUI.
