#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace sql {

/**
 * @brief Tipos de tokens SQL
 */
enum class TokenType {
    // Keywords
    SELECT, INSERT, INTO, CREATE, TABLE, WHERE, FROM, VALUES, COUNT,
    
    // Operadores
    STAR, COMMA, SEMICOLON, LPAREN, RPAREN,
    
    // Tipos de datos
    INT, DOUBLE, VARCHAR, POINT, GEOMETRY,
    
    // Funciones espaciales
    SPATIAL_INTERSECT,
    
    // Literales e identificadores
    IDENTIFIER, NUMBER, STRING,
    
    // Control
    END_OF_FILE, INVALID
};

/**
 * @brief Token SQL
 */
struct Token {
    TokenType type;
    std::string value;
    
    Token(TokenType t, const std::string& v = "") : type(t), value(v) {}
};

/**
 * @brief Lexer SQL simple
 */
class SQLLexer {
private:
    std::string input;
    size_t position;
    
    char peek() const {
        if (position >= input.length()) return '\0';
        return input[position];
    }
    
    char advance() {
        if (position >= input.length()) return '\0';
        return input[position++];
    }
    
    void skipWhitespace() {
        while (position < input.length() && std::isspace(peek())) {
            advance();
        }
    }
    
    std::string readIdentifier() {
        std::string result;
        while (position < input.length() && 
               (std::isalnum(peek()) || peek() == '_')) {
            result += advance();
        }
        return result;
    }
    
    std::string readNumber() {
        std::string result;
        while (position < input.length() && 
               (std::isdigit(peek()) || peek() == '.')) {
            result += advance();
        }
        return result;
    }
    
    std::string readString() {
        std::string result;
        advance(); // Skip opening quote
        while (position < input.length() && peek() != '\'') {
            result += advance();
        }
        advance(); // Skip closing quote
        return result;
    }
    
    TokenType keywordOrIdentifier(const std::string& word) {
        std::string upper = word;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        
        if (upper == "SELECT") return TokenType::SELECT;
        if (upper == "INSERT") return TokenType::INSERT;
        if (upper == "INTO") return TokenType::INTO;
        if (upper == "CREATE") return TokenType::CREATE;
        if (upper == "TABLE") return TokenType::TABLE;
        if (upper == "WHERE") return TokenType::WHERE;
        if (upper == "FROM") return TokenType::FROM;
        if (upper == "VALUES") return TokenType::VALUES;
        if (upper == "COUNT") return TokenType::COUNT;
        if (upper == "INT") return TokenType::INT;
        if (upper == "DOUBLE") return TokenType::DOUBLE;
        if (upper == "VARCHAR") return TokenType::VARCHAR;
        if (upper == "POINT") return TokenType::POINT;
        if (upper == "GEOMETRY") return TokenType::GEOMETRY;
        if (upper == "SPATIAL_INTERSECT") return TokenType::SPATIAL_INTERSECT;
        
        return TokenType::IDENTIFIER;
    }
    
public:
    explicit SQLLexer(const std::string& sql) : input(sql), position(0) {}
    
    Token nextToken() {
        skipWhitespace();
        
        if (position >= input.length()) {
            return Token(TokenType::END_OF_FILE);
        }
        
        char current = peek();
        
        // Símbolos especiales
        if (current == '*') {
            advance();
            return Token(TokenType::STAR, "*");
        }
        if (current == ',') {
            advance();
            return Token(TokenType::COMMA, ",");
        }
        if (current == ';') {
            advance();
            return Token(TokenType::SEMICOLON, ";");
        }
        if (current == '(') {
            advance();
            return Token(TokenType::LPAREN, "(");
        }
        if (current == ')') {
            advance();
            return Token(TokenType::RPAREN, ")");
        }
        
        // Strings
        if (current == '\'') {
            std::string str = readString();
            return Token(TokenType::STRING, str);
        }
        
        // Numbers
        if (std::isdigit(current)) {
            std::string num = readNumber();
            return Token(TokenType::NUMBER, num);
        }
        
        // Identifiers and keywords
        if (std::isalpha(current) || current == '_') {
            std::string word = readIdentifier();
            TokenType type = keywordOrIdentifier(word);
            return Token(type, word);
        }
        
        advance();
        return Token(TokenType::INVALID, std::string(1, current));
    }
    
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        Token tok = nextToken();
        while (tok.type != TokenType::END_OF_FILE) {
            tokens.push_back(tok);
            tok = nextToken();
        }
        tokens.push_back(tok); // Add EOF
        return tokens;
    }
};

} // namespace sql
