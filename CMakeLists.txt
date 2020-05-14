cmake_minimum_required(VERSION 3.10)
project(seexperiment)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

if(DEFINED LLVM_PREFIX)
  set(LLVM_CONFIG ${LLVM_PREFIX}/bin/llvm-config)
  # specify LLVM include dir
  set(LLVM_INCLUDE ${LLVM_PREFIX}/include)
  # specify LLVM lib dir
  set(LLVM_LIB ${LLVM_PREFIX}/lib)
elseif(DEFINED LLVM_BUILD)
  set(LLVM_CONFIG ${LLVM_BUILD}/bin/llvm-config)
  # run `llvm-config --src-root` to get llvm source root path, and output it to variable LLVM_SRC
  execute_process(
    COMMAND ${LLVM_CONFIG} --src-root
    OUTPUT_VARIABLE LLVM_SRC
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  # specify LLVM include dir
  set(LLVM_INCLUDE
    ${LLVM_SRC}/include ${LLVM_SRC}/tools/clang/include
    ${LLVM_BUILD}/include ${LLVM_BUILD}/tools/clang/include
  )
  # specify LLVM lib dir
  set(LLVM_LIB ${LLVM_BUILD}/lib)
else()
  message(FATAL_ERROR "You must define one of the two variables: LLVM_PREFIX, LLVM_BUILD")
endif()

# run `llvm-config --libs` to get llvm libs list, and output it to variable LLVM_LIBS
execute_process(
  COMMAND ${LLVM_CONFIG} --libs
  OUTPUT_VARIABLE LLVM_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# define CLANG_LIBS: some clang libs to be used in link time
# !!! DO NOT change the order of the libs !!!
# libs will be linked in this order, and changing the order could lead to link errors
set(CLANG_LIBS
  clangTooling
  clangFrontendTool
  clangFrontend
  clangDriver
  clangSerialization
  clangCodeGen
  clangParse
  clangSema
  clangStaticAnalyzerFrontend
  clangStaticAnalyzerCheckers
  clangStaticAnalyzerCore
  clangAnalysis
  clangARCMigrate
  clangRewriteFrontend
  clangEdit
  clangAST
  clangASTMatchers
  clangLex
  clangBasic
)

# define OTHER_LIBS: other libs to be used in link time
set(OTHER_LIBS pthread z dl curses)

include_directories(include ${LLVM_INCLUDE} third_party/rapidjson/include)
link_directories(${LLVM_LIB})

add_subdirectory(lib)
add_subdirectory(tools)
