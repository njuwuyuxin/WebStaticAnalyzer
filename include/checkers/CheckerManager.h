#include "framework/BasicChecker.h"
#include "framework/Config.h"
#include "framework/CallGraph.h"
#include <vector>
using namespace std;
using std::vector;

class CheckerManager{
public:
    CheckerManager(Config* conf);
    void add_checker(BasicChecker* c);
    void check_all();
private:
    Config* configure;
    vector<BasicChecker*> checkers;
};