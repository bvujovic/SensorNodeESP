#include "SimpleEventHandler.h"

const char* SimpleEventHandler::unknownDevice = "Unknown device";

SimpleEventHandler::SimpleEventHandler()
{
    devices.push_back(std::make_pair((const unsigned char *)"\x24\x6F\x28\xAA\xBB\xCC", "Kitchen/Sink"));

    const unsigned char *mac = devices[0].first;
    printf("%02X:%02X:%02X:%02X:%02X:%02X \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool SimpleEventHandler::newMessage(const char *msg, const unsigned char *mac)
{
    // parsing msg
    unsigned long msgId;
    int res = sscanf(msg, "%lu;%s", &msgId, &msgText);
    if (res != 2)
        return false;

    // if it's in msgIDs discard msg (return false)
    for (size_t i = 0; i < msgIDs.size(); i++)
        if (msgIDs[i] == msgId)
            return false;
    msgIDs.push_back(msgId);

    // find device name from mac
    deviceName = NULL;
    for (size_t i = 0; i < devices.size(); i++)
    {
        const unsigned char *dmac = devices[i].first;
        bool match = true;
        for (size_t j = 0; j < 6; j++)
            if (mac[j] != dmac[j])
            {
                match = false;
                break;
            }
        if (match)
        {
            deviceName = (char *)devices[i].second;
            break;
        }
    }
    if (deviceName == NULL)
        deviceName = (char *)unknownDevice;
    // TODO maybe a vector for multiple events that should be processed

    return true;
    /*
        #include <iostream>
    #include <vector>
    #include <string>

    int main() {
        // 1. Declaration and Initialization
        // Create a vector of integers with an initializer list
        std::vector<int> numbers = {10, 20, 30, 40};
        std::vector<std::string> colors; // Create an empty vector of strings

        // 2. Adding Elements
        // Use push_back() to add elements to the end (dynamically resizes if needed)
        numbers.push_back(50);
        colors.push_back("Red");
        colors.push_back("Blue");
        colors.push_back("Green");

        // 3. Accessing Elements
        // Elements can be accessed using the index operator []
        std::cout << "First number: " << numbers[0] << std::endl;

        // Or use the .at() function (which provides bounds checking and throws an exception on error)
        std::cout << "Second color: " << colors.at(1) << std::endl;

        // Access the last element
        std::cout << "Last number: " << numbers.back() << std::endl;

        // 4. Modifying Elements
        numbers[2] = 99; // Overwrite the element at index 2 (originally 30)

        // 5. Iterating (Looping) through the Vector
        std::cout << "\\nUpdated numbers: ";
        // Use a range-based for loop for clean iteration
        for (int n : numbers) {
            std::cout << n << " ";
        }
        std::cout << std::endl;

        // 6. Utility Functions
        std::cout << "Vector size: " << numbers.size() << std::endl; // Current number of elements
        std::cout << "Is vector empty? " << (colors.empty() ? "Yes" : "No") << std::endl;

        // 7. Removing Elements
        colors.pop_back(); // Removes the last element ("Green")
        std::cout << "Colors after pop_back(): ";
        for (const std::string& c : colors) {
            std::cout << c << " ";
        }
        std::cout << std::endl;

        return 0;
    }

    */
}

void SimpleEventHandler::EventHandled()
{
    msgText[0] = '\0';
    deviceName = NULL;
}
