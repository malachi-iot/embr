#include <catch2/catch.hpp>

#include <embr/observer.h>
#include <embr/property/v1/notifier.h>

#include "property-test.h"

using namespace embr;
using namespace embr::property::v1;

typedef int32_t int_type;

struct Source1 : PropertyContainer
{
    typedef Source1 this_type;

    enum Properties
    {
        VALUE1,
        VALUE2,
        VALUE3
    };

    enum Value2Values
    {
        Synthetic1,
        Synthetic2,
        Synthetic3
    };

    EMBR_PROPERTIES_BEGIN

        EMBR_PROPERTY_ID(value1, int_type, VALUE1, "value #1");
        EMBR_PROPERTY_ID(value2, Value2Values, VALUE2, "value #2");

    EMBR_PROPERTIES_END

    EMBR_PROPERTY_RUNTIME_BEGIN(PropertyContainer)

    protected:
        EMBR_PROPERTY(value1)
        EMBR_PROPERTY(value2)

    public:
        void emit()
        {
            value1(value1() + 1);
            value2(value2() == Synthetic1 ? Synthetic2 : Synthetic1);
        }

    EMBR_PROPERTY_RUNTIME_END
};

struct WithConstructor : PropertyContainer
{
    typedef WithConstructor this_type;

    const int value;

    WithConstructor(int v) : value(v) {}

    EMBR_PROPERTY_RUNTIME_BEGIN(PropertyContainer)

    runtime(int v) : base_type(v) {}

    EMBR_PROPERTY_RUNTIME_END
};

static int_type filter1_value1_value = 0;
static Source1::Value2Values filter1_value2_value;

class Filter1 : Filter1Base
{
public:
    template <class TSubject>
    class runtime : public PropertyNotifier<TSubject>
    {
        typedef PropertyNotifier<TSubject> base_type;

    public:
        void on_notify(event::PropertyChanged<Source1::id::value1> e)
        {
            filter1_value1_value = e.value;
        }

        void on_notify(event::PropertyChanged<Source1::Value2Values> e)
        {
            filter1_value2_value = e.value;
        }
    };
};

TEST_CASE("properties")
{
    SECTION("v1")
    {
        SECTION("event conversion")
        {
            typedef embr::layer0::subject<Filter1::runtime<void_subject> > subject_type;
            //typedef Filter1::runtime<subject_type> filter_type;
            typedef Source1::runtime<subject_type> source_type;

            source_type source;

            source.emit();

            REQUIRE(filter1_value1_value == 1);
            REQUIRE(filter1_value2_value == Source1::Synthetic2);
        }
        SECTION("with constructor")
        {
            WithConstructor::runtime<void_subject> s(10);

            REQUIRE(s.value == 10);
        }
    }
}