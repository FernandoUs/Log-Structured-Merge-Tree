# Script de compilación rápida para Windows (PowerShell)
# Usar: .\build.ps1

Write-Host "=== LSM-Tree Spatial Database - Build Script ===" -ForegroundColor Cyan
Write-Host ""

# Verificar si CMake está instalado
try {
    $cmakeVersion = cmake --version
    Write-Host "✓ CMake encontrado" -ForegroundColor Green
} catch {
    Write-Host "✗ Error: CMake no encontrado. Por favor instalar CMake." -ForegroundColor Red
    exit 1
}

# Crear directorio build si no existe
if (!(Test-Path -Path "build")) {
    Write-Host "Creando directorio build..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Navegar a build
Set-Location -Path "build"

Write-Host ""
Write-Host "Generando archivos de compilación..." -ForegroundColor Yellow

# Intentar con MinGW primero
try {
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
    Write-Host "✓ Configuración con MinGW exitosa" -ForegroundColor Green
    
    Write-Host ""
    Write-Host "Compilando..." -ForegroundColor Yellow
    mingw32-make
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "✓ Compilación exitosa!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Ejecutable creado en: build\lsm_spatial_db.exe" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Para ejecutar:" -ForegroundColor Yellow
        Write-Host "  .\lsm_spatial_db.exe          - Modo interactivo" -ForegroundColor White
        Write-Host "  .\lsm_spatial_db.exe demo     - Modo demo" -ForegroundColor White
        Write-Host "  .\lsm_spatial_db.exe benchmark - Modo benchmark" -ForegroundColor White
    } else {
        Write-Host "✗ Error en la compilación" -ForegroundColor Red
        exit 1
    }
} catch {
    Write-Host "⚠ MinGW no encontrado, intentando con Visual Studio..." -ForegroundColor Yellow
    
    cmake ..
    cmake --build . --config Release
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "✓ Compilación exitosa con Visual Studio!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Ejecutable creado en: build\Release\lsm_spatial_db.exe" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Para ejecutar:" -ForegroundColor Yellow
        Write-Host "  .\Release\lsm_spatial_db.exe          - Modo interactivo" -ForegroundColor White
        Write-Host "  .\Release\lsm_spatial_db.exe demo     - Modo demo" -ForegroundColor White
        Write-Host "  .\Release\lsm_spatial_db.exe benchmark - Modo benchmark" -ForegroundColor White
    } else {
        Write-Host "✗ Error en la compilación" -ForegroundColor Red
        exit 1
    }
}

Set-Location -Path ..
