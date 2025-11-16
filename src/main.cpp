#include "cli/CLI.h"
#include "workload/Workload.h"
#include <iostream>
#include <iomanip>

/**
 * @brief Programa principal del sistema LSM-tree espacial
 */
int main(int argc, char* argv[]) {
    try {
        // Crear CLI
        cli::CLI<int> cli;
        
        // Verificar modo de ejecución
        if (argc > 1) {
            std::string mode(argv[1]);
            
            if (mode == "benchmark") {
                // Modo benchmark - ejecutar evaluación completa
                std::cout << "Running benchmark mode...\n";
                
                // Generar datasets
                workload::DatasetGenerator generator;
                
                // Dataset Random (uniforme)
                auto randomDataset = generator.generateRandomDataset<int>(10000);
                std::cout << "Generated " << randomDataset.size() << " random points\n";
                
                // Dataset Clustered (OSM simulado)
                auto clusteredDataset = generator.generateClusteredDataset<int>(10000, 20);
                std::cout << "Generated " << clusteredDataset.size() << " clustered points\n";
                
                // Generar queries con selectividad alta (10^-3) y baja (10^-5)
                std::vector<spatial::MBR> highSelectivityQueries;
                std::vector<spatial::MBR> lowSelectivityQueries;
                
                for (int i = 0; i < 10; ++i) {
                    highSelectivityQueries.push_back(generator.generateQueryBox(1e-3));
                    lowSelectivityQueries.push_back(generator.generateQueryBox(1e-5));
                }
                
                std::cout << "Generated query sets\n";
                
                // Configuraciones a evaluar (9+ configuraciones del paper)
                std::vector<workload::BenchmarkRunner<int>::BenchmarkConfig> configs = {
                    {"Binomial k=4 / Simple", "Binomial", "Simple", "Size", 4},
                    {"Binomial k=10 / Simple", "Binomial", "Simple", "Size", 10},
                    {"Binomial k=4 / Hilbert", "Binomial", "Hilbert", "Size", 4},
                    {"Tiered B=4 / Simple", "Tiered", "Simple", "Size", 4},
                    {"Tiered B=10 / Simple", "Tiered", "Simple", "Size", 10},
                    {"Leveled / STR / Simple", "Leveled", "Simple", "STR", 10},
                    {"Leveled / STR / Hilbert", "Leveled", "Hilbert", "STR", 10},
                    {"Leveled / RStarGrove / Simple", "Leveled", "Simple", "RStarGrove", 10},
                    {"Concurrent / Simple", "Concurrent", "Simple", "Size", 2}
                };
                
                // Ejecutar benchmark comparativo
                workload::BenchmarkRunner<int> runner;
                
                std::cout << "\n=== Testing with Random Dataset ===\n";
                auto randomResults = runner.runComparison(configs, randomDataset, highSelectivityQueries);
                workload::BenchmarkRunner<int>::printResults(randomResults);
                
                std::cout << "\n=== Testing with Clustered Dataset ===\n";
                auto clusteredResults = runner.runComparison(configs, clusteredDataset, lowSelectivityQueries);
                workload::BenchmarkRunner<int>::printResults(clusteredResults);
                
            } else if (mode == "demo") {
                // Modo demo - ejemplo interactivo
                std::cout << "Running demo mode...\n\n";
                
                // Crear tabla de ejemplo
                cli.executeCommand("CREATE TABLE cities (id INT, location POINT, population DOUBLE)");
                
                // Insertar algunos puntos
                std::cout << "\nInserting sample data...\n";
                cli.executeCommand("INSERT INTO cities VALUES (0.1, 0.1, 1000000)");
                cli.executeCommand("INSERT INTO cities VALUES (0.5, 0.5, 500000)");
                cli.executeCommand("INSERT INTO cities VALUES (0.9, 0.9, 2000000)");
                cli.executeCommand("INSERT INTO cities VALUES (0.3, 0.7, 750000)");
                cli.executeCommand("INSERT INTO cities VALUES (0.8, 0.2, 300000)");
                
                // Ejecutar consultas
                std::cout << "\nExecuting spatial queries...\n";
                std::cout << cli.executeCommand("SELECT COUNT(*) FROM cities WHERE spatial_intersect(location, 0, 0, 0.5, 0.5)") << "\n";
                std::cout << cli.executeCommand("SELECT COUNT(*) FROM cities WHERE spatial_intersect(location, 0, 0, 1, 1)") << "\n";
                
                // Mostrar métricas
                std::cout << "\nMetrics:\n";
                auto& trees = cli.getLSMTrees();
                if (trees.find("cities") != trees.end()) {
                    const auto& metrics = trees["cities"]->getMetrics();
                    std::cout << "  Total writes: " << metrics.totalWrites << "\n";
                    std::cout << "  Total reads: " << metrics.totalReads << "\n";
                    std::cout << "  Avg latency: " << metrics.avgQueryLatency << " ms\n";
                }
                
                std::cout << "\nDemo complete. Starting interactive mode...\n";
                cli.start();
                
            } else {
                std::cout << "Unknown mode: " << mode << "\n";
                std::cout << "Usage: " << argv[0] << " [benchmark|demo]\n";
                std::cout << "  benchmark - Run full performance evaluation\n";
                std::cout << "  demo      - Run interactive demo\n";
                std::cout << "  (no args) - Start interactive CLI\n";
                return 1;
            }
        } else {
            // Modo interactivo por defecto
            cli.start();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
