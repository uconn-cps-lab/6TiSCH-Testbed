/*
 * Copyright (c) 2017, Universitat Oberta de Catalunya (UOC)
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
 * *  Neither the name of Universitat Oberta de Catalunya nor the names of
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

#ifndef HCT_H_
#define HCT_H_

#include <stdint.h>
#include <stdbool.h>

#define HCT_FIX_PKT_SIZE 80
#define HCT_RPY_SHIFT         6
#define HCT_ORG_SHIFT         7
#define HCT_ORGRPY_MASK       0xC0
#define HCT_SEQ_MASK          0x0F
#define HCT_H_CRC_LEN         2
#define HCT_P_CRC_LEN         2

#define TAG_COAP_SENSORS 0xf1
#define TAG_COAP_PING 0xf2
#define TAG_COAP_DIAGNOSIS 0xf3


typedef enum {
    HCT_CMD_CONFIG_GET  = 0x00,
    HCT_CMD_DATA_GET    = 0x01,
    HCT_CMD_ERROR       = 0x02
} HCT_MessageType_t;

typedef enum {
    HCT_ORG_NETPROC        = 1,
    HCT_ORG_FUSIONPROC     = 0,
    HCT_RPY_ACK_REQUIRED   = 1,
    HCT_RPY_NO_ACK         = 0,
} HCT_Flags_t;

typedef enum {
    HCT_TLV_TYPE_COAP_OBSERVE_PERIOD  = 0x10,
    HCT_TLV_TYPE_TEMP                 = 0x11,
    HCT_TLV_TYPE_HUM                  = 0x12,
    HCT_TLV_TYPE_LIGHT                = 0x13,
    HCT_TLV_TYPE_PRE                  = 0x14,
    HCT_TLV_TYPE_MOT                  = 0x15,
    HCT_TLV_TYPE_LED                  = 0x16,
    HCT_TLV_TYPE_CHANNEL              = 0x17,
    HCT_TLV_TYPE_BAT                  = 0x18,
    HCT_TLV_TYPE_EH                   = 0x19,
    HCT_TLV_TYPE_CC2650_POWER         = 0x1A,
    HCT_TLV_TYPE_MSP432_POWER         = 0x1B,
    HCT_TLV_TYPE_SENSOR_POWER         = 0x1C,
    HCT_TLV_TYPE_OTHER_POWER          = 0x1D,
    HCT_TLV_TYPE_SEQUENCE             = 0x1E,
    HCT_TLV_TYPE_ASN_STAMP            = 0x1F,
    HCT_TLV_TYPE_LAST_SENT_ASN        = 0x20, // Jiachen
    HCT_TLV_TYPE_COAP_DATA_TYPE       = 0X21, // Jiachen
    HCT_TLV_TYPE_RTT         = 0X22, // Jiachen
    HCT_TLV_TYPE_SLOT_OFFSET          = 0X23, // Jiachen
} HCT_TLVType_t;

typedef struct{
    uint8_t type;
    uint8_t flags;
    uint16_t length;
    uint16_t crcHeader;
    uint16_t crcPayload;
} HCT_Frame_t;

#define HCT_HEADER_LENGTH sizeof(HCT_Frame_t)

typedef struct{
    uint8_t type;
    uint8_t length;
    uint8_t * value;
} HCT_TLV_t;

/**
 *
 */
void HCTAddHeader(uint8_t* pkt, HCT_MessageType_t type,
                  HCT_Flags_t org, HCT_Flags_t rpy, uint8_t seq,
                  uint16_t len, uint16_t** crcHPos, uint16_t** crcPPos);

/**
 *
 */
bool HCTRetrieveHeader(uint8_t* pkt, HCT_Frame_t** header,
                       uint8_t** payload, uint16_t* payloadLength);

/**
 *
 */
void HCTAddTLV(uint8_t* pkt, uint8_t type, uint8_t len, uint8_t* value);

/**
 *
 */
uint8_t HCTParseTLV(uint8_t* pkt, HCT_TLV_t* tlv);

uint8_t HCTRetrieveTLV(uint8_t* pkt, HCT_TLV_t* tlv);

/**
 *
 */
void HCTRetrieveFlags(uint8_t flags, uint8_t* org, uint8_t* rpy, uint8_t* seq);

/**
 *
 */
void HCTComputeCRC(uint8_t * pkt, uint16_t len, uint16_t * crcPos);

#endif /* HCT_H_ */
