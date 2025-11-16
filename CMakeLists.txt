cmake_minimum_required(VERSION 3.10)
project(LSM_Spatial_DB VERSION 1.0 LANGUAGES CXX)

# C++17 estándar
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configuración de compilación
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Flags de compilación
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Directorios de inclusión
include_directories(${PROJECT_SOURCE_DIR}/include)

# Archivos fuente del proyecto
set(SOURCES
    src/main.cpp
)

# Headers (para IDEs)
set(HEADERS
    include/spatial/Point.h
    include/spatial/MBR.h
    include/spatial/SpatialComparators.h
    include/spatial/RTree.h
    include/lsm/LSMComponent.h
    include/lsm/LSMTree.h
    include/lsm/MergePolicy.h
    include/lsm/PartitioningStrategy.h
    include/sql/Lexer.h
    include/sql/Parser.h
    include/sql/QueryExecutor.h
    include/cli/CLI.h
    include/workload/Workload.h
)

# Ejecutable principal
add_executable(lsm_spatial_db ${SOURCES} ${HEADERS})

# Propiedades del target
set_target_properties(lsm_spatial_db PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Configuración de instalación
install(TARGETS lsm_spatial_db DESTINATION bin)

# Información de compilación
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
