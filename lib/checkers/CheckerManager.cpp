#include "checkers/CheckerManager.h"
#include <utility>
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <assert.h>

using namespace rapidjson;

void Serialize(PrettyWriter<StringBuffer> &writer, Result r) {
  writer.StartObject();
  writer.Key("checkerName");
  writer.String(r.checkerName);
  writer.Key("defects");
  writer.StartArray();
  for (auto &&d : r.defects) {
    writer.StartObject();
    writer.Key("location");
    writer.String(d.location);
    writer.Key("info");
    writer.String(d.info);
    writer.EndObject();
  }
  writer.EndArray();
  writer.EndObject();
}

CheckerManager::CheckerManager(Config *conf) { configure = conf; }

void CheckerManager::add_checker(BasicChecker *checher, string name) {
  checkers.push_back(make_pair(checher, name));
}

void CheckerManager::check_all() {
  ofstream process_file("time.txt", ios::app);
  if (!process_file.is_open()) {
    cerr << "can't open time.txt\n";
    return;
  }

  auto enable = configure->getOptionBlock("CheckerEnable");

  StringBuffer s;
  PrettyWriter<StringBuffer> writer(s);
  writer.StartArray();

  for (auto checker : checkers) {
    if (enable.find(checker.second)->second == "true") {
      process_file << "Starting " + checker.second + " check" << endl;
      clock_t start, end;
      start = clock();

      Serialize(writer, {checker.second, checker.first->check()});

      end = clock();
      unsigned sec = unsigned((end - start) / CLOCKS_PER_SEC);
      unsigned min = sec / 60;
      process_file << "Time: " << min << "min" << sec % 60 << "sec" << endl;
      process_file
          << "End of " + checker.second +
                 " "
                 "check\n-----------------------------------------------"
                 "------------"
          << endl;
    }
  }
  process_file.close();
  writer.EndArray();

  time_t t = time(0);
  char ch[64];
  strftime(ch, sizeof(ch), "%Y-%m-%d %H-%M-%S", localtime(&t)); //年-月-日 时-分-秒
  string repo(ch);
  repo+=".json";
  ofstream repo_file(repo);
  if (!repo_file.is_open()) {
    cerr << "can't open report.json\n";
    return;
  }
  repo_file << s.GetString() << endl;
  repo_file.close();
}
