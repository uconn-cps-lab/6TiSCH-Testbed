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
 *  ====================== hct_tx.c =============================================
 *  This module contains HCT TX API.
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */


#include "hct.h"

#include "crc16.h"

#include "rtimer.h"

//#include <string.h> /* for memcpy() */

#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__)
//#include <stdio.h>

#include <ti/sysbios/family/arm/m3/Hwi.h>
#else
#include <stdlib.h>
#endif

extern uint8_t mac_hdr_conf_hct_buf[5];

/**************************************************************************************************//**
 *
 * @brief       HCT TX function that purges incoming octets until line silence is observed.
 *              This is a hack until we have a timeout on the I/O fct.
 *
 * @param[in]
 *
 *
 * @return      first received data after purge
 ***************************************************************************************************/

uint8_t HCT_TX_purge(void)
{
    uint32_t end_time;
    uint8_t data_byte;
    uint16_t count;

    /*
    ** pull the packet off the line until there is a period of line silence
    **  note that there is a small possibility this won't work because
    **  wraparound:
    **    1. puts and a loooong delay puts it right back in the gap.
    **    2. wraparound causes no line silence
    */

    HCT_Hnd->err_tx_purge++;

    count = 0;

    do
    {
        /* set the end time */
        end_time = (RTIMER_NOW() + HCT_PURGE_INPUT_TIME_MS);

        /* read a single octet, which blocks until we get one */
        UART_read(HCT_Hnd->uart_hnd,&data_byte,1);

        count++;

    } while(RTIMER_CLOCK_LT(RTIMER_NOW(), end_time));

    return (data_byte & 0xff);

}

/**************************************************************************************************//**
 *
 * @brief       This function will HCT TX function that receives a message from the host.
 *
 *
 *
 * @param[in]   msg_type_tag    - HCT message type
 *              pdata1          - BM_BUF pointer of first portion of data
 *              pdata2          - BM_BUF pointer of second portion of data
 *
 *
 * @return      operation status
 ***************************************************************************************************/
uint32_t HCT_TX_recv_msg(bool *pMsgErr,HCT_PKT_BUF_s *pPkt)
{
    uint16_t msghdr[(HCT_MSG_HDR_SIZE + HCT_MSG_EXT_HDR_SIZE ) >> 1];
    HCT_MSG_EXT_HEADER_s ext_hdr_crcs;
    uint16_t crc16,buf_len;
    uint8_t *pHeader;
    bool bError;

    // prefill the header
    msghdr[0] = HCT_MSG_TYPE_INVALID_MSG;
    msghdr[1] = 0;

    // zero
    pPkt->pdata1 = NULL;
    pPkt->pdata2 = NULL;

    //wait until a byte comes
    while (1)
    {
       int rdcount = UART_read(HCT_Hnd->uart_hnd,(uint8_t*)msghdr,1);
     
       if (rdcount == 1)
       {
          break;
       }
       else if (rdcount < 0)
       {
          HCT_Hnd->numUartReadError++;
          // Light up LED to signal error 
          HAL_LED_set(2,1);
       }
    }
    
    //Every following UART_read() has timeout. 
    //Receive the remaining header bytes. 
    if(HCT_MSG_HDR_SIZE+HCT_MSG_EXT_HDR_SIZE-1
       != UART_read(HCT_Hnd->uart_hnd,(uint8_t*)msghdr+1,HCT_MSG_HDR_SIZE+ HCT_MSG_EXT_HDR_SIZE-1))
    {
        *pMsgErr=TRUE;
        return 0;
    }

    do
    {
        bError = FALSE;

        // get header and payload CRC
        pHeader = (uint8_t *)msghdr;
        // save message type
        pPkt->msg_type = *pHeader;
        pHeader +=4;
        memcpy (&ext_hdr_crcs,pHeader,sizeof(ext_hdr_crcs));

        // compute the header CRC16
        crc16 = crc16_getCRC16(INIT_CRC16, (uint8_t *)&msghdr[0],HCT_MSG_HDR_SIZE);

        if (crc16 !=ext_hdr_crcs.Header_CRC16)
        {   // there is error in the HCT header
            HCT_Hnd->err_header_crc++;
            bError = TRUE;
        }

        if (bError == FALSE)
        {   // header is good, we try to receive whole HCT message
            // remove header CRC and payload CRC
            pPkt->msg_len = msghdr[1]-4;

            if (pPkt->msg_len  > 0x100)
            {
                bError = TRUE;
            }
        }

        // save the message payload to BM buffer
        if (bError == FALSE)
        {
            uint8_t idx=0;
            uint8_t *pbyte,read_len;
            BM_BUF_t * pdata;
            
            crc16 = INIT_CRC16;

            buf_len = pPkt->msg_len;

            while (buf_len)
            {
                if (idx>=2)
                {
                    bError = TRUE;
                    break;
                }

                if (buf_len > BM_BUFFER_SIZE)
                {
                    read_len = BM_BUFFER_SIZE;
                }
                else
                {
                    read_len = buf_len;
                }

                pdata = BM_alloc(8);
                if (pdata == NULL)
                {
                    bError = TRUE;
                    break;
                }

                // read data from UART
                pbyte = BM_getDataPtr(pdata);
                if(read_len != UART_read(HCT_Hnd->uart_hnd,pbyte, read_len))
                {
                    bError = TRUE;
                    BM_free_6tisch(pdata);
                    break;
                }             
                // compute the payload CRC16
                crc16 = crc16_getCRC16(crc16, pbyte, read_len);
                
                BM_setDataLen(pdata,read_len );

                if (idx == 0)
                {
                    pPkt->pdata1 = pdata;
                }
                else
                {
                    pPkt->pdata2 = pdata;
                }

                // update index
                idx++;

                // update the remaining buffer len
                buf_len -= read_len;
            }

            if (bError == FALSE && crc16 !=ext_hdr_crcs.Payload_CRC16)
            {   // there is error in the HCT payload
                HCT_Hnd->err_payload_crc++;
                bError = TRUE;
            }
        }


    }while(0);

    *pMsgErr = bError;
    return 0;
}

