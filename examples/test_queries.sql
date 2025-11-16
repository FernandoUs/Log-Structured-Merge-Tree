-- Script de ejemplo para testing del sistema LSM-tree espacial
-- Ejecutar en el CLI interactivo

-- ========================================
-- Test 1: Operaciones Básicas
-- ========================================

CREATE TABLE test1 (id INT, location POINT, value DOUBLE)

INSERT INTO test1 VALUES (0.1, 0.1, 10)
INSERT INTO test1 VALUES (0.2, 0.2, 20)
INSERT INTO test1 VALUES (0.3, 0.3, 30)

SELECT COUNT(*) FROM test1 WHERE spatial_intersect(location, 0, 0, 0.5, 0.5)
SELECT * FROM test1 WHERE spatial_intersect(location, 0, 0, 1, 1)

-- ========================================
-- Test 2: Carga de Datos Mayor
-- ========================================

CREATE TABLE test2 (id INT, coords POINT, data DOUBLE)

-- Insertar 20 puntos distribuidos
INSERT INTO test2 VALUES (0.05, 0.05, 1)
INSERT INTO test2 VALUES (0.15, 0.15, 2)
INSERT INTO test2 VALUES (0.25, 0.25, 3)
INSERT INTO test2 VALUES (0.35, 0.35, 4)
INSERT INTO test2 VALUES (0.45, 0.45, 5)
INSERT INTO test2 VALUES (0.55, 0.55, 6)
INSERT INTO test2 VALUES (0.65, 0.65, 7)
INSERT INTO test2 VALUES (0.75, 0.75, 8)
INSERT INTO test2 VALUES (0.85, 0.85, 9)
INSERT INTO test2 VALUES (0.95, 0.95, 10)

INSERT INTO test2 VALUES (0.1, 0.9, 11)
INSERT INTO test2 VALUES (0.2, 0.8, 12)
INSERT INTO test2 VALUES (0.3, 0.7, 13)
INSERT INTO test2 VALUES (0.4, 0.6, 14)
INSERT INTO test2 VALUES (0.5, 0.5, 15)
INSERT INTO test2 VALUES (0.6, 0.4, 16)
INSERT INTO test2 VALUES (0.7, 0.3, 17)
INSERT INTO test2 VALUES (0.8, 0.2, 18)
INSERT INTO test2 VALUES (0.9, 0.1, 19)
INSERT INTO test2 VALUES (0.95, 0.05, 20)

-- Consultas con diferentes selectividades
SELECT COUNT(*) FROM test2 WHERE spatial_intersect(coords, 0, 0, 0.3, 0.3)
SELECT COUNT(*) FROM test2 WHERE spatial_intersect(coords, 0, 0, 0.5, 0.5)
SELECT COUNT(*) FROM test2 WHERE spatial_intersect(coords, 0, 0, 1, 1)

-- Ver métricas
metrics

-- ========================================
-- Test 3: Datos Clustered
-- ========================================

CREATE TABLE clusters (id INT, position POINT, cluster_id INT)

-- Cluster 1: Esquina inferior izquierda
INSERT INTO clusters VALUES (0.05, 0.05, 1)
INSERT INTO clusters VALUES (0.08, 0.07, 1)
INSERT INTO clusters VALUES (0.10, 0.10, 1)
INSERT INTO clusters VALUES (0.12, 0.08, 1)
INSERT INTO clusters VALUES (0.07, 0.12, 1)

-- Cluster 2: Centro
INSERT INTO clusters VALUES (0.45, 0.45, 2)
INSERT INTO clusters VALUES (0.50, 0.50, 2)
INSERT INTO clusters VALUES (0.48, 0.52, 2)
INSERT INTO clusters VALUES (0.52, 0.48, 2)
INSERT INTO clusters VALUES (0.50, 0.55, 2)

-- Cluster 3: Esquina superior derecha
INSERT INTO clusters VALUES (0.90, 0.90, 3)
INSERT INTO clusters VALUES (0.92, 0.88, 3)
INSERT INTO clusters VALUES (0.95, 0.95, 3)
INSERT INTO clusters VALUES (0.88, 0.92, 3)
INSERT INTO clusters VALUES (0.93, 0.90, 3)

-- Buscar cada cluster
SELECT COUNT(*) FROM clusters WHERE spatial_intersect(position, 0, 0, 0.15, 0.15)
SELECT COUNT(*) FROM clusters WHERE spatial_intersect(position, 0.4, 0.4, 0.6, 0.6)
SELECT COUNT(*) FROM clusters WHERE spatial_intersect(position, 0.85, 0.85, 1, 1)

-- ========================================
-- Test 4: Performance Metrics
-- ========================================

clear
CREATE TABLE perf_test (id INT, pt POINT, val DOUBLE)

-- Insertar 50 puntos para evaluar métricas
INSERT INTO perf_test VALUES (0.01, 0.01, 1)
INSERT INTO perf_test VALUES (0.02, 0.02, 2)
INSERT INTO perf_test VALUES (0.03, 0.03, 3)
INSERT INTO perf_test VALUES (0.04, 0.04, 4)
INSERT INTO perf_test VALUES (0.05, 0.05, 5)
INSERT INTO perf_test VALUES (0.06, 0.06, 6)
INSERT INTO perf_test VALUES (0.07, 0.07, 7)
INSERT INTO perf_test VALUES (0.08, 0.08, 8)
INSERT INTO perf_test VALUES (0.09, 0.09, 9)
INSERT INTO perf_test VALUES (0.10, 0.10, 10)

-- Ejecutar múltiples queries para medir latencia promedio
SELECT COUNT(*) FROM perf_test WHERE spatial_intersect(pt, 0, 0, 0.1, 0.1)
SELECT COUNT(*) FROM perf_test WHERE spatial_intersect(pt, 0, 0, 0.2, 0.2)
SELECT COUNT(*) FROM perf_test WHERE spatial_intersect(pt, 0, 0, 0.3, 0.3)
SELECT COUNT(*) FROM perf_test WHERE spatial_intersect(pt, 0, 0, 0.5, 0.5)
SELECT COUNT(*) FROM perf_test WHERE spatial_intersect(pt, 0, 0, 1, 1)

-- Ver métricas finales
metrics

-- ========================================
-- Test 5: Multiple Tables
-- ========================================

CREATE TABLE cities (id INT, location POINT, population DOUBLE)
CREATE TABLE roads (id INT, start_point POINT, length DOUBLE)
CREATE TABLE poi (id INT, coords POINT, category INT)

INSERT INTO cities VALUES (0.25, 0.25, 100000)
INSERT INTO cities VALUES (0.75, 0.75, 500000)

INSERT INTO roads VALUES (0.1, 0.1, 50)
INSERT INTO roads VALUES (0.9, 0.9, 100)

INSERT INTO poi VALUES (0.5, 0.5, 1)
INSERT INTO poi VALUES (0.6, 0.6, 2)

-- Listar todas las tablas
tables

-- Consultar cada tabla
SELECT COUNT(*) FROM cities WHERE spatial_intersect(location, 0, 0, 1, 1)
SELECT COUNT(*) FROM roads WHERE spatial_intersect(start_point, 0, 0, 1, 1)
SELECT COUNT(*) FROM poi WHERE spatial_intersect(coords, 0, 0, 1, 1)

-- Métricas completas
metrics
