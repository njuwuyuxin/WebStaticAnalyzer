#include "framework/Logger.h"

TLogLevel Logger::configurationLevel = LOG_DEBUG_3;
TCheck Logger::checkType = TAINT_CHECK;
bool Logger::options[6] = {false, false, false, false, false, false};

void Logger::configure(Config &c) {
  auto block = c.getOptionBlock("PrintLog");
  int level = atoi(block.find("level")->second.c_str());
  switch (level) {
  case 1:
    configurationLevel = LOG_DEBUG_1;
    break;
  case 2:
    configurationLevel = LOG_DEBUG_2;
    break;
  case 3:
    configurationLevel = LOG_DEBUG_3;
    break;
  case 4:
    configurationLevel = LOG_DEBUG_4;
    break;
  case 5:
    configurationLevel = LOG_DEBUG_5;
    break;
  default:
    configurationLevel = LOG_DEBUG_3;
  }

  if (block.find("taintChecker")->second == "true")
    options[TAINT_CHECK] = true;
  if (block.find("TemplateChecker")->second == "true")
    options[TEMP_CHECKER] = true;
  if (block.find("arrayBound")->second == "true")
    options[ARRAY_BOUND] = true;
  if (block.find("recursiveCall")->second == "true")
    options[RECURSIVE_CALL] = true;
  if (block.find("divideChecker")->second == "true")
    options[DIVIDE_CHECK] = true;
  if (block.find("memoryOPChecker")->second == "true")
    options[MEMORY_OP] = true;
}
