# Quick Start Guide

## 🚀 Inicio Rápido (5 minutos)

### 1. Compilar

**Windows (PowerShell):**
```powershell
.\build.ps1
```

**Windows (CMD):**
```cmd
build.bat
```

### 2. Ejecutar Demo

```powershell
cd build
.\lsm_spatial_db.exe demo
```

### 3. Probar Interactivo

```powershell
.\lsm_spatial_db.exe
```

Luego escribir:
```sql
CREATE TABLE test (id INT, location POINT, value DOUBLE)
INSERT INTO test VALUES (0.5, 0.5, 100)
SELECT COUNT(*) FROM test WHERE spatial_intersect(location, 0, 0, 1, 1)
```

## 📚 Documentación Completa

- **README.md** - Información general del proyecto
- **BUILD_WINDOWS.md** - Instrucciones detalladas de compilación
- **USAGE.md** - Guía completa de uso
- **ARCHITECTURE.md** - Detalles técnicos de arquitectura
- **PROJECT_SUMMARY.md** - Resumen ejecutivo

## 🎯 Archivos Importantes

| Archivo | Descripción |
|---------|-------------|
| `build.ps1` | Script de compilación (PowerShell) |
| `build.bat` | Script de compilación (Batch) |
| `CMakeLists.txt` | Configuración de CMake |
| `src/main.cpp` | Programa principal |
| `include/` | Headers C++ |
| `examples/test_queries.sql` | Ejemplos SQL |

## ⚡ Comandos Útiles

### En el CLI
```
help       - Ver ayuda
metrics    - Ver métricas de rendimiento
tables     - Listar tablas
clear      - Limpiar métricas
exit       - Salir
```

### SQL Básico
```sql
-- Crear tabla
CREATE TABLE points (id INT, location POINT, data DOUBLE)

-- Insertar
INSERT INTO points VALUES (x, y, data)

-- Consultar
SELECT COUNT(*) FROM points WHERE spatial_intersect(location, x1, y1, x2, y2)
```

## 🐛 Problemas Comunes

**Error: cmake not found**
→ Instalar CMake y agregarlo al PATH

**Error: compilador no encontrado**
→ Instalar MinGW o Visual Studio

**Error: C++17 not supported**
→ Actualizar compilador (MinGW 8.0+ o VS 2017+)

## 💡 Tips

- Usar modo **demo** para ver ejemplos
- Usar modo **benchmark** para evaluación completa
- Compilar en **Release** para mejor rendimiento
- Ver `USAGE.md` para ejemplos detallados

## 📞 Ayuda

Si tienes problemas:
1. Leer `BUILD_WINDOWS.md`
2. Revisar `CHECKLIST.md`
3. Ver ejemplos en `examples/`

---

**¡Listo para empezar!** 🎉

Para más detalles, ver `README.md`
