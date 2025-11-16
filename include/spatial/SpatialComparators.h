#pragma once

#include "Point.h"
#include "MBR.h"
#include <functional>
#include <cstdint>

namespace spatial {

/**
 * @brief Registro espacial que contiene un punto y datos asociados
 */
template<typename T>
struct SpatialRecord {
    Point point;
    T data;
    bool isTombstone;  // Para soporte de borrado (antimatter records)
    
    SpatialRecord() : point(), data(), isTombstone(false) {}
    SpatialRecord(const Point& p, const T& d, bool tombstone = false) 
        : point(p), data(d), isTombstone(tombstone) {}
};

/**
 * @brief SimpleComparator - Comparador Nearest-X
 * Ordena puntos por la primera dimensión (luego segunda, etc.)
 * Referencia: SIMPLECOMPARE (Algoritmo 2) del paper
 */
class SimpleComparator {
public:
    template<typename T>
    bool operator()(const SpatialRecord<T>& a, const SpatialRecord<T>& b) const {
        const auto& p1 = a.point;
        const auto& p2 = b.point;
        
        size_t dims = std::min(p1.dimensions(), p2.dimensions());
        for (size_t i = 0; i < dims; ++i) {
            if (p1[i] < p2[i]) return true;
            if (p1[i] > p2[i]) return false;
        }
        return false;  // Son iguales
    }
    
    bool operator()(const Point& p1, const Point& p2) const {
        size_t dims = std::min(p1.dimensions(), p2.dimensions());
        for (size_t i = 0; i < dims; ++i) {
            if (p1[i] < p2[i]) return true;
            if (p1[i] > p2[i]) return false;
        }
        return false;
    }
};

/**
 * @brief HilbertCurveComparator - Comparador basado en curva de Hilbert
 * Mapea puntos multidimensionales a valores 1D preservando localidad espacial
 * Referencia: SFCCOMPARE (Algoritmo 2) del paper
 */
class HilbertCurveComparator {
private:
    static const int MAX_ITERATIONS = 16;  // Precisión de la curva
    
    /**
     * @brief Calcula el índice de Hilbert para un punto 2D
     * Adaptado para D-dimensiones usando proyección
     */
    static uint64_t hilbertIndex2D(int x, int y, int order) {
        uint64_t index = 0;
        int rx, ry, s;
        
        for (s = order - 1; s >= 0; s--) {
            rx = (x >> s) & 1;
            ry = (y >> s) & 1;
            index += ((uint64_t)(3 * rx) ^ ry) << (2 * s);
            
            // Rotar
            if (ry == 0) {
                if (rx == 1) {
                    x = order - 1 - x;
                    y = order - 1 - y;
                }
                std::swap(x, y);
            }
        }
        return index;
    }
    
    /**
     * @brief Normaliza coordenadas al rango [0, 2^order - 1]
     */
    static int normalize(double value, double min, double max, int order) {
        if (max - min < 1e-9) return 0;
        int maxVal = (1 << order) - 1;
        double normalized = (value - min) / (max - min);
        return static_cast<int>(normalized * maxVal);
    }
    
public:
    /**
     * @brief Calcula el índice de Hilbert para un punto
     * Para D>2, usa las primeras dos dimensiones más significativas
     */
    static uint64_t computeHilbertIndex(const Point& p, const MBR& bounds) {
        if (p.dimensions() < 2 || bounds.dimensions() < 2) {
            // Fallback a coordenada X para 1D
            if (p.dimensions() == 1) {
                double range = bounds.getUpper()[0] - bounds.getLower()[0];
                if (range < 1e-9) return 0;
                return static_cast<uint64_t>(
                    ((p[0] - bounds.getLower()[0]) / range) * 0xFFFFFFFF
                );
            }
            return 0;
        }
        
        // Usar primeras dos dimensiones
        int x = normalize(p[0], bounds.getLower()[0], bounds.getUpper()[0], MAX_ITERATIONS);
        int y = normalize(p[1], bounds.getLower()[1], bounds.getUpper()[1], MAX_ITERATIONS);
        
        return hilbertIndex2D(x, y, MAX_ITERATIONS);
    }
    
    template<typename T>
    bool operator()(const SpatialRecord<T>& a, const SpatialRecord<T>& b, const MBR& bounds) const {
        uint64_t idxA = computeHilbertIndex(a.point, bounds);
        uint64_t idxB = computeHilbertIndex(b.point, bounds);
        return idxA < idxB;
    }
    
    bool operator()(const Point& p1, const Point& p2, const MBR& bounds) const {
        uint64_t idx1 = computeHilbertIndex(p1, bounds);
        uint64_t idx2 = computeHilbertIndex(p2, bounds);
        return idx1 < idx2;
    }
};

/**
 * @brief Z-order (Morton) comparator - Alternativa a Hilbert
 */
class ZOrderComparator {
private:
    static uint64_t interleaveBits(uint32_t x, uint32_t y) {
        uint64_t result = 0;
        for (int i = 0; i < 32; ++i) {
            result |= ((uint64_t)(x & (1u << i)) << i) | ((uint64_t)(y & (1u << i)) << (i + 1));
        }
        return result;
    }
    
public:
    static uint64_t computeZOrder(const Point& p, const MBR& bounds) {
        if (p.dimensions() < 2) return 0;
        
        double rangeX = bounds.getUpper()[0] - bounds.getLower()[0];
        double rangeY = bounds.getUpper()[1] - bounds.getLower()[1];
        
        if (rangeX < 1e-9 || rangeY < 1e-9) return 0;
        
        uint32_t x = static_cast<uint32_t>(
            ((p[0] - bounds.getLower()[0]) / rangeX) * 0xFFFFFFFF
        );
        uint32_t y = static_cast<uint32_t>(
            ((p[1] - bounds.getLower()[1]) / rangeY) * 0xFFFFFFFF
        );
        
        return interleaveBits(x, y);
    }
    
    template<typename T>
    bool operator()(const SpatialRecord<T>& a, const SpatialRecord<T>& b, const MBR& bounds) const {
        uint64_t zA = computeZOrder(a.point, bounds);
        uint64_t zB = computeZOrder(b.point, bounds);
        return zA < zB;
    }
};

} // namespace spatial
