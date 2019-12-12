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

extern "C" void hello_task()
{
    uint8_t buffer[] = "Hello, World!\r\n";

    /* Infinite loop */
    for(;;)
    {
        // Verified works after a few seconds of bringup, miniterm.py in debian
        // can see our Hello, World! though there's some junk inbetween
        CDC_Transmit_FS(buffer, sizeof(buffer));

        osDelay(3000);
    }
}


