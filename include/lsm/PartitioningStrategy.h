#pragma once

#include "../spatial/SpatialComparators.h"
#include "../spatial/MBR.h"
#include "LSMComponent.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

namespace lsm {

using namespace spatial;

/**
 * @brief Estrategia base de particionamiento
 */
template<typename T>
class PartitioningStrategy {
public:
    virtual ~PartitioningStrategy() = default;
    
    /**
     * @brief Particiona registros en múltiples componentes
     */
    virtual std::vector<std::shared_ptr<LSMComponent<T>>> partition(
        const std::vector<SpatialRecord<T>>& records,
        size_t targetLevel,
        size_t dimensions,
        size_t maxComponentSize) const = 0;
};

/**
 * @brief Size Partitioning
 * Particiona registros ordenados en componentes de tamaño similar
 * Referencia: Size Partitioning del paper (evaluado con ambos comparadores)
 */
template<typename T>
class SizePartitioning : public PartitioningStrategy<T> {
private:
    enum ComparatorType { SIMPLE, HILBERT };
    ComparatorType comparatorType;
    
public:
    explicit SizePartitioning(bool useHilbert = false) 
        : comparatorType(useHilbert ? HILBERT : SIMPLE) {}
    
    std::vector<std::shared_ptr<LSMComponent<T>>> partition(
        const std::vector<SpatialRecord<T>>& records,
        size_t targetLevel,
        size_t dimensions,
        size_t maxComponentSize) const override {
        
        if (records.empty()) return {};
        
        // 1. Ordenar registros según comparador seleccionado
        auto sortedRecords = records;
        
        if (comparatorType == SIMPLE) {
            std::sort(sortedRecords.begin(), sortedRecords.end(),
                [](const SpatialRecord<T>& a, const SpatialRecord<T>& b) {
                    SimpleComparator cmp;
                    return cmp(a, b);
                });
        } else {
            // Para Hilbert, necesitamos el MBR de todos los datos
            MBR bounds(dimensions);
            for (const auto& rec : sortedRecords) {
                bounds.expand(rec.point);
            }
            
            std::sort(sortedRecords.begin(), sortedRecords.end(),
                [&bounds](const SpatialRecord<T>& a, const SpatialRecord<T>& b) {
                    HilbertCurveComparator cmp;
                    return cmp(a, b, bounds);
                });
        }
        
        // 2. Particionar en chunks de tamaño maxComponentSize
        std::vector<std::shared_ptr<LSMComponent<T>>> components;
        
        for (size_t i = 0; i < sortedRecords.size(); i += maxComponentSize) {
            size_t end = std::min(i + maxComponentSize, sortedRecords.size());
            
            std::vector<SpatialRecord<T>> partition(
                sortedRecords.begin() + i,
                sortedRecords.begin() + end
            );
            
            auto component = std::make_shared<LSMComponent<T>>(targetLevel, dimensions);
            component->build(partition);
            components.push_back(component);
        }
        
        return components;
    }
};

/**
 * @brief STR (Sort-Tile-Recursive) Partitioning
 * Garantiza componentes espacialmente disjuntos
 * Referencia: STR algorithm del paper
 */
template<typename T>
class STRPartitioning : public PartitioningStrategy<T> {
public:
    std::vector<std::shared_ptr<LSMComponent<T>>> partition(
        const std::vector<SpatialRecord<T>>& records,
        size_t targetLevel,
        size_t dimensions,
        size_t maxComponentSize) const override {
        
        if (records.empty()) return {};
        
        return strPartitionRecursive(records, targetLevel, dimensions, maxComponentSize, 0);
    }
    
private:
    /**
     * @brief Implementación recursiva de STR
     */
    std::vector<std::shared_ptr<LSMComponent<T>>> strPartitionRecursive(
        const std::vector<SpatialRecord<T>>& records,
        size_t targetLevel,
        size_t dimensions,
        size_t maxComponentSize,
        size_t currentDim) const {
        
        if (records.size() <= maxComponentSize) {
            // Caso base: crear un solo componente
            auto component = std::make_shared<LSMComponent<T>>(targetLevel, dimensions);
            component->build(records);
            return {component};
        }
        
        // Calcular número de slices en esta dimensión
        size_t numSlices = static_cast<size_t>(
            std::ceil(std::pow(records.size() / static_cast<double>(maxComponentSize), 
                              1.0 / dimensions))
        );
        
        if (numSlices < 2) numSlices = 2;
        
        // Ordenar por dimensión actual
        auto sortedRecords = records;
        size_t dim = currentDim % dimensions;
        
        std::sort(sortedRecords.begin(), sortedRecords.end(),
            [dim](const SpatialRecord<T>& a, const SpatialRecord<T>& b) {
                return a.point[dim] < b.point[dim];
            });
        
        // Dividir en slices
        std::vector<std::shared_ptr<LSMComponent<T>>> allComponents;
        size_t sliceSize = (sortedRecords.size() + numSlices - 1) / numSlices;
        
        for (size_t i = 0; i < numSlices; ++i) {
            size_t start = i * sliceSize;
            size_t end = std::min(start + sliceSize, sortedRecords.size());
            
            if (start >= sortedRecords.size()) break;
            
            std::vector<SpatialRecord<T>> slice(
                sortedRecords.begin() + start,
                sortedRecords.begin() + end
            );
            
            // Recursión con siguiente dimensión
            auto sliceComponents = strPartitionRecursive(
                slice, targetLevel, dimensions, maxComponentSize, currentDim + 1
            );
            
            allComponents.insert(allComponents.end(), 
                               sliceComponents.begin(), 
                               sliceComponents.end());
        }
        
        return allComponents;
    }
};

/**
 * @brief R*-Grove Partitioning
 * Diseñado para crear MBRs más cuadrados (square-like)
 * Referencia: R*-Grove algorithm del paper (3 fases: Sampling, Boundary, Final)
 */
template<typename T>
class RStarGrovePartitioning : public PartitioningStrategy<T> {
private:
    double sampleRatio;  // Ratio de muestreo (típicamente 0.1)
    
public:
    explicit RStarGrovePartitioning(double sampling = 0.1) 
        : sampleRatio(sampling) {}
    
