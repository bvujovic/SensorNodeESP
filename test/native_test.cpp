//* https://piolabs.com/blog/insights/unit-testing-part-1.html

#include <unity.h>

#include "SimpleEventHandler.h"
SimpleEventHandler seh;
const unsigned char * mac = (const unsigned char *)"\x24\x6F\x28\xAA\xBB\xCC";

void setUp(void)
{
    // set stuff up here
}

void tearDown(void)
{
    // clean stuff up here
}

void newMessage()
{
    TEST_ASSERT_FALSE(seh.newMessage("abc", mac));
    TEST_ASSERT_FALSE(seh.newMessage("", mac));
    TEST_ASSERT_TRUE(seh.newMessage("123;HelloWorld", mac));
    TEST_ASSERT_FALSE(seh.newMessage("123;ASD", mac)); // duplicate msgID
    TEST_ASSERT_TRUE(seh.newMessage("124;AnotherMessage", mac));

    // TEST_ASSERT_EQUAL_STRING("Kitchen/Sink", seh.getDeviceName());
    // TEST_ASSERT_EQUAL_STRING("AnotherMessage", seh.getMessageText());

    // TEST_ASSERT_EQUAL(4, sizeof(unsigned long));
    // TEST_ASSERT_EQUAL(4, sizeof(unsigned int));
}

void getters()
{
    TEST_ASSERT_TRUE(seh.newMessage("125;AnotherMessage", mac));
    TEST_ASSERT_EQUAL_STRING("Kitchen/Sink", seh.getDeviceName());
    TEST_ASSERT_EQUAL_STRING("AnotherMessage", seh.getMessageText());
}

// #include <stdio.h>
// int SleepSeconds(int m, int s, int itv)
// {
//     if (itv % 60 != 0) // ako itv ne predstavlja tacan broj minuta, onda nema namestanja na pocetak minuta
//         return itv;
//     int x = itv / 60;                     // 300/60 = 5
//     int nextMin = m / x * x + x;    // 56/5 * 5 + 5 = 11*5 + 5 = 55 + 5 = 60
//     // printf("%d\n", nextMin);
//     int min = nextMin - m;                // 60 - 56 = 4
//     // printf("%d\n", min);
//     int sec = min * 60 - s;               // 4*60 - 30 = 240 - 30 = 210
//     return sec;
// }

// void TestSS()
// {
//     TEST_ASSERT_EQUAL(15, SleepSeconds(5, 45, 15));
//     TEST_ASSERT_EQUAL(30, SleepSeconds(25, 45, 30));

//     TEST_ASSERT_EQUAL(3 * 60 + 30, SleepSeconds(56, 30, 300));
//     TEST_ASSERT_EQUAL(2 * 60 + 00, SleepSeconds(58, 00, 300));
//     TEST_ASSERT_EQUAL(0 * 60 + 15, SleepSeconds(59, 45, 300));
//     TEST_ASSERT_EQUAL(3 * 60 + 30, SleepSeconds(16, 30, 300));
//     TEST_ASSERT_EQUAL(2 * 60 + 00, SleepSeconds(38, 00, 300));
//     TEST_ASSERT_EQUAL(0 * 60 + 15, SleepSeconds(29, 45, 300));

//     TEST_ASSERT_EQUAL(4 * 60 + 15, SleepSeconds(0, 45, 300));
//     TEST_ASSERT_EQUAL(0 * 60 + 15, SleepSeconds(0, 45, 60));
//     TEST_ASSERT_EQUAL(1 * 60 + 45, SleepSeconds(02, 15, 120));
// }

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    // RUN_TEST(TestSS);
    RUN_TEST(newMessage);
    RUN_TEST(getters);

    UNITY_END();
    return 0;
}
