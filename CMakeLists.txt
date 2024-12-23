cmake_minimum_required(VERSION 3.1.0)
project(assign3)
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
add_executable(assignment3 ${SOURCE}) 

target_compile_options(assignment3 PRIVATE -fno-rtti)

target_link_libraries(assignment3
	${LLVM_LINK_COMPONENTS}
	)

enable_testing()
set(CMAKE_CTEST_ARGUMENTS "--verbose")

set(test_data
  "test00\;^14 : ((plus, minus)|(minus, plus))\n24 : foo\n27 : foo\n$"
  "test01\\;^22 : ((plus, minus)|(minus, plus))\n$"
  "test02\\;^28 : ((plus, minus)|(minus, plus))\n$"
  "test03\\;^27 : ((plus, minus)|(minus, plus))\n$"
  "test04\\;^10 : ((plus, minus)|(minus, plus))\n26 : foo\n33 : foo\n$"
  "test05\\;^33 : ((plus, minus)|(minus, plus))\n$"
  "test06\\;^11 : malloc\n15 : plus\n21 : minus\n$"
  "test07\\;^19 : plus\n25 : minus\n$"
  "test08\\;^25 : plus\n31 : minus\n$"
  "test09\\;^21 : malloc\n27 : plus\n33 : minus\n$"
  "test10\\;^25 : malloc\n31 : minus\n37 : plus\n$"
  "test11\\;^11 : ((plus, minus)|(minus, plus))\n18 : malloc\n27 : clever\n$"
  "test12\\;^11 : ((plus, minus)|(minus, plus))\n21 : malloc\n30 : clever\n$"
  "test13\\;^15 : ((plus, minus)|(minus, plus))\n31 : clever\n$"
  "test14\\;^10 : ((plus, minus)|(minus, plus))\n14 : foo\n30 : clever\n$"
  "test15\\;^15 : ((plus, minus)|(minus, plus))\n19 : foo\n35 : clever\n$"
  "test16\\;^16 : foo\n17 : plus\n24 : malloc\n32 : clever\n$"
  "test17\\;^20 : foo\n21 : ((plus, minus)|(minus, plus))\n37 : clever\n$"
  "test18\\;^30 : ((foo, clever)|(clever, foo))\n31 : ((plus, minus)|(minus, plus))\n$"
  "test19\\;^24 : foo\n28 : clever\n30 : plus\n$"
  "test20\\;^47 : ((foo, clever)|(clever, foo))\n48 : ((plus, minus)|(minus, plus))\n$"
  "test21\\;^15 : ((plus, minus)|(minus, plus))\n31 : clever\n$"
  "test22\\;^17 : plus\n31 : make_simple_alias\n32 : foo\n$"
  "test23\\;^14 : ((plus, minus)|(minus, plus))\n25 : malloc\n26 : malloc\n30 : foo\n31 : make_simple_alias\n33 : foo\n$"
  "test24\\;^17 : minus\n29 : make_no_alias\n30 : foo\n$"
  "test25\\;^21 : plus\n37 : make_alias\n38 : foo\n$"
  "test26\\;^31 : malloc\n39 : plus\n40 : make_alias\n45 : minus\n$"
  "test27\\;^22 : plus\n27 : foo\n44 : clever\n$"
  "test28\\;^22 : ((plus, minus)|(minus, plus))\n29 : foo\n34 : malloc\n36 : malloc\n38 : malloc\n47 : clever\n$"
  "test29\\;^21 : ((plus, minus)|(minus, plus))\n26 : clever\n27 : ((plus, minus)|(minus, plus))\n41 : malloc\n46 : foo\n51 : foo\n$"
  "test30\\;^41 : ((foo, clever)|(clever, foo))\n42 : ((plus, minus)|(minus, plus))\n$"
  "test31\\;^39 : foo\n40 : plus\n44 : clever\n45 : ((plus, minus)|(minus, plus))\n47 : ((plus, minus)|(minus, plus))\n$"
  "test32\\;^38 : foo\n56 : clever\n57 : minus\n58 : minus\n$"
  "test33\\;^27 : minus\n38 : foo\n70 : make_simple_alias\n75 : make_alias\n81 : clever\n82 : minus\n83 : ((plus, minus)|(minus, plus))\n84 : swap_w\n85 : minus\n$"
  "test34\\;^39 : swap_w\n40 : ((plus, minus)|(minus, plus))\n49 : make_simple_alias\n51 : make_simple_alias\n52 : foo\n68 : make_simple_alias\n70 : make_simple_alias\n75 : make_alias\n79 : clever\n80 : clever\n81 : ((plus, minus)|(minus, plus))\n$"  
)

foreach(test_info ${test_data})
  list(GET test_info 0 test_name)
  list(GET test_info 1 test_val)
  add_test(
    NAME ${test_name}
    COMMAND bash -c "$<TARGET_FILE:assignment3> ${CMAKE_CURRENT_SOURCE_DIR}/testcases/class/${test_name}.bc"
  )
  set_tests_properties(${test_name} PROPERTIES
    PASS_REGULAR_EXPRESSION ${test_val}
    LABELS "official"
  )
endforeach()