/*******************************************************************************
 *  Copyright (c) 2017 Texas Instruments Inc.
 *  All Rights Reserved This program is the confidential and proprietary
 *  product of Texas Instruments Inc.  Any Unauthorized use, reproduction or
 *  transfer of this program is strictly prohibited.
 *******************************************************************************
  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
 *******************************************************************************
 * FILE PURPOSE:
 *
 *******************************************************************************
 * DESCRIPTION:
 *
 *******************************************************************************
 * HISTORY:
 *
 ******************************************************************************/
#ifndef MAC_DEP_H
#define MAC_DEP_H

//MAC Layer dependencies
#define MAC_ATTR_ID_ROOT_SCHEDULE                           (0x4D)
#define MAC_ATTR_ID_PAN_ID                                  (0x50)
#define MAC_ATTR_ID_ASSOC_REQ_TIMEOUT_SEC                   (0x51)
#define MAC_ATTR_ID_SLOTFRAME_SIZE                          (0x52)
#define MAC_ATTR_ID_SHORT_ADDR                              (0x53)
#define MAC_ATTR_ID_BCN_CHAN                                (0x54)
#define MAC_ATTR_ID_MAC_PSK_07                              (0x55)
#define MAC_ATTR_ID_MAC_PSK_8F                              (0x56)
#define MAC_ATTR_ID_NVM                                     (0x58)
#define MAC_ATTR_ID_KEEPALIVE_PERIOD                        (0x5A)
#define MAC_ATTR_ID_COAP_RESOURCE_CHECK_TIME                (0x5B)
#define MAC_ATTR_ID_COAP_DEFAULT_PORT                       (0x5C)
#define MAC_ATTR_ID_COAP_DEFAULT_DTLS_PORT                  (0x60)
#define MAC_ATTR_ID_TX_POWER                                (0x61)
#define MAC_ATTR_ID_BCN_CH_MODE                             (0x62)
#define MAC_ATTR_ID_SCAN_INTERVAL                           (0x63)
#define MAC_ATTR_ID_NUM_SHARED_SLOT                         (0x64)
#define MAC_ATTR_ID_FIXED_CHANNEL_NUM                       (0x65)
#define MAC_ATTR_ID_RPL_DIO_DOUBLINGS                       (0x66)
#define MAC_ATTR_ID_COAP_OBS_MAX_NON                        (0x67)
#define MAC_ATTR_ID_COAP_DEFAULT_RESPONSE_TIMEOUT           (0x68)
#define MAC_ATTR_ID_DEBUG_LEVEL                             (0x69)
#define MAC_ATTR_ID_PHY_MODE                                (0x6A)
#define MAC_ATTR_ID_FIXED_PARENT                            (0x6B)
#define MAC_ATTR_ID_MAC_PAIR_KEY_ADDR                       (0x6C)
#define MAC_ATTR_ID_MAC_PAIR_KEY_07                         (0x6D)
#define MAC_ATTR_ID_MAC_PAIR_KEY_8F                         (0x6E)
#define MAC_ATTR_ID_MAC_DELETE_PAIR_KEY                     (0x6F)
#define MAC_ATTR_ID_EUI                                     (0xF1)

#define EUI_SIZE 8
#define EXTADDR_OFFSET  			0x2F0     // in FCFG; LSB..MSB

//from tsch_api.h
#define MAC_SUCCESS 				0x00
#define MAC_NO_ACK                  0xE9

extern uint16_t panId;
extern unsigned char node_mac[EUI_SIZE];


/* Generic MAC return values. */
enum
{
   /**< The MAC layer transmission was OK. */
   MAC_TX_OK,

   /**< The MAC layer transmission could not be performed due to a
      collision. */
   MAC_TX_COLLISION,

   /**< The MAC layer did not get an acknowledgement for the packet. */
   MAC_TX_NOACK,

   /**< The MAC layer deferred the transmission for a later time. */
   MAC_TX_DEFERRED,

   /**< The MAC layer transmission could not be performed because of an
      error. The upper layer may try again later. */
   MAC_TX_ERR,

   /**< The MAC layer transmission could not be performed because of a
      fatal error. The upper layer does not need to try again, as the
      error will be fatal then as well. */
   MAC_TX_ERR_FATAL,
};


#endif
