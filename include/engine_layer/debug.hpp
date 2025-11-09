#pragma once

#include <iostream>
#include <string>

#define ENGINE_DEBUG_FLAG "CRUMBS"
#define WARNING_FLAG "_WARNING"
#define ERROR_FLAG "_ERROR"
#define NORMAL_FLAG "_LOG"
#define WARNING_COLOR "33m" // yellow
#define ERROR_COLOR "31m"   // red
#define NORMAL_COLOR "36m"

class Debug {
public:
    static void LogWarning(const std::string& msg) {
        std::cout << "\033[" << WARNING_COLOR << ENGINE_DEBUG_FLAG << WARNING_FLAG
                  << "\033[0m " << msg << "\033[0m\n";
    }

    static void LogError(const std::string& msg) {
        std::cout << "\033[" << ERROR_COLOR << ENGINE_DEBUG_FLAG << ERROR_FLAG
                  << "\033[0m " << msg << "\033[0m\n";
    }

    static void Log(const std::string& msg) { 
        std::cout << "\033[" << NORMAL_COLOR << ENGINE_DEBUG_FLAG << NORMAL_FLAG
                  << " " << msg << "\033[0m\n";
    }
};

