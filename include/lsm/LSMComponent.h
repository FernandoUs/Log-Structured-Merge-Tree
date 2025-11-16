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
        recordCount = records.size();
        
        if (records.empty()) {
            totalMBR = MBR(rtree.getTotalMBR().dimensions());
            return;
        }
        
        // Construir R-tree con bulk-loading
        rtree.build(records);
        
        // Actualizar MBR total
        totalMBR = rtree.getTotalMBR();
    }
    
    /**
     * @brief Búsqueda por rango espacial
     * Primero filtra por MBR, luego busca en R-tree
     */
    std::vector<SpatialRecord<T>> rangeSearch(const MBR& queryBox) const {
        // Filtrado MBR - optimización clave del paper
        if (!totalMBR.intersects(queryBox)) {
            return std::vector<SpatialRecord<T>>();
        }
        
        // Buscar en R-tree local
        return rtree.rangeSearch(queryBox);
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
        std::string fullPath = directory + "/" + filename;
        std::ofstream ofs(fullPath, std::ios::binary);
        if (!ofs) return false;
        
        // Formato simple: metadata + registros
        ofs.write(reinterpret_cast<const char*>(&level), sizeof(level));
        ofs.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
        ofs.write(reinterpret_cast<const char*>(&recordCount), sizeof(recordCount));
        
        // TODO: Serializar R-tree y MBR
        
        ofs.close();
        return true;
    }
    
    /**
     * @brief Carga el componente desde disco
     */
    bool loadFromDisk(const std::string& filepath) {
        std::ifstream ifs(filepath, std::ios::binary);
        if (!ifs) return false;
        
        ifs.read(reinterpret_cast<char*>(&level), sizeof(level));
        ifs.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
        ifs.read(reinterpret_cast<char*>(&recordCount), sizeof(recordCount));
        
        // TODO: Deserializar R-tree y MBR
        
        ifs.close();
        return true;
    }
};

} // namespace lsm
