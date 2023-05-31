/*
 * Copyright (c) 2006 -2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  ====================== hct.h =============================================
 *  Host Control Transport API.
 */

#ifndef HCT_API_H
#define HCT_API_H

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__)
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <stdio.h>
#include <string.h>
#endif

#include <ti/drivers/UART.h>

#include "BM_api.h"

#include "tsch_api.h"
#include "ulpsmacaddr.h"
#include "mac_pib.h"
#include "nhldb-tsch-cmm.h"
#include "nhl_mlme_cb.h"
#include "nhl_mgmt.h"
#include "tsch_os_fun.h"

#include "nhl_api.h"


/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */


#define HCT_MSG_OFFSET_MSG_TYPE_TAG         0
#define HCT_MSG_OFFSET_MSG_LENGTH           2
#define HCT_MSG_SIZE_MSG_TYPE_TAG           2
#define HCT_MSG_SIZE_MSG_LENGTH             2
#define HCT_MSG_HDR_SIZE                    4

#define HCT_MSG_EXT_HDR_OFFSET_HEADER_CRC16                 0
#define HCT_MSG_EXT_HDR_OFFSET_PAYLOAD_CRC16                2
#define HCT_MSG_EXT_HDR_SIZE_HEADER_CRC16                   2
#define HCT_MSG_EXT_HDR_SIZE_PAYLOAD_CRC16                  2
#define HCT_MSG_EXT_HDR_SIZE                                4

/* message header tag definitions */
#define HCT_MSG_TYPE_MASK                   0x00ff
#define HCT_MSG_SEQ_MASK                    0x0f00
// reserved bit mask
#define HCT_MSG_RESERVED_MASK               0x3000
#define HCT_MSG_RESERVED_BIT_PRIMARY        (0x0000)
#define HCT_MSG_RESERVED_BIT_SECONDARY      (0x1000)

#define HCT_MSG_TAG_MASK                    0xf000
#define HCT_MSG_TAG_RESV_MASK               0x3000
#define HCT_MSG_TAG_ORG_MASK                0x8000
#define HCT_MSG_TAG_RPY_MASK                0x4000
#define HCT_MSG_TAG_ORG_MAC                 0x0000  // originated from the MAC (device)
#define HCT_MSG_TAG_ORG_HOST                0x8000  // originated from the Host (pc)
#define HCT_MSG_TAG_MSG_RPY_ACK             0x4000
#define HCT_MSG_TAG_MSG_RPY_NO_ACK          0x0000


/* message types */
#define HCT_MSG_TYPE_DATA_TRANSFER                          0x0000
#define HCT_MSG_TYPE_GET_SYSTEM_INFO                        0x0001
#define HCT_MSG_TYPE_GET_PHY_PIB                            0x0002
#define HCT_MSG_TYPE_GET_MAC_PIB                            0x0003
#define HCT_MSG_TYPE_SET_INFO                               0x0004
#define HCT_MSG_TYPE_SHUT_DOWN                              0x0005
#define HCT_MSG_TYPE_SETUP_ALARM                            0x0006
#define HCT_MSG_TYPE_ALARM                                  0x0007
#define HCT_MSG_TYPE_NETWORK_REGISTER                       0x0008
#define HCT_MSG_TYPE_NETWORK_START                          0x0008
#define HCT_MSG_TYPE_NETWORK_UNREGISTER                     0x0009
#define HCT_MSG_TYPE_CONNECT                                0x000a
#define HCT_MSG_TYPE_DISCONNECT                             0x000b
#define HCT_MSG_TYPE_LOAD_SYSTEM_CONFIG                     0x000c
#define HCT_MSG_TYPE_SET_MAC_PIB                            0x000d
#define HCT_MSG_TYPE_CLEAR_PHY_PIB                          0x000e
#define HCT_MSG_TYPE_CLEAR_MAC_PIB                          0x000f
#define HCT_MSG_TYPE_CL_ESTABLISH                           0x0010
#define HCT_MSG_TYPE_ATTACH                                 0x0010
#define HCT_MSG_TYPE_CL_RELEASE                             0x0011
#define HCT_MSG_TYPE_DETACH                                 0x0011
#define HCT_MSG_TYPE_DISCOVER                               0x0012
#define HCT_MSG_TYPE_FW_UPGRADE                             0x0013
#define HCT_MSG_TYPE_GET_INFO                               0x0014

#define HCT_MSG_TYPE_CONFIG_HCT_PKT                         0x0015

//New node schedule request and response
#define HCT_MSG_TYPE_SCHEDULE                               0x0016
//Slot add or remove request from HCT
#define HCT_MSG_TYPE_NHL_SCHEDULE                           0x0017

// post from ISR to NHL
#define HCT_MSG_TYPE_RX_RF_HCT_CONFIG                       0x00F0

#define HCT_MSG_TYPE_INVALID_MSG                            0x00ff

// API return codes (RCODE)
#define HCT_STATUS_SUCCESS           (0)
#define HCT_STATUS_FAILURE           (-1)
#define HCT_STATUS_ILLEGAL_PARAMS    (-2)
#define HCT_STATUS_ILLEGAL_OPERATION (-3) // operation illegal in current state
#define HCT_STATUS_HARDWARE_ERROR    (-4)
#define HCT_STATUS_WAIT_FOR_RESULT   (-5)
#define HCT_STATUS_UNSUPPORTED       (-6)


#define HW_PLATFORM_CC26XX                  (0)
#define HW_PLATFORM_CC13XX                  (1)

#define MAC_PROTOCOL_TSCH                   (0)

