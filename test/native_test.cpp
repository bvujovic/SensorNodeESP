//* https://piolabs.com/blog/insights/unit-testing-part-1.html

#include <unity.h>

#include "Enums.h"
#include "ToString.h"
#include <string.h>
#include "SimpleEventHandler.h"
SimpleEventHandler seh;
peer_info peer;

void setUp(void)
{
    const unsigned char *mac = (const unsigned char *)"\x2C\xBC\xBB\x92\x1A\x34";
    memcpy(peer.peer_addr, mac, 6);
    // peer.peer_addr = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};
    peer.type = SimpleEvent;
    peer.device = ESP32BattConn;
}

void tearDown(void)
{
    // clean stuff up here
}

void newMessage()
{
    char msg1[] = "125";
    seh.newMessage(msg1, &peer);
    TEST_ASSERT_FALSE(seh.isNewMessageReceived());
    msg1[0] = '\0';
    seh.newMessage(msg1, &peer);
    TEST_ASSERT_FALSE(seh.isNewMessageReceived());
    
    char msg2[] = "123;HelloWorld";
    seh.newMessage(msg2, &peer);
    TEST_ASSERT_TRUE(seh.isNewMessageReceived());
    TEST_ASSERT_EQUAL_STRING("HelloWorld", seh.getMessageText());
    seh.clearEventData();
    TEST_ASSERT_FALSE(seh.isNewMessageReceived());
    
    seh.newMessage(msg2, &peer);
    TEST_ASSERT_FALSE(seh.isNewMessageReceived()); // duplicate msgID, should not update

    // TEST_ASSERT_EQUAL(4, sizeof(unsigned long));
    // TEST_ASSERT_EQUAL(4, sizeof(unsigned int));
}

void getters()
{
    char msg[] = "125;AnotherMessage";
    seh.newMessage(msg, &peer);
    TEST_ASSERT_EQUAL_STRING("Kitchen/Sink", ToString::Devices[seh.getPeerInfo()->device]);
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
