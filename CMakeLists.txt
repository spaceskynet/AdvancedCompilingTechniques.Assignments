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

enable_testing()
set(CMAKE_CTEST_ARGUMENTS "--verbose")

set(test_data
  "test00\;^10 : plus\n$"
  "test01\;^(21 : ((plus, NULL)|(NULL, plus))\n)?22 : plus\n$"
  "test02\;^24 : ((plus, minus)|(minus, plus))\n$"
  "test03\;^27 : ((plus, minus)|(minus, plus))\n$"
  "test04\;^10 : ((plus, minus)|(minus, plus))\n26 : foo\n33 : foo\n$"
  "test05\;^33 : ((plus, minus)|(minus, plus))\n$"
  "test06\;^10 : ((plus, minus)|(minus, plus))\n26 : clever\n$"
  "test07\;^10 : ((plus, minus)|(minus, plus))\n28 : clever\n30 : clever\n$"
  "test08\;^10 : ((plus, minus)|(minus, plus))\n26 : clever\n$"
  "test09\;^10 : ((plus, minus)|(minus, plus))\n14 : foo\n30 : clever\n$"
  "test10\;^15 : ((plus, minus)|(minus, plus))\n19 : foo\n35 : clever\n$"
  "test11\;^15 : foo\n16 : plus\n32 : clever\n$"
  "test12\;^15 : foo\n16 : ((plus, minus)|(minus, plus))\n32 : clever\n$"
  "test13\;^30 : ((clever, foo)|(foo, clever))\n31 : ((plus, minus)|(minus, plus))\n$"
  "test14\;^24 : foo\n31 : ((clever, foo)|(foo, clever))\n32 : plus\n$"
  "test15\;^14 : ((plus, minus)|(minus, plus))\n24 : foo\n27 : foo\n$"
  "test16\;^14 : foo\n17 : clever\n24 : clever1\n25 : plus\n$"
  "test17\;^14 : foo\n17 : clever\n24 : clever1\n25 : plus\n$"
  "test18\;^14 : foo\n18 : ((clever, foo)|(foo, clever))\n30 : clever1\n31 : plus\n$"
  "test19\;^14 : foo\n18 : ((clever, foo)|(foo, clever))\n24 : ((clever, foo)|(foo, clever))\n36 : clever1\n37 : plus\n$"
)

foreach(test_info ${test_data})
  list(GET test_info 0 test_name)
  list(GET test_info 1 test_val)
  add_test(
    NAME ${test_name}
    COMMAND bash -c "$<TARGET_FILE:llvmassignment> ${CMAKE_CURRENT_SOURCE_DIR}/testcases/class/${test_name}.bc"
  )
  set_tests_properties(${test_name} PROPERTIES
    PASS_REGULAR_EXPRESSION ${test_val}
    LABELS "official"
  )
endforeach()