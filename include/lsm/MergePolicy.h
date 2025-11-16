#pragma once

#include "LSMComponent.h"
#include "../spatial/SpatialComparators.h"
#include <vector>
#include <memory>
#include <queue>
#include <algorithm>

namespace lsm {

using namespace spatial;

/**
 * @brief Política base de merge/compactación
 * Referencia: Merge/Compaction del paper
 */
template<typename T>
class MergePolicy {
public:
    virtual ~MergePolicy() = default;
    
    /**
     * @brief Determina si se debe ejecutar un merge
     */
    virtual bool shouldMerge(const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const = 0;
    
    /**
     * @brief Selecciona componentes para merge
     */
    virtual std::vector<std::shared_ptr<LSMComponent<T>>> selectComponentsToMerge(
        const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const = 0;
    
    /**
     * @brief Ejecuta el merge de componentes
     * Escanea registros, usa cola de prioridad para ordenar,
     * elimina obsoletos/antimateria y escribe nuevos componentes
     */
    std::shared_ptr<LSMComponent<T>> mergeComponents(
        const std::vector<std::shared_ptr<LSMComponent<T>>>& components,
        size_t targetLevel,
        size_t dimensions) const {
        
        if (components.empty()) return nullptr;
        
        // 1. Recolectar todos los registros de componentes seleccionados
        std::vector<SpatialRecord<T>> allRecords;
        for (const auto& comp : components) {
            MBR fullMBR = comp->getMBR();
            auto records = comp->rangeSearch(fullMBR);
            allRecords.insert(allRecords.end(), records.begin(), records.end());
        }
        
        // 2. Ordenar registros usando comparador simple
        std::sort(allRecords.begin(), allRecords.end(), 
            [](const SpatialRecord<T>& a, const SpatialRecord<T>& b) {
                SimpleComparator cmp;
                return cmp(a, b);
            });
        
        // 3. Eliminar duplicados y tombstones
        std::vector<SpatialRecord<T>> mergedRecords;
        mergedRecords.reserve(allRecords.size());
        
        for (size_t i = 0; i < allRecords.size(); ++i) {
            // Saltar si es el mismo punto que el siguiente (duplicado)
            if (i + 1 < allRecords.size() && 
                allRecords[i].point == allRecords[i + 1].point) {
                continue;  // Mantener solo el más reciente
            }
            
            // No incluir tombstones
            if (!allRecords[i].isTombstone) {
                mergedRecords.push_back(allRecords[i]);
            }
        }
        
        // 4. Crear nuevo componente
        auto newComponent = std::make_shared<LSMComponent<T>>(targetLevel, dimensions);
        newComponent->build(mergedRecords);
        
        return newComponent;
    }
};

/**
 * @brief Política Binomial
 * Stack-based con ratio k (típicamente 4 o 10)
 * Referencia: Binomial policy del paper
 */
template<typename T>
class BinomialMergePolicy : public MergePolicy<T> {
private:
    size_t k;  // Ratio de merge (4 o 10 típicamente)
    
public:
    explicit BinomialMergePolicy(size_t ratio = 4) : k(ratio) {}
    
    bool shouldMerge(const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        if (components.size() < k) return false;
        
        // Verificar si hay k componentes del mismo nivel
        std::map<size_t, size_t> levelCounts;
        for (const auto& comp : components) {
            levelCounts[comp->getLevel()]++;
        }
        
        for (const auto& [level, count] : levelCounts) {
            if (count >= k) return true;
        }
        
        return false;
    }
    
    std::vector<std::shared_ptr<LSMComponent<T>>> selectComponentsToMerge(
        const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        
        // Agrupar por nivel
        std::map<size_t, std::vector<std::shared_ptr<LSMComponent<T>>>> levelGroups;
        for (const auto& comp : components) {
            levelGroups[comp->getLevel()].push_back(comp);
        }
        
        // Seleccionar primer nivel con k componentes
        for (auto& [level, comps] : levelGroups) {
            if (comps.size() >= k) {
                // Ordenar por timestamp (más antiguos primero)
                std::sort(comps.begin(), comps.end(),
                    [](const auto& a, const auto& b) {
                        return a->getTimestamp() < b->getTimestamp();
                    });
                
                // Retornar primeros k componentes
                return std::vector<std::shared_ptr<LSMComponent<T>>>(
                    comps.begin(), comps.begin() + k);
            }
        }
        
        return {};
    }
};

/**
 * @brief Política Tiered (SizeTiered)
 * Stack-based con agrupación por tamaño
 * Referencia: Tiered policy del paper
 */
template<typename T>
class TieredMergePolicy : public MergePolicy<T> {
private:
    size_t B;  // Factor de branching (4 o 10 típicamente)
    
public:
    explicit TieredMergePolicy(size_t branchingFactor = 4) : B(branchingFactor) {}
    
    bool shouldMerge(const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        if (components.size() < B) return false;
        
        // Agrupar por tamaño similar
        std::vector<std::vector<std::shared_ptr<LSMComponent<T>>>> sizeGroups;
        
        for (const auto& comp : components) {
            bool grouped = false;
            for (auto& group : sizeGroups) {
                if (!group.empty()) {
                    size_t groupSize = group[0]->size();
                    size_t compSize = comp->size();
                    
                    // Considerar mismo grupo si tamaños están dentro del factor B
                    if (compSize >= groupSize / B && compSize <= groupSize * B) {
                        group.push_back(comp);
                        grouped = true;
                        break;
                    }
                }
            }
            
            if (!grouped) {
                sizeGroups.push_back({comp});
            }
        }
        
        // Verificar si algún grupo tiene B componentes
        for (const auto& group : sizeGroups) {
            if (group.size() >= B) return true;
        }
        
        return false;
    }
    
    std::vector<std::shared_ptr<LSMComponent<T>>> selectComponentsToMerge(
        const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        
        // Similar a shouldMerge, pero retorna el primer grupo con B componentes
        std::vector<std::vector<std::shared_ptr<LSMComponent<T>>>> sizeGroups;
        
        for (const auto& comp : components) {
            bool grouped = false;
            for (auto& group : sizeGroups) {
                if (!group.empty()) {
                    size_t groupSize = group[0]->size();
                    size_t compSize = comp->size();
                    
                    if (compSize >= groupSize / B && compSize <= groupSize * B) {
                        group.push_back(comp);
                        grouped = true;
                        break;
                    }
                }
            }
            
            if (!grouped) {
                sizeGroups.push_back({comp});
            }
        }
        
        for (const auto& group : sizeGroups) {
            if (group.size() >= B) {
                return std::vector<std::shared_ptr<LSMComponent<T>>>(
                    group.begin(), group.begin() + B);
            }
        }
        
        return {};
    }
};

/**
 * @brief Política Concurrent (Default Stack-based)
 * Merge continuo de componentes adyacentes
 * Referencia: Concurrent policy del paper
 */
template<typename T>
class ConcurrentMergePolicy : public MergePolicy<T> {
private:
    size_t minComponents;
    
public:
    explicit ConcurrentMergePolicy(size_t minComps = 2) : minComponents(minComps) {}
    
    bool shouldMerge(const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        return components.size() >= minComponents;
    }
    
    std::vector<std::shared_ptr<LSMComponent<T>>> selectComponentsToMerge(
        const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        
        if (components.size() < minComponents) return {};
        
        // Ordenar por timestamp (más antiguos primero)
        auto sorted = components;
        std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) {
                return a->getTimestamp() < b->getTimestamp();
            });
        
        // Seleccionar los dos más antiguos
        return {sorted[0], sorted[1]};
    }
};

/**
 * @brief Política Leveled
 * Arquitectura de niveles con merge selectivo
 * Referencia: Leveled Architecture del paper
 */
template<typename T>
class LeveledMergePolicy : public MergePolicy<T> {
private:
    size_t sizeRatio;  // Ratio de crecimiento entre niveles (típicamente 10)
    size_t baseSize;   // Tamaño base del nivel 0
    
public:
    explicit LeveledMergePolicy(size_t ratio = 10, size_t base = 1000)
        : sizeRatio(ratio), baseSize(base) {}
    
