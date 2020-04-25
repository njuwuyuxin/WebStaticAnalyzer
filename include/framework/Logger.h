#ifndef _LOGGER_H
#define _LOGGER_H

#include <sstream>
#include <stdio.h>
#include <string>

#include "Config.h"

inline std::string NowTime();

enum TLogLevel {
  LOG_DEBUG_1,
  LOG_DEBUG_2,
  LOG_DEBUG_3,
  LOG_DEBUG_4,
  LOG_DEBUG_5
};

enum TCheck {
  ARRAY_BOUND,
  TEMP_CHECKER,
  DIVIDE_CHECK,
  MEMORY_OP,
  RECURSIVE_CALL,
  TAINT_CHECK
};

class Logger {
public:
  Logger();
  virtual ~Logger();
  std::ostringstream &log(TCheck t, TLogLevel level = LOG_DEBUG_3);

public:
  static void configure(Config &c);
  static void setCheckType(TCheck t);
  static TLogLevel &getConfigurationLevel();
  static std::string ToString(TLogLevel level);

protected:
  std::ostringstream os;

private:
  Logger(const Logger &);
  Logger &operator=(const Logger &);

private:
  TLogLevel messageLevel;
  static TLogLevel configurationLevel;
  static TCheck checkType;
  static bool options[6];
};

inline Logger::Logger() {}

inline std::ostringstream &Logger::log(TCheck t, TLogLevel level) {
  messageLevel = level;
  checkType = t;
  return os;
}

inline Logger::~Logger() {
  if (messageLevel >= Logger::getConfigurationLevel() && options[checkType]) {
    fprintf(stderr, "%s", os.str().c_str());
    fflush(stderr);
  }
}

inline TLogLevel &Logger::getConfigurationLevel() { return configurationLevel; }

inline std::string Logger::ToString(TLogLevel level) {
  static const char *const buffer[] = {"DEBUG_1", "DEBUG_2", "DEBUG_3",
                                       "DEBUG_4", "DEBUG_5"};
  return buffer[level];
}

#endif
