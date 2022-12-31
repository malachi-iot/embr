#include "unity.h"

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
    static void log(esp_err_t e)
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(e);
    }
    static void assert_(esp_err_t e)
    {
        ESP_ERROR_CHECK(e);
    }
};

template <class T>
struct scoped_guard_traits
{
    typedef estd::monostate status_type;
};

template <class T, bool with_status = false>
class scoped_guard_base;

template <class T>
class scoped_guard_base<T, false>
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

    reference value() { return value_; }

    void status(status_type s)
    {
        status_traits::assert_(s);
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

template <class T>
class scoped_guard_base<T, true> : public scoped_guard_base<T, false>
{
    typedef scoped_guard_base<T, false> base_type;

public:
    typedef typename base_type::guard_traits guard_traits;
    typedef typename guard_traits::status_type status_type;
    typedef typename base_type::status_traits status_traits;

protected:
    status_type status_;

    void status(status_type s)
    {
        status_ = s;
        status_traits::log(s);
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
    public scoped_guard_base<embr::esp_idf::pm_lock, fail_action != SCOPED_GUARD_ASSERT>
{
    typedef scoped_guard_base<embr::esp_idf::pm_lock, fail_action != SCOPED_GUARD_ASSERT> base_type;

public:
    scoped_guard(esp_pm_lock_type_t type, int arg = 0, const char* name = nullptr)
    {
        base_type::status(base_type::value_.create(type, arg, name));
    }

    ~scoped_guard()
    {
        base_type::status(base_type::value_.free());
    }
};


TEST_CASE("power management", "[esp_pm]")
{
    esp_chip_id_t id = ESP_CHIP_ID_ESP32;

    scoped_guard<pm_lock, SCOPED_GUARD_WARN> pml(ESP_PM_NO_LIGHT_SLEEP);

    TEST_ASSERT_TRUE(pml.good());
    
    pml->acquire();
    pml->release();
}

