#ifndef CONFIG_H
#define CONFIG_H

#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Config {
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      options;
  std::pair<std::string, std::string> parseOptionLine(std::string optionLine);
  std::string trim(std::string s);

public:
  Config(std::string configFile);
  std::unordered_map<std::string, std::string>
  getOptionBlock(std::string blockName);
  friend std::ostream &operator<<(std::ostream &os, Config &c);
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
  getAllOptionBlocks();
};

#endif
