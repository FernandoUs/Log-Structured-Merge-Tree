#pragma once

#include "Point.h"
#include "MBR.h"
#include "SpatialComparators.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace spatial {

/**
 * @brief Nodo del R-tree
 * Implementa R*-tree optimizado para bulk-loading
 * Referencia: R*-tree del paper, usado como índice local en componentes LSM
 */
template<typename T>
class RTreeNode {
public:
    MBR mbr;
    std::vector<std::shared_ptr<RTreeNode<T>>> children;
    std::vector<SpatialRecord<T>> records;
    bool isLeaf;
    
    RTreeNode(bool leaf = true) : isLeaf(leaf) {}
    
    void updateMBR() {
        // TODO: Actualizar MBR del nodo
        // - Si es hoja: calcular MBR que contenga todos los records
        // - Si es interno: calcular MBR que contenga todos los children
    }
};

/**
 * @brief R-tree para indexación espacial local
 * Implementa bulk-loading eficiente mediante STR (Sort-Tile-Recursive)
 */
template<typename T>
class RTree {
private:
    std::shared_ptr<RTreeNode<T>> root;
    size_t maxEntriesPerNode;
    size_t minEntriesPerNode;
    size_t dimensions;
    
    /**
     * @brief Bulk-load usando Sort-Tile-Recursive (STR)
     */
    std::shared_ptr<RTreeNode<T>> bulkLoad(std::vector<SpatialRecord<T>>& records, size_t dim = 0) {
        // TODO: Implementar STR bulk-loading
        // 1. Si records <= maxEntriesPerNode, crear hoja directamente
        // 2. Ordenar por dimensión actual (dim % dimensions)
        // 3. Particionar en S slices (S = sqrt(N/M))
        // 4. Recursión con siguiente dimensión
        // 5. Crear nodos internos si necesario
        return std::make_shared<RTreeNode<T>>(true);
    }
    
    /**
     * @brief Búsqueda recursiva por rango
     */
    void rangeSearchRecursive(const std::shared_ptr<RTreeNode<T>>& node, 
                             const MBR& queryBox,
                             std::vector<SpatialRecord<T>>& results) const {
        // TODO: Implementar range search recursivo
        // 1. Verificar si node->mbr intersecta queryBox
        // 2. Si es hoja: revisar cada record, agregar si está en queryBox (y no es tombstone)
        // 3. Si es interno: recursión sobre children que intersectan
    }
    
public:
    RTree(size_t dims = 2, size_t maxEntries = 50, size_t minEntries = 20)
        : root(nullptr), 
          maxEntriesPerNode(maxEntries),
          minEntriesPerNode(minEntries),
          dimensions(dims) {}
    
    /**
     * @brief Construye el R-tree desde un conjunto de registros
     * Usa bulk-loading para eficiencia
     */
    void build(std::vector<SpatialRecord<T>> records) {
        // TODO: Construir R-tree usando bulkLoad()
        root = std::make_shared<RTreeNode<T>>(true);
    }
    
    /**
     * @brief Búsqueda por rango espacial
     */
    std::vector<SpatialRecord<T>> rangeSearch(const MBR& queryBox) const {
        // TODO: Ejecutar rangeSearchRecursive desde root
        return {};
    }
    
    /**
     * @brief Obtiene el MBR total del árbol
     */
    MBR getTotalMBR() const {
        // TODO: Retornar root->mbr si existe, sino MBR vacío
        return MBR(dimensions);
    }
    
    /**
     * @brief Verifica si el árbol está vacío
     */
    bool isEmpty() const {
        // TODO: Verificar si root es nullptr o root->records está vacío
        return true;
    }
    
    /**
     * @brief Cuenta total de registros
     */
    size_t size() const {
        // TODO: Contar recursivamente registros en el árbol
        return 0;
    }
    
private:
    size_t countRecords(const std::shared_ptr<RTreeNode<T>>& node) const {
        // TODO: Contar registros recursivamente
        // Si es hoja: retornar node->records.size()
        // Si es interno: sumar countRecords de todos los children
        return 0;
    }
};

} // namespace spatial
