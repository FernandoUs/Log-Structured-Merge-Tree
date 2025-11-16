# Arquitectura Técnica Detallada

## Visión General del Sistema

```
┌─────────────────────────────────────────────────────────────┐
│                    CLI / REPL Interface                     │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│                    SQL Layer                                │
│  ┌──────────┐  ┌──────────┐  ┌─────────────────────────┐  │
│  │  Lexer   │─▶│  Parser  │─▶│  Query Executor         │  │
│  └──────────┘  └──────────┘  └───────────┬─────────────┘  │
└────────────────────────────────────────────┼────────────────┘
                                             │
┌────────────────────────────────────────────▼────────────────┐
│                    LSM-Tree Layer                           │
│  ┌─────────────┐  ┌────────────────────────────────────┐   │
│  │  MemTable   │  │    Disk Components                 │   │
│  │  (Active)   │  │  ┌──────┐ ┌──────┐ ┌──────┐       │   │
│  │             │─▶│  │ C0   │ │ C1   │ │ C2   │ ...   │   │
│  │  - Map      │  │  └──┬───┘ └──┬───┘ └──┬───┘       │   │
│  │  - Sorted   │  │     │        │        │            │   │
│  └─────────────┘  └─────┼────────┼────────┼────────────┘   │
└───────────────────────────┼────────┼────────┼────────────────┘
                            │        │        │
┌───────────────────────────▼────────▼────────▼────────────────┐
│              Spatial Indexing Layer                          │
│  ┌──────────────────┐  ┌─────────────────────────────────┐  │
│  │  MBR Filtering   │  │      R-tree (Local Index)       │  │
│  │                  │  │  ┌────────────────────────────┐ │  │
│  │  Component 0: ✓  │  │  │   Internal Nodes          │ │  │
│  │  Component 1: ✗  │  │  │  ┌────┐ ┌────┐ ┌────┐    │ │  │
│  │  Component 2: ✓  │  │  │  │ N1 │ │ N2 │ │ N3 │    │ │  │
│  │                  │  │  │  └─┬──┘ └─┬──┘ └─┬──┘    │ │  │
│  └──────────────────┘  │  └────┼──────┼──────┼────────┘ │  │
│                        │  ┌────▼──────▼──────▼────────┐ │  │
│                        │  │   Leaf Nodes              │ │  │
│                        │  │  [Records with Points]    │ │  │
│                        │  └───────────────────────────┘ │  │
│                        └─────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## Componentes Principales

### 1. Spatial Layer (Capa Espacial)

#### Point
- Representa puntos en D-dimensiones
- Operaciones: distancia, comparación, acceso por índice

#### MBR (Minimum Bounding Rectangle)
- Rectángulo envolvente mínimo
- Operaciones clave:
  - `intersects()`: Verifica intersección con otro MBR
  - `contains()`: Verifica si contiene un punto
  - `expand()`: Expande para incluir punto/MBR
  - `area()`: Calcula área (volumen D-dimensional)

#### Comparadores Espaciales

**SimpleComparator (Nearest-X)**
```
Ordenamiento: (x₁, y₁) < (x₂, y₂) ↔ (x₁ < x₂) ∨ (x₁ = x₂ ∧ y₁ < y₂)
```

**HilbertCurveComparator**
```
Mapeo: Point(x, y) → h = HilbertIndex(x, y)
Ordenamiento: p₁ < p₂ ↔ h₁ < h₂
```

Preserva localidad espacial mediante curva de llenado del espacio.

**ZOrderComparator (Morton)**
```
Interleaving: z = interleave(x_bits, y_bits)
```

#### R-tree
- Índice espacial jerárquico
- Bulk-loading usando STR (Sort-Tile-Recursive)
- Búsqueda eficiente por rango: O(log n + k)

### 2. LSM-Tree Layer (Capa LSM)

#### MemTable
- Estructura en memoria (std::map)
- Ordenada por SimpleComparator
- Flush cuando alcanza tamaño máximo

#### LSMComponent (Disk Component)
```
Component {
    R-tree localIndex;      // Índice espacial local
    MBR totalMBR;          // MBR del componente completo
    level;                 // Nivel en arquitectura Leveled
    timestamp;             // Para ordenamiento temporal
}
```

#### Operación de Flush
```
MemTable → Disk Component:
1. Obtener registros ordenados de MemTable
2. Construir R-tree con bulk-loading
3. Calcular MBR total
4. Escribir componente a disco
5. Limpiar MemTable
```

#### SpatialRangeQuery
```python
def spatialRangeQuery(queryBox):
    results = []
    
    # 1. Buscar en MemTable
    results += memTable.rangeSearch(queryBox)
    
    # 2. Filtrado MBR de componentes
    for component in diskComponents:
        if component.totalMBR.intersects(queryBox):  # MBR filtering
            # 3. Búsqueda en R-tree local
            results += component.rtree.rangeSearch(queryBox)
            componentsScanned++  # Read Amplification
    
    # 4. Eliminar duplicados y tombstones
    results = removeDuplicatesAndTombstones(results)
    
    return results