    std::vector<std::shared_ptr<LSMComponent<T>>> partition(
        const std::vector<SpatialRecord<T>>& records,
        size_t targetLevel,
        size_t dimensions,
        size_t maxComponentSize) const override {
        
        if (records.empty()) return {};
        
        if (records.size() <= maxComponentSize) {
            auto component = std::make_shared<LSMComponent<T>>(targetLevel, dimensions);
            component->build(records);
            return {component};
        }
        
        // Fase 1: Sampling - Seleccionar muestra representativa
        auto sample = selectSample(records);
        
        // Fase 2: Boundary - Crear boundaries usando STR en la muestra
        auto boundaries = computeBoundaries(sample, dimensions, maxComponentSize);
        
        // Fase 3: Final - Asignar todos los registros a componentes
        return assignToComponents(records, boundaries, targetLevel, dimensions);
    }
    
private:
    /**
     * @brief Fase 1: Seleccionar muestra aleatoria
     */
    std::vector<SpatialRecord<T>> selectSample(
        const std::vector<SpatialRecord<T>>& records) const {
        
        size_t sampleSize = static_cast<size_t>(records.size() * sampleRatio);
        if (sampleSize < 100) sampleSize = std::min(size_t(100), records.size());
        
        std::vector<SpatialRecord<T>> sample;
        sample.reserve(sampleSize);
        
        // Muestreo uniforme
        size_t step = records.size() / sampleSize;
        if (step == 0) step = 1;
        
        for (size_t i = 0; i < records.size() && sample.size() < sampleSize; i += step) {
            sample.push_back(records[i]);
        }
        
        return sample;
    }
    
