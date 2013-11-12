set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${LLVM_ROOT}")
cmake_minimum_required(VERSION 2.8)

set(LLVM_COMPILER_IS_GCC_COMPATIBLE 1)

find_package(LLVM)

# Define add_llvm_* macro's.
include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${LLVM_ROOT}/include)
include_directories(${LLVM_SRC_ROOT}/include)
link_directories(${LLVM_LIBRARY_DIRS})
link_directories(${LLVM_ROOT}/lib)

add_subdirectory(BaggyBounds)
