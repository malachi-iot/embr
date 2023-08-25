#pragma once

#include "ledc.h"

namespace embr::esp_idf {

namespace service { inline namespace v1 {

template <class Subject, class Impl>
void LEDControl::runtime<Subject, Impl>::config(const ledc_channel_config_t& c)
{
    configuring(&c);

    ledc& v = base_type::ledc_[c.channel];

    v.config(c);

    ledc_cbs_t cbs { callback };
    v.cb_register(&cbs, this);

    configured(&c);
}


// Be advised this is called in ISR context!
// FIX: We still get warning this isn't in IRAM.  I forgot the nuance to this,
// there's some trickery with templated methods
template <class Subject, class Impl>
IRAM_ATTR bool LEDControl::runtime<Subject, Impl>::callback(
    const ledc_cb_param_t* param,
    void* user_arg)
{
    bool woken = false;
    auto this_ = (runtime*) user_arg;
    //this_->notify(event::callback{*param, &woken});
    return woken;
}


}}

}