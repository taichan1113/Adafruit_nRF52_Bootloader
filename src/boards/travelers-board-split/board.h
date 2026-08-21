/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2026
 */

#ifndef _TRAVELERS_BOARD_SPLIT_H_
#define _TRAVELERS_BOARD_SPLIT_H_

/*
 * No status LED or DFU button is defined until its physical connection is
 * verified. Enter USB DFU using a double reset instead.
 */
#define LEDS_NUMBER 0

/*
 * BLE DFU is not an update path for this board. These strings are required by
 * the bootloader's compiled-in BLE DFU service, but do not enable BLE DFU.
 */
#define BLEDIS_MANUFACTURER "Travelers Board"
#define BLEDIS_MODEL        "travelers-board-split (NINA-B302-00B)"

/*
 * Temporary private-test VID/PID only. 0x1209:0x000A is not unique and must
 * be replaced with an assigned VID/PID before the bootloader is distributed.
 */
#define USB_DESC_VID          0x1209
#define USB_DESC_UF2_PID      0x000A
#define USB_DESC_CDC_ONLY_PID 0x000A

#define UF2_PRODUCT_NAME "travelers-board-split"
#define UF2_VOLUME_LABEL "TRAVELERS"
#define UF2_BOARD_ID     "nRF52840-travelers-board-split-v1"
#define UF2_INDEX_URL    "https://github.com/"

#endif // _TRAVELERS_BOARD_SPLIT_H_