```

### 3. Merge Policies (Políticas de Fusión)

#### Stack-based Architecture

**Binomial (k-way merge)**
```
Componentes: [C0, C1, C2, C3, C4, C5, ...]
Si hay k componentes del mismo nivel → merge a nivel superior

Ejemplo k=4:
[L0, L0, L0, L0] → [L1]
```

**Tiered (Size-based)**
```
Agrupa componentes de tamaño similar
Si grupo tiene B componentes → merge

Ejemplo B=4:
Grupo de tamaño ~1000: [C1, C2, C3, C4] → [Merged]
```

**Concurrent**
```
Merge continuo de los 2 componentes más antiguos
Menor latencia, más merges frecuentes
```

#### Leveled Architecture

```
L0:  [C0] [C1] [C2]           (overlapping)
L1:  [C3] [C4] [C5] [C6]      (non-overlapping)
L2:  [C7] [C8] [C9] [C10] ... (non-overlapping)

Tamaño máximo por nivel: size(L_i) = baseSize × ratio^i

Si L_i excede capacidad:
    1. Seleccionar componentes de L_i
    2. Encontrar componentes solapados en L_{i+1}
    3. Merge todos los seleccionados → nuevos componentes en L_{i+1}
```

### 4. Partitioning Strategies

#### Size Partitioning
```
Input: [sorted records]
Output: [Component1, Component2, ..., ComponentN]

Cada componente tiene maxComponentSize registros
```

#### STR (Sort-Tile-Recursive)
```
def STR(records, dim):
    if len(records) <= maxSize:
        return [createComponent(records)]
    
    S = sqrt(len(records) / maxSize)
    slices = []
    
    # Ordenar por dimensión actual
    records.sort(by=dim)
    
    # Dividir en S slices
    for slice in partition(records, S):
        slices += STR(slice, (dim + 1) % D)
    
    return slices
```

Garantiza componentes espacialmente disjuntos.

#### R*-Grove
```
Fase 1: Sampling
    sample = randomSample(records, ratio=0.1)

Fase 2: Boundary Calculation
    boundaries = STR(sample)
    
Fase 3: Assignment
    for record in records:
        bestBoundary = findMinExpansion(record, boundaries)
        assign(record, bestBoundary)
```

Crea MBRs más cuadrados (square-like).

### 5. SQL Layer

#### Lexer → Parser → AST → Executor

**Ejemplo de flujo:**
```sql
SELECT COUNT(*) FROM cities WHERE spatial_intersect(location, 0, 0, 1, 1)
```

1. **Lexer** tokeniza:
   ```
   [SELECT, COUNT, LPAREN, STAR, RPAREN, FROM, IDENTIFIER(cities), 
    WHERE, SPATIAL_INTERSECT, LPAREN, IDENTIFIER(location), NUMBER(0), ...]
   ```

2. **Parser** crea AST:
   ```
   SelectStmt
   ├── CountExpr
   ├── Identifier(cities)
   └── WhereClause
       └── SpatialIntersectExpr
           ├── Identifier(location)
           └── Coordinates(0, 0, 1, 1)
   ```

3. **Executor** traduce a operaciones LSM:
   ```cpp
   MBR queryBox(Point(0, 0), Point(1, 1));
   results = lsmTree->spatialRangeQuery(queryBox);
   return "COUNT(*): " + results.size();
   ```

### 6. Métricas de Rendimiento

#### Write Amplification (WA)
```
WA = Total bytes written to disk / Total bytes inserted by user

