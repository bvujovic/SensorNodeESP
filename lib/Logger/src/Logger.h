#pragma once

#include <LittleFS.h>

/// @brief Logger object will read and write messages to files and retrieve data about file system.
class Logger
{
private:
    //TODO mozda ovi folderName i filePath da se stope u jedan strBuffer ili tako nesto
    char folderName[10];
    char strBuffer[25];
    //? char lineBuffer[80];
    struct tm ti;

public:
    void setTimeInfo(const struct tm &ti) { this->ti = ti; }

    //? Citanje kompletnog tekst fajla sa prosledjenim imenom.
    String read(const String &fileName);
    //? Dodavanje teksta u fajl sa prosledjenim imenom.
    bool add(const char *source, const char *message);
    // // Pisanje teksta (stari sadrzaj se brise) u fajl sa prosledjenim imenom.
    // static bool write(const String &fileName, const String &text) { return _write(fileName, text, "w"); }

    //? lista svih foldera u rootu (mozda bez nekih posebnih: web server i sl)
    String listFolders();
    // lista svih fajlova u nekom folderu
    String listFiles(const String &folderName);

    size_t getTotalKB() { return LittleFS.totalBytes() / 1024; }
    size_t getUsedKB() { return LittleFS.usedBytes() / 1024; }
};
