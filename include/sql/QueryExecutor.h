#pragma once

#include "Parser.h"
#include "../lsm/LSMTree.h"
#include "../spatial/Point.h"
#include "../spatial/MBR.h"
#include <map>
#include <string>
#include <memory>
#include <sstream>

namespace sql {

using namespace spatial;
using namespace lsm;

/**
 * @brief Esquema de tabla
 */
struct TableSchema {
    std::string name;
    std::vector<std::string> columns;
    std::vector<std::string> types;
    std::string spatialColumn;  // Nombre de la columna espacial
    
    TableSchema() = default;
    TableSchema(const std::string& n) : name(n) {}
};

/**
 * @brief Catálogo de metadatos
 */
class CatalogManager {
private:
    std::map<std::string, TableSchema> tables;
    
public:
    void createTable(const TableSchema& schema) {
        tables[schema.name] = schema;
    }
    
    bool tableExists(const std::string& name) const {
        return tables.find(name) != tables.end();
    }
    
    const TableSchema& getTable(const std::string& name) const {
        auto it = tables.find(name);
        if (it == tables.end()) {
            throw std::runtime_error("Table not found: " + name);
        }
        return it->second;
    }
    
    TableSchema& getTable(const std::string& name) {
        auto it = tables.find(name);
        if (it == tables.end()) {
            throw std::runtime_error("Table not found: " + name);
        }
        return it->second;
    }
};

/**
 * @brief Motor de ejecución de consultas SQL
 * Traduce AST a operaciones sobre LSM-tree
 */
template<typename T = int>
class QueryExecutor {
private:
    CatalogManager& catalog;
    std::map<std::string, std::shared_ptr<LSMTree<T>>>& lsmTrees;
    
public:
    QueryExecutor(CatalogManager& cat, 
                 std::map<std::string, std::shared_ptr<LSMTree<T>>>& trees)
        : catalog(cat), lsmTrees(trees) {}
    
    /**
     * @brief Ejecuta una consulta SQL
     */
    std::string execute(const std::string& sql) {
        // Lexer
        SQLLexer lexer(sql);
        auto tokens = lexer.tokenize();
        
        // Parser
        SQLParser parser(tokens);
        auto ast = parser.parse();
        
        // Ejecutar según tipo de statement
        if (ast->type == ASTNodeType::SELECT_STMT) {
            return executeSelect(ast);
        } else if (ast->type == ASTNodeType::INSERT_STMT) {
            return executeInsert(ast);
        } else if (ast->type == ASTNodeType::CREATE_TABLE_STMT) {
            return executeCreateTable(ast);
        }
        
        return "Error: Unknown statement type";
    }
    
private:
    /**
     * @brief Ejecuta SELECT COUNT(*) ... WHERE spatial_intersect(...)
     */
    std::string executeSelect(const std::shared_ptr<ASTNode>& ast) {
        // Extraer nombre de tabla
        std::string tableName;
        for (const auto& child : ast->children) {
            if (child->type == ASTNodeType::IDENTIFIER) {
                tableName = child->value;
                break;
            }
        }
        
        if (!catalog.tableExists(tableName)) {
            return "Error: Table '" + tableName + "' does not exist";
        }
        
        // Obtener LSM-tree de la tabla
        auto it = lsmTrees.find(tableName);
        if (it == lsmTrees.end()) {
            return "Error: LSM-tree not found for table '" + tableName + "'";
        }
        
        auto& lsmTree = it->second;
        
        // Buscar cláusula WHERE con spatial_intersect
        MBR queryBox(2);  // Asumimos 2D por defecto
        bool hasWhere = false;
        
        for (const auto& child : ast->children) {
            if (child->type == ASTNodeType::WHERE_CLAUSE) {
                hasWhere = true;
                
                // Extraer parámetros de spatial_intersect
                for (const auto& whereChild : child->children) {
                    if (whereChild->type == ASTNodeType::SPATIAL_INTERSECT_EXPR) {
                        queryBox = extractQueryBox(whereChild);
                    }
                }
            }
        }
        
        // Ejecutar búsqueda espacial
        std::vector<SpatialRecord<T>> results;
        
        if (hasWhere) {
            results = lsmTree->spatialRangeQuery(queryBox);
        } else {
            // Sin WHERE: retornar todos los registros
            // Para esto necesitamos un MBR que cubra todo
            MBR fullBox(2);
            Point lower({-1e9, -1e9});
            Point upper({1e9, 1e9});
            fullBox.setLower(lower);
            fullBox.setUpper(upper);
            results = lsmTree->spatialRangeQuery(fullBox);
        }
        
        // Verificar si es COUNT(*)
        bool isCount = false;
        for (const auto& child : ast->children) {
            if (child->type == ASTNodeType::COUNT_EXPR) {
                isCount = true;
                break;
            }
        }
        
        if (isCount) {
            return "COUNT(*): " + std::to_string(results.size());
        } else {
            // Retornar resultados formateados
            std::stringstream ss;
            ss << "Results (" << results.size() << " rows):\n";
            for (const auto& rec : results) {
                ss << "Point: (";
                for (size_t i = 0; i < rec.point.dimensions(); ++i) {
                    if (i > 0) ss << ", ";
                    ss << rec.point[i];
                }
                ss << ")\n";
            }
            return ss.str();
        }
    }
    
