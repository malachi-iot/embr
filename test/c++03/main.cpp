#include <unit-test.h>

void setUp (void) {}
void tearDown (void) {}

int main()
{
    UNITY_BEGIN();
    test_bits();
    test_word();
    UNITY_END();

    return 0;
}