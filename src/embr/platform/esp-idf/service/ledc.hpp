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
// WARNING: Due to:
// https://github.com/espressif/esp-idf/issues/4542
// You will have to expressly call above 'config' itself within an IRAM function, so that this
// generated code is also IRAM 
template <class Subject, class Impl>
IRAM_ATTR bool LEDControl::runtime<Subject, Impl>::callback(
    const ledc_cb_param_t* param,
    void* user_arg)
{
    bool woken = false;
    auto this_ = (runtime*) user_arg;
    this_->notify(event::callback{*param, &woken});
    return woken;
}


}}

}