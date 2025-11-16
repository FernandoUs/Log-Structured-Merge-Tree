# LSM-Tree Spatial Database System

Implementación completa de un sistema de base de datos espacial basado en LSM-tree con soporte para consultas SQL simples.

## 📚 Descripción

Este proyecto implementa la investigación del paper **"Comparison of LSM indexing techniques for storing spatial data"**, construyendo un sistema de gestión de bases de datos espacial mínimo en C++.

## 🏗️ Arquitectura

El sistema está organizado en 6 fases principales:

### Fase 1: Estructuras de Datos Fundamentales
- **Point**: Puntos multidimensionales (D-dimensional)
- **MBR** (Minimum Bounding Rectangle): Rectángulos envolventes para filtrado espacial
- **Comparadores Espaciales**:
  - `SimpleComparator` (Nearest-X): Ordenamiento por dimensión
  - `HilbertCurveComparator`: Curva de Hilbert para preservar localidad espacial
  - `ZOrderComparator`: Curva Z-order (Morton)
- **R-tree**: Índice espacial local con bulk-loading eficiente

### Fase 2: LSM-tree Core
- **LSMComponent**: Componentes de disco con R-tree local y MBR total
- **MemTable**: Componente activo en memoria
- **Flush**: Operación MemTable → Disco
- **Tombstones**: Soporte para borrado mediante registros antimateria
- **SpatialRangeQuery**: API de búsqueda espacial con filtrado MBR

### Fase 3: Políticas de Fusión (Merge Policies)
- **Stack-based**:
  - `BinomialMergePolicy` (k=4, 10)
  - `TieredMergePolicy` (B=4, 10)
  - `ConcurrentMergePolicy`
- **Leveled**:
  - `LeveledMergePolicy`: Arquitectura de niveles con fusión selectiva

### Fase 4: Algoritmos de Particionamiento Espacial
- **SizePartitioning**: Partición por tamaño con SimpleComparator o Hilbert
- **STRPartitioning** (Sort-Tile-Recursive): Componentes espacialmente disjuntos
- **RStarGrovePartitioning**: MBRs cuadrados (3 fases: Sampling, Boundary, Final)

### Fase 5: Capa SQL
- **Lexer y Parser**: Análisis de SQL simple
- **CatalogManager**: Gestión de metadatos de tablas
- **QueryExecutor**: Motor de ejecución de consultas

Comandos SQL soportados:
```sql
CREATE TABLE points (id INT, location POINT, value DOUBLE)
INSERT INTO points VALUES (0.5, 0.5, 100)
SELECT COUNT(*) FROM points WHERE spatial_intersect(location, 0, 0, 1, 1)
SELECT * FROM points WHERE spatial_intersect(location, x1, y1, x2, y2)
```

### Fase 6: Evaluación y Pruebas
- **CLI** (REPL): Interfaz interactiva de línea de comandos
- **Generación de Workloads**:
  - Dataset Random (uniforme)
  - Dataset Clustered (simulando OpenStreetMap)
  - Ciclo Load → Insert → Read
- **Métricas**:
  - Write Amplification (WA)
  - Read Amplification (RA)
  - Latencia promedio de queries
- **Benchmark**: Comparación de 9+ configuraciones

## 🚀 Compilación

### Requisitos
- C++17 o superior
- CMake 3.10+
- Compilador compatible (GCC, Clang, MSVC)

### Compilar (Linux/Mac)
```bash
mkdir build
cd build
cmake ..
make
```

### Compilar (Windows con Visual Studio)
```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Compilar (Windows con MinGW)
```powershell
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

## 📖 Uso

### Modo Interactivo (CLI)
```bash
./lsm_spatial_db
```

Comandos disponibles en el CLI:
- `help` - Mostrar ayuda
- `metrics` - Mostrar métricas de rendimiento
- `tables` - Listar todas las tablas
- `clear` - Limpiar métricas
- `exit` / `quit` - Salir

### Modo Demo
```bash
./lsm_spatial_db demo
```
Ejecuta una demostración interactiva con datos de ejemplo.

