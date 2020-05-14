#include "framework/BasicChecker.h"
#include "framework/CallGraph.h"
#include "framework/Config.h"
#include <string>
#include <utility>
#include <vector>
using namespace std;
using std::vector;

class CheckerManager {
public:
  CheckerManager(Config *conf);
  void add_checker(BasicChecker *checker, string name);
  void check_all();

private:
  Config *configure;
  vector<pair<BasicChecker *, string>> checkers;
};
