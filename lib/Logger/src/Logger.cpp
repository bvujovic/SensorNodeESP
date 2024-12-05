#include "Logger.h"

// bool Logger::add(const String &text)
bool Logger::add(const char *source, const char *message)
{
    if (!getLocalTime(&ti))
        return false;

    // strftime(folderName, sizeof(folderName), "/%Y_%m", &ti);
    // if (!LittleFS.exists(folderName))
    //     LittleFS.mkdir(folderName);
    // folder name: createIN
    strftime(strBuffer, sizeof(strBuffer), "/%Y_%m", &ti);
    if (!LittleFS.exists(strBuffer))
        LittleFS.mkdir(strBuffer);
    // file path
    strftime(strBuffer, sizeof(strBuffer), "/%Y_%m/%d_%a.log", &ti);

    File fp = LittleFS.open(strBuffer, "a");
    if (fp)
    {
        strftime(strBuffer, sizeof(strBuffer), "%H:%M:%S", &ti);
        fp.printf("%s\t%s\t%s\n", strBuffer, source, message);
        // fp.printf(filePath);
        // fp.printf(source);
        // fp.print('\t');
        // fp.println(message);
        fp.close();
        return true;
    }
    else
        return false;
}

String Logger::read(const String &fileName)
{
    File fp = LittleFS.open(fileName, "r");
    String s = "";
    if (fp)
    {
        s = fp.readString();
        fp.close();
    }
    return s;
}

String Logger::listFolders()
{
    String str;
    File root = LittleFS.open("/");
    if (root)
    {
        File f;
        while (f = root.openNextFile())
            // B if (listFolders || strHour[0] == '\0' || strstr(f.name(), strHour))
            // (str += f.name()) += "\n";
            if (f.isDirectory())
                str += String(f.name()) + "\n";
        return str;
    }
    return String();
}

String Logger::listFiles(const String &folderName)
{
    String str;
    File folder = LittleFS.open(folderName);
    if (folder)
    {
        File f;
        while (f = folder.openNextFile())
            str += String(f.name()) + "\n";
        return str;
    }
    return String();
}
