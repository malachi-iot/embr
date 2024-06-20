#pragma once

#include <tinyusb.h>
#include <tusb_cdc_acm.h>
#include <tusb.h>

#include <estd/streambuf.h>

// Initial guidance from
// https://github.com/espressif/esp-idf/blob/v5.1.4/examples/peripherals/usb/device/tusb_serial_device/main/tusb_serial_device_main.c
// https://github.com/hathach/tinyusb/blob/0.15.0/examples/device/cdc_dual_ports/src/usb_descriptors.c

// "Documentation"
// https://github.com/hathach/tinyusb/blob/0.15.0/src/class/cdc/cdc_device.h

namespace embr { namespace tinyusb { inline namespace v1 {

namespace impl {

template <class CharTraits>
struct cdc_streambuf_base
{
    uint8_t itf_;

    using traits_type = CharTraits;

    typedef typename traits_type::char_type char_type;
    typedef typename traits_type::int_type int_type;

};

}

}}}