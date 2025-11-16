# Guía de Compilación para Windows

## Opción 1: MinGW (Recomendado para Windows)

### Prerequisitos
1. Instalar MinGW-w64: https://www.mingw-w64.org/downloads/
2. Instalar CMake: https://cmake.org/download/
3. Agregar ambos al PATH de Windows

### Compilación

```powershell
# Navegar al directorio del proyecto
cd Log-Structured-Merge-Tree

# Crear directorio de compilación
mkdir build
cd build

# Generar makefiles con MinGW
cmake -G "MinGW Makefiles" ..

# Compilar
mingw32-make

# Ejecutar
.\lsm_spatial_db.exe
```

## Opción 2: Visual Studio

### Prerequisitos
1. Instalar Visual Studio 2019 o superior (Community Edition es gratuita)
2. Incluir "Desktop development with C++" workload
3. Instalar CMake (incluido en Visual Studio o por separado)

### Compilación desde PowerShell

```powershell
cd Log-Structured-Merge-Tree
mkdir build
cd build

# Generar solución de Visual Studio
cmake ..

# Compilar (Release mode)
cmake --build . --config Release

# Ejecutar
.\Release\lsm_spatial_db.exe
```

### Compilación desde Visual Studio IDE

1. Abrir Visual Studio
2. File → Open → CMake...
3. Seleccionar `CMakeLists.txt` del proyecto
4. Visual Studio configurará automáticamente
5. Build → Build All
6. Ejecutar desde Debug → Start Without Debugging

## Opción 3: WSL (Windows Subsystem for Linux)

### Prerequisitos
1. Habilitar WSL en Windows
2. Instalar Ubuntu desde Microsoft Store
3. En Ubuntu, instalar herramientas:

```bash
sudo apt update
sudo apt install build-essential cmake g++
```

### Compilación

```bash
cd Log-Structured-Merge-Tree
mkdir build
cd build
cmake ..
make
./lsm_spatial_db
```

## Verificar la Compilación

Después de compilar exitosamente, deberías poder ejecutar:

```powershell
# Modo interactivo
.\lsm_spatial_db.exe

# Modo demo
.\lsm_spatial_db.exe demo

# Modo benchmark
.\lsm_spatial_db.exe benchmark
```

## Solución de Problemas Comunes

### Error: "cmake: command not found"
- Agregar CMake al PATH de Windows
- Reiniciar PowerShell/CMD después de instalar

### Error: "mingw32-make: command not found"
- Agregar MinGW bin directory al PATH
- Ejemplo: `C:\mingw-w64\bin`

### Error: Compilador no encontrado
```powershell
# Verificar que g++ está disponible
g++ --version

# Si no, reinstalar MinGW y verificar PATH
```

### Error: C++17 not supported
- Asegurar que el compilador soporta C++17
- MinGW-w64 versión 8.0+ requerida
- Visual Studio 2017 versión 15.3+ requerida

### Warning: "deprecated" o "unsafe functions"
- Estos son warnings de Visual Studio, no afectan funcionalidad
- Agregar `/W3` en lugar de `/W4` en CMakeLists.txt si molestan

## Recomendaciones

### Para Desarrollo
- Usar Visual Studio Code con extensiones:
  - C/C++ (Microsoft)
  - CMake Tools
  - C/C++ Extension Pack

### Para Producción
- Compilar en modo Release para mejor rendimiento:
  ```powershell
  cmake -DCMAKE_BUILD_TYPE=Release ..
  ```

### Para Debugging
- Compilar en modo Debug:
  ```powershell
  cmake -DCMAKE_BUILD_TYPE=Debug ..
  ```

## Estructura después de Compilar

```
Log-Structured-Merge-Tree/
├── build/
│   ├── lsm_spatial_db.exe    # (MinGW)
│   ├── Release/
│   │   └── lsm_spatial_db.exe # (Visual Studio)
│   └── Debug/
│       └── lsm_spatial_db.exe # (Visual Studio Debug)
├── include/
├── src/
└── ...
```

## Ejecutar Tests

```powershell
# Ir al directorio build
cd build

# Ejecutar demo interactivo
.\lsm_spatial_db.exe demo

# O desde build/Release si usaste Visual Studio
.\Release\lsm_spatial_db.exe demo
```

## Limpiar Build

```powershell
# Eliminar directorio build completo
cd ..
Remove-Item -Recurse -Force build

# Recompilar desde cero
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

## Performance Tips

1. **Siempre compilar en Release mode para benchmarks**
   ```powershell
   cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" ..
   ```

2. **Habilitar optimizaciones del compilador**
   - Ya configuradas en CMakeLists.txt

3. **Para benchmarks serios**
   - Cerrar otras aplicaciones
   - Desactivar antivirus temporalmente
   - Ejecutar desde PowerShell, no desde IDE

## Next Steps

Después de compilar exitosamente:
1. Leer `USAGE.md` para aprender a usar el sistema
2. Ejecutar `.\lsm_spatial_db.exe demo` para ver ejemplos
3. Experimentar con `examples/test_queries.sql`
4. Ejecutar benchmarks completos con `.\lsm_spatial_db.exe benchmark`
