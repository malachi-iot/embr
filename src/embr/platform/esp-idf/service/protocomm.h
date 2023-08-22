#pragma once

#include <estd/span.h>

#include <protocomm.h>

namespace embr { namespace esp_idf { namespace service { inline namespace v1 {

class Protocomm : public embr::service::v1::Service
{
    using this_type = Protocomm;

protected:
    static constexpr const char* TAG = "Protocomm";

    protocomm_t* pc;

    state_result on_stop();

public:
    static constexpr const char* name() { return TAG; }

    struct event
    {
        struct request
        {
            using in_type = estd::span<const uint8_t>;

            const uint32_t session_id;
            const in_type in;
            uint8_t** outbuf;
            ssize_t* outlen;
            esp_err_t ret;
        };

        template <class T>
        struct tag : request
        {
            constexpr tag(const uint32_t session_id,
                const uint8_t* inbuf, ssize_t inlen,
                uint8_t** outbuf, ssize_t* outlen) :
                request{session_id, in_type{inbuf, (size_t)inlen}, outbuf, outlen, ESP_OK}
            {}
        };
    };

    esp_err_t remove_endpoint(const char* ep_name)
    {
        return protocomm_remove_endpoint(pc, ep_name);
    }

    esp_err_t set_security(const char* ep_name, const protocomm_security_t* sec, const void* sec_params);
    esp_err_t unset_security(const char* ep_name);

    //esp_err_t set_security(const char* ep_name, const protocomm_security1_params_t*);
    //esp_err_t set_security(const char* ep_name, const protocomm_security2_params_t*);

    EMBR_SERVICE_RUNTIME_BEGIN(embr::service::v1::Service)

    state_result on_start();

    template <class Tag>
    static esp_err_t endpoint(uint32_t, const uint8_t*, ssize_t, uint8_t**, ssize_t*, void*);

    esp_err_t add_endpoint(const char*, protocomm_req_handler_t h, void*);

    template <class Tag>
    esp_err_t add_endpoint(const char*);

    EMBR_SERVICE_RUNTIME_END
};


}}}}