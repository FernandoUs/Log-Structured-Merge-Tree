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
        // TODO: Implementar verificación de contención
        return false;
    }
    
    /**
     * @brief Verifica si este MBR intersecta con otro
     * Usado para filtrado en búsquedas espaciales
     */
    bool intersects(const MBR& other) const {
        // TODO: Implementar verificación de intersección
        return false;
    }
    
    /**
     * @brief Expande este MBR para incluir un punto
     */
    void expand(const Point& point) {
        // TODO: Implementar expansión para incluir punto
    }
    
    /**
     * @brief Expande este MBR para incluir otro MBR
     */
    void expand(const MBR& other) {
        // TODO: Implementar expansión para incluir otro MBR
    }
    
    /**
     * @brief Calcula el área (volumen en D dimensiones)
     */
    double area() const {
        // TODO: Implementar cálculo de área/volumen
        return 0.0;
    }
    
    /**
     * @brief Calcula el perímetro (margen en D dimensiones)
     */
    double perimeter() const {
        // TODO: Implementar cálculo de perímetro/margen
        return 0.0;
    }
    
    /**
     * @brief Calcula el centro del MBR
     */
    Point center() const {
        // TODO: Implementar cálculo del centro
        return Point();
    }
    
    /**
     * @brief Verifica si el MBR es válido (no vacío)
     */
    bool isValid() const {
        // TODO: Implementar verificación de validez
        return true;
    }
};

} // namespace spatial
