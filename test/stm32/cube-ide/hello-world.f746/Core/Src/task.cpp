/*
 * @file
 * task.cpp
 *
 *  Created on: Dec 11, 2019
 *      Author: Malachi
 */

// Reference: http://shawnhymel.com/1795/getting-started-with-stm32-nucleo-usb-virtual-com-port/
// Good guidance here: https://stackoverflow.com/questions/33549084/stm32cubemx-usb-cdc-vcp/33555364#33555364
// (we want to do device and not host mode, for now)

#include "usbd_cdc_if.h"
#include "cmsis_os.h"
#include "usb_device.h"

// Just to test we're in fact getting *something* -- but looking good
//#include <estd/string.h>
// OK, these *were* working but symlinking lied to me and I accidentally deleted the
// support code.  The changes are:
// 1. add a platform-detector in estd
// 2. stringconvert.cpp: notice it's ARM/STM32 platform to enable itoa and friends
// 3. stringconvert.cpp: notice it's STM32 and disable the sprintf float code

extern "C" void hello_task()
{
    uint8_t buffer[] = "Hello, World!\r\n";

    //estd::layer2::const_string s = "Hi2u:"; // the ptr-to-null-terminated variety
    //uint8_t* data = reinterpret_cast<uint8_t*>(const_cast<char*>(s.data()));

    /* Infinite loop */
    for(;;)
    {
        // Verified works after a few seconds of bringup, miniterm.py in debian
        // can see our Hello, World! though there's some junk inbetween

        //CDC_Transmit_FS(data, s.length());
        //osDelay(500);
        CDC_Transmit_FS(buffer, sizeof(buffer) - 1);

        osDelay(3000);
    }
}


