/*
 * @file
 * task.cpp
 *
 *  Created on: Dec 11, 2019
 *      Author: Malachi
 */

// Reference: http://shawnhymel.com/1795/getting-started-with-stm32-nucleo-usb-virtual-com-port/

#include "usbh_cdc.h"

extern USBH_HandleTypeDef hUsbHostFS;

extern "C" void hello_task()
{
    uint8_t buffer[] = "Hello, World!\r\n";

    // Not necessary, as MX_USB_HOST_Init does this for us
    //USBH_Start(&hUsbHostFS);

    /* Infinite loop */
    for(;;)
    {
        USBH_StatusTypeDef usb_status;

        usb_status = USBH_CDC_Transmit(&hUsbHostFS, buffer, sizeof(buffer));

        osDelay(3000);
    }
}


