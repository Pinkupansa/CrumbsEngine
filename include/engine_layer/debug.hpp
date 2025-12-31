#pragma once

#include <string>

class Debug {
 public:
  static void LogWarning(const std::string& msg);
  static void LogError(const std::string& msg);
  static void Log(const std::string& msg);
};