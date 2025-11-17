#pragma once

#include "../spatial/RTree.h"
#include "../spatial/SpatialComparators.h"
#include "LSMComponent.h"
#include <map>
#include <mutex>
#include <memory>
#include <vector>
#include <algorithm>

namespace lsm {

using namespace spatial;

/**
 * @brief MemTable - Componente activo en memoria del LSM-tree
 * Mantiene los datos más recientes antes del flush a disco
 * Referencia: Active Memory Component del paper
 */
template<typename T>
class MemTable {
private:
    std::map<Point, SpatialRecord<T>, SimpleComparator> data;
    size_t maxSize;
    size_t currentSize;
    mutable std::mutex mutex;
    
public:
    explicit MemTable(size_t maxSizeBytes = 64 * 1024 * 1024) // 64MB por defecto
        : maxSize(maxSizeBytes), currentSize(0) {}
    
    /**
     * @brief Inserta un registro en la MemTable
     */
    bool insert(const SpatialRecord<T>& record) {
        // TODO: Implementar inserción
        // 1. Estimar tamaño del record
        // 2. Verificar si cabe (currentSize + recordSize <= maxSize)
        // 3. Si no cabe, retornar false
        // 4. Insertar en map data[record.point] = record
        // 5. Actualizar currentSize
        return false;
    }
    
    /**
     * @brief Marca un registro como borrado (Tombstone)
     */
    bool remove(const Point& point) {
        // TODO: Implementar remoción
        // Crear SpatialRecord con isTombstone = true
        // Insertar en map
        return false;
    }
    
    /**
     * @brief Búsqueda en MemTable
     */
    std::vector<SpatialRecord<T>> rangeSearch(const MBR& queryBox) const {
        // TODO: Implementar búsqueda en MemTable
        // Iterar sobre data, agregar records que queryBox.contains(point) y no son tombstone
        return {};
    }
    
    /**
     * @brief Obtiene todos los registros (para flush)
     */
    std::vector<SpatialRecord<T>> getAllRecords() const {
        // TODO: Copiar todos los records de data a un vector
        return {};
    }
    
    /**
     * @brief Limpia la MemTable después del flush
     */
    void clear() {
        // TODO: Limpiar data y resetear currentSize
    }
    
    // Getters
    bool isFull() const {
        std::lock_guard<std::mutex> lock(mutex);
        return currentSize >= maxSize;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return data.size();
    }
    
    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return data.empty();
    }
};

/**
 * @brief Métricas de rendimiento del LSM-tree
 */
struct LSMMetrics {
    uint64_t writeAmplification;   // Write Amplification (WA)
    uint64_t readAmplification;    // Read Amplification (RA) - componentes escaneados
    uint64_t totalWrites;
    uint64_t totalReads;
    uint64_t totalMerges;
    double avgQueryLatency;        // Latencia promedio de queries (ms)
    
    LSMMetrics() : writeAmplification(0), readAmplification(0),
                   totalWrites(0), totalReads(0), totalMerges(0),
                   avgQueryLatency(0.0) {}
    
    void reset() {
        writeAmplification = 0;
        readAmplification = 0;
        totalWrites = 0;
        totalReads = 0;
        totalMerges = 0;
        avgQueryLatency = 0.0;
    }
};

/**
 * @brief LSM-tree principal con soporte espacial
 * Gestiona MemTable, componentes de disco, flush y merge
 */
template<typename T>
class LSMTree {
private:
    MemTable<T> memTable;
    std::vector<std::shared_ptr<LSMComponent<T>>> diskComponents;
    size_t dimensions;
    mutable std::mutex treeMutex;
    LSMMetrics metrics;
    
    // Parámetros de configuración
    size_t maxComponentsBeforeMerge;
    
public:
    explicit LSMTree(size_t dims = 2, size_t maxComponents = 10)
        : memTable(), dimensions(dims), maxComponentsBeforeMerge(maxComponents) {}
    
    /**
     * @brief Inserta un registro espacial
     */
    bool insert(const Point& point, const T& data) {
        // TODO: Implementar inserción en LSM-tree
        // 1. Crear SpatialRecord
        // 2. Intentar insertar en memTable
        // 3. Si memTable.insert() retorna false (llena), ejecutar flush()
        // 4. Reintentar inserción
        // 5. Actualizar metrics.totalWrites
        return false;
    }
    
    /**
     * @brief Borra un registro (usando Tombstone)
     * Referencia: Antimatter records del paper
     */
    bool remove(const Point& point) {
        // TODO: Llamar memTable.remove(point)
        // Actualizar metrics.totalWrites
        return false;
    }
    
    /**
     * @brief Flush: MemTable → Disco
     * Referencia: Operación Flush del paper
     */
    void flush() {
        // TODO: Implementar flush
        // 1. Verificar si memTable está vacía
        // 2. Obtener records con memTable.getAllRecords()
        // 3. Crear LSMComponent, llamar component->build(records)
        // 4. Agregar component a diskComponents
        // 5. Limpiar memTable con memTable.clear()
        // 6. Actualizar metrics.writeAmplification
        // 7. Verificar si necesita merge (diskComponents.size >= maxComponentsBeforeMerge)
    }
    
    /**
     * @brief Búsqueda espacial por rango
     * Referencia: SPATIALSEARCH (Algoritmo 3) del paper
     */
    std::vector<SpatialRecord<T>> spatialRangeQuery(const MBR& queryBox) {
        // TODO: Implementar SPATIALSEARCH
        // 1. Buscar en memTable.rangeSearch(queryBox)
        // 2. Iterar diskComponents, llamar component->rangeSearch(queryBox)
        // 3. Combinar resultados
        // 4. Eliminar duplicados/tombstones con removeDuplicatesAndTombstones()
        // 5. Actualizar métricas (totalReads, readAmplification, avgQueryLatency)
        return {};
    }
    
    /**
     * @brief Búsqueda de punto exacto
     */
    std::vector<SpatialRecord<T>> pointQuery(const Point& point) {
        // TODO: Crear MBR pequeño alrededor del punto
        // Llamar spatialRangeQuery(queryBox)
        return {};
    }
    
    /**
     * @brief Elimina duplicados y registros tombstone
     */
    void removeDuplicatesAndTombstones(std::vector<SpatialRecord<T>>& results) const {
        // TODO: Usar map para mantener versión única de cada punto
        // Filtrar tombstones
        // Reconstruir vector results sin duplicados ni tombstones
    }
    
    // Getters de métricas
    const LSMMetrics& getMetrics() const { return metrics; }
    void resetMetrics() { metrics.reset(); }
    
    size_t getComponentCount() const {
        std::lock_guard<std::mutex> lock(treeMutex);
        return diskComponents.size();
    }
    
    size_t getTotalRecords() const {
        std::lock_guard<std::mutex> lock(treeMutex);
        size_t total = memTable.size();
        for (const auto& comp : diskComponents) {
            total += comp->size();
        }
        return total;
    }
};

} // namespace lsm
