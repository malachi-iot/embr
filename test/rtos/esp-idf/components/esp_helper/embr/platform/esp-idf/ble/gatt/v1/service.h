#pragma once

// Bluedroid specific

#include <esp_gatts_api.h>
#include <cstring>

namespace embr::esp_idf { inline namespace ble {
namespace gatt { inline namespace v1 {

class Service
{
    const esp_gatts_attr_db_t* db_;
    uint16_t* handles_;
    int id_;

    using evt_param = esp_ble_gatts_cb_param_t::gatts_add_attr_tab_evt_param;

public:
    constexpr Service(int id, const esp_gatts_attr_db_t* db, uint16_t* handles) :
        db_{db},
        handles_{handles},
        id_{id}
    {}

    constexpr int id() const { return id_; }

    void init(const evt_param& add_attr_tab) const
    {
        // DEBT: I wonder if handles_ is in fact dynamically allocated by Bluedroid
        // gatts system itself?  Maybe, maybe not.  Might be affected by GATT_DYNAMIC_MEMORY
        // If it is (there's some evidence that it is since Kconfig asks for maximum handles)
        // then this memcpy may be unnecessary and perhaps we merely assign handles_
        std::memcpy(handles_, add_attr_tab.handles, add_attr_tab.num_handle);
    }

    esp_err_t start() const
    {
        return esp_ble_gatts_start_service(handles_[0]);
    }

    esp_err_t create_attr_tab(int gatts_if, int max) const
    {
        return esp_ble_gatts_create_attr_tab(db_, gatts_if, max, id_);
    }

    bool on_create_attr_tab_event(
        const evt_param& add_attr_tab) const
    {
        if(add_attr_tab.svc_inst_id != id()) return false;

        init(add_attr_tab);
        start();

        return true;
    }

    bool valid(const evt_param& add_attr_tab, int count) const
    {
        // If not checking this service, it's automatically valid
        if (add_attr_tab.svc_inst_id != id()) return true;

        return add_attr_tab.num_handle == count;
    }
};

}}
}}