    /**
     * @brief Ejecuta INSERT INTO table VALUES (...)
     */
    std::string executeInsert(const std::shared_ptr<ASTNode>& ast) {
        // Extraer nombre de tabla
        std::string tableName = ast->children[0]->value;
        
        if (!catalog.tableExists(tableName)) {
            return "Error: Table '" + tableName + "' does not exist";
        }
        
        // Obtener o crear LSM-tree
        if (lsmTrees.find(tableName) == lsmTrees.end()) {
            lsmTrees[tableName] = std::make_shared<LSMTree<T>>(2);
        }
        
        auto& lsmTree = lsmTrees[tableName];
        
        // Extraer valores
        auto valuesNode = ast->children[1];
        std::vector<double> coords;
        T data = T();
        
        for (const auto& value : valuesNode->children) {
            if (value->type == ASTNodeType::NUMBER) {
                coords.push_back(std::stod(value->value));
            }
        }
        
        // Asumir que las primeras 2 coordenadas son el punto espacial
        if (coords.size() >= 2) {
            Point point({coords[0], coords[1]});
            
            // El resto son datos (simplificado)
            if (coords.size() > 2) {
                data = static_cast<T>(coords[2]);
            }
            
            lsmTree->insert(point, data);
            return "INSERT successful";
        }
        
        return "Error: Invalid INSERT values";
    }
    
    /**
     * @brief Ejecuta CREATE TABLE
     */
    std::string executeCreateTable(const std::shared_ptr<ASTNode>& ast) {
        TableSchema schema;
        schema.name = ast->children[0]->value;
        
        // Extraer columnas
        auto colListNode = ast->children[1];
        for (const auto& col : colListNode->children) {
            std::string colDef = col->value;
            size_t colonPos = colDef.find(':');
            
            if (colonPos != std::string::npos) {
                std::string colName = colDef.substr(0, colonPos);
                std::string colType = colDef.substr(colonPos + 1);
                
                schema.columns.push_back(colName);
                schema.types.push_back(colType);
                
                // Identificar columna espacial
                if (colType == "POINT" || colType == "GEOMETRY" || colType == "point" || colType == "geometry") {
                    schema.spatialColumn = colName;
                }
            }
        }
        
        catalog.createTable(schema);
        
        // Crear LSM-tree para la tabla
        lsmTrees[schema.name] = std::make_shared<LSMTree<T>>(2);
        
        return "Table '" + schema.name + "' created successfully";
    }
    
    /**
     * @brief Extrae MBR de nodo spatial_intersect
     */
    MBR extractQueryBox(const std::shared_ptr<ASTNode>& node) {
        // spatial_intersect tiene: column, x1, y1, x2, y2
        std::vector<double> coords;
        
        for (size_t i = 1; i < node->children.size(); ++i) {
            if (node->children[i]->type == ASTNodeType::NUMBER) {
                coords.push_back(std::stod(node->children[i]->value));
            }
        }
        
        if (coords.size() >= 4) {
            Point lower({coords[0], coords[1]});
            Point upper({coords[2], coords[3]});
            return MBR(lower, upper);
        }
        
        return MBR(2);
    }
};

} // namespace sql
