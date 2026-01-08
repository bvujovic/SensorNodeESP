//* https://piolabs.com/blog/insights/unit-testing-part-1.html

#include <unity.h>

#include "Enums.h"
#include <string.h>
#include "SimpleEventHandler.h"
SimpleEventHandler seh;
const unsigned char *mac = (const unsigned char *)"\x2C\xBC\xBB\x92\x1A\x34";
peer_info peer;

void setUp(void)
{
    memcpy(peer.peer_addr, mac, 6);
    // peer.peer_addr = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};
    peer.type = SimpleEvent;
    peer.device = ESP32DevKit;
}

void tearDown(void)
{
    // clean stuff up here
}

void newMessage()
{
    char msg1[] = "125";
    seh.newMessage(mac, msg1, &peer);
    TEST_ASSERT_FALSE(seh.isNewMessageReceived());
    msg1[0] = '\0';
    seh.newMessage(mac, msg1, &peer);
    TEST_ASSERT_FALSE(seh.isNewMessageReceived());
    
    char msg2[] = "123;HelloWorld";
    seh.newMessage(mac, msg2, &peer);
    TEST_ASSERT_TRUE(seh.isNewMessageReceived());
    TEST_ASSERT_EQUAL_STRING("Kitchen/Sink", seh.getDeviceName());
    TEST_ASSERT_EQUAL_STRING("HelloWorld", seh.getMessageText());
    seh.clearEventData();
    TEST_ASSERT_FALSE(seh.isNewMessageReceived());
    
    seh.newMessage(mac, msg2, &peer);
    TEST_ASSERT_FALSE(seh.isNewMessageReceived()); // duplicate msgID, should not update

    // TEST_ASSERT_EQUAL(4, sizeof(unsigned long));
    // TEST_ASSERT_EQUAL(4, sizeof(unsigned int));
}

void getters()
{
    char msg[] = "125;AnotherMessage";
    seh.newMessage(mac, msg, &peer);
    TEST_ASSERT_EQUAL_STRING("Kitchen/Sink", seh.getDeviceName());
    TEST_ASSERT_EQUAL_STRING("AnotherMessage", seh.getMessageText());
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(newMessage);
    RUN_TEST(getters);

    UNITY_END();
    return 0;
}
