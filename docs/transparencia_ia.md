# Anexo de transparencia técnica de IA

Estudiante: **Miguel Angel Alzate Osorno**. Modalidad: **individual**. Repositorio personal: [https://github.com/Alzate4664/sistema-donaciones-post-desastre](https://github.com/Alzate4664/sistema-donaciones-post-desastre). Repositorio de entrega público. Fecha de trabajo: 5 de noviembre de 2026.

## Prompts relevantes de esta conversación

1. El estudiante proporcionó el repositorio `https://github.com/evalenciEAFIT/EstructurasDeDatos` y el PDF del parcial y pidió: «Revisa que algoritmos o codigos trabajados en clase (en el repo) nos pueden servir para la solucion del parcial. Utiliza las skills disponibles y necesarias para solucionarlo y hagamoslo».
2. Elección de tecnología: «Sigamos con c++».
3. Aprobación del plan: «Perfecto, empecemos!».
4. Aclaración de modalidad y autoría: el estudiante indicó que realiza el trabajo solo y proporcionó su nombre, Miguel Angel Alzate Osorno. Se personalizaron la memoria y la documentación, manteniendo la atribución del código de clase y la asistencia de IA.

La IA leyó el PDF, consultó el repositorio con la conexión de GitHub, preparó análisis y plan, escribió código C++, datos sintéticos, pruebas y documentación, compiló con herramientas existentes y operó la GUI sobre una copia de prueba. No se consultaron datos operativos reales. Posteriormente, con autorización del estudiante, se preparó la publicación del proyecto y la documentación en su repositorio personal; no se realizó la entrega institucional.

## Relación con la clase

- Dijkstra deriva del patrón de `T4_Red_Donantes/Solucion_medicos/06_logistica_traslado.cpp`, atribuido al Prof. Edison Valencia Díaz. Se conservaron el heap de mínimos, la relajación y el descarte de entradas obsoletas; se añadieron predecesores, resultado opcional, vías cerradas y frío.
- `HashTable.cpp` y `BST.cpp` del mismo taller se estudiaron como referencias. No se reutilizó la tabla fija de diez buckets ni se presentó el BST sin balance como AVL. Se usa hash estándar y se escribió un AVL con pruebas de rotaciones y eliminación.
- El patrón de generación CSV de `GenerarDatos/generar_datos.py` sirvió de referencia para separar datos sintéticos y código. Los datos de esta entrega se construyeron para el nuevo dominio.
- Se revisó merge sort en `OrdenaListaEnlazada/src/divide_y_venceras.c`; no se incorporó como sustituto de índices o heaps, porque no cumple esa función.

Enlace al ejemplo de Dijkstra: https://github.com/evalenciEAFIT/EstructurasDeDatos/blob/main/T4_Red_Donantes/Solucion_medicos/06_logistica_traslado.cpp

## Decisiones asistidas y justificación

C++20: continuidad con el taller y tipos de fecha seguros. GUI Win32: aprovecha el entorno Windows sin instalar Qt. AVL propio: demostrar balanceo y consultas de rango. Hash y heap estándar: contenedores probados y foco en la política de asignación; debe confirmarse la expectativa docente sobre implementación manual. FEFO + urgencia: política transparente, sin atribuir optimalidad global. Snapshots: impedir que un fallo de I/O mezcle inventario nuevo con historial antiguo.

Una revisión independiente de código identificó el riesgo de guardado parcial y la validación incompleta de rutas históricas. Se escribieron pruebas que reprodujeron ambos fallos, luego se corrigieron y se ejecutó la suite. También se endureció el rechazo de saldos negativos importados.
