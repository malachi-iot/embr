#include "unit-test.h"

int main()
{
    UNITY_BEGIN();
    test_bits();
    test_delegate_queue();
    test_observer();
    test_word();
    UNITY_END();

    return 0;
}
