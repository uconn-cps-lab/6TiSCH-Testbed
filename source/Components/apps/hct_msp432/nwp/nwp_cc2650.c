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

#include "../hct/hct.h"
#include "nwp_cc2650.h"


uint16_t nwpConfigSet(uint8_t* spiTransmitBuffer, uint8_t sequenceNumber) {
    //uint8_t * pktCurrent = NULL;
    uint8_t * p_Payload = NULL;
    uint8_t * p_Header = NULL;
    uint16_t * p_crcHeader = NULL;
    uint16_t * p_crcPayload = NULL;

    uint16_t pktLen;

    /* Restore pointer to the beginning of the buffer */
    p_Header   = spiTransmitBuffer;
    p_Payload  = spiTransmitBuffer + sizeof(HCT_Frame_t);
    p_Payload = p_Payload;
    /* Calculate packet length (4B of CRCs from header + TLV element is 2B for TL and 2B for V. */
    pktLen = sizeof(HCT_Frame_t);

    /* Add header */
    HCTAddHeader(p_Header, HCT_CMD_CONFIG_GET, HCT_ORG_NETPROC, HCT_RPY_NO_ACK, sequenceNumber, pktLen - 4, &p_crcHeader, &p_crcPayload);

    /* Compute CRC of header and payload */
    HCTComputeCRC(p_Header, 4, p_crcHeader);

    return pktLen;
}


uint16_t nwpDataSet(uint8_t* spiTransmitBuffer, uint8_t sequenceNumber) {
        //uint8_t * pktCurrent = NULL;
        uint8_t * p_Payload = NULL;
        uint8_t * p_Header = NULL;
        uint16_t * p_crcHeader = NULL;
        uint16_t * p_crcPayload = NULL;

        uint16_t pktLen;

        /* Restore pointer to the beginning of the buffer */
        p_Header   = spiTransmitBuffer;
        p_Payload  = spiTransmitBuffer + sizeof(HCT_Frame_t);
        p_Payload = p_Payload;
        /* Calculate packet length (4B of CRCs from header + TLV element is 2B for TL and 2B for V. */
        pktLen = sizeof(HCT_Frame_t);

        /* Add header */
        HCTAddHeader(p_Header, HCT_CMD_DATA_GET, HCT_ORG_NETPROC, HCT_RPY_NO_ACK, sequenceNumber, pktLen - 4, &p_crcHeader, &p_crcPayload);

        /* Compute CRC of header and payload */
        HCTComputeCRC(p_Header, 4, p_crcHeader);

        return pktLen;
}
