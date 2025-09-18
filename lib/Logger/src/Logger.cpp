#include "Logger.h"

bool Logger::add(const char *type, const char *device, const char *message)
{
    if (!getLocalTime(&ti))
        return false;
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
        fp.printf("%s\t%s\t%s\t%s\n", strBuffer, type, device, message);
        fp.close();
        return true;
    }
    else
        return false;
}

String Logger::read(const String &fileName)
{
    // if (!LittleFS.exists(fileName))
    //     return "! File doesn't exist.";
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
            if (f.isDirectory() && strcmp(f.name(), "ws") != 0) // exclude web server folder
                str += String(f.name()) + "\n";
        // str += String(" * Used ") + getUsedKB() + " KB / " + getTotalKB() + " KB total.";
        return str;
    }
    return str;
}

String Logger::listFiles(const String &folderName)
{
    // if (!LittleFS.exists(folderName))
    //     return "! Folder doesn't exist.";
    String str;
    File folder = LittleFS.open(folderName);
    if (folder)
    {
        File f;
        while (f = folder.openNextFile())
            str += String(f.name()) + "\n";
        // str += String(f.name()) + " - " + (f.size() / 1024) + " KB\n";
        return str;
    }
    return str;
}

bool Logger::removeFolder(const String &dir)
{
    File root = LittleFS.open(dir);
    if (!root || !root.isDirectory())
        return false;
    File file = root.openNextFile();
    while (file)
    {
        String path = String(dir) + "/" + file.name();
        Serial.println(path);
        file.close();
        if (!LittleFS.remove(path))
        {
            file.close();
            root.close();
            return false;
        }
        file = root.openNextFile();
    }
    LittleFS.rmdir(dir);
    // Serial.println(res ? "Folder removed." : "Folder NOT removed.");
    return true;
}
