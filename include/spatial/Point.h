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
    // TODO: Implementar constructores
    Point() : coords() {}
    explicit Point(size_t dimensions) {}
    Point(const std::vector<double>& coordinates) {}
    Point(std::initializer_list<double> coordinates) {}
    
    // TODO: Implementar getters
    size_t dimensions() const { return 0; }
    double operator[](size_t index) const { return 0.0; }
    double& operator[](size_t index) { static double dummy = 0; return dummy; }
    const std::vector<double>& getCoords() const { return coords; }
    
    // TODO: Implementar distancia euclidiana
    double distanceTo(const Point& other) const {
        return 0.0;
    }
    
    // TODO: Implementar operadores de comparación
    bool operator==(const Point& other) const {
        return false;
    }
    
    bool operator!=(const Point& other) const {
        return true;
    }
};

} // namespace spatial
