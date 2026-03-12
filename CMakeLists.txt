cmake_minimum_required(VERSION 3.16)
project(HE_PCA LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenFHE REQUIRED)

include_directories(${OPENMP_INCLUDES})
include_directories(${OpenFHE_INCLUDE})

# macOS: Homebrew libomp is keg-only; add its lib path for the linker.
if(APPLE)
    execute_process(COMMAND brew --prefix libomp OUTPUT_VARIABLE OMP_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(OMP_PREFIX)
        link_directories(${OMP_PREFIX}/lib)
    endif()
endif()
include_directories(${OpenFHE_INCLUDE}/third-party/include)
include_directories(${OpenFHE_INCLUDE}/core)
include_directories(${OpenFHE_INCLUDE}/pke)
include_directories(${OpenFHE_INCLUDE}/binfhe)
link_directories(${OpenFHE_LIBDIR})
link_libraries(${OpenFHE_LIBRARIES})

set(CMAKE_CXX_FLAGS "${OpenFHE_CXX_FLAGS}")

add_library(he_pca_lib STATIC
    src/he_context.cpp
    src/packing.cpp
    src/he_linalg.cpp
    src/power_iteration.cpp
    src/pca.cpp
    src/utils.cpp
)

add_executable(he_pca src/main.cpp)
target_link_libraries(he_pca he_pca_lib)

add_executable(test_packing tests/test_packing.cpp)
target_link_libraries(test_packing he_pca_lib)

add_executable(test_he_linalg tests/test_he_linalg.cpp)
target_link_libraries(test_he_linalg he_pca_lib)

add_executable(test_pca tests/test_pca.cpp)
target_link_libraries(test_pca he_pca_lib)
