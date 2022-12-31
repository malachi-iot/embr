#include "unity.h"

#include <esp_log.h>

#include <estd/variant.h>

#include <embr/platform/esp-idf/timer.h>
#include <embr/platform/esp-idf/app_format.h>
#include <embr/platform/esp-idf/pm.h>

using namespace embr::esp_idf;

enum scoped_guard_fail_action
{
    SCOPED_GUARD_ASSERT = 0,

    // These two retain status
    SCOPED_GUARD_WARN = 1,
    SCOPED_GUARD_SILENT = 2
};

// DEBT: I've done something like this elsewhere, but this naming matches
// std c++11 fairly well so cascade this out to estd
// See: https://en.cppreference.com/w/cpp/thread/scoped_lock for similar naming
// All that said, something like 'unique_value' would match the expected behavior
// better, so maybe consider that (ala unique_ptr)

// NOTE: Using scoped_guard implies an assert/exception style behavior - expect
// the system to halt if ctor/dtor is not fully successful
template <class T, scoped_guard_fail_action = SCOPED_GUARD_WARN>
class scoped_guard;

template <class T>
struct scoped_status_traits
{
    constexpr static bool good(T) { return true; }
    constexpr static void log(T) {}
    constexpr static void assert_(T) {}
};

template <>
struct scoped_status_traits<esp_err_t>
{
    static bool good(esp_err_t e) { return e == ESP_OK; }
    inline static void log(esp_err_t e)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(e);
    }
    inline static void assert_(esp_err_t e)
    {
        ESP_ERROR_CHECK(e);
    }
};

template <class T>
struct scoped_guard_traits
{
    typedef estd::monostate status_type;
};



template <class T, bool with_status, scoped_guard_fail_action action>
class scoped_guard_base;

template <class T, scoped_guard_fail_action action>
class scoped_guard_base<T, false, action>
{
public:
    typedef T value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef scoped_guard_traits<T> guard_traits;
    typedef typename guard_traits::status_type status_type;
    typedef scoped_status_traits<status_type> status_traits;

protected:
    T value_;

    ESTD_CPP_CONSTEXPR_RET reference value() { return value_; }

    void status(status_type s)
    {
        switch(action)
        {
            case SCOPED_GUARD_ASSERT:
                status_traits::assert_(s);
                break;

            case SCOPED_GUARD_WARN:
                status_traits::log(s);
                break;

            // implicit action is silent
            default:    break;
        }
    }

public:
    //operator reference() { return value_; }
    //operator const_reference() const { return value_; }
    ESTD_CPP_CONSTEXPR_RET const_pointer get() const { return &value_; }

    ESTD_CPP_CONSTEXPR_RET const_pointer operator*() { return get(); }
    ESTD_CPP_CONSTEXPR_RET pointer operator->() { return &value_; }
    ESTD_CPP_CONSTEXPR_RET const_pointer operator->() const { return &value_; }

    // Success is always implied when no status is specifically tracked
    constexpr bool good() const { return true; }
};

template <class T, scoped_guard_fail_action action>
class scoped_guard_base<T, true, action> : public scoped_guard_base<T, false, action>
{
    typedef scoped_guard_base<T, false, action> base_type;

public:
    typedef typename base_type::guard_traits guard_traits;
    typedef typename guard_traits::status_type status_type;
    typedef typename base_type::status_traits status_traits;

protected:
    status_type status_;

    void status(status_type s)
    {
        status_ = s;
        base_type::status(s);
    }

public:
    constexpr status_type status() const { return status_; }
    constexpr bool good() const { return status_traits::good(status_); }
};

template <>
struct scoped_guard_traits<embr::esp_idf::pm_lock>
{
    typedef esp_err_t status_type;
    //typedef scoped_status_traits<esp_err_t> status_traits;
};


template <scoped_guard_fail_action fail_action>
class scoped_guard<embr::esp_idf::pm_lock, fail_action> :
    public scoped_guard_base<embr::esp_idf::pm_lock, fail_action != SCOPED_GUARD_ASSERT, fail_action>
{
    typedef scoped_guard_base<embr::esp_idf::pm_lock, fail_action != SCOPED_GUARD_ASSERT, fail_action> base_type;

public:
    scoped_guard(esp_pm_lock_type_t type, int arg = 0, const char* name = nullptr)
    {
        base_type::status(base_type::value().create(type, arg, name));
    }

    ~scoped_guard()
    {
        base_type::status(base_type::value().free());
    }
};



TEST_CASE("power management", "[esp_pm]")
{
    static const char* TAG = "esp_pm test";

    typedef chip_traits<chip_id()> chip_traits;

    ESP_LOGI(TAG, "chip_id=%s", chip_traits::name());

    scoped_guard<pm_lock, SCOPED_GUARD_SILENT> pml(ESP_PM_NO_LIGHT_SLEEP);

#if CONFIG_PM_ENABLE
    TEST_ASSERT_EQUAL(ESP_OK, pml.status());
    TEST_ASSERT_TRUE(pml.good());
    
    pml->acquire();
    pml->release();
#else
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_SUPPORTED, pml.status());
#endif
}

