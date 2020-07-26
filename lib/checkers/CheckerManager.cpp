#include "checkers/CheckerManager.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <assert.h>
#include <utility>

using namespace rapidjson;
using namespace std;

void Serialize(PrettyWriter<StringBuffer> &writer, Result r) {
  writer.StartObject();
  writer.Key("checkerName");
  writer.String(r.checkerName);
  writer.Key("defects");
  writer.StartArray();
  for (auto &&d : r.defects) {
    writer.StartObject();
    writer.Key("location");
    writer.String(get<0>(d));
    writer.Key("info");
    writer.String(get<1>(d));
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

  /* config.txt
  FileSettings
  {
    ReportSavePath = $savepath (default: 当前目录)
    ReportFileName = $filename (default: 当前时间)
  }
  */
  string ReportSavePath = "";
  time_t t = time(0);
  char ch[64];
  strftime(ch, sizeof(ch), "%Y-%m-%d %H-%M-%S",
           localtime(&t)); //年-月-日 时-分-秒
  string ReportFileName(ch);
  //保证兼容性，即使Config中没有配置FileSettings也可正常运行
  //即缺陷报告保存在当前目录，报告文件名用当前时间代替
  // ReportSavePath设置应为绝对路径，即以/开头并以/结尾
  auto AllBlocks = configure->getAllOptionBlocks();
  if (AllBlocks.find("FileSettings") != AllBlocks.end()) {
    auto FileSettings = configure->getOptionBlock("FileSettings");
    if (FileSettings.find("ReportSavePath") != FileSettings.end())
      ReportSavePath = FileSettings.find("ReportSavePath")->second;
    if (FileSettings.find("ReportFileName") != FileSettings.end())
      ReportFileName = FileSettings.find("ReportFileName")->second;
  }

  StringBuffer s;
  PrettyWriter<StringBuffer> writer(s);
  writer.StartArray();

  for (auto &&c : checkers) {
    BasicChecker* checker = c.first;
    string checkerName = c.second;
    if (enable.find(checkerName) == enable.end()) {
      cout << "Checker: " << checkerName << " not found in config.txt" << endl;
      cout << checkerName << " not executed" << endl;
      continue;
    }
    if (enable.find(checkerName)->second == "true") {
      process_file << "Starting " + checkerName + " check" << endl;
      clock_t start, end;
      start = clock();

      checker->check();
      Serialize(writer, {checkerName, checker->getDefects()});

      end = clock();
      unsigned sec = unsigned((end - start) / CLOCKS_PER_SEC);
      unsigned min = sec / 60;
      process_file << "Time: " << min << "min" << sec % 60 << "sec" << endl;
      process_file
          << "End of " + checkerName +
                 " "
                 "check\n-----------------------------------------------"
                 "------------"
          << endl;
    }
  }
  process_file.close();
  writer.EndArray();

  ReportFileName += ".json";
  ReportFileName = ReportSavePath + ReportFileName;
  ofstream repo_file(ReportFileName);
  if (!repo_file.is_open()) {
    cerr << "can't open report.json\n";
    return;
  }
  repo_file << s.GetString() << endl;
  repo_file.close();
}
