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
 *  ====================== tlv.c =============================================
 *  This module contains TLV (Type, Length, Value) related API. 
 */

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */


#include "hct.h"
#include "tlv.h"

#include "tsch_api.h"
#include "ulpsmacaddr.h"
#include "mac_pib.h"

//#include <string.h> /* for memcpy() */

#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__)
//#include <stdio.h>

#include <ti/sysbios/family/arm/m3/Hwi.h>
#else
#include <stdlib.h>
#endif



/**************************************************************************************************//**
 *
 * @brief       This function get the TLV from specified input data
 *
 *
 *
 * @param[in]   usm_buf         -- the BM buffer pointer to user data payload
 *
 *
 * @return
 *
 ***************************************************************************************************/
uint8_t TLV_get(uint8_t *pdata,TLV_s *ptlv)
{
    uint8_t total_size=0;

    // get TLV type : 2 bytes
    memcpy(&ptlv->type,pdata,TLV_TYPE_SIZE);
    pdata += TLV_TYPE_SIZE;

    // get TLV length : 2 bytes
    memcpy(&ptlv->len,pdata,TLV_LEN_SIZE);
    pdata += TLV_LEN_SIZE;

    if (ptlv->len > MAX_TLV_VALUE_STATIC_BUFFER_LENGTH)
    {   // error case
        return total_size;
    }

    memcpy(ptlv->value,pdata,ptlv->len);

    total_size = TLV_TYPE_SIZE + TLV_LEN_SIZE + ptlv->len;

    return total_size;

}



