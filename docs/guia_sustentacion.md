# Guía de sustentación individual

**Estudiante:** Miguel Angel Alzate Osorno.

Esta guía organiza el estudio y la demostración. No certifica dominio del código: cada respuesta debe poder justificarse abriendo la función correspondiente y ejecutando un caso.

## Recorrido de estudio

| Orden | Archivo | Qué debes poder explicar |
|---|---|---|
| 1 | `include/donaciones/modelo.hpp` | Entidades, unidades y diferencia entre datos del lote y de la solicitud |
| 2 | `src/modelo.cpp` | Validación de fechas, identificadores y excepciones |
| 3 | `include/donaciones/avl.hpp` | Alturas, rotaciones, eliminación y consultas de rango |
| 4 | `src/red.cpp` | Cola de distancias, relajación, predecesores y filtros |
| 5 | `src/sistema.cpp` | Prioridad, FEFO, propuestas y confirmación coherente |
| 6 | `src/persistencia.cpp` | CSV, carga temporal y publicación del snapshot |
| 7 | `tests/pruebas.cpp` | Qué fallos detectan las pruebas y cuáles no cubren |

## Demostración sugerida: ocho minutos

Trabaja sobre una copia de `datos/` para conservar el ejemplo original. Indica esa carpeta en la aplicación y pulsa Cargar. Los valores siguientes presuponen el ejemplo sin despachos previos.

1. **Problema y modelo — un minuto.** Explica que se distribuyen lotes a solicitudes municipales, con restricciones de caducidad, transporte, peso, volumen y frío. Muestra recibido 940, disponible 940 y despachado 0.
2. **Prioridad y propuesta — dos minutos.** Pulsa Siguiente por urgencia. Debe proponer S1, lote L1, 200 unidades y AC1 → M1 en 45 minutos. Explica por qué S1 precede a S2 y por qué el stock todavía no cambia.
3. **Confirmación — un minuto.** Confirma: disponible 740, despachado 200; L1 queda con 300 disponibles. S1 desaparece de pendientes y ENV1 aparece en historial. Señala que el botón de confirmación se deshabilita.
4. **Ruta de varios tramos — un minuto.** Pulsa Siguiente por urgencia otra vez. Propone S2, L2, 40 unidades y AC1 → M1 → M2 en 135 minutos, mejor que los 180 del tramo directo. Explica la relajación de Dijkstra.
5. **Validación — un minuto.** En Registrar lote deja el ID vacío y pulsa Registrar/aplicar. Debe aparecer el error y conservarse el inventario. Comenta los casos de CSV inválido automatizados.
6. **Persistencia y pruebas — dos minutos.** Guarda y carga: el historial debe conservarse. Muestra la salida `22/22 pruebas correctas` y una comparación del benchmark. Distingue mediciones observadas de garantías matemáticas.

No uses datos guardados de una práctica anterior para exigir los mismos resultados iniciales: el programa carga el snapshot activo si existe `actual.csv`.

## Preguntas frecuentes y razonamiento esperado

**¿Por qué cuatro estructuras distintas?** El acceso por ID, la extracción del más urgente, las consultas por intervalo y los caminos mínimos son operaciones diferentes. Una sola lista funcionaría para algunos casos, pero obligaría a recorridos repetidos y no cumpliría la selección de estructuras del enunciado.

**¿El hash siempre es O(1)?** No. La búsqueda es O(1) esperada bajo supuestos adecuados de distribución y carga; el peor caso puede ser O(n). Además, calcular el hash depende de la longitud de la clave.

**¿Por qué un AVL y no el BST del taller?** El BST puede degenerar si las fechas llegan ordenadas. El AVL mantiene diferencia de alturas de máximo uno, asegurando altura O(log n). Muestra una rotación con las fechas en orden 3, 2, 1 y luego uno de los casos dobles.

**¿Por qué se agrega el ID a la fecha?** Dos lotes pueden vencer el mismo día. La clave compuesta los distingue y conserva un orden determinista.

**¿Por qué el rango no cuesta solamente O(log n)?** Hay que devolver k lotes. Localizar los límites es logarítmico, pero recorrer y producir resultados agrega O(k).

**¿Qué está dentro del heap de triaje?** Tuplas con urgencia negada, llegada e ID. Un min-heap pone primero la urgencia mayor porque su negativo es menor. `prioridades()` reconstruye y vacía una cola local; ordenar el conjunto completo cuesta O(s log s).

**¿Dijkstra elige la carretera más corta en cada cruce?** Selecciona la menor distancia acumulada provisional desde el origen, que es diferente de elegir la menor arista vecina. Los pesos deben ser no negativos. Una mejora de distancia agrega una entrada al heap; las anteriores se descartan cuando quedan obsoletas.

**¿Qué ocurre si cierran una vía después de proponer?** Cambia la versión de la red. La confirmación rechaza la propuesta obsoleta y obliga a recalcular. Un cierre actual no invalida por sí solo un despacho histórico.

**¿Cómo se evita descontar dos veces?** ID de despacho único, versión de propuesta, recálculo previo y posterior invalidación. La prueba de reintento comprueba que el stock no vuelve a disminuir.

**¿Qué significa FEFO?** Primero vence, primero sale, entre lotes que también cumplen categoría, disponibilidad, ruta y restricciones. No significa enviar el lote más próximo a vencer aunque esté vencido o no pueda llegar a tiempo.

**¿La asignación es globalmente óptima?** No está demostrado. Prioriza urgencia y usa FEFO con restricciones. No resuelve un modelo global de flota, múltiples viajes y ventanas temporales.

**¿Qué garantiza el guardado?** Los CSV de una nueva versión se escriben y validan antes de publicar el puntero. Si falla antes de publicarlo, se conserva la versión anterior. No es una base de datos multiusuario ni una garantía frente a daño físico del disco.

**¿Qué hiciste con ayuda de IA?** Describe lo que realmente hiciste: elección del problema y tecnología, revisión y aprendizaje personal que hayas completado, y el alcance de asistencia registrado en el anexo. No atribuyas como trabajo manual propio la generación de código realizada por el asistente.

## Ejercicios para comprobar comprensión

- Predice la nueva prioridad si un municipio tiene urgencia 5 y otro 4, pero el segundo pidió ayuda antes. Comprueba la predicción con nuevas solicitudes.
- Registra dos lotes de agua con fechas diferentes y predice cuál se propone primero.
- Cierra M1 → M2 y comprueba que S2 utiliza AC1 → M2 en 180 minutos si sigue siendo viable.
- Reduce la capacidad a 10 kg para agua de 1 kg por unidad y calcula cuántas unidades pueden enviarse.
- Localiza en la prueba de guardado fallido cómo se bloquea la publicación y por qué el snapshot anterior sigue legible.

## Pendientes administrativos

El enlace al repositorio personal ya está incorporado en la memoria y el anexo. Comprobar que el repositorio público contiene la entrega y que el ZIP final se abre; revisar la modalidad individual con las instrucciones del curso y entregar personalmente en el buzón institucional. La publicación en GitHub fue autorizada por el estudiante. No se ha realizado la entrega institucional.
