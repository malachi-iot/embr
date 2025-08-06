#define FEATURE_EMBR_WORD_STRICTNESS 0

#include <embr/word.h>
#include <embr/internal/word/v2/word.h>

#include "unit-test.h"

#include <esp_log.h>

static const char* TAG = "unity::word";

template <unsigned N, typename TInt>
static void test_word_basics()
{
    typedef embr::word<N> word_type;

    TEST_ASSERT_EQUAL(
        estd::numeric_limits<TInt>::digits,
        estd::numeric_limits<typename word_type::type>::digits);

    word_type w(5);

    TEST_ASSERT_EQUAL(5, w.value());

    w <<= 1;

    TEST_ASSERT_EQUAL(10, w.value());

    w |= embr::word<1>(1);

    TEST_ASSERT_EQUAL(11, w.value());
}

static void test_word_16bit()
{
    test_word_basics<16, uint16_t>();
    test_word_basics<11, uint16_t>();
}

static void test_word_32bit()
{
    {
        test_word_basics<32, uint32_t>();
    }

    {
        test_word_basics<24, uint32_t>();
    }
}


struct __attribute__((packed)) packer
{
    uint16_t v;
    embr::v2::word<16> words[4];
};


constexpr const embr::v2::word<16> words_rom[4] { 0, 1, 2, 3 };

// Alignment testing
static void test_v2_words(const embr::v2::word<16>* words)
{
    for(int i = 0; i < 4; i++, words++)
        TEST_ASSERT_EQUAL(i, words->value());
}

static void test_v2_word_16bit()
{
    using word_type = embr::v2::word<16>;
#if CONFIG_SPIRAM
    auto p = (packer*)heap_caps_malloc(sizeof(packer), MALLOC_CAP_SPIRAM);
    if(p == nullptr)
    {
        ESP_LOGD(TAG, "SPIRAM alloc failed, fallback to regular RAM");
        p = (packer*)malloc(sizeof(packer));
    }
    TEST_ASSERT_NOT_NULL(p);
#else
    packer _p;
    packer* p = &_p;
#endif

    // FIX: We have a warning about address of a packet member
    word_type* words = p->words;

    // Alignment testing
    for(int i = 0; i < 4; i++)
        words[i] = i;

    test_v2_words(words);
    test_v2_words(words_rom);

    word_type v(10);

    TEST_ASSERT_EQUAL(10, v.value());

#if CONFIG_SPIRAM
    free(p);
#endif
}

#ifdef ESP_IDF_TESTING
TEST_CASE("bit manipulator tests", "[bits]")
#else
void test_word()
#endif
{
    RUN_TEST(test_word_16bit);
    RUN_TEST(test_word_32bit);

    RUN_TEST(test_v2_word_16bit);
}