#define HCT_MSG_DATA_EVENT_ID_CONFIRM           (0x01)
#define HCT_MSG_DATA_EVENT_ID_INDICATION        (0x02)

#define HCT_MSG_DATA_INDICATION_DST_ADDR_MODE_MASK              (0x03)
#define HCT_MSG_DATA_INDICATION_DST_ADDR_MODE_SHIFT             (0)

#define HCT_MSG_DATA_INDICATION_SRC_ADDR_MODE_MASK              (0x03)
#define HCT_MSG_DATA_INDICATION_SRC_ADDR_MODE_SHIFT             (4)


#define HCT_PURGE_INPUT_TIME_MS             (500)

/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

// maximum TLV messages
#define HCT_MSG_TLV_LOAD_SYSTEM_CONFIG_COUNT        (3)

// LOAD SYSTEM_CONFIG TLV TYPE

#define LOAD_SYSTEM_CONFIG_TLV_TYPE_BEACON_FREQ                             (0x0001)
#define LOAD_SYSTEM_CONFIG_TLV_TYPE_SLOT_FRAME_SIZE                         (0x0002)
#define LOAD_SYSTEM_CONFIG_TLV_TYPE_NUMBER_SHARED_SLOT                      (0x0003)
#define LOAD_SYSTEM_CONFIG_TLV_TYPE_GATEWAY_ADDRESS                         (0x0004)

#define HCT_ATTACH_ASSOCIATION_REQUEST_FROM_INTERMEDIATE_NODE               (0x0003)


#define HCT_SET_INFO_TYPE_FSK_PHY                                           (0x0000)

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */

/* HCT_MSG_EXT_HEADER */
typedef struct
{
  uint16_t Header_CRC16;
  uint16_t Payload_CRC16;
} HCT_MSG_EXT_HEADER_s;

typedef struct _HCT_Object
{
    UART_Handle uart_hnd;

    // debug cnt
    uint32_t num_rx_to_host;
    uint32_t num_tx_from_host;
    uint16_t data_payload_len_in_two_buf;
    uint16_t numUartReadError;
    uint16_t numUartWriteError;
    uint16_t err_header_crc;
    uint16_t err_payload_crc;
    uint16_t err_tx_purge;
    uint16_t num_data_req_from_host;
    uint16_t num_data_conf_to_host;
    uint16_t num_data_ind_to_host;
    uint16_t num_get_pib_request;
    uint16_t num_set_pib_request;
    uint8_t num_rx_invalid_para;
    uint8_t err_tx_recv_msg;
    uint8_t err_unknown_msg;
    uint8_t err_tx_data1_len;
    uint8_t err_tx_data2_len;
    uint8_t err_unsupported_tlv;
    uint8_t rx_load_sys_config;
    uint8_t rx_rf_config;
} HCT_OBJ_s, *HCT_HND_s;

typedef struct __HCT_packet_buf
{
    uint8_t  msg_type;  // message type
    uint16_t msg_len;   // messge length
    BM_BUF_t *pdata1;   // first data  buffer
    BM_BUF_t *pdata2;   // second data buffer

} HCT_PKT_BUF_s;

typedef struct __hct_phy_info
{
    uint16_t    freq_band;
    uint16_t    channel;
    uint16_t    txPower;
    uint16_t    mode;
    uint16_t    dateRate;
} HCT_PHY_INFO_s;

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */

extern HCT_OBJ_s HCT_Obj;
extern HCT_HND_s HCT_Hnd;

/* ------------------------------------------------------------------------------------------------
 *                                           Function Prototypes
 * ------------------------------------------------------------------------------------------------
 */

int8_t HCT_RX_send(HCT_PKT_BUF_s *pPkt);

uint32_t HCT_TX_recv_msg(bool *pMsgErr,HCT_PKT_BUF_s *pPkt);

void HCT_IF_MAC_data_conf_proc(NHL_McpsDataConf_t *confArg);
void HCT_IF_MAC_data_req_proc(HCT_PKT_BUF_s *pPkt);
void NHL_dataIndCb(NHL_McpsDataInd_t* dataIndcArg);

void HCT_MSG_handle_message(HCT_PKT_BUF_s *pPkt);
void HCT_IF_MAC_post_msg_to_Host(uint8_t msgID,BM_BUF_t *payload);
void HCT_IF_MAC_set_pib_proc(HCT_PKT_BUF_s *pPkt);
void HCT_IF_MAC_get_pib_proc(HCT_PKT_BUF_s *pPkt);

void HCT_IF_load_system_config_req_proc(HCT_PKT_BUF_s *pPkt);
void HCT_IF_Shutdown_req_proc(HCT_PKT_BUF_s *pPkt);
void HCT_IF_Network_Start_req_proc(HCT_PKT_BUF_s *pPkt);
void HCT_IF_Attach_request_proc(HCT_PKT_BUF_s *pPkt);

void HCT_IF_MAC_Join_indc_proc(uint8_t* pEUI, uint16_t short_addr, uint8_t eventId);
void HCT_IF_MAC_Schedule_req_proc(uint8_t *pEUI, uint16_t childShortAddr, 
                                  uint16_t parentShortAddr,
                                  uint16_t nextHopShortAddr);
void HCT_IF_Schedule_resp_proc(HCT_PKT_BUF_s *pPkt);
void HCT_MSG_cb(uint16_t hct_msg,BM_BUF_t *ppayload);
uint16_t HCT_IF_proc_TSCH_Msg(NHLDB_TSCH_CMM_cmdmsg_t* cmm_msg);
void HCT_IF_NHL_Schedule_proc(HCT_PKT_BUF_s *pPkt);
/**************************************************************************************************
*/

#endif /* HCT_API_H */



