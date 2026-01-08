#include "SimpleEventHandler.h"

const char *SimpleEventHandler::unknownDevice = "Unknown device";

SimpleEventHandler::SimpleEventHandler()
{
    clearEventData();

    // list of known devices - peers from my_esp_now.h
    devices.push_back(std::make_pair((const unsigned char *)"\x2C\xBC\xBB\x92\x1A\x34", "Kitchen/Sink"));

    //? testing: print first MAC address
    const unsigned char *mac = devices[0].first;
    printf("%02X:%02X:%02X:%02X:%02X:%02X \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void SimpleEventHandler::newMessage(const unsigned char *mac, char *msg, peer_info *p)
{
    // printf("SimpleEventHandler::newMessage called with msg='%s'\n", msg);
    // parsing msg
    unsigned long msgId;
    // int res = sscanf(msg, "%lu;%s", &msgId, &msgText);
    // if (res != 2)
    //     return; // invalid message format

    // find index of ; in msg
    int idxSeparator = -1;
    for (size_t i = 0; msg[i] != '\0'; i++)
    {
        if (msg[i] == ';')
        {
            idxSeparator = i;
            break;
        }
    }
    if (idxSeparator == -1)
        return;

    msg[idxSeparator] = '\0';
    // parse msg from to msgId
    msgId = strtoul(msg, NULL, 10);
    msgText = msg + idxSeparator + 1;

    // if it's in msgIDs discard msg
    for (size_t i = 0; i < msgIDs.size(); i++)
        if (msgIDs[i] == msgId)
            return;
    msgIDs.push_back(msgId);

    // find device name from mac
    deviceName = NULL;
    for (size_t i = 0; i < devices.size(); i++)
    {
        const unsigned char *dmac = devices[i].first;
        // TODO change for loop to if(mac[0]==dmac[0] && ...)
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
    peer = p;
    // TODO maybe a vector for multiple events that should be processed
    // printf("SimpleEventHandler::newMessage finished: deviceName='%s', msgText='%s'\n", deviceName, msgText);

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

void SimpleEventHandler::clearEventData()
{
    // msgText[0] = '\0';
    msgText = NULL;
    deviceName = NULL;
    peer = NULL;
}
