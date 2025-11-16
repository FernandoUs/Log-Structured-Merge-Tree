#pragma once

#include "../spatial/Point.h"
#include "../lsm/LSMTree.h"
#include <vector>
#include <random>
#include <cmath>

namespace workload {

using namespace spatial;

/**
 * @brief Generador de datasets para evaluación
 * Referencia: OpenStreetMap (clustered) y Random (uniform) del paper
 */
class DatasetGenerator {
private:
    std::mt19937 rng;
    
public:
    DatasetGenerator(unsigned seed = 42) : rng(seed) {}
    
    /**
     * @brief Genera dataset uniforme aleatorio
     * Referencia: Random dataset del paper
     */
    template<typename T>
    std::vector<SpatialRecord<T>> generateRandomDataset(
        size_t count,
        double minX = 0.0, double maxX = 1.0,
        double minY = 0.0, double maxY = 1.0) {
        
        std::uniform_real_distribution<double> distX(minX, maxX);
        std::uniform_real_distribution<double> distY(minY, maxY);
        
        std::vector<SpatialRecord<T>> records;
        records.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            Point p({distX(rng), distY(rng)});
            T data = static_cast<T>(i);
            records.emplace_back(p, data, false);
        }
        
        return records;
    }
    
    /**
     * @brief Genera dataset agrupado (clustered)
     * Simula OpenStreetMap del paper
     */
    template<typename T>
    std::vector<SpatialRecord<T>> generateClusteredDataset(
        size_t count,
        size_t numClusters = 10,
        double clusterRadius = 0.05) {
        
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        std::normal_distribution<double> clusterDist(0.0, clusterRadius);
        
        // Generar centros de clusters
        std::vector<Point> clusterCenters;
        for (size_t i = 0; i < numClusters; ++i) {
            Point center({dist(rng), dist(rng)});
            clusterCenters.push_back(center);
        }
        
        // Generar puntos alrededor de clusters
        std::vector<SpatialRecord<T>> records;
        records.reserve(count);
        
        std::uniform_int_distribution<size_t> clusterChoice(0, numClusters - 1);
        
        for (size_t i = 0; i < count; ++i) {
            // Elegir cluster aleatorio
            size_t clusterIdx = clusterChoice(rng);
            const Point& center = clusterCenters[clusterIdx];
            
            // Generar punto cerca del centro
            double x = center[0] + clusterDist(rng);
            double y = center[1] + clusterDist(rng);
            
            // Clamp to [0, 1]
            x = std::max(0.0, std::min(1.0, x));
            y = std::max(0.0, std::min(1.0, y));
            
            Point p({x, y});
            T data = static_cast<T>(i);
            records.emplace_back(p, data, false);
        }
        
        return records;
    }
    
    /**
     * @brief Genera query box con selectividad específica
     * Referencia: Selectividad 10^-3 y 10^-5 del paper
     */
    MBR generateQueryBox(double selectivity, 
                         double minX = 0.0, double maxX = 1.0,
                         double minY = 0.0, double maxY = 1.0) {
        
        std::uniform_real_distribution<double> distX(minX, maxX);
        std::uniform_real_distribution<double> distY(minY, maxY);
        
        // Calcular área del query box basado en selectividad
        double totalArea = (maxX - minX) * (maxY - minY);
        double queryArea = totalArea * selectivity;
        
        // Asumir query box cuadrado
        double side = std::sqrt(queryArea);
        
        // Posición aleatoria
        double x1 = distX(rng);
        double y1 = distY(rng);
        
        double x2 = std::min(x1 + side, maxX);
        double y2 = std::min(y1 + side, maxY);
        
        Point lower({x1, y1});
        Point upper({x2, y2});
        
        return MBR(lower, upper);
    }
};

/**
 * @brief Ejecutor de workloads de evaluación
 * Referencia: Load → Insert → Read cycle del paper
 */
template<typename T>
class WorkloadExecutor {
private:
    lsm::LSMTree<T>& lsmTree;
    DatasetGenerator generator;
    
public:
    explicit WorkloadExecutor(lsm::LSMTree<T>& tree) : lsmTree(tree) {}
    
    /**
     * @brief Fase de carga inicial (Bulk Load)
     */
    void loadPhase(const std::vector<SpatialRecord<T>>& records) {
        for (const auto& rec : records) {
            lsmTree.insert(rec.point, rec.data);
        }
        
        // Forzar flush
        lsmTree.flush();
    }
    
    /**
     * @brief Fase de inserciones adicionales
     */
    void insertPhase(const std::vector<SpatialRecord<T>>& records) {
        for (const auto& rec : records) {
            lsmTree.insert(rec.point, rec.data);
        }
    }
    
