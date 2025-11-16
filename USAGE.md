# Guía de Uso - LSM-Tree Spatial Database

## Inicio Rápido

### 1. Compilación

```powershell
# Windows con MinGW
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make

# Windows con Visual Studio
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 2. Ejecución Básica

```bash
# Modo interactivo
.\lsm_spatial_db.exe

# Modo demo
.\lsm_spatial_db.exe demo

# Modo benchmark
.\lsm_spatial_db.exe benchmark
```

## Ejemplos de Comandos SQL

### Crear una tabla

```sql
CREATE TABLE locations (id INT, position POINT, value DOUBLE)
```

### Insertar datos

```sql
-- Formato: INSERT INTO table VALUES (x, y, data)
INSERT INTO locations VALUES (0.25, 0.75, 100)
INSERT INTO locations VALUES (0.50, 0.50, 200)
INSERT INTO locations VALUES (0.75, 0.25, 300)
```

### Consultas espaciales

```sql
-- Contar puntos en un área rectangular
SELECT COUNT(*) FROM locations WHERE spatial_intersect(position, 0, 0, 0.5, 0.5)

-- Obtener todos los puntos en un área
SELECT * FROM locations WHERE spatial_intersect(position, 0, 0, 1, 1)
```

## Casos de Uso

### Caso 1: Sistema de Ubicaciones

```sql
-- Crear tabla de ciudades
CREATE TABLE cities (id INT, location POINT, population DOUBLE)

-- Insertar ciudades
INSERT INTO cities VALUES (0.1, 0.1, 1000000)  -- Ciudad A
INSERT INTO cities VALUES (0.5, 0.5, 500000)   -- Ciudad B
INSERT INTO cities VALUES (0.9, 0.9, 2000000)  -- Ciudad C

-- Buscar ciudades en región noroeste
SELECT COUNT(*) FROM cities WHERE spatial_intersect(location, 0, 0, 0.5, 0.5)

-- Ver métricas de rendimiento
metrics
```

### Caso 2: Análisis de Datos Geoespaciales

```sql
-- Crear tabla de puntos de interés
CREATE TABLE poi (id INT, coords POINT, category INT)

-- Cargar múltiples puntos (ejemplo con loop en benchmark)
-- Usar modo benchmark para carga masiva

-- Análisis de densidad en cuadrante
SELECT COUNT(*) FROM poi WHERE spatial_intersect(coords, 0.25, 0.25, 0.75, 0.75)
```

## Comandos Especiales del CLI

### Ver métricas de rendimiento

```
metrics
```

Muestra:
- Write Amplification (WA)
- Read Amplification (RA)
- Total de escrituras y lecturas
- Latencia promedio de queries
- Número de componentes en disco

### Listar tablas

```
tables
```

### Limpiar métricas

```
clear
```

### Ayuda

```
help
```

## Workload de Prueba Manual

### Workload Simple

```sql
-- 1. Crear tabla
CREATE TABLE test (id INT, point POINT, data DOUBLE)

-- 2. Insertar 10 puntos
INSERT INTO test VALUES (0.1, 0.1, 1)
INSERT INTO test VALUES (0.2, 0.2, 2)
INSERT INTO test VALUES (0.3, 0.3, 3)
INSERT INTO test VALUES (0.4, 0.4, 4)
INSERT INTO test VALUES (0.5, 0.5, 5)
INSERT INTO test VALUES (0.6, 0.6, 6)
INSERT INTO test VALUES (0.7, 0.7, 7)
INSERT INTO test VALUES (0.8, 0.8, 8)
INSERT INTO test VALUES (0.9, 0.9, 9)
INSERT INTO test VALUES (0.95, 0.95, 10)

-- 3. Queries con diferentes selectividades
SELECT COUNT(*) FROM test WHERE spatial_intersect(point, 0, 0, 0.3, 0.3)
SELECT COUNT(*) FROM test WHERE spatial_intersect(point, 0, 0, 0.6, 0.6)
SELECT COUNT(*) FROM test WHERE spatial_intersect(point, 0, 0, 1, 1)

-- 4. Ver métricas
metrics
```

## Benchmark Automático

### Ejecutar benchmark completo

```bash
.\lsm_spatial_db.exe benchmark
```

Esto ejecutará:
1. Generación de datasets (Random y Clustered)
2. Pruebas con 9+ configuraciones diferentes
3. Evaluación con selectividad alta (10^-3) y baja (10^-5)
4. Reporte comparativo de métricas

### Interpretación de Resultados

#### Write Amplification (WA)
- **Bajo**: Menos escrituras redundantes, mejor para workloads write-heavy
- **Alto**: Más merges, peor rendimiento de escritura

#### Read Amplification (RA)
- **Bajo**: Menos componentes a escanear, queries más rápidas
- **Alto**: Más componentes, queries más lentas

#### Latencia
- Tiempo promedio de ejecución de queries en milisegundos
- **Menor es mejor**

## Mejores Prácticas

### 1. Inserción Eficiente

- Usar batch inserts cuando sea posible
- Dejar que el sistema haga flush automático
- Monitorear métricas de Write Amplification

### 2. Consultas Espaciales

- Usar query boxes apropiados para la selectividad deseada
- Queries más pequeñas (alta selectividad) son más rápidas
- Monitorear Read Amplification

### 3. Gestión de Métricas

- Usar `clear` antes de benchmarks importantes
- Verificar `metrics` regularmente
- Observar el número de componentes

## Troubleshooting

### Error: "Table not found"
- Verificar que la tabla fue creada con `CREATE TABLE`
- Usar `tables` para listar tablas existentes

### Rendimiento lento
- Verificar número de componentes con `metrics`
- Considerar que muchos componentes aumentan RA
- El sistema automáticamente hace merge según la política

### Error de compilación
- Verificar C++17 o superior
- Asegurar que CMake encuentra el compilador correcto
- Revisar que todos los headers están en `include/`

## Extensiones y Personalización

### Modificar política de merge

Editar en `main.cpp` o crear nueva configuración en benchmark

### Ajustar parámetros

- `maxComponentsBeforeMerge` en `LSMTree` constructor
- `maxEntriesPerNode` en `RTree` constructor
- `maxSize` en `MemTable` constructor

### Agregar nuevas funciones SQL

Extender `Lexer.h`, `Parser.h` y `QueryExecutor.h`
