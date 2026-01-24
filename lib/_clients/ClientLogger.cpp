#include "ClientLogger.h"

void ClientLogger::setLogPath(const String &path) { logPath = path; }

String ClientLogger::getLogPath() const { return logPath; }

void ClientLogger::add(const String &msg, const String &time)
{
    File f = LittleFS.open(logPath, "a");
    if (f)
    {
        // timestamp: if available use current time (e.g. from TSS), else calc mins&secs based on millis()
        if (time != "")
            f.printf("[%s]\t", time.c_str());
        else
        {
            auto ms = millis();
            if (ms < 1000)
                f.printf("[%03lu]\t\t", ms);
            else
            {
                auto seconds = (ms + 500) / 1000;
                auto minutes = seconds / 60;
                f.printf("[%2lu:%2lu]\t\t", minutes, seconds % 60);
            }
        }
        f.println(msg);
        f.close();
    }
    else
    {
        Serial.println("Failed to open log file for appending.");
    }
}

void ClientLogger::print(const String &msg)
{
    if (msg != "")
        Serial.println(msg);
    File f = LittleFS.open(logPath, "r");
    if (!f)
    {
        Serial.println("Log file not found.");
        return;
    }
    while (f.available())
        Serial.write(f.read());
    f.close();
}

void ClientLogger::clear(const String &msg)
{
    LittleFS.remove(logPath);
    if (msg != "")
        Serial.println(msg);
}
