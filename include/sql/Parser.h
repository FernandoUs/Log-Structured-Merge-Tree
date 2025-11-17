#pragma once

#include "Lexer.h"
#include <memory>
#include <stdexcept>

namespace sql {

/**
 * @brief Tipos de nodos AST
 */
enum class ASTNodeType {
    SELECT_STMT,
    INSERT_STMT,
    CREATE_TABLE_STMT,
    WHERE_CLAUSE,
    SPATIAL_INTERSECT_EXPR,
    COUNT_EXPR,
    COLUMN_LIST,
    VALUE_LIST,
    IDENTIFIER,
    NUMBER,
    STRING
};

/**
 * @brief Nodo base del AST
 */
struct ASTNode {
    ASTNodeType type;
    std::string value;
    std::vector<std::shared_ptr<ASTNode>> children;
    
    explicit ASTNode(ASTNodeType t, const std::string& v = "") 
        : type(t), value(v) {}
    
    void addChild(std::shared_ptr<ASTNode> child) {
        children.push_back(child);
    }
};

/**
 * @brief Parser SQL simple
 * Soporta:
 * - SELECT COUNT(*) FROM table WHERE spatial_intersect(column, box)
 * - INSERT INTO table VALUES (...)
 * - CREATE TABLE table (columns...)
 */
class SQLParser {
private:
    std::vector<Token> tokens;
    size_t position;
    
    const Token& peek() const {
        if (position >= tokens.size()) {
            return tokens.back(); // EOF
        }
        return tokens[position];
    }
    
    const Token& advance() {
        if (position < tokens.size()) {
            return tokens[position++];
        }
        return tokens.back();
    }
    
    bool match(TokenType type) {
        if (peek().type == type) {
            advance();
            return true;
        }
        return false;
    }
    
    void expect(TokenType type, const std::string& msg = "Unexpected token") {
        if (peek().type != type) {
            throw std::runtime_error(msg + ": expected " + std::to_string(static_cast<int>(type)));
        }
        advance();
    }
    
    /**
     * @brief SELECT statement
     * SELECT COUNT(*) FROM table [WHERE condition]
     */
    std::shared_ptr<ASTNode> parseSelect() {
        // TODO: Implementar parser de SELECT
        // 1. expect(SELECT)
        // 2. Parsear COUNT(*) o *
        // 3. expect(FROM), leer tabla
        // 4. Parsear WHERE opcional con parseWhere()
        return std::make_shared<ASTNode>(ASTNodeType::SELECT_STMT);
    }
    
    /**
     * @brief WHERE clause
     * WHERE spatial_intersect(column, box)
     */
    std::shared_ptr<ASTNode> parseWhere() {
        auto node = std::make_shared<ASTNode>(ASTNodeType::WHERE_CLAUSE);
        
        expect(TokenType::WHERE);
        
        // Parse spatial_intersect function
        if (peek().type == TokenType::SPATIAL_INTERSECT) {
            auto spatialNode = parseSpatialIntersect();
            node->addChild(spatialNode);
        }
        
        return node;
    }
    
    /**
     * @brief spatial_intersect(column, x1, y1, x2, y2)
     */
    std::shared_ptr<ASTNode> parseSpatialIntersect() {
        auto node = std::make_shared<ASTNode>(ASTNodeType::SPATIAL_INTERSECT_EXPR);
        
        expect(TokenType::SPATIAL_INTERSECT);
        expect(TokenType::LPAREN);
        
        // Column name
        if (peek().type == TokenType::IDENTIFIER) {
            auto colNode = std::make_shared<ASTNode>(ASTNodeType::IDENTIFIER, peek().value);
            node->addChild(colNode);
            advance();
        }
        
        expect(TokenType::COMMA);
        
        // Bounding box coordinates: x1, y1, x2, y2
        for (int i = 0; i < 4; ++i) {
            if (peek().type == TokenType::NUMBER) {
                auto numNode = std::make_shared<ASTNode>(ASTNodeType::NUMBER, peek().value);
                node->addChild(numNode);
                advance();
            }
            
            if (i < 3) {
                expect(TokenType::COMMA);
            }
        }
        
        expect(TokenType::RPAREN);
        
        return node;
    }
    
