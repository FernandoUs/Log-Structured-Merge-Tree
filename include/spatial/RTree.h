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
        if (isLeaf && !records.empty()) {
            mbr = MBR(records[0].point.dimensions());
            for (const auto& rec : records) {
                mbr.expand(rec.point);
            }
        } else if (!isLeaf && !children.empty()) {
            mbr = children[0]->mbr;
            for (size_t i = 1; i < children.size(); ++i) {
                mbr.expand(children[i]->mbr);
            }
        }
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
        if (records.empty()) {
            return std::make_shared<RTreeNode<T>>(true);
        }
        
        // Si caben en un nodo hoja, crear hoja
        if (records.size() <= maxEntriesPerNode) {
            auto leaf = std::make_shared<RTreeNode<T>>(true);
            leaf->records = records;
            leaf->updateMBR();
            return leaf;
        }
        
        // Ordenar por dimensión actual
        std::sort(records.begin(), records.end(), 
            [dim](const SpatialRecord<T>& a, const SpatialRecord<T>& b) {
                return a.point[dim % a.point.dimensions()] < b.point[dim % b.point.dimensions()];
            });
        
        // Calcular número de slices
        size_t S = static_cast<size_t>(std::ceil(std::sqrt(records.size() / maxEntriesPerNode)));
        size_t sliceSize = (records.size() + S - 1) / S;
        
        std::vector<std::shared_ptr<RTreeNode<T>>> childNodes;
        
        for (size_t i = 0; i < records.size(); i += sliceSize) {
            size_t end = std::min(i + sliceSize, records.size());
            std::vector<SpatialRecord<T>> slice(records.begin() + i, records.begin() + end);
            
            // Recursión con siguiente dimensión
            auto child = bulkLoad(slice, dim + 1);
            childNodes.push_back(child);
        }
        
        // Si hay pocos hijos, retornar nodo interno
        if (childNodes.size() <= maxEntriesPerNode) {
            auto node = std::make_shared<RTreeNode<T>>(false);
            node->children = childNodes;
            node->updateMBR();
            return node;
        }
        
        // Recursivamente crear nivel superior
        std::vector<SpatialRecord<T>> parentRecords;
        for (auto& child : childNodes) {
            parentRecords.emplace_back(child->mbr.center(), T(), false);
        }
        
        auto parent = bulkLoad(parentRecords, dim + 1);
        parent->isLeaf = false;
        parent->children = childNodes;
        parent->updateMBR();
        
        return parent;
    }
    
    /**
     * @brief Búsqueda recursiva por rango
     */
    void rangeSearchRecursive(const std::shared_ptr<RTreeNode<T>>& node, 
                             const MBR& queryBox,
                             std::vector<SpatialRecord<T>>& results) const {
        if (!node || !node->mbr.intersects(queryBox)) {
            return;
        }
        
        if (node->isLeaf) {
            for (const auto& rec : node->records) {
                if (!rec.isTombstone && queryBox.contains(rec.point)) {
                    results.push_back(rec);
                }
            }
        } else {
            for (const auto& child : node->children) {
                rangeSearchRecursive(child, queryBox, results);
            }
        }
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
        if (records.empty()) {
            root = std::make_shared<RTreeNode<T>>(true);
            return;
        }
        
        root = bulkLoad(records);
    }
    
    /**
     * @brief Búsqueda por rango espacial
     */
    std::vector<SpatialRecord<T>> rangeSearch(const MBR& queryBox) const {
        std::vector<SpatialRecord<T>> results;
        if (root) {
            rangeSearchRecursive(root, queryBox, results);
        }
        return results;
    }
    
    /**
     * @brief Obtiene el MBR total del árbol
     */
    MBR getTotalMBR() const {
        if (root) {
            return root->mbr;
        }
        return MBR(dimensions);
    }
    
    /**
     * @brief Verifica si el árbol está vacío
     */
    bool isEmpty() const {
        return !root || (root->isLeaf && root->records.empty());
    }
    
    /**
     * @brief Cuenta total de registros
     */
    size_t size() const {
        return countRecords(root);
    }
    
private:
    size_t countRecords(const std::shared_ptr<RTreeNode<T>>& node) const {
        if (!node) return 0;
        if (node->isLeaf) return node->records.size();
        
        size_t count = 0;
        for (const auto& child : node->children) {
            count += countRecords(child);
        }
        return count;
    }
};

} // namespace spatial
