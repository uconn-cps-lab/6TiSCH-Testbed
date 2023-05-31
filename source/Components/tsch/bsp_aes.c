/*
* Copyright (c) 2010 - 2014 Texas Instruments Incorporated
*
* All rights reserved not granted herein.
* Limited License. 
*
* Texas Instruments Incorporated grants a world-wide, royalty-free,
* non-exclusive license under copyrights and patents it now or hereafter
* owns or controls to make, have made, use, import, offer to sell and sell ("Utilize")
* this software subject to the terms herein.  With respect to the foregoing patent
*license, such license is granted  solely to the extent that any such patent is necessary
* to Utilize the software alone.  The patent license shall not apply to any combinations which
* include this software, other than combinations with devices manufactured by or for TI (“TI Devices”). 
* No hardware patent is licensed hereunder.
*
* Redistributions must preserve existing copyright notices and reproduce this license (including the
* above copyright notice and the disclaimer and (if applicable) source code license limitations below)
* in the documentation and/or other materials provided with the distribution
*
* Redistribution and use in binary form, without modification, are permitted provided that the following
* conditions are met:
*
*       * No reverse engineering, decompilation, or disassembly of this software is permitted with respect to any
*     software provided in binary form.
*       * any redistribution and use are licensed by TI for use only with TI Devices.
*       * Nothing shall obligate TI to provide you with source code for the software licensed and provided to you in object code.
*
* If software source code is provided to you, modification and redistribution of the source code are permitted
* provided that the following conditions are met:
*
*   * any redistribution and use of the source code, including any resulting derivative works, are licensed by
*     TI for use only with TI Devices.
*   * any redistribution and use of any object code compiled from the source code and any resulting derivative
*     works, are licensed by TI for use only with TI Devices.
*
* Neither the name of Texas Instruments Incorporated nor the names of its suppliers may be used to endorse or
* promote products derived from this software without specific prior written permission.
*
* DISCLAIMER.
*
* THIS SOFTWARE IS PROVIDED BY TI AND TI’S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TI’S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== bsp_aes.c =============================================
 */
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26xx.h>
#include "bsp_aes.h"

#ifdef FEATURE_MAC_SECURITY
#define bsp_assert() {while(1) {asm("");}}
//*****************************************************************************
static CryptoCC26XX_Handle cryptoHandle;
static CryptoCC26XX_Params cryptoParams;

/***************************************************************************//**
 * @fn          bspAesCcmAuthEncrypt
 *
 * @brief      Executes CCM ang generate tag
 *
 * @param[in]  key - pointer to the key
 *             keylocation - index to key store area
 *             authlen - length of authentication tag
 *             nonce - pointer to nonce
 *             plaintext - pointer to m data
 *             plaintextlen - length of m data
 *             header - poiner to a data
 *             headerlen - length of a data
 *             fieldlen - length of length field
 *             tag - pointer to authentication tag
 *             encrypt - flag to indicate encrypt
 *
 * @return     Cryto Status
 ******************************************************************************/
int bspAesCcmAuthEncrypt(const char *key, CryptoCC26XX_KeyLocation keylocation,
                          uint32_t authlen, char *nonce,
                          char *plaintext, uint32_t plaintextlen,
                          char *header, uint32_t headerlen,
                          uint32_t fieldlen, char *tag)
{
   int status = CRYPTOCC26XX_STATUS_SUCCESS;
   uint32_t  keyIndex;
   CryptoCC26XX_AESCCM_Transaction ccmTransaction;

   uint32_t tsState = Task_disable();
   CryptoCC26XX_Transac_init((CryptoCC26XX_Transaction *) &ccmTransaction, CRYPTOCC26XX_OP_AES_CCM);

   keyIndex = CryptoCC26XX_allocateKey(cryptoHandle, keylocation, (uint32_t const *)key);

   if (keyIndex != CRYPTOCC26XX_STATUS_ERROR)
   {
      // Setup transaction
      ccmTransaction.keyIndex      = keyIndex;
      ccmTransaction.authLength    = authlen;
      ccmTransaction.nonce         = nonce;
      ccmTransaction.msgIn         = plaintext;
      ccmTransaction.header        = header;
      ccmTransaction.msgOut        = tag;
      ccmTransaction.fieldLength   = fieldlen;
      ccmTransaction.msgInLength   = plaintextlen;
      ccmTransaction.headerLength  = headerlen;

      // Do AES-CCM operation
      status = CryptoCC26XX_transactPolling(cryptoHandle, (CryptoCC26XX_Transaction *) &ccmTransaction);
      CryptoCC26XX_releaseKey(cryptoHandle, (int *)&keyIndex);
      Task_restore(tsState);
   }
   else
   {
      Task_restore(tsState);
      return CRYPTOCC26XX_STATUS_ERROR;
   }

   return status;
}