    /**
     * @brief Fase 2: Computar boundaries usando STR en la muestra
     */
    std::vector<MBR> computeBoundaries(
        const std::vector<SpatialRecord<T>>& sample,
        size_t dimensions,
        size_t maxComponentSize) const {
        
        std::vector<MBR> boundaries;
        
        if (sample.empty()) return boundaries;
        
        // Aplicar STR a la muestra para obtener MBRs representativos
        size_t numPartitions = (sample.size() + maxComponentSize - 1) / maxComponentSize;
        
        // Para 2D, usar grid approach
        if (dimensions == 2) {
            size_t gridSize = static_cast<size_t>(std::ceil(std::sqrt(numPartitions)));
            
            // Calcular bounds totales
            MBR totalBounds(dimensions);
            for (const auto& rec : sample) {
                totalBounds.expand(rec.point);
            }
            
            // Crear grid de MBRs
            double xStep = (totalBounds.getUpper()[0] - totalBounds.getLower()[0]) / gridSize;
            double yStep = (totalBounds.getUpper()[1] - totalBounds.getLower()[1]) / gridSize;
            
            for (size_t i = 0; i < gridSize; ++i) {
                for (size_t j = 0; j < gridSize; ++j) {
                    Point lower(2);
                    Point upper(2);
                    
                    lower[0] = totalBounds.getLower()[0] + i * xStep;
                    lower[1] = totalBounds.getLower()[1] + j * yStep;
                    upper[0] = totalBounds.getLower()[0] + (i + 1) * xStep;
                    upper[1] = totalBounds.getLower()[1] + (j + 1) * yStep;
                    
                    boundaries.emplace_back(lower, upper);
                }
            }
        } else {
            // Para D > 2, usar particionamiento por dimensión principal
            std::sort(const_cast<std::vector<SpatialRecord<T>>&>(sample).begin(), 
                     const_cast<std::vector<SpatialRecord<T>>&>(sample).end(),
                [](const SpatialRecord<T>& a, const SpatialRecord<T>& b) {
                    SimpleComparator cmp;
                    return cmp(a, b);
                });
            
            size_t partSize = (sample.size() + numPartitions - 1) / numPartitions;
            
            for (size_t i = 0; i < sample.size(); i += partSize) {
                size_t end = std::min(i + partSize, sample.size());
                
                MBR mbr(dimensions);
                for (size_t j = i; j < end; ++j) {
                    mbr.expand(sample[j].point);
                }
                boundaries.push_back(mbr);
            }
        }
        
        return boundaries;
    }
    
    /**
     * @brief Fase 3: Asignar registros a componentes basados en boundaries
     */
    std::vector<std::shared_ptr<LSMComponent<T>>> assignToComponents(
        const std::vector<SpatialRecord<T>>& records,
        const std::vector<MBR>& boundaries,
        size_t targetLevel,
        size_t dimensions) const {
        
        // Crear bins para cada boundary
        std::vector<std::vector<SpatialRecord<T>>> bins(boundaries.size());
        
        for (const auto& record : records) {
            // Encontrar mejor boundary (mínima expansión)
            size_t bestIdx = 0;
            double minExpansion = std::numeric_limits<double>::max();
            
            for (size_t i = 0; i < boundaries.size(); ++i) {
                MBR expanded = boundaries[i];
                double originalArea = expanded.area();
                expanded.expand(record.point);
                double newArea = expanded.area();
                double expansion = newArea - originalArea;
                
                if (expansion < minExpansion) {
                    minExpansion = expansion;
                    bestIdx = i;
                }
            }
            
            bins[bestIdx].push_back(record);
        }
        
        // Crear componentes de bins no vacíos
        std::vector<std::shared_ptr<LSMComponent<T>>> components;
        
        for (auto& bin : bins) {
            if (!bin.empty()) {
                auto component = std::make_shared<LSMComponent<T>>(targetLevel, dimensions);
                component->build(bin);
                components.push_back(component);
            }
        }
        
        return components;
    }
};

} // namespace lsm
