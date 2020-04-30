#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

#include <clang/Tooling/CommonOptionsParser.h>
#include <llvm-c/Target.h>
#include <llvm/Support/CommandLine.h>

#include "checkers/CharArrayBound.h"
#include "checkers/TemplateChecker.h"
#include "framework/ASTManager.h"
#include "framework/BasicChecker.h"
#include "framework/CallGraph.h"
#include "framework/Config.h"
#include "framework/Logger.h"

using namespace clang;
using namespace llvm;
using namespace clang::tooling;

#define check(checkerName)                                                     \
  {                                                                            \
    if (enable.find(#checkerName)->second == "true") {                         \
      process_file << "Starting " #checkerName " check" << endl;               \
      clock_t start, end;                                                      \
      start = clock();                                                         \
                                                                               \
      checkerName checker(&resource, &manager, &call_graph, &configure);       \
      checker.check();                                                         \
                                                                               \
      end = clock();                                                           \
      unsigned sec = unsigned((end - start) / CLOCKS_PER_SEC);                 \
      unsigned min = sec / 60;                                                 \
      process_file << "Time: " << min << "min" << sec % 60 << "sec" << endl;   \
      process_file << "End of " #checkerName " "                               \
                      "check\n-----------------------------------------------" \
                      "------------"                                           \
                   << endl;                                                    \
    }                                                                          \
  }

int main(int argc, const char *argv[]) {
  ofstream process_file("time.txt");
  if (!process_file.is_open()) {
    cerr << "can't open time.txt\n";
    return -1;
  }
  clock_t startCTime, endCTime;
  startCTime = clock();

  LLVMInitializeNativeTarget();
  LLVMInitializeNativeAsmParser();

  std::vector<std::string> ASTs = initialize(argv[1]);

  Config configure(argv[2]);

  ASTResource resource;
  ASTManager manager(ASTs, resource, configure);
  CallGraph call_graph(manager, resource);

  auto enable = configure.getOptionBlock("CheckerEnable");

  Logger::configure(configure);

  check(CharArrayBound);
  check(TemplateChecker);

  if (enable.find("CallGraphChecker")->second == "true") {
    process_file << "Starting CallGraphChecker check" << endl;
    clock_t start, end;
    start = clock();

    call_graph.printCallGraph(std::cout);
    std::fstream out("outTest.dot", ios::out);
    if (out.is_open()) {
      call_graph.writeDotFile(out);
    }
    out.close();

    end = clock();
    unsigned sec = unsigned((end - start) / CLOCKS_PER_SEC);
    unsigned min = sec / 60;
    process_file << "Time: " << min << "min" << sec % 60 << "sec" << endl;
    process_file
        << "End of CallGraphChecker "
           "check\n-----------------------------------------------------------"
        << endl;
  }

  endCTime = clock();
  unsigned sec = unsigned((endCTime - startCTime) / CLOCKS_PER_SEC);
  unsigned min = sec / 60;
  process_file << "-----------------------------------------------------------"
                  "\nTotal time: "
               << min << "min" << sec % 60 << "sec" << endl;
  return 0;
}
