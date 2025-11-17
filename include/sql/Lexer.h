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
        // TODO: Avanzar position mientras peek() sea espacio en blanco
    }
    
    std::string readIdentifier() {
        // TODO: Leer caracteres alfanuméricos + '_' para identificadores
        return "";
    }
    
    std::string readNumber() {
        // TODO: Leer dígitos + '.' para números
        return "";
    }
    
    std::string readString() {
        // TODO: Leer desde ' hasta ' para strings
        return "";
    }
    
    TokenType keywordOrIdentifier(const std::string& word) {
        // TODO: Convertir word a uppercase, mapear a TokenType
        // SELECT, INSERT, CREATE, etc. -> keywords
        // Otros -> IDENTIFIER
        return TokenType::IDENTIFIER;
    }
    
public:
    explicit SQLLexer(const std::string& sql) : input(sql), position(0) {}
    
    Token nextToken() {
        // TODO: Implementar tokenización
        // 1. skipWhitespace()
        // 2. Verificar EOF
        // 3. Reconocer símbolos: *, ,, ;, (, )
        // 4. Reconocer strings: '...'
        // 5. Reconocer números
        // 6. Reconocer identificadores/keywords
        return Token(TokenType::INVALID);
    }
    
    std::vector<Token> tokenize() {
        // TODO: Llamar nextToken() hasta END_OF_FILE
        return {};
    }
};

} // namespace sql