/***************************************************************************//**
 * @fn          bspAesCcmDecryptAuth
 *
 * @brief      Executes inverse CCM ang verify tag
 *
 * @param[in]  key - pointer to the key
 *             keylocation - index to key store area
 *             authlen - length of authentication tag
 *             nonce - pointer to nonce
 *             ciphertext - pointer to c data
 *             ciphertextlen - length of c data
 *             header - poiner to a data
 *             headerlen - length of a data
 *             fieldlen - length of length field
 *             tag - pointer to authentication tag
 *             decrypt - flag to indicate decrypt
 *
 * @return     AES_SUCCESS if successful
 ******************************************************************************/
bool bspAesCcmDecryptAuth(const char *key, CryptoCC26XX_KeyLocation keylocation,
                          uint32_t authlen, char *nonce,
                          char *ciphertext, uint32_t ciphertextlen,
                          char *header, uint32_t headerlen,
                          uint32_t fieldlen, char *tag)
{
   uint32_t status = CRYPTOCC26XX_STATUS_SUCCESS;
   uint32_t keyIndex;
   CryptoCC26XX_AESCCM_Transaction ccmTransaction;

   uint32_t tsState = Task_disable();

   CryptoCC26XX_Transac_init((CryptoCC26XX_Transaction *) &ccmTransaction, CRYPTOCC26XX_OP_AES_CCMINV);

   keyIndex = CryptoCC26XX_allocateKey(cryptoHandle, keylocation, (uint32_t const *)key);

   if (keyIndex != CRYPTOCC26XX_STATUS_ERROR)
   {
      // Setup transaction
      ccmTransaction.keyIndex      = keyIndex;
      ccmTransaction.authLength    = authlen;
      ccmTransaction.nonce         = nonce;
      ccmTransaction.msgIn         = ciphertext;
      ccmTransaction.header        = header;
      ccmTransaction.msgOut        = tag;
      ccmTransaction.fieldLength   = fieldlen;
      ccmTransaction.msgInLength   = ciphertextlen;
      ccmTransaction.headerLength  = headerlen;

      // Do AES-CCMINV operation
      status = CryptoCC26XX_transactPolling(cryptoHandle, (CryptoCC26XX_Transaction *) &ccmTransaction);
      CryptoCC26XX_releaseKey(cryptoHandle, (int *)&keyIndex);
      Task_restore(tsState);
   }
   else
   {
      Task_restore(tsState);
      return CRYPTOCC26XX_STATUS_ERROR;
   }

   return status;
}

/***************************************************************************//**
 * @fn          bspAesInit
 *
 * @brief      Initialize AES peripheral
 *
 * @param[in]  None
 *
 * @return     None
 ******************************************************************************/
void bspAesInit(uint32_t index)
{
   CryptoCC26XX_init();     // Setup AES  and initialize the driver.
   CryptoCC26XX_Params_init(&cryptoParams);

   /* Attempt to open AES */
   cryptoHandle = CryptoCC26XX_open(index, false, &cryptoParams);

   if (cryptoHandle == NULL)
   {
      bsp_assert();
   }
}
#endif
