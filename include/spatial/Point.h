#pragma once

#include <vector>
#include <cmath>
#include <stdexcept>

namespace spatial {

/**
 * @brief Punto multidimensional en espacio D-dimensional
 * Soporta dimensiones arbitrarias para flexibilidad espacial
 */
class Point {
private:
    std::vector<double> coords;
    
public:
    // Constructor por defecto (punto en el origen)
    Point() : coords() {}
    
    // Constructor con dimensión específica
    explicit Point(size_t dimensions) : coords(dimensions, 0.0) {}
    
    // Constructor con coordenadas
    Point(const std::vector<double>& coordinates) : coords(coordinates) {}
    
    // Constructor de lista de inicialización
    Point(std::initializer_list<double> coordinates) : coords(coordinates) {}
    
    // Getters
    size_t dimensions() const { return coords.size(); }
    double operator[](size_t index) const {
        if (index >= coords.size()) {
            throw std::out_of_range("Point index out of range");
        }
        return coords[index];
    }
    
    double& operator[](size_t index) {
        if (index >= coords.size()) {
            throw std::out_of_range("Point index out of range");
        }
        return coords[index];
    }
    
    const std::vector<double>& getCoords() const { return coords; }
    
    // Distancia euclidiana
    double distanceTo(const Point& other) const {
        if (dimensions() != other.dimensions()) {
            throw std::invalid_argument("Points must have same dimensions");
        }
        
        double sum = 0.0;
        for (size_t i = 0; i < dimensions(); ++i) {
            double diff = coords[i] - other.coords[i];
            sum += diff * diff;
        }
        return std::sqrt(sum);
    }
    
    // Operadores de comparación
    bool operator==(const Point& other) const {
        if (dimensions() != other.dimensions()) return false;
        for (size_t i = 0; i < dimensions(); ++i) {
            if (std::abs(coords[i] - other.coords[i]) > 1e-9) {
                return false;
            }
        }
        return true;
    }
    
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
};

} // namespace spatial