/**************************************************************************************************//**
 *
 * @brief       HCT MSG handle message function.  Parse out the message components and
 *               call the appropriate handler for the message.
 *
 *
 * @param[in]   msg_type_tag    - HCT message type
 *              pdata1          - BM_BUF pointer of first portion of data
 *              pdata2          - BM_BUF pointer of second portion of data
 *
 *
 * @return      operation status
 ***************************************************************************************************/


void HCT_MSG_handle_message(HCT_PKT_BUF_s *pPkt)
{
    // parse the message type
    switch (pPkt->msg_type)
    {
#if IS_ROOT
        case HCT_MSG_TYPE_DATA_TRANSFER:
            HCT_IF_MAC_data_req_proc(pPkt);
        break;
#endif
        case HCT_MSG_TYPE_LOAD_SYSTEM_CONFIG:
            HCT_IF_load_system_config_req_proc(pPkt);
        break;
        case HCT_MSG_TYPE_SET_MAC_PIB:
            HCT_IF_MAC_set_pib_proc(pPkt);
        break;
        case HCT_MSG_TYPE_GET_MAC_PIB:
          HCT_IF_MAC_get_pib_proc(pPkt);
        break;
        case HCT_MSG_TYPE_SHUT_DOWN:
          HCT_IF_Shutdown_req_proc(pPkt);
        break;
#if IS_ROOT
        case HCT_MSG_TYPE_NETWORK_START:
          HCT_IF_Network_Start_req_proc(pPkt);
        break;
        case HCT_MSG_TYPE_SCHEDULE:
          HCT_IF_Schedule_resp_proc(pPkt);
        break;
        case HCT_MSG_TYPE_NHL_SCHEDULE:
          HCT_IF_NHL_Schedule_proc(pPkt);
        break;
#endif
        case HCT_MSG_TYPE_ATTACH:
            HCT_IF_Attach_request_proc(pPkt);
        break;
        default:
          {
            if(pPkt->pdata1){
              BM_free_6tisch(pPkt->pdata1);
            }
            if(pPkt->pdata2){
              BM_free_6tisch(pPkt->pdata2);
            }
            // unknown message type
            HCT_Hnd->err_unknown_msg++;
          }
    }

    return ;
}
