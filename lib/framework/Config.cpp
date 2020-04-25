#include "framework/Config.h"

Config::Config(std::string configFile) {
  std::ifstream infile(configFile.c_str());
  std::string line;
  while (std::getline(infile, line)) {
    line = trim(line);
    if (line == "") {
      continue;
    }
    std::unordered_map<std::string, std::string> oneBlockOption;
    std::string optionBlockName = line;
    std::getline(infile, line);
    line = trim(line);
    if (line != "{") {
      std::cout << "config file format error\n";
      exit(1);
    }
    while (std::getline(infile, line)) {
      line = trim(line);
      if (line == "") {
        continue;
      }
      if (line == "}") {
        options.insert(std::make_pair(optionBlockName, oneBlockOption));
        break;
      }
      auto parsedLine = parseOptionLine(line);
      oneBlockOption.insert(parsedLine);
    }
  }
}

std::pair<std::string, std::string>
Config::parseOptionLine(std::string optionLine) {
  int index = optionLine.find("=");
  std::string name = optionLine.substr(0, index);
  std::string value = optionLine.substr(index + 1);
  name = trim(name);
  value = trim(value);
  return std::make_pair(name, value);
}

std::unordered_map<std::string, std::string>
Config::getOptionBlock(std::string blockName) {
  std::unordered_map<
      std::string, std::unordered_map<std::string, std::string>>::const_iterator
      got = options.find(blockName);
  if (got == options.end()) {
    std::cout << "block name not found\n";
    exit(1);
  } else {
    return got->second;
  }
}

std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
Config::getAllOptionBlocks() {
  return options;
}

std::ostream &operator<<(std::ostream &os, Config &c) {
  for (auto block : c.getAllOptionBlocks()) {
    os << "block name: " << block.first << "\n";
    for (auto blockOpt : block.second) {
      os << "\t"
         << "option name = " << blockOpt.first << "\n";
      os << "\t"
         << "option value = " << blockOpt.second << "\n";
    }
  }
  return os;
}

std::string Config::trim(std::string s) {
  std::string result = s;
  result.erase(0, result.find_first_not_of(" \t\r\n"));
  result.erase(result.find_last_not_of(" \t\r\n") + 1);
  return result;
}
