#pragma once

#include <LittleFS.h>

/// @brief Simple client logger for logging events to a file in LittleFS
class ClientLogger
{
private:
    String logPath;

public:
    ClientLogger(const String &path = "/log.txt") : logPath(path) { LittleFS.begin(); };
    
    void setLogPath(const String &path);
    String getLogPath() const;

    /// @brief  Add a log entry
    /// @param msg Message (string) to log
    /// @param time Optional time string to include in the log entry
    void add(const String &msg, const String &time = "");
    /// @brief  Print the log file contents to Serial
    /// @param msg Optional message to print before log contents
    void print(const String &msg = "Log file content:");
    /// @brief  Clear the log file
    /// @param msg Optional message to print after clearing the log
    void clear(const String &msg = "Log file cleared.");

};
