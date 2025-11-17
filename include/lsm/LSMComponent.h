#pragma once

#include "../spatial/RTree.h"
#include "../spatial/MBR.h"
#include "../spatial/Point.h"
#include "../spatial/SpatialComparators.h"
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <chrono>

namespace lsm {

using namespace spatial;

/**
 * @brief Componente de disco del LSM-tree
 * Contiene un R-tree local y su MBR total para filtrado
 * Referencia: Sorted Run del paper con índice R-tree local
 */
template<typename T>
class LSMComponent {
private:
    RTree<T> rtree;
    MBR totalMBR;
    size_t level;
    uint64_t timestamp;
    std::string filename;
    size_t recordCount;
    
public:
    LSMComponent(size_t lvl = 0, size_t dims = 2) 
        : rtree(dims), totalMBR(dims), level(lvl), 
          timestamp(0), recordCount(0) {
        // Generar nombre único basado en timestamp
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        filename = "component_L" + std::to_string(level) + "_" + std::to_string(timestamp) + ".dat";
    }
    
    /**
     * @brief Construye el componente desde registros ordenados
     */
    void build(std::vector<SpatialRecord<T>> records) {
        // TODO: Construir componente LSM
        // 1. Actualizar recordCount
        // 2. Construir R-tree usando rtree.build()
        // 3. Actualizar totalMBR desde rtree.getTotalMBR()
        recordCount = 0;
    }
    
    /**
     * @brief Búsqueda por rango espacial
     * Primero filtra por MBR, luego busca en R-tree
     */
    std::vector<SpatialRecord<T>> rangeSearch(const MBR& queryBox) const {
        // TODO: Implementar range search con filtrado MBR
        // 1. Verificar totalMBR.intersects(queryBox) - si no, retornar vacío
        // 2. Si intersecta, buscar en rtree.rangeSearch(queryBox)
        return {};
    }
    
    // Getters
    const MBR& getMBR() const { return totalMBR; }
    size_t getLevel() const { return level; }
    uint64_t getTimestamp() const { return timestamp; }
    size_t size() const { return recordCount; }
    const std::string& getFilename() const { return filename; }
    
    /**
     * @brief Serializa el componente a disco
     */
    bool saveToDisk(const std::string& directory = "./data") const {
        // TODO: Implementar serialización
        // 1. Crear archivo binario en directory/filename
        // 2. Escribir metadata (level, timestamp, recordCount)
        // 3. Serializar R-tree y MBR
        return false;
    }
    
    /**
     * @brief Carga el componente desde disco
     */
    bool loadFromDisk(const std::string& filepath) {
        // TODO: Implementar deserialización
        // 1. Abrir archivo binario
        // 2. Leer metadata
        // 3. Deserializar R-tree y MBR
        return false;
    }
};

} // namespace lsm
