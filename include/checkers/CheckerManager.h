#include "framework/BasicChecker.h"
#include "framework/CallGraph.h"
#include "framework/Config.h"
#include <string>
#include <utility>
#include <vector>

struct Result {
  std::string checkerName;
  DefectSet defects;
};

class CheckerManager {
public:
  CheckerManager(Config *conf);
  void add_checker(BasicChecker *checker, std::string name);
  void check_all();

private:
  Config *configure;
  std::vector<std::pair<BasicChecker *, std::string>> checkers;
};
