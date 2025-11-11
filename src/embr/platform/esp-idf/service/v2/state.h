#include <esp_event.h>

namespace embr::esp_idf::event {

template <class Data>
esp_err_t post(
    esp_event_base_t event_base,
    int32_t event_id,
    Data* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post(
        event_base,
        event_id,
        event_data,
        sizeof(Data),
        ticks_to_wait);
}


template <class Data>
esp_err_t post_to(
    esp_event_loop_handle_t el,
    esp_event_base_t event_base,
    int32_t event_id,
    Data* event_data,
    TickType_t ticks_to_wait = portMAX_DELAY)
{
    return esp_event_post_to(
        el,
        event_base,
        event_id,
        event_data,
        sizeof(Data),
        ticks_to_wait);
}

}

namespace embr::esp_idf::service::inline v2 {

namespace detail {

template <class State, class Origin, Origin* o = {}>
struct state_base_traits
{
    using state_type = State;
    using origin_type = Origin;

    //static constexpr esp_event_base_t event_base = eb;
    static constexpr origin_type* origin = o;
};

template <class Traits>
class state_base
{
    using traits = Traits;
    using origin_type = typename traits::origin_type;
    using state_type = typename traits::state_type;

    state_type state_;

public:
    struct event_data
    {
        origin_type& origin;
        state_type changing_state;
        state_type changed_state;
    };

private:
    //void post(esp_event_loop_handle_t el, esp_event_base_t event_base)
    //{
    //}

public:
    void set(state_type v, origin_type& origin, esp_event_base_t event_base, int32_t event_id)
    {
        event_data e{origin, state_, v};

        event::post(event_base, event_id, &e);
        state_ = v;
        event::post(event_base, event_id + 1, &e);
    }

    constexpr state_type get() const
    {
        return state_;
    }

    state_base& operator =(state_type v)
    {
        static_assert(traits::o != nullptr, "Origin pointer must be provided in traits");
    
        state_ = v;
        return *this;
    }

    constexpr operator state_type() const
    {
        return state_;
    }
};

}

}
