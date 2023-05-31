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

#include "hct.h"
#include <string.h>
#include "../nwp/crc16.h"


void HCTAddHeader(uint8_t* pkt, HCT_MessageType_t type,
                  HCT_Flags_t org, HCT_Flags_t rpy, uint8_t seq, uint16_t len,
                  uint16_t** crcHPos, uint16_t** crcPPos) {
    ((HCT_Frame_t *)pkt)->type  = type;

    /* org|rpy|00|seq */
    ((HCT_Frame_t *)pkt)->flags  = (uint8_t)((org << HCT_ORG_SHIFT)|(rpy << HCT_RPY_SHIFT)) & HCT_ORGRPY_MASK;
    ((HCT_Frame_t *)pkt)->flags |= seq & HCT_SEQ_MASK;
    ((HCT_Frame_t *)pkt)->length     = len;
    ((HCT_Frame_t *)pkt)->crcHeader  = 0;
    ((HCT_Frame_t *)pkt)->crcPayload = 0;

    /* Save position for CRC calculation */
    *crcHPos = &(((HCT_Frame_t *)pkt)->crcHeader);
    *crcPPos = &(((HCT_Frame_t *)pkt)->crcPayload);
}


bool HCTRetrieveHeader(uint8_t* pkt, HCT_Frame_t** header, uint8_t** payload, uint16_t* payloadLength) {
    uint16_t payload_length;
    uint16_t crc_header, crc_payload;

    /* Point header to start of buffer */
    *header = ((HCT_Frame_t *)pkt);

    /* Calculate header CRC */
    crc_header = getCRC16(INIT_CRC16, pkt, HCT_HEADER_LENGTH - HCT_H_CRC_LEN - HCT_P_CRC_LEN);

    /* If header CRC check passes */
    if (crc_header == (*header)->crcHeader) {

        /* Calculate packet length */
        /* TODO CHECK THIS IS CORRECT */
        payload_length = (*header)->length - HCT_H_CRC_LEN - HCT_P_CRC_LEN;

        /* If packet carries payload */
        if (payload_length > 0 && (*header)->crcPayload != 0) {

            /* Calculate payload CRC */
            crc_payload = getCRC16(INIT_CRC16, pkt + HCT_HEADER_LENGTH, payload_length);

            /* Check payload CRC */
            if (crc_payload == (*header)->crcPayload) {
                /* Set payload pointer and length */
                *payload = pkt + HCT_HEADER_LENGTH;
                *payloadLength = payload_length;

                return true;
            } else {
                return false;
            }
        }

        return true;
    }

    return false;
}

void HCTAddTLV(uint8_t* pkt, uint8_t type, uint8_t len, uint8_t* value) {
    pkt[0] = type;
    pkt[1] = len;
    memcpy(&pkt[2], value, len);
}

uint8_t HCTParseTLV(uint8_t* pkt, HCT_TLV_t* tlv){
    tlv->type   = pkt[0];
    tlv->length = pkt[1];
    tlv->value  = &pkt[2];

    return sizeof(tlv->type) + sizeof(tlv->length) + tlv->length;
}

uint8_t HCTRetrieveTLV(uint8_t* pkt, HCT_TLV_t* tlv){
    tlv->type   = pkt[0];
    tlv->length = pkt[1];
    tlv->value  = &pkt[2];

    return tlv->length;
}

void HCTRetrieveFlags(uint8_t flags, uint8_t * org, uint8_t * rpy, uint8_t * seq) {
    *org = (flags >> HCT_ORG_SHIFT)& 0x01;
    *rpy = (flags >> HCT_RPY_SHIFT)& 0x01;
    *seq = flags & HCT_SEQ_MASK;
}

void HCTComputeCRC(uint8_t * pkt, uint16_t len, uint16_t * crcPos) {
    uint16_t crc;
    crc = getCRC16(INIT_CRC16, pkt, len);
    *crcPos = crc;
}

