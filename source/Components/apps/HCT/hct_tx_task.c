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
 *  ====================== hct_tx_task.c =============================================
 *  HCT Task 
 */

/* ------------------------------------------------------------------------------------------------
*                                          Includes
* ------------------------------------------------------------------------------------------------
*/


#include "hct.h"
#include "mac_reset.h"   //fengmo NVM

HCT_OBJ_s HCT_Obj;
HCT_HND_s HCT_Hnd;

extern UART_Handle  uartHandle;
int globalRestart = 0;

void HCT_TX_packet(void)
{
    bool bMsgErr;
    HCT_PKT_BUF_s Pkt;

    memset(&Pkt, 0, sizeof(HCT_PKT_BUF_s));

    /* call the library function to receive one packet */
    HCT_TX_recv_msg(&bMsgErr, &Pkt);
    if (bMsgErr == TRUE)
    {
        if(Pkt.pdata1){
            BM_free_6tisch(Pkt.pdata1);
        }
        if(Pkt.pdata2){
            BM_free_6tisch(Pkt.pdata2);
        }
        /* no memory space to receive data */
        HCT_Hnd->err_tx_recv_msg++;
        return;
    }

    HCT_Hnd->num_tx_from_host++;

    /* handle the message */
    HCT_MSG_handle_message(&Pkt);
}

void HCT_TX_task(UArg arg0, UArg arg1)
{
    memset(&HCT_Obj,0x0,sizeof(HCT_Obj));
    HCT_Obj.uart_hnd = uartHandle;
    HCT_Hnd = &HCT_Obj;

    while (1)
    {
        HCT_TX_packet();
        if (globalRestart)  //fengmo NVM
        {
           Task_sleep(2*CLOCK_SECOND);
           Board_reset();
        }
    }
}