    /**
     * @brief INSERT statement
     * INSERT INTO table VALUES (val1, val2, ...)
     */
    std::shared_ptr<ASTNode> parseInsert() {
        auto node = std::make_shared<ASTNode>(ASTNodeType::INSERT_STMT);
        
        expect(TokenType::INSERT);
        expect(TokenType::INTO);
        
        // Table name
        if (peek().type == TokenType::IDENTIFIER) {
            auto tableNode = std::make_shared<ASTNode>(ASTNodeType::IDENTIFIER, peek().value);
            node->addChild(tableNode);
            advance();
        }
        
        expect(TokenType::VALUES);
        expect(TokenType::LPAREN);
        
        // Values list
        auto valuesNode = std::make_shared<ASTNode>(ASTNodeType::VALUE_LIST);
        
        while (peek().type != TokenType::RPAREN && peek().type != TokenType::END_OF_FILE) {
            if (peek().type == TokenType::NUMBER) {
                auto numNode = std::make_shared<ASTNode>(ASTNodeType::NUMBER, peek().value);
                valuesNode->addChild(numNode);
                advance();
            } else if (peek().type == TokenType::STRING) {
                auto strNode = std::make_shared<ASTNode>(ASTNodeType::STRING, peek().value);
                valuesNode->addChild(strNode);
                advance();
            }
            
            if (peek().type == TokenType::COMMA) {
                advance();
            }
        }
        
        expect(TokenType::RPAREN);
        node->addChild(valuesNode);
        
        return node;
    }
    
    /**
     * @brief CREATE TABLE statement
     * CREATE TABLE name (col1 type1, col2 type2, ...)
     */
    std::shared_ptr<ASTNode> parseCreateTable() {
        auto node = std::make_shared<ASTNode>(ASTNodeType::CREATE_TABLE_STMT);
        
        expect(TokenType::CREATE);
        expect(TokenType::TABLE);
        
        // Table name
        if (peek().type == TokenType::IDENTIFIER) {
            auto tableNode = std::make_shared<ASTNode>(ASTNodeType::IDENTIFIER, peek().value);
            node->addChild(tableNode);
            advance();
        }
        
        expect(TokenType::LPAREN);
        
        // Column definitions
        auto colListNode = std::make_shared<ASTNode>(ASTNodeType::COLUMN_LIST);
        
        while (peek().type != TokenType::RPAREN && peek().type != TokenType::END_OF_FILE) {
            // Column name
            if (peek().type == TokenType::IDENTIFIER) {
                std::string colName = peek().value;
                advance();
                
                // Column type
                std::string colType;
                if (peek().type == TokenType::INT || 
                    peek().type == TokenType::DOUBLE ||
                    peek().type == TokenType::VARCHAR ||
                    peek().type == TokenType::POINT ||
                    peek().type == TokenType::GEOMETRY) {
                    colType = peek().value;
                    advance();
                }
                
                auto colNode = std::make_shared<ASTNode>(ASTNodeType::IDENTIFIER, colName + ":" + colType);
                colListNode->addChild(colNode);
            }
            
            if (peek().type == TokenType::COMMA) {
                advance();
            }
        }
        
        expect(TokenType::RPAREN);
        node->addChild(colListNode);
        
        return node;
    }
    
public:
    explicit SQLParser(const std::vector<Token>& toks) : tokens(toks), position(0) {}
    
    /**
     * @brief Parse SQL statement
     */
    std::shared_ptr<ASTNode> parse() {
        if (peek().type == TokenType::SELECT) {
            return parseSelect();
        } else if (peek().type == TokenType::INSERT) {
            return parseInsert();
        } else if (peek().type == TokenType::CREATE) {
            return parseCreateTable();
        }
        
        throw std::runtime_error("Unknown SQL statement");
    }
};

} // namespace sql
