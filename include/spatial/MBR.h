#pragma once

#include "Point.h"
#include <algorithm>
#include <limits>

namespace spatial {

/**
 * @brief Minimum Bounding Rectangle (MBR) - Rectángulo envolvente mínimo
 * Usado para filtrado espacial eficiente en LSM-tree
 * Referencia: Paper sections sobre MBR filtering
 */
class MBR {
private:
    Point lower;  // Esquina inferior (min coords)
    Point upper;  // Esquina superior (max coords)
    
public:
    // Constructor por defecto (MBR vacío/inválido)
    MBR() : lower(), upper() {}
    
    // Constructor con dimensión específica
    explicit MBR(size_t dimensions) 
        : lower(dimensions), upper(dimensions) {
        for (size_t i = 0; i < dimensions; ++i) {
            lower[i] = std::numeric_limits<double>::max();
            upper[i] = std::numeric_limits<double>::lowest();
        }
    }
    
    // Constructor con puntos lower y upper
    MBR(const Point& lowerBound, const Point& upperBound) 
        : lower(lowerBound), upper(upperBound) {
        if (lower.dimensions() != upper.dimensions()) {
            throw std::invalid_argument("MBR bounds must have same dimensions");
        }
    }
    
    // Getters
    const Point& getLower() const { return lower; }
    const Point& getUpper() const { return upper; }
    size_t dimensions() const { return lower.dimensions(); }
    
    // Setters
    void setLower(const Point& p) { lower = p; }
    void setUpper(const Point& p) { upper = p; }
    
    /**
     * @brief Verifica si el MBR contiene un punto
     */
    bool contains(const Point& point) const {
        if (point.dimensions() != dimensions()) return false;
        
        for (size_t i = 0; i < dimensions(); ++i) {
            if (point[i] < lower[i] || point[i] > upper[i]) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Verifica si este MBR intersecta con otro
     * Usado para filtrado en búsquedas espaciales
     */
    bool intersects(const MBR& other) const {
        if (dimensions() != other.dimensions()) return false;
        
        for (size_t i = 0; i < dimensions(); ++i) {
            if (upper[i] < other.lower[i] || lower[i] > other.upper[i]) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Expande este MBR para incluir un punto
     */
    void expand(const Point& point) {
        if (point.dimensions() != dimensions()) {
            throw std::invalid_argument("Point must have same dimensions as MBR");
        }
        
        for (size_t i = 0; i < dimensions(); ++i) {
            lower[i] = std::min(lower[i], point[i]);
            upper[i] = std::max(upper[i], point[i]);
        }
    }
    
    /**
     * @brief Expande este MBR para incluir otro MBR
     */
    void expand(const MBR& other) {
        if (other.dimensions() != dimensions()) {
            throw std::invalid_argument("MBR must have same dimensions");
        }
        
        for (size_t i = 0; i < dimensions(); ++i) {
            lower[i] = std::min(lower[i], other.lower[i]);
            upper[i] = std::max(upper[i], other.upper[i]);
        }
    }
    
    /**
     * @brief Calcula el área (volumen en D dimensiones)
     */
    double area() const {
        if (dimensions() == 0) return 0.0;
        
        double result = 1.0;
        for (size_t i = 0; i < dimensions(); ++i) {
            result *= (upper[i] - lower[i]);
        }
        return result;
    }
    
    /**
     * @brief Calcula el perímetro (margen en D dimensiones)
     */
    double perimeter() const {
        double sum = 0.0;
        for (size_t i = 0; i < dimensions(); ++i) {
            sum += (upper[i] - lower[i]);
        }
        return sum;
    }
    
    /**
     * @brief Calcula el centro del MBR
     */
    Point center() const {
        Point c(dimensions());
        for (size_t i = 0; i < dimensions(); ++i) {
            c[i] = (lower[i] + upper[i]) / 2.0;
        }
        return c;
    }
    
    /**
     * @brief Verifica si el MBR es válido (no vacío)
     */
    bool isValid() const {
        for (size_t i = 0; i < dimensions(); ++i) {
            if (lower[i] > upper[i]) return false;
        }
        return true;
    }
};

} // namespace spatial
