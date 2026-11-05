# Diseño y sustentación

## Modelo e invariantes

Donante identifica una persona u organización y su aporte monetario en centavos COP. Centro declara refrigeración. Municipio declara urgencia 1–5. Lote identifica donante, centro, categoría, caducidad, unidades recibidas y saldos, peso/volumen por unidad y frío. Solicitud registra categoría, municipio, cantidad y orden de llegada. Despacho conserva lote, solicitud, unidades, fecha y recorrido.

Cada lote cumple `recibido = disponible + reservado + despachado`. Cada solicitud cumple `cantidad = pendiente + suma de despachos`. El loader contrasta ambos con el historial, rechaza duplicados y no publica un estado parcialmente importado.

Las cantidades son enteros; pesos y volúmenes positivos finitos. Dinero usa `int64_t`, evitando mezclar pesos monetarios con kg o unidades físicas. Fechas usan `std::chrono::sys_days`; el parser valida calendario real y formato estricto.

## Estructuras: por qué y cómo explicarlas

| Estructura | Clave y función | Costo |
|---|---|---|
| Hash | ID de donantes, centros, municipios, lotes y solicitudes | O(1) esperado en búsqueda e inserción amortizada; O(n) peor caso |
| Hash compuesto | Centro + separador reservado + categoría | O(1) esperado para localizar grupo; devolver k IDs cuesta O(k) |
| Min-heap | `(-urgencia, llegada, id)` | Insertar/extraer O(log s), consultar cima O(1) |
| AVL | `(caducidad, id_lote)` | Insertar/borrar O(log n); intervalo O(log n+k) |
| Grafo dirigido | Listas de adyacencia por ID; minutos por vía | Espacio O(V+E) |
| Heap de Dijkstra | `(distancia acumulada provisional, nodo)` | O((V+E) log V) para grafo simple; heap con entradas repetidas O(E) |

Las claves de longitud variable introducen además su costo de hashing/comparación. Las cotas usuales de la tabla suponen identificadores acotados. En el AVL, las comparaciones de fechas suelen decidir antes que los IDs.

### AVL

Cada nodo almacena altura `1 + max(altura_izquierda, altura_derecha)`. El factor de balance es izquierda menos derecha y debe pertenecer a {-1, 0, 1}. Insertar y borrar recorren un camino y rebalancean al regresar. LL usa rotación derecha; RR izquierda; LR y RL combinan dos. Al borrar con dos hijos se toma el sucesor in-order del subárbol derecho.

El mínimo número de nodos de altura h satisface `N(h)=1+N(h-1)+N(h-2)`. Crece como Fibonacci, por lo que h=O(log n). Las operaciones recorren O(h); cada rotación cuesta O(1). Un intervalo poda ramas fuera de las fechas y visita los k resultados: O(log n+k). Memoria O(n) y pila O(log n).

### Heap de solicitudes

La tupla negativa hace que mayor urgencia tenga menor clave; llegada e ID dan desempate determinista. En esta versión `prioridades()` reconstruye una cola local a partir del estado actual y extrae el orden completo, por lo que cuesta O(s log s) y espacio O(s). No se afirma O(log s) para refrescar todo el panel: esa cota corresponde a una operación individual del heap. Reconstruir evita conservar claves obsoletas, a cambio de trabajo extra al refrescar.

### Dijkstra

La distancia de origen empieza en cero. Se extrae del heap la distancia acumulada provisional más pequeña, se ignoran entradas obsoletas y se relajan sus aristas. Si `d[u]+peso(u,v)<d[v]`, se mejora d[v] y su predecesor. Se excluyen carreteras cerradas y, si corresponde, vías sin cadena de frío. El camino se reconstruye desde destino mediante predecesores y luego se invierte.

No se elige simplemente el vecino cuya carretera es más corta. La corrección depende de pesos no negativos: al extraer el mínimo vigente, ninguna ruta posterior a través de un vértice más distante puede mejorarlo. Los pesos negativos se rechazan. Una ruta inexistente se devuelve como `std::nullopt`, no como una distancia ficticia.

## Asignación

Primero se ordenan solicitudes por urgencia. Para cada una se buscan lotes compatibles en orden FEFO (primero vence, primero sale). Se comprueban stock, categoría, ruta, frío, capacidad de kg/m³ y llegada antes de vencimiento. La cantidad es el mínimo entre pendiente, disponible y capacidades enteras por unidad.

`proponer_siguiente` devuelve la primera solicitud viable por triaje; mantiene pendientes las que no puede atender. El operador revisa y confirma. Existe también selección manual para examinar casos concretos.

Una propuesta contiene versión del inventario y de la red. Confirmar verifica versiones y vuelve a calcular lote, cantidad y recorrido; una modificación o reintento la invalida. Solo después se registra el movimiento y se actualizan saldos.

La política es ávida y explicable; no demuestra un óptimo global para múltiples vehículos, capacidades y ventanas temporales. Una consulta puede recorrer k lotes y calcular varias rutas: O(log n+k·(V+E)log V) como cota conservadora, además de prioridades. Planificar todas las solicitudes puede repetir ese costo s veces. Es una limitación explícita para futuros índices de rutas/categorías y cachés invalidados por versión de red.

## Persistencia y GUI

CSV acepta comas dentro de comillas y comillas escapadas duplicándolas; rechaza campos multilinea, cabeceras incorrectas, filas truncadas, enteros desbordados y números no finitos. El diagnóstico incluye archivo y fila. Se valida todo en una instancia temporal antes de sustituir el sistema activo.

Cada guardado crea un snapshot nuevo, lo valida y sustituye un único puntero `actual.csv`. En Windows usa `MoveFileExW` con reemplazo y escritura completada. Si falla antes, el puntero anterior sigue activo; pueden quedar versiones huérfanas, conservadas por seguridad.

La interfaz utiliza controles nativos, teclado y etiquetas. Carga, guardado y planificación trabajan en segundo plano; el hilo de la ventana recoge los resultados mediante un temporizador. Mientras tanto se bloquean cambios y cierre. Confirmar y modificar inventario ocurre en el hilo de UI. El refresco de tablas aún es síncrono y no virtualizado; no se ha validado la fluidez del panel con cientos de miles de filas.

## Preguntas para la sustentación

1. ¿Qué pasaría si reemplazamos el AVL por el BST del curso y entran fechas ordenadas?
2. ¿Por qué FIFO no sirve para atender urgencia 5 antes de urgencia 2?
3. ¿Por qué la consulta de rango cuesta O(log n+k), no solo O(log n)?
4. ¿Qué representa una entrada obsoleta del heap de Dijkstra?
5. ¿Qué cambia si una carretera se cierra después de proponer?
6. ¿Cómo comprobamos que cargar historial no invente unidades ni acepte un camino inexistente?
7. ¿Por qué un heap local reconstruido tiene costo de refresco distinto al de una extracción individual?
8. ¿Qué garantiza el snapshot y qué queda fuera de su garantía?