    /**
     * @brief Calcula el tamaño máximo permitido para un nivel
     */
    size_t getMaxSizeForLevel(size_t level) const {
        size_t maxSize = baseSize;
        for (size_t i = 0; i < level; ++i) {
            maxSize *= sizeRatio;
        }
        return maxSize;
    }
    
    bool shouldMerge(const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        // Agrupar por nivel
        std::map<size_t, size_t> levelSizes;
        for (const auto& comp : components) {
            levelSizes[comp->getLevel()] += comp->size();
        }
        
        // Verificar si algún nivel excede su capacidad
        for (const auto& [level, size] : levelSizes) {
            if (size > getMaxSizeForLevel(level)) {
                return true;
            }
        }
        
        return false;
    }
    
    std::vector<std::shared_ptr<LSMComponent<T>>> selectComponentsToMerge(
        const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        
        // Agrupar por nivel
        std::map<size_t, std::vector<std::shared_ptr<LSMComponent<T>>>> levelGroups;
        for (const auto& comp : components) {
            levelGroups[comp->getLevel()].push_back(comp);
        }
        
        // Encontrar primer nivel que exceda capacidad
        for (auto& [level, comps] : levelGroups) {
            size_t totalSize = 0;
            for (const auto& comp : comps) {
                totalSize += comp->size();
            }
            
            if (totalSize > getMaxSizeForLevel(level)) {
                // Seleccionar todos los componentes del nivel actual
                // y componentes solapados del siguiente nivel
                std::vector<std::shared_ptr<LSMComponent<T>>> selected = comps;
                
                // Calcular MBR combinado del nivel actual
                if (!comps.empty() && !comps[0]->getMBR().isValid()) {
                    continue;
                }
                
                MBR combinedMBR = comps[0]->getMBR();
                for (size_t i = 1; i < comps.size(); ++i) {
                    combinedMBR.expand(comps[i]->getMBR());
                }
                
                // Agregar componentes solapados del siguiente nivel
                auto nextLevel = levelGroups.find(level + 1);
                if (nextLevel != levelGroups.end()) {
                    for (const auto& comp : nextLevel->second) {
                        if (comp->getMBR().intersects(combinedMBR)) {
                            selected.push_back(comp);
                        }
                    }
                }
                
                return selected;
            }
        }
        
        return {};
    }
};

} // namespace lsm
