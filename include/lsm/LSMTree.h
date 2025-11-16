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
        std::lock_guard<std::mutex> lock(mutex);
        
        // Estimar tamaño (simplificado)
        size_t recordSize = sizeof(record) + record.point.dimensions() * sizeof(double);
        
        if (currentSize + recordSize > maxSize) {
            return false;  // MemTable llena, necesita flush
        }
        
        data[record.point] = record;
        currentSize += recordSize;
        return true;
    }
    
    /**
     * @brief Marca un registro como borrado (Tombstone)
     */
    bool remove(const Point& point) {
        std::lock_guard<std::mutex> lock(mutex);
        
        SpatialRecord<T> tombstone(point, T(), true);
        data[point] = tombstone;
        currentSize += sizeof(tombstone);
        
        return true;
    }
    
    /**
     * @brief Búsqueda en MemTable
     */
    std::vector<SpatialRecord<T>> rangeSearch(const MBR& queryBox) const {
        std::lock_guard<std::mutex> lock(mutex);
        
        std::vector<SpatialRecord<T>> results;
        for (const auto& [point, record] : data) {
            if (!record.isTombstone && queryBox.contains(point)) {
                results.push_back(record);
            }
        }
        return results;
    }
    
    /**
     * @brief Obtiene todos los registros (para flush)
     */
    std::vector<SpatialRecord<T>> getAllRecords() const {
        std::lock_guard<std::mutex> lock(mutex);
        
        std::vector<SpatialRecord<T>> records;
        records.reserve(data.size());
        for (const auto& [point, record] : data) {
            records.push_back(record);
        }
        return records;
    }
    
    /**
     * @brief Limpia la MemTable después del flush
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        data.clear();
        currentSize = 0;
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
        SpatialRecord<T> record(point, data, false);
        
        if (!memTable.insert(record)) {
            // MemTable llena - ejecutar flush
            flush();
            
            // Reintentar inserción
            if (!memTable.insert(record)) {
                return false;
            }
        }
        
        metrics.totalWrites++;
        return true;
    }
    
    /**
     * @brief Borra un registro (usando Tombstone)
     * Referencia: Antimatter records del paper
     */
    bool remove(const Point& point) {
        memTable.remove(point);
        metrics.totalWrites++;
        return true;
    }
    
    /**
     * @brief Flush: MemTable → Disco
     * Referencia: Operación Flush del paper
     */
    void flush() {
        if (memTable.isEmpty()) return;
        
        std::lock_guard<std::mutex> lock(treeMutex);
        
        // Obtener registros de MemTable
        auto records = memTable.getAllRecords();
        
        // Crear nuevo componente de disco
        auto component = std::make_shared<LSMComponent<T>>(0, dimensions);
        component->build(records);
        
        // Agregar a componentes de disco
        diskComponents.push_back(component);
        
        // Limpiar MemTable
        memTable.clear();
        
        // Incrementar write amplification
        metrics.writeAmplification += records.size();
        
        // Verificar si necesitamos merge
        if (diskComponents.size() >= maxComponentsBeforeMerge) {
            // TODO: Trigger merge policy
        }
    }
    
    /**
     * @brief Búsqueda espacial por rango
     * Referencia: SPATIALSEARCH (Algoritmo 3) del paper
     */
    std::vector<SpatialRecord<T>> spatialRangeQuery(const MBR& queryBox) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::vector<SpatialRecord<T>> results;
        size_t componentsScanned = 0;
        
        // 1. Buscar en MemTable
        auto memResults = memTable.rangeSearch(queryBox);
        results.insert(results.end(), memResults.begin(), memResults.end());
        
        // 2. Buscar en componentes de disco (con filtrado MBR)
        std::lock_guard<std::mutex> lock(treeMutex);
        for (const auto& component : diskComponents) {
            auto compResults = component->rangeSearch(queryBox);
            results.insert(results.end(), compResults.begin(), compResults.end());
            componentsScanned++;
        }
        
        // 3. Eliminar duplicados y tombstones
        removeDuplicatesAndTombstones(results);
        
        // Actualizar métricas
        metrics.totalReads++;
        metrics.readAmplification += componentsScanned;
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        metrics.avgQueryLatency = (metrics.avgQueryLatency * (metrics.totalReads - 1) + duration.count()) / metrics.totalReads;
        
        return results;
    }
    
    /**
     * @brief Búsqueda de punto exacto
     */
    std::vector<SpatialRecord<T>> pointQuery(const Point& point) {
        // Crear MBR pequeño alrededor del punto
        MBR queryBox(point, point);
        return spatialRangeQuery(queryBox);
    }
    
    /**
     * @brief Elimina duplicados y registros tombstone
     */
    void removeDuplicatesAndTombstones(std::vector<SpatialRecord<T>>& results) const {
        std::map<Point, SpatialRecord<T>, SimpleComparator> uniqueRecords;
        
        // Mantener versión más reciente de cada punto
        for (const auto& record : results) {
            auto it = uniqueRecords.find(record.point);
            if (it == uniqueRecords.end()) {
                uniqueRecords[record.point] = record;
            }
            // Si ya existe, mantener el más reciente (asumimos orden de búsqueda)
        }
        
        // Reconstruir resultados sin tombstones
        results.clear();
        for (const auto& [point, record] : uniqueRecords) {
            if (!record.isTombstone) {
                results.push_back(record);
            }
        }
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
