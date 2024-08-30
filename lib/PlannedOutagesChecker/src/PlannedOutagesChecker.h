#pragma once

#include <Arduino.h>
#include <LinkedList.h> // lib_deps = ivanseidel/LinkedList@0.0.0-alpha+sha.dac3874d28

struct Location
{
    String municipality;
    String street;
    bool found;
};

/// @brief PlannedOutagesChecker check error.
enum PocResult
{
    /// @brief There was an error during connecting to website or parsing a web page.
    PocError,     
    /// @brief POC successfully connected to website and parsed all pages, but specified locations are not found.
    PocNoOutages, 
    /// @brief Some of specified locations are found.
    PocOutagesFound
};

class PlannedOutagesChecker
{
private:
    LinkedList<Location *> locations;
    void searchForLocations(String &html);
    void foundLocationsToString(const String &webFile, String &msg);

public:
    void addLocation(const String &municipality, const String &street);
    //todo clearLocations()

    /// @brief Check if specified locations are listed in one of web pages for planned power outages.
    /// @param msg Error message of list of found locations.
    /// @return Status: error, locations found or not found.
    PocResult check(String &msg);
};
