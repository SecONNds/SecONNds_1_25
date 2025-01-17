cmake_minimum_required(VERSION 3.11)
project(Cheetah LANGUAGES CXX CUDA)
include(CMakeFindDependencyMacro)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -Xcompiler -fPIC")

add_compile_options(-fPIC)
add_compile_options(-D_GLIBCXX_USE_CXX11_ABI=0)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

## Dependencies
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Eigen3 3.3.3 REQUIRED)
find_package(SEAL 4.0 REQUIRED)
find_package(emp-tool REQUIRED)
find_package(emp-ot REQUIRED)
find_package(CUDA REQUIRED)
find_package(Troy REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR}
                    ${EMP-OT_INCLUDE_DIRS} 
                    ${EMP-TOOL_INCLUDE_DIRS}
                    ${CUDA_INCLUDE_DIRS})

include_directories(/usr/local/cuda-11.7/targets/x86_64-linux/include)
link_directories(${CUDA_LIBRARY_DIRS})

## cheetah-core
add_library(gemini SHARED)
include_directories(include)
add_subdirectory(include)
target_link_libraries(gemini SEAL::seal Eigen3::Eigen Threads::Threads Troy::troy ${CUDA_LIBRARIES})

if (USE_APPROX_RESHARE)
  target_compile_definitions(gemini PUBLIC USE_APPROX_RESHARE=1 
    VERIFY_LAYERWISE=${VERIFY_LAYERWISE} HE_DEBUG=${TRACK_HE_NOISE}
    MILL_PRINT_COMP=${MILL_PRINT_COMP} MILL_PRINT_COMM=${MILL_PRINT_COMM} MILL_PRINT_TIME=${MILL_PRINT_TIME})
else()
  target_compile_definitions(gemini PUBLIC USE_APPROX_RESHARE=0 
    VERIFY_LAYERWISE=${VERIFY_LAYERWISE} HE_DEBUG=${TRACK_HE_NOISE}
    MILL_PRINT_COMP=${MILL_PRINT_COMP} MILL_PRINT_COMM=${MILL_PRINT_COMM} MILL_PRINT_TIME=${MILL_PRINT_TIME})
endif()

## SCI
add_subdirectory(SCI)
include_directories(SCI/src/)

# # Set a default value for RUN_TRIP_TRIALS if not provided
# if (NOT DEFINED RUN_TRIP_TRIALS)
#   set(RUN_TRIP_TRIALS ON)
# endif()

## demo
macro (add_network_cheetah _name)
  add_executable(${_name}-cheetah "networks/main_${_name}.cpp")
  target_link_libraries(${_name}-cheetah gemini SCI-Cheetah)
  message(STATUS "Adding ${_name}-cheetah with RUN_TRIP_TRIALS=${RUN_TRIP_TRIALS}")
  target_compile_definitions(${_name}-cheetah PUBLIC SCI_OT=1 USE_CHEETAH=1
      RUN_TRIP_TRIALS=${RUN_TRIP_TRIALS})
endmacro()

add_network_cheetah(sqnet)
add_network_cheetah(resnet50)
add_network_cheetah(densenet121)
