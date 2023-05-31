/*
* Copyright (c) 2018 Texas Instruments Incorporated
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
* include this software, other than combinations with devices manufactured by or for TI (â€œTI Devicesâ€�).
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
* THIS SOFTWARE IS PROVIDED BY TI AND TIâ€™S LICENSORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
* BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TIâ€™S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *  ====================== bsp_aes.c =============================================
 */
#include <ti/sysbios/gates/GateMutexPri.h>
#include <ti/sysbios/knl/Task.h>
#include "bsp_aes.h"
#include "mac_tsch_security.h"

#ifdef FEATURE_MAC_SECURITY
//*****************************************************************************
static AESCCM_Handle cryptoHandle;
static AESCCM_Params cryptoParams;
static GateMutexPri_Struct gmpStruct;
static GateMutexPri_Params gmpParams;
GateMutexPri_Handle gmpHandle;

#define bsp_assert() {while(1) {asm("");}}
/***************************************************************************//**
 * @fn          bspAesCcmAuthEncrypt
 *
 * @brief      Executes CCM ang generate tag
 *
 * @param[in]  key - pointer to the key
 *             authlen - length of authentication tag
 *             nonce - pointer to nonce
 *             plaintext - pointer to m data
 *             plaintextlen - length of m data
 *             header - poiner to a data
 *             headerlen - length of a data
 *             fieldlen - length of length field
 *             tag - pointer to authentication tag
 *
 * @return     Cryto Status
 ******************************************************************************/
int bspAesCcmAuthEncrypt( uint8_t *key,
                          uint32_t authlen, uint8_t *nonce,
                          uint8_t *plaintext, uint32_t plaintextlen,
                          uint8_t *header, uint32_t headerlen,
                          uint32_t fieldlen, uint8_t *tag)
{
   AESCCM_Operation operation;
   CryptoKey cryptoKey;
   int_fast16_t encryptionResult;

   IArg mutexState = GateMutexPri_enter(gmpHandle);
  
   AESCCM_Operation_init(&operation);

   CryptoKeyPlaintext_initKey(&cryptoKey, key, MAC_SEC_KEY_SIZE);

    operation.key               = &cryptoKey;
    operation.aad               = header;
    operation.aadLength         = headerlen;
    operation.input             = plaintext;
    operation.output            = plaintext;
    operation.inputLength       = plaintextlen;
    operation.nonce             = nonce;
    operation.nonceLength       = 15-fieldlen;
    operation.mac               = tag;
    operation.macLength         = authlen;

    encryptionResult = AESCCM_oneStepEncrypt(cryptoHandle, &operation);
    if(encryptionResult != AESCCM_STATUS_SUCCESS){
        AESCCM_close(cryptoHandle);
        AESCCM_init();
        cryptoHandle = AESCCM_open(0, &cryptoParams);
        ULPSMAC_Dbg.numAesCcmError++;
    }
    GateMutexPri_leave(gmpHandle, mutexState);

   return encryptionResult;
}

/***************************************************************************//**
 * @fn          bspAesCcmDecryptAuth
 *
 * @brief      Executes inverse CCM ang verify tag
 *
 * @param[in]  key - pointer to the key
 *             authlen - length of authentication tag
 *             nonce - pointer to nonce
 *             ciphertext - pointer to c data
 *             ciphertextlen - length of c data
 *             header - poiner to a data
 *             headerlen - length of a data
 *             fieldlen - length of length field
 *             tag - pointer to authentication tag
 *
 * @return     AES_SUCCESS if successful
 ******************************************************************************/
int bspAesCcmDecryptAuth( uint8_t *key,
                          uint32_t authlen, uint8_t *nonce,
                          uint8_t *ciphertext, uint32_t ciphertextlen,
                          uint8_t *header, uint32_t headerlen,
                          uint32_t fieldlen, uint8_t *tag)
{
   AESCCM_Operation operation;
   CryptoKey cryptoKey;
   int_fast16_t encryptionResult;

  
   IArg mutexState = GateMutexPri_enter(gmpHandle);

   AESCCM_Operation_init(&operation);

   CryptoKeyPlaintext_initKey(&cryptoKey, key, MAC_SEC_KEY_SIZE);

    operation.key               = &cryptoKey;
    operation.aad               = header;
    operation.aadLength         = headerlen;
    operation.input             = ciphertext;
    operation.output            = ciphertext;
    operation.inputLength       = ciphertextlen;
    operation.nonce             = nonce;
    operation.nonceLength       = 15-fieldlen;
    operation.mac               = tag;
    operation.macLength         = authlen;

    encryptionResult = AESCCM_oneStepEncrypt(cryptoHandle, &operation);
    if(encryptionResult != AESCCM_STATUS_SUCCESS){
        AESCCM_close(cryptoHandle);
        AESCCM_init();
        cryptoHandle = AESCCM_open(0, &cryptoParams);
        ULPSMAC_Dbg.numAesCcmError++;
    }
    GateMutexPri_leave(gmpHandle, mutexState);

   return encryptionResult;
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
   AESCCM_init();
   AESCCM_Params_init(&cryptoParams);
   cryptoParams.timeout = 100000;//1s timeout

   cryptoHandle = AESCCM_open(index, &cryptoParams);

   if (cryptoHandle == NULL)
   {
      bsp_assert();
   }
   GateMutexPri_Params_init(&gmpParams);
   GateMutexPri_construct(&gmpStruct, &gmpParams);
   gmpHandle = GateMutexPri_handle(&gmpStruct);
}
#endif
