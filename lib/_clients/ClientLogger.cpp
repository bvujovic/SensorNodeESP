#include "ClientLogger.h"

void ClientLogger::setLogPath(const String &path) { logPath = path; }

String ClientLogger::getLogPath() const { return logPath; }

void ClientLogger::add(const String &msg)
{
    File f = LittleFS.open(logPath, "a");
    if (f)
    {
        auto seconds = (millis() + 500) / 1000;
        auto minutes = seconds / 60;
        f.printf("[%lu:%lu]\t\t", minutes, seconds % 60);
        f.println(msg);
        f.close();
    }
}

void ClientLogger::print(const String &msg)
{
    if (msg != "")
        Serial.println(msg);
    File f = LittleFS.open(logPath, "r");
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
