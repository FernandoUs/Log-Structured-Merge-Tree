#pragma once

#include "../sql/QueryExecutor.h"
#include <iostream>
#include <string>
#include <memory>

namespace cli {

using namespace sql;

/**
 * @brief Interfaz de línea de comandos (REPL)
 * Para interactuar con el sistema de base de datos espacial
 */
template<typename T = int>
class CLI {
private:
    CatalogManager catalog;
    std::map<std::string, std::shared_ptr<lsm::LSMTree<T>>> lsmTrees;
    QueryExecutor<T> executor;
    bool running;
    
public:
    CLI() : executor(catalog, lsmTrees), running(false) {}
    
    /**
     * @brief Inicia el REPL
     */
    void start() {
        running = true;
        
        printBanner();
        printHelp();
        
        std::string input;
        
        while (running) {
            std::cout << "\nspatial-db> ";
            std::getline(std::cin, input);
            
            if (input.empty()) continue;
            
            // Comandos especiales
            if (input == "exit" || input == "quit") {
                running = false;
                std::cout << "Goodbye!\n";
                break;
            }
            
            if (input == "help") {
                printHelp();
                continue;
            }
            
            if (input == "metrics") {
                printMetrics();
                continue;
            }
            
            if (input == "tables") {
                printTables();
                continue;
            }
            
            if (input == "clear") {
                clearMetrics();
                continue;
            }
            
            // Ejecutar SQL
            try {
                std::string result = executor.execute(input);
                std::cout << result << "\n";
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << "\n";
            }
        }
    }
    
    /**
     * @brief Ejecuta un comando SQL directamente
     */
    std::string executeCommand(const std::string& sql) {
        try {
            return executor.execute(sql);
        } catch (const std::exception& e) {
            return "Error: " + std::string(e.what());
        }
    }
    
    /**
     * @brief Obtiene referencia al catálogo
     */
    CatalogManager& getCatalog() { return catalog; }
    
    /**
     * @brief Obtiene referencia a LSM trees
     */
    std::map<std::string, std::shared_ptr<lsm::LSMTree<T>>>& getLSMTrees() {
        return lsmTrees;
    }
    
private:
    void printBanner() {
        std::cout << R"(
╔═══════════════════════════════════════════════════════════╗
║       LSM-Tree Spatial Database System                   ║
║       Implementation based on research paper              ║
║       "Comparison of LSM indexing techniques"            ║
╚═══════════════════════════════════════════════════════════╝
)" << "\n";
    }
    
    void printHelp() {
        std::cout << R"(
Available Commands:
  SQL Statements:
    CREATE TABLE name (col1 type1, col2 type2, ...)
    INSERT INTO table VALUES (x, y, data)
    SELECT COUNT(*) FROM table WHERE spatial_intersect(col, x1, y1, x2, y2)
    SELECT * FROM table WHERE spatial_intersect(col, x1, y1, x2, y2)
  
  Special Commands:
    help       - Show this help message
    metrics    - Display performance metrics
    tables     - List all tables
    clear      - Clear metrics
    exit/quit  - Exit the system
  
  Example Usage:
    CREATE TABLE points (id INT, location POINT, value DOUBLE)
    INSERT INTO points VALUES (0.5, 0.5, 100)
    SELECT COUNT(*) FROM points WHERE spatial_intersect(location, 0, 0, 1, 1)
)" << "\n";
    }
    
    void printMetrics() {
        std::cout << "\n=== Performance Metrics ===\n";
        
        for (const auto& [tableName, tree] : lsmTrees) {
            const auto& metrics = tree->getMetrics();
            
            std::cout << "\nTable: " << tableName << "\n";
            std::cout << "  Write Amplification: " << metrics.writeAmplification << "\n";
            std::cout << "  Read Amplification: " << metrics.readAmplification << "\n";
            std::cout << "  Total Writes: " << metrics.totalWrites << "\n";
            std::cout << "  Total Reads: " << metrics.totalReads << "\n";
            std::cout << "  Total Merges: " << metrics.totalMerges << "\n";
            std::cout << "  Avg Query Latency: " << metrics.avgQueryLatency << " ms\n";
            std::cout << "  Component Count: " << tree->getComponentCount() << "\n";
            std::cout << "  Total Records: " << tree->getTotalRecords() << "\n";
        }
    }
    
    void printTables() {
        std::cout << "\n=== Tables ===\n";
        
        for (const auto& [tableName, tree] : lsmTrees) {
            std::cout << "  - " << tableName << " (" << tree->getTotalRecords() << " records)\n";
        }
        
        if (lsmTrees.empty()) {
            std::cout << "  No tables created yet.\n";
        }
    }
    
    void clearMetrics() {
        for (auto& [tableName, tree] : lsmTrees) {
            tree->resetMetrics();
        }
        std::cout << "Metrics cleared.\n";
    }
};

} // namespace cli
