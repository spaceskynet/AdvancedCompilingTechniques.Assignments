cmake_minimum_required(VERSION 3.1.0)
project(assign2)

if (DEFINED ENV{LLVM_DIR})
    set(LLVM_DIR $ENV{LLVM_DIR})
    set(LLVM_INCLUDE_DIRS $ENV{LLVM_DIR}/include)
    set(CLANG_INCLUDE_DIRS $ENV{LLVM_DIR}/include)
    set(LLVM_LIBRARY_DIRS $ENV{LLVM_DIR}/lib)
endif()

find_package(LLVM REQUIRED CONFIG HINTS ${LLVM_DIR} ${LLVM_DIR}/lib/cmake/llvm
	                NO_DEFAULT_PATH)

include_directories(${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS} SYSTEM)
link_directories(${LLVM_LIBRARY_DIRS})
message(STATUS "LLVM_LIB DIR : ${LLVM_LIBRARY_DIRS}")
set(LLVM_LINK_COMPONENTS
  LLVMCore
  LLVMIRReader
  LLVMPasses
  )

message(STATUS "LLVM LIBS : ${LLVM_LINK_COMPONENTS}")
# Support plugins.

file(GLOB SOURCE "./src/*.cpp")

add_executable(llvmassignment ${SOURCE})

target_compile_options(llvmassignment PRIVATE -fno-rtti)

target_link_libraries(llvmassignment
	${LLVM_LINK_COMPONENTS}
	)
