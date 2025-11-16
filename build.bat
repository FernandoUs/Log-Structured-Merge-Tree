@echo off
REM Script de compilación para Windows (Batch)
REM Usar: build.bat

echo === LSM-Tree Spatial Database - Build Script ===
echo.

REM Verificar CMake
cmake --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: CMake no encontrado. Por favor instalar CMake.
    pause
    exit /b 1
)

echo CMake encontrado
echo.

REM Crear directorio build
if not exist build mkdir build
cd build

echo Generando archivos de compilacion...
echo.

REM Intentar con MinGW
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release .. >nul 2>&1
if %errorlevel% equ 0 (
    echo Configuracion con MinGW exitosa
    echo.
    echo Compilando...
    mingw32-make
    
    if %errorlevel% equ 0 (
        echo.
        echo Compilacion exitosa!
        echo.
        echo Ejecutable: build\lsm_spatial_db.exe
        echo.
        echo Para ejecutar:
        echo   lsm_spatial_db.exe          - Modo interactivo
        echo   lsm_spatial_db.exe demo     - Modo demo
        echo   lsm_spatial_db.exe benchmark - Modo benchmark
    ) else (
        echo Error en la compilacion
        pause
        exit /b 1
    )
) else (
    echo MinGW no encontrado, intentando con Visual Studio...
    
    cmake ..
    cmake --build . --config Release
    
    if %errorlevel% equ 0 (
        echo.
        echo Compilacion exitosa con Visual Studio!
        echo.
        echo Ejecutable: build\Release\lsm_spatial_db.exe
        echo.
        echo Para ejecutar:
        echo   Release\lsm_spatial_db.exe          - Modo interactivo
        echo   Release\lsm_spatial_db.exe demo     - Modo demo
        echo   Release\lsm_spatial_db.exe benchmark - Modo benchmark
    ) else (
        echo Error en la compilacion
        pause
        exit /b 1
    )
)

cd ..
pause
