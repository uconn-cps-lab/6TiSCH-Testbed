/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
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

#include "nwp_msp432.h"

uint16_t nwpConfigGet(uint8_t* spiTransmitBuffer, uint8_t sequenceNumber, uint16_t coapObservePeriod) {
    uint8_t * p_Payload = NULL;
    uint8_t * p_Header = NULL;
    uint16_t * p_crcHeader = NULL;
    uint16_t * p_crcPayload = NULL;

    uint16_t pktLen;

    /* Restore pointer to the beginning of the buffer */
    p_Header   = spiTransmitBuffer;
    p_Payload  = spiTransmitBuffer + sizeof(HCT_Frame_t);

    /* Calculate packet length (4B of CRCs from header + TLV element is 2B for TL and 2B for V. */
    pktLen = sizeof(HCT_Frame_t) + 1 + 1 + 2;

    /* Add header */
    HCTAddHeader(p_Header, HCT_CMD_CONFIG_GET, HCT_ORG_FUSIONPROC, HCT_RPY_NO_ACK, sequenceNumber, pktLen - 4 , &p_crcHeader, &p_crcPayload);

    /* Add TLV value */
    HCTAddTLV(p_Payload, HCT_TLV_TYPE_COAP_OBSERVE_PERIOD, sizeof(uint16_t), (uint8_t*) &coapObservePeriod);

    /* Compute CRC of header and payload */
    HCTComputeCRC(p_Header,  4, p_crcHeader);
    HCTComputeCRC(p_Payload, 4, p_crcPayload);

    return pktLen;
}

uint16_t nwpDataGet(uint8_t* spiTransmitBuffer, uint8_t sequenceNumber, HCT_TLV_t* tlvs, uint8_t numTLVs, uint16_t lenTLVs) {
    uint8_t * p_Current = NULL;
    uint8_t * p_Payload = NULL;
    uint8_t * p_Header = NULL;
    uint16_t * p_crcHeader = NULL;
    uint16_t * p_crcPayload = NULL;

    uint16_t pktLen;

    /* Point to the beginning of the buffer */
    p_Current = spiTransmitBuffer;
    p_Header   = spiTransmitBuffer;
    p_Payload  = spiTransmitBuffer + sizeof(HCT_Frame_t);

    /* Calculate the packet length */
    pktLen = sizeof(HCT_Frame_t) + lenTLVs;

    /* Add header to buffer */
    HCTAddHeader(p_Header, HCT_CMD_DATA_GET, HCT_ORG_FUSIONPROC, HCT_RPY_NO_ACK, sequenceNumber, pktLen - 4, &p_crcHeader, &p_crcPayload);

    /* advance pointer */
    p_Current = p_Current + sizeof(HCT_Frame_t);

    /* Add TLV to payload */
    uint8_t i;
    for (i = 0; i < numTLVs; i++) {
        HCTAddTLV(p_Current, tlvs[i].type , tlvs[i].length, tlvs[i].value);

        /* Advance pointer with the bytes that the TLV occupy */
        p_Current = p_Current + tlvs[i].length + 2 ;
    }

    /* Compute CRC of header and payload */
    HCTComputeCRC(p_Header,  4, p_crcHeader);
    HCTComputeCRC(p_Payload, lenTLVs, p_crcPayload);

    return pktLen;
}