Incremento en cada:
- Flush: +size(MemTable)
- Merge: +size(merged components)
```

#### Read Amplification (RA)
```
RA = Number of components scanned per query

Menor RA → mejor rendimiento de lectura
Mayor RA → más componentes, queries más lentas
```

#### Latencia
```
Latency = Time to execute query (ms)

Factores:
- Número de componentes (RA)
- Tamaño de componentes
- Eficiencia de MBR filtering
- Profundidad de R-trees
```

## Flujos de Datos Completos

### Inserción de Datos

```
User: INSERT INTO table VALUES (0.5, 0.5, 100)
│
├─▶ SQL Parser
│   └─▶ AST: InsertStmt
│
├─▶ Query Executor
│   └─▶ lsmTree.insert(Point(0.5, 0.5), 100)
│
├─▶ LSMTree
│   ├─▶ memTable.insert(record)
│   │   └─▶ if memTable.isFull():
│   │       └─▶ flush()
│   │           ├─▶ Create LSMComponent
│   │           ├─▶ Build R-tree (bulk-load)
│   │           ├─▶ Calculate MBR
│   │           ├─▶ Add to diskComponents
│   │           └─▶ Clear MemTable
│   │
│   └─▶ if needsMerge():
│       └─▶ executeMergePolicy()
│
└─▶ Return: "INSERT successful"
```

### Consulta Espacial

```
User: SELECT COUNT(*) FROM table WHERE spatial_intersect(col, 0, 0, 0.5, 0.5)
│
├─▶ SQL Parser
│   └─▶ AST: SelectStmt with WhereClause
│
├─▶ Query Executor
│   ├─▶ Extract queryBox = MBR(Point(0,0), Point(0.5,0.5))
│   └─▶ results = lsmTree.spatialRangeQuery(queryBox)
│
├─▶ LSMTree.spatialRangeQuery(queryBox)
│   │
│   ├─▶ Search MemTable
│   │   └─▶ results += memTable.rangeSearch(queryBox)
│   │
│   ├─▶ For each diskComponent:
│   │   ├─▶ if component.totalMBR.intersects(queryBox):  ← MBR Filtering
│   │   │   ├─▶ results += component.rtree.rangeSearch(queryBox)
│   │   │   └─▶ RA++
│   │   └─▶ else: skip component  ← Optimization!
│   │
│   └─▶ removeDuplicatesAndTombstones(results)
│
└─▶ Return: "COUNT(*): N"
```

## Optimizaciones Clave

### 1. MBR Filtering
- Evita búsquedas en componentes no relevantes
- Reduce Read Amplification efectivo
- Crítico para rendimiento de queries

### 2. Bulk-loading R-trees
- Construcción eficiente: O(n log n) vs inserción incremental O(n log² n)
- Mejor factor de llenado
- MBRs más compactos

### 3. Hilbert Curve Ordering
- Preserva localidad espacial
- Mejora clustering en componentes
- Reduce área de MBRs

### 4. Particionamiento STR
- Componentes disjuntos en espacio
- Maximiza efectividad de MBR filtering
- Reduce overlapping

## Trade-offs

| Aspecto | Stack-based | Leveled |
|---------|-------------|---------|
| Write Amplification | Bajo | Alto |
| Read Amplification | Alto | Bajo |
| Latencia Escritura | Baja | Media |
| Latencia Lectura | Alta | Baja |
| Espacio | Más (overlapping) | Menos |

| Comparador | Ventajas | Desventajas |
|------------|----------|-------------|
| Simple | Rápido, simple | No preserva localidad espacial |
| Hilbert | Excelente localidad | Más complejo, overhead |
| Z-order | Balance | Localidad inferior a Hilbert |
