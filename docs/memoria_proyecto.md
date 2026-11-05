# Evaluación práctica 3
# Sistema de gestión y distribución de donaciones post-desastre

**Estudiante:** Miguel Angel Alzate Osorno  
**Modalidad:** individual  
**Institución:** Universidad EAFIT  
**Asignatura:** Estructuras de Datos y Algoritmos  
**Fecha de elaboración:** 5 de noviembre de 2026  
**Nombre de la aplicación:** AYUDA  
**Repositorio personal:** [https://github.com/Alzate4664/sistema-donaciones-post-desastre](https://github.com/Alzate4664/sistema-donaciones-post-desastre)

## 1. Problema y objetivo

Después de un desastre, los recursos pueden concentrarse en puntos de entrada mientras los municipios afectados siguen sin suministros. La descoordinación puede ocasionar despachos duplicados, vencimientos y rutas inviables. AYUDA simula la recepción, consulta, priorización y distribución de lotes, preservando la coherencia del inventario y la trazabilidad de cada despacho.

El objetivo es aplicar estructuras de datos al problema: tablas hash para identificar entidades, heaps para priorizar solicitudes y distancias, un AVL para ordenar caducidades y un grafo para calcular caminos mínimos. La simulación usa C++20 y una interfaz de escritorio nativa de Windows. Los lugares y tiempos de transporte son sintéticos y no describen condiciones actuales de emergencia.

## 2. Modelado y fuentes

El dominio contiene donantes, centros de acopio, municipios, lotes, solicitudes y despachos. Cada lote relaciona su donante y centro con una categoría, una fecha de caducidad, unidades y condiciones logísticas. Los municipios tienen urgencia de 1 a 5; las solicitudes cuantifican necesidades por categoría.

Los datos iniciales se leen de ocho CSV en `datos/`. El ejemplo incluye dos centros, tres municipios, cuatro lotes y tres solicitudes. Los tipos de fecha se validan contra el calendario, las cantidades deben ser coherentes y los números reales deben ser finitos. El parser rechaza filas mal formadas y referencias inexistentes, con diagnóstico de archivo y fila.

La carga construye una instancia temporal, que solo reemplaza la activa después de superar la validación completa. El guardado crea una versión nueva de todos los archivos y publica un puntero a ella después de comprobar que puede cargarse. Así se evita mezclar saldos nuevos con un historial antiguo cuando falla una escritura.

## 3. Operaciones

La aplicación permite registrar entidades y vías, abrir o cerrar carreteras, consultar inventario por ID, categoría o caducidad, generar propuestas de despacho y confirmarlas. El panel informa unidades recibidas, disponibles y despachadas, aportes en COP y solicitudes pendientes.

La opción automática atiende la primera solicitud viable en orden de urgencia, antigüedad e ID. Para ella selecciona un lote compatible por FEFO: primero vence, primero sale. Antes de proponer comprueba ruta, frío, capacidad y llegada antes de caducidad. El operador revisa y confirma; proponer por sí solo no consume existencias.

La selección manual de una solicitud permite examinar un caso concreto. Cada despacho utiliza un único lote; una solicitud que requiere varios lotes se completa mediante varios despachos. Las necesidades sin suministro viable se mantienen pendientes.

## 4. Arquitectura y algoritmos

El núcleo se separa de la GUI. `modelo.hpp` define entidades; `persistencia.cpp` gestiona CSV y versiones; `avl.hpp` implementa el índice temporal; `red.cpp` calcula rutas; y `sistema.cpp` coordina reglas, índices y movimientos. `gui_win32.cpp` presenta formularios y resultados.

La tabla hash estándar proporciona acceso esperado O(1) por ID, con O(n) en el peor caso. El heap ordena tuplas `(-urgencia, llegada, id)`. El AVL usa `(caducidad, id_lote)`, lo que permite que dos lotes tengan la misma fecha sin colisionar como claves. Insertar y eliminar cuestan O(log n); una consulta de rango cuesta O(log n+k) para k resultados.

Dijkstra usa una lista de adyacencia y un heap de distancias acumuladas. Excluye carreteras cerradas y vías sin condiciones de frío cuando el lote lo requiere. Reconstruye el camino mediante predecesores; requiere pesos no negativos. Para un grafo simple su costo usual es O((V+E) log V). La implementación adapta el ejemplo de logística del curso y conserva su atribución.

El análisis completo, incluida la diferencia entre operaciones individuales y refrescos completos, aparece en [Diseño y sustentación](diseno.md). No se atribuye optimalidad global a la política FEFO y triaje bajo múltiples restricciones logísticas.

## 5. Consistencia y trazabilidad

Para cada lote se mantiene `recibido = disponible + reservado + despachado`. Para cada solicitud, lo solicitado se descompone en pendiente y cantidades enviadas. En esta versión no hay reservas pendientes: la confirmación es inmediata y reservado permanece en cero.

Una propuesta conserva la versión del inventario y de la red. Confirmar exige que ambas sigan vigentes y vuelve a validar la propuesta. El identificador único de despacho y la invalidación posterior evitan descontar dos veces. El historial importado debe concordar con los saldos y describir un recorrido existente con llegada anterior al vencimiento.

## 6. Interfaz y pruebas

La GUI muestra inventario, solicitudes ordenadas, formularios, resumen, alertas e historial. Representa el recorrido como una secuencia de nodos y flechas. Carga, guardado y planificación se ejecutan en segundo plano; las modificaciones del inventario se confirman en el hilo de interfaz.

La suite contiene 22 casos automatizados y pasó sin fallos en Windows x64 con MSVC 19.44. El benchmark incluye 90 mediciones y compara hash frente a búsqueda lineal, heap frente a extracción en lista y AVL frente a BST con entradas ordenadas. También se operó la GUI para comprobar un despacho, guardado, recarga y priorización automática.

El [informe de pruebas](pruebas.md) presenta resultados, metodología y límites. Sus anexos de evidencia contienen las salidas observadas. No se afirma cobertura total de líneas ni validación de la fluidez de la GUI con grandes volúmenes.

## 7. Asistencia de IA y alcance individual

Miguel Angel Alzate Osorno presenta el proyecto como estudiante único. El desarrollo tuvo asistencia de IA en análisis, implementación, generación de pruebas y documentación. Se conserva la atribución del profesor para el ejemplo algorítmico adaptado. Esta presentación individual no implica que el código se haya producido sin asistencia.

El [anexo de transparencia](transparencia_ia.md) identifica prompts y decisiones asistidas.

## 8. Alcance de la entrega

El paquete contiene fuentes C++, configuración de compilación, ejecutables Windows x64, datos sintéticos, pruebas, benchmark e informes. El estudiante proporcionó el enlace a su repositorio personal. El repositorio personal es público y contiene los archivos de la entrega. No se ha enviado la entrega institucional.

La GUI requiere Windows y no presenta un mapa geográfico. El programa no soporta escritura simultánea desde varias instancias ni rutas operativas reales. Se usa un AVL propio y contenedores estándar para hash y heap; debe verificarse con el docente si su expectativa incluye implementarlos manualmente.
