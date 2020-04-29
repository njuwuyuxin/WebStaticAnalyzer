add_executable(Checker main.cpp)

# specify the libs that the target 'Checker' needs
# !!! DO NOT change the order of the libs !!!
# libs will be linked in this order, and changing the order could lead to link errors
target_link_libraries(Checker
  checkers
  framework
  ${CLANG_LIBS}
  ${LLVM_LIBS}
  ${OTHER_LIBS}
)
