cmake_minimum_required(VERSION 3.1.0)
# 设置 C/C++ 编译器为 Clang
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

project(assign1)

if (DEFINED ENV{LLVM_DIR})
    set(LLVM_DIR $ENV{LLVM_DIR})
    set(LLVM_INCLUDE_DIRS $ENV{LLVM_DIR}/include)
    set(CLANG_INCLUDE_DIRS $ENV{LLVM_DIR}/include)
    set(LLVM_LIBRARY_DIRS $ENV{LLVM_DIR}/lib)
endif()

find_package(Clang REQUIRED CONFIG HINTS ${LLVM_DIR} ${LLVM_DIR}/lib/cmake/clang NO_DEFAULT_PATH)

include_directories(${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS} SYSTEM)
link_directories(${LLVM_LIBRARY_DIRS})

file(GLOB SOURCE "./src/*.cpp")

add_executable(ast-interpreter ${SOURCE})

set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Option
  Support
  )

target_compile_options(ast-interpreter PRIVATE -fno-rtti)

target_link_libraries(ast-interpreter
  clangAST
  clangBasic
  clangFrontend
  clangTooling
  )


install(TARGETS ast-interpreter
  RUNTIME DESTINATION bin)
