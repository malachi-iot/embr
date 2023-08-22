#include "protocomm.h"

namespace embr { namespace esp_idf { namespace service { inline namespace v1 {

template <class TSubject, class TImpl>
auto Protocomm::runtime<TSubject, TImpl>::on_start() -> state_result
{
    base_type::pc = protocomm_new();

    if(base_type::pc == nullptr) return { Error, ErrMemory };

    return state_result::started();
}


inline auto Protocomm::on_stop() -> state_result
{
    protocomm_delete(pc);

    return { Stopped, Finished };
}

template <class TSubject, class TImpl>
esp_err_t Protocomm::runtime<TSubject, TImpl>::add_endpoint(const char* ep_name, protocomm_req_handler_t h, void* priv_data)
{
    esp_err_t e = protocomm_add_endpoint(base_type::pc, ep_name, h, priv_data);

    if(e == ESP_OK) return ESP_OK;

    if(state() == Started)  state(Degraded);

    return e;
}


template <class TSubject, class TImpl>
template <class Tag>
esp_err_t Protocomm::runtime<TSubject, TImpl>::add_endpoint(const char* ep_name)
{
    protocomm_req_handler_t h = endpoint<Tag>;
    return add_endpoint(ep_name, h, this);
}


template <class TSubject, class TImpl>
template <class Tag>
esp_err_t Protocomm::runtime<TSubject, TImpl>::endpoint(
    uint32_t session_id,
    const uint8_t* inbuf,
    ssize_t inlen,
    uint8_t** outbuf,
    ssize_t* outlen,
    void* priv_data)
{
    // DEBT: Not sure why negative inlen is a thing -- maybe to indicate no payload?
    //event::request r{session_id, event::request::in_type{inbuf, (size_t)inlen}, outbuf, outlen};
    event::tag<Tag> e{session_id, inbuf, inlen, outbuf, outlen};
    ((runtime*) priv_data)->notify(e);
    return e.ret;
}



}}}}