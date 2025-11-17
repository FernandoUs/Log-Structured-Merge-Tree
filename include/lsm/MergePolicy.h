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
        
        // TODO: Implementar merge de componentes
        // 1. Recolectar registros de todos los componentes
        // 2. Ordenar registros (SimpleComparator)
        // 3. Eliminar duplicados (mantener más reciente)
        // 4. Filtrar tombstones
        // 5. Crear nuevo componente con build(mergedRecords)
        return nullptr;
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
        // TODO: Implementar lógica Binomial
        // Verificar si hay >= k componentes en algún nivel
        // Agrupar por nivel, retornar true si count >= k
        return false;
    }
    
    std::vector<std::shared_ptr<LSMComponent<T>>> selectComponentsToMerge(
        const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        // TODO: Seleccionar k componentes del mismo nivel
        // Agrupar por nivel, ordenar por timestamp, retornar primeros k
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
        // TODO: Implementar lógica Tiered
        // 1. Agrupar componentes por tamaño similar (factor B)
        // 2. Retornar true si algún grupo tiene >= B componentes
        return false;
    }
    
    std::vector<std::shared_ptr<LSMComponent<T>>> selectComponentsToMerge(
        const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        // TODO: Seleccionar primer grupo con B componentes
        // Agrupar por tamaño, retornar primeros B del grupo
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
        // TODO: Implementar lógica Concurrent
        // Permitir merge si components >= minComponents
        return false;
    }
    
    std::vector<std::shared_ptr<LSMComponent<T>>> selectComponentsToMerge(
        const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        // TODO: Seleccionar componentes para merge concurrente
        // Ordenar por timestamp, retornar los 2 más antiguos
        return {};
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
        // TODO: Implementar cálculo de tamaño máximo
        // baseSize * (sizeRatio ^ level)
        return 0;
    }
    
    bool shouldMerge(const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        // TODO: Implementar lógica Leveled
        // Agrupar por nivel, sumar tamaños, verificar si excede getMaxSizeForLevel()
        return false;
    }
    
    std::vector<std::shared_ptr<LSMComponent<T>>> selectComponentsToMerge(
        const std::vector<std::shared_ptr<LSMComponent<T>>>& components) const override {
        // TODO: Seleccionar componentes del nivel que excede
        // Incluir componentes solapados del siguiente nivel (intersección MBR)
        return {};
    }
};

} // namespace lsm