    /**
     * @brief Fase de lectura (range queries)
     */
    std::vector<size_t> readPhase(const std::vector<MBR>& queryBoxes) {
        std::vector<size_t> resultCounts;
        
        for (const auto& box : queryBoxes) {
            auto results = lsmTree.spatialRangeQuery(box);
            resultCounts.push_back(results.size());
        }
        
        return resultCounts;
    }
    
    /**
     * @brief Ejecuta workload completo
     * Load → Insert → Read
     */
    void runWorkload(
        const std::vector<SpatialRecord<T>>& loadData,
        const std::vector<SpatialRecord<T>>& insertData,
        const std::vector<MBR>& queries) {
        
        std::cout << "=== Workload Execution ===\n";
        
        // Load phase
        std::cout << "Loading " << loadData.size() << " records...\n";
        loadPhase(loadData);
        std::cout << "Load complete.\n";
        
        // Insert phase
        std::cout << "Inserting " << insertData.size() << " additional records...\n";
        insertPhase(insertData);
        std::cout << "Inserts complete.\n";
        
        // Read phase
        std::cout << "Executing " << queries.size() << " range queries...\n";
        auto results = readPhase(queries);
        
        std::cout << "Read phase complete.\n";
        std::cout << "Query results:\n";
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "  Query " << i << ": " << results[i] << " results\n";
        }
        
        // Mostrar métricas
        const auto& metrics = lsmTree.getMetrics();
        std::cout << "\n=== Performance Metrics ===\n";
        std::cout << "Write Amplification: " << metrics.writeAmplification << "\n";
        std::cout << "Read Amplification: " << metrics.readAmplification << "\n";
        std::cout << "Avg Query Latency: " << metrics.avgQueryLatency << " ms\n";
    }
};

/**
 * @brief Benchmark comparativo de configuraciones
 */
template<typename T>
class BenchmarkRunner {
public:
    struct BenchmarkConfig {
        std::string name;
        std::string mergePolicy;      // Binomial, Tiered, Concurrent, Leveled
        std::string comparator;       // Simple, Hilbert
        std::string partitioning;     // Size, STR, RStarGrove
        int policyParameter;          // k para Binomial, B para Tiered, etc.
    };
    
    struct BenchmarkResult {
        std::string configName;
        double writeAmplification;
        double readAmplification;
        double avgQueryLatency;
        size_t componentCount;
    };
    
    /**
     * @brief Ejecuta benchmark comparativo
     * Referencia: Comparación de 9+ configuraciones del paper
     */
    std::vector<BenchmarkResult> runComparison(
        const std::vector<BenchmarkConfig>& configs,
        const std::vector<SpatialRecord<T>>& dataset,
        const std::vector<MBR>& queries) {
        
        std::vector<BenchmarkResult> results;
        
        for (const auto& config : configs) {
            std::cout << "\n=== Testing Configuration: " << config.name << " ===\n";
            
            // Crear LSM-tree con configuración
            lsm::LSMTree<T> tree(2);
            
            // Ejecutar workload
            WorkloadExecutor<T> executor(tree);
            
            // Split dataset: 80% load, 20% insert
            size_t loadSize = static_cast<size_t>(dataset.size() * 0.8);
            std::vector<SpatialRecord<T>> loadData(dataset.begin(), dataset.begin() + loadSize);
            std::vector<SpatialRecord<T>> insertData(dataset.begin() + loadSize, dataset.end());
            
            executor.runWorkload(loadData, insertData, queries);
            
            // Recoger métricas
            const auto& metrics = tree.getMetrics();
            
            BenchmarkResult result;
            result.configName = config.name;
            result.writeAmplification = metrics.writeAmplification;
            result.readAmplification = metrics.readAmplification;
            result.avgQueryLatency = metrics.avgQueryLatency;
            result.componentCount = tree.getComponentCount();
            
            results.push_back(result);
        }
        
        return results;
    }
    
    /**
     * @brief Imprime resultados de benchmark en formato tabla
     */
    static void printResults(const std::vector<BenchmarkResult>& results) {
        std::cout << "\n=== Benchmark Results ===\n";
        std::cout << std::string(100, '=') << "\n";
        std::cout << std::left << std::setw(30) << "Configuration"
                  << std::setw(15) << "WA"
                  << std::setw(15) << "RA"
                  << std::setw(20) << "Latency (ms)"
                  << std::setw(15) << "Components" << "\n";
        std::cout << std::string(100, '-') << "\n";
        
        for (const auto& result : results) {
            std::cout << std::left << std::setw(30) << result.configName
                      << std::setw(15) << result.writeAmplification
                      << std::setw(15) << result.readAmplification
                      << std::setw(20) << result.avgQueryLatency
                      << std::setw(15) << result.componentCount << "\n";
        }
        
        std::cout << std::string(100, '=') << "\n";
    }
};

} // namespace workload