### Modo Benchmark
```bash
./lsm_spatial_db benchmark
```
Ejecuta evaluación completa comparando:
- 9+ configuraciones de políticas de merge y particionamiento
- Datasets Random vs Clustered
- Selectividad alta (10^-3) vs baja (10^-5)

## 📊 Ejemplo de Uso

```sql
-- Crear tabla
CREATE TABLE cities (id INT, location POINT, population DOUBLE)

-- Insertar datos
INSERT INTO cities VALUES (0.1, 0.1, 1000000)
INSERT INTO cities VALUES (0.5, 0.5, 500000)
INSERT INTO cities VALUES (0.9, 0.9, 2000000)

-- Consulta espacial por rango
SELECT COUNT(*) FROM cities WHERE spatial_intersect(location, 0, 0, 0.5, 0.5)

-- Ver métricas
metrics
```

## 🧪 Configuraciones de Benchmark

El sistema evalúa las siguientes configuraciones:

1. **Binomial k=4 / Simple**
2. **Binomial k=10 / Simple**
3. **Binomial k=4 / Hilbert**
4. **Tiered B=4 / Simple**
5. **Tiered B=10 / Simple**
6. **Leveled / STR / Simple**
7. **Leveled / STR / Hilbert**
8. **Leveled / R*-Grove / Simple**
9. **Concurrent / Simple**

## 📁 Estructura del Proyecto

```
Log-Structured-Merge-Tree/
├── include/
│   ├── spatial/
│   │   ├── Point.h                    # Puntos multidimensionales
│   │   ├── MBR.h                      # Minimum Bounding Rectangle
│   │   ├── SpatialComparators.h       # Comparadores (Simple, Hilbert, Z-order)
│   │   └── RTree.h                    # R-tree con bulk-loading
│   ├── lsm/
│   │   ├── LSMComponent.h             # Componente de disco
│   │   ├── LSMTree.h                  # LSM-tree principal
│   │   ├── MergePolicy.h              # Políticas de fusión
│   │   └── PartitioningStrategy.h     # Algoritmos de particionamiento
│   ├── sql/
│   │   ├── Lexer.h                    # Analizador léxico SQL
│   │   ├── Parser.h                   # Parser SQL
│   │   └── QueryExecutor.h            # Motor de ejecución
│   ├── cli/
│   │   └── CLI.h                      # Interfaz de línea de comandos
│   └── workload/
│       └── Workload.h                 # Generación de workloads y benchmarks
├── src/
│   └── main.cpp                       # Programa principal
├── CMakeLists.txt                     # Configuración de CMake
└── README.md                          # Este archivo
```

## 📈 Métricas de Rendimiento

El sistema calcula y reporta:

- **Write Amplification (WA)**: Cantidad de escrituras amplificadas por merges
- **Read Amplification (RA)**: Número de componentes escaneados por query
- **Latencia Promedio**: Tiempo promedio de ejecución de queries (ms)
- **Cantidad de Componentes**: Número de componentes en disco
- **Total de Registros**: Registros totales en el sistema

## 🔬 Basado en Investigación

Este proyecto implementa los conceptos del paper de investigación:
> "Comparison of LSM indexing techniques for storing spatial data"

Características clave implementadas:
- ✅ Filtrado MBR para componentes
- ✅ Comparadores espaciales (Simple, Hilbert)
- ✅ Políticas de merge (Binomial, Tiered, Leveled, Concurrent)
- ✅ Particionamiento espacial (Size, STR, R*-Grove)
- ✅ R-tree local por componente
- ✅ Tombstones para borrado
- ✅ Workloads experimentales (Random, Clustered)
- ✅ Métricas de amplificación

## 🛠️ Extensiones Futuras

- [ ] Persistencia completa en disco
- [ ] Compresión de componentes
- [ ] Bloom filters para optimización
- [ ] Soporte para más tipos de consultas espaciales
- [ ] Paralelización de queries
- [ ] Visualización de MBRs y R-trees
- [ ] Integración con datasets reales de OpenStreetMap

## 📝 Licencia

Proyecto académico - UTEC 2025-2

## 👥 Autor

Fernando - Proyecto de Estructuras de Datos Avanzadas