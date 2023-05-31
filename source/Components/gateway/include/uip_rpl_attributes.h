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
#ifndef __UIP_RPL_ATTRIBUTES_H__
#define __UIP_RPL_ATTRIBUTES_H__

//Default values
#define UIP_RPL_ATTR_DIO_INTERVAL_MIN     12
#define UIP_RPL_ATTR_DIO_INTERVAL_DOUBLINGS  8
#define UIP_RPL_ATTR_DAO_LATENCY          4
#define UIP_RPL_ATTR_PREFIX                     0x20,0x01,0x0d,0xb8,0x12,0x34,0xff,0xff

#define UIP_RPL_ATTR_PREFIX_LEN                 (8)
#define UIP_RPL_ATTR_ROUTING_TABLE_ENTRY_LEN    (38)

#define UIP_RPL_ATTR_NBR_TABLE_ENTRY_LEN        (21)

typedef enum
{
   UIP_RPL_ATTR_OK = 0,
   UIP_RPL_ATTR_ERROR_INVALID_INDEX,
   UIP_RPL_ATTR_ERROR_STACK_STARTED,
   UIP_RPL_ATTR_ERROR_STACK_NOT_STARTED,
   UIP_RPL_ATTR_ERROR
} uip_rpl_attr_status;

typedef struct
{
   uint8_t       dio_interval_min;
   uint8_t       dio_interval_doublings;
   uint32_t      dao_latency;
   uint8_t       prefix[UIP_RPL_ATTR_PREFIX_LEN];
} uip_rpl_attr_t;

extern uip_rpl_attr_t uip_rpl_attributes;

//The UIP RPL attributes must be init before the Stack is initiated
void uip_rpl_attr_init();

//This value is the current DIO transmission delay. Cannot be obtained if the stack was not started.
uip_rpl_attr_status uip_rpl_attr_get_dio_next_delay(uint32_t *delay);

//This is the IPv6 routing table. Cannot be obtained if the stack was not started.
uip_rpl_attr_status uip_rpl_attr_get_ip_routing_table_entry(uint16_t index, char* entry);

//This is the IPv6 neighbor table. Cannot be obtained if the stack was not started.
uip_rpl_attr_status uip_rpl_attr_get_neighbor_table_entry(uint16_t index, char* entry);

//This is the RPL DIO interval min parameter.
uip_rpl_attr_status uip_rpl_attr_get_dio_interval_min(uint8_t *int_min);

//This is the RPL DIO interval min parameter. Cannot be set of the stack was already started
uip_rpl_attr_status uip_rpl_attr_set_dio_interval_min(uint8_t int_min);

//This is the RPL DIO interval doublings parameter.
uip_rpl_attr_status uip_rpl_attr_get_dio_interval_doublings(uint8_t *int_doublings);

//This is the RPL DIO interval doublings parameter. Cannot be set of the stack was already started
uip_rpl_attr_status uip_rpl_attr_set_dio_interval_doublings(uint8_t int_doublings);

//This is the RPL DAO latency. The DAO messages will be scheduled to be sent within a random timer between dao_latency and dao_latency/2.
uip_rpl_attr_status uip_rpl_attr_get_dao_latency(uint8_t *dao_latency);

//This is the RPL DAO latency.
uip_rpl_attr_status uip_rpl_attr_set_dao_latency(uint8_t dao_latency);

//This is the IPv6 prefix used for RPL Root. For every node it will be set as 6LoWPAN Context 0 for header compression
uip_rpl_attr_status uip_rpl_attr_get_ip_global_prefix(void* prefix);

//This is the IPv6 prefix used for RPL Root.
uip_rpl_attr_status uip_rpl_attr_set_ip_global_prefix(void* prefix);

#endif