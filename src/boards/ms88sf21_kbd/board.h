/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Kazuyuki Arimatsu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef MS88SF21_KBD_H
#define MS88SF21_KBD_H

/*------------------------------------------------------------------*/
/* Power
 *------------------------------------------------------------------*/

#define UICR_REGOUT0_VALUE  UICR_REGOUT0_VOUT_3V3
#define ENABLE_DCDC_0       1
#define ENABLE_DCDC_1       1

/*------------------------------------------------------------------*/
/* LED
 *------------------------------------------------------------------*/

#define LEDS_NUMBER       1
#define LED_PRIMARY_PIN   PINNUM(0, 26)
#define LED_STATE_ON      1

/*------------------------------------------------------------------*/
/* BUTTON
 *------------------------------------------------------------------*/

//--------------------------------------------------------------------+
// BLE OTA
//--------------------------------------------------------------------+
#define BLEDIS_MANUFACTURER  "BP"
#define BLEDIS_MODEL         "MS88SF21 KBD"

//--------------------------------------------------------------------+
// USB
//--------------------------------------------------------------------+
#define USB_DESC_VID           0xBF30U
#define USB_DESC_UF2_PID       0xC0FEU
#define USB_DESC_CDC_ONLY_PID  0xC0FEU

#define UF2_PRODUCT_NAME    "MS88SF21 KBD"
#define UF2_VOLUME_LABEL    "MS88BOOT"
#define UF2_BOARD_ID        "MS88SF21_KBD"
#define UF2_INDEX_URL       "https://x.com/BParound30"

#endif // MS88SF21_KBD_H
