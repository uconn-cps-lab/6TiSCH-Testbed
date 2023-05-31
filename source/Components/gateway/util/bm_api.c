/*******************************************************************************
 *  Copyright (c) 2017 Texas Instruments Inc.
 *  All Rights Reserved This program is the confidential and proprietary
 *  product of Texas Instruments Inc.  Any Unauthorized use, reproduction or
 *  transfer of this program is strictly prohibited.
 *******************************************************************************
  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
 *******************************************************************************
 * FILE PURPOSE:
 *
 *******************************************************************************
 * DESCRIPTION:
 *
 *******************************************************************************
 * HISTORY:
 *
 ******************************************************************************/

#include <pthread.h>
#include "bm_api.h"
#include "lib/memb.h"

#include <stdlib.h>

MEMB(BM_BUF, BM_BUF_t, BM_NUM_BUFFER);

pthread_mutex_t mutexBufAPI;   // Lock

/**************************************************************************************************//**
 *
 * @brief       This function will disable the interrupt and allow caller enter critical
 *              section.
 *
 * @param[in]
 *
 *
 * @return      current interrupt status. This value will be used by SpinUnlock function call
 ***************************************************************************************************/
uint32_t BM_spinLock(void)
{
   int rc = pthread_mutex_lock(&mutexBufAPI);

   return rc;
}

/**************************************************************************************************//**
 *
 * @brief       This function will restore the interrupts which are disable by the previous SpinLock
 *              function call.and allow caller enter critical
 *
 *
 * @param[in]   st -- interrupt status (from previous SpinLock
 *
 * @return      None
 ***************************************************************************************************/
void BM_spinUnlock(uint32_t st)
{
   //Hwi_restore(st);

   pthread_mutex_unlock(&mutexBufAPI);
}

/**************************************************************************************************//**
 *
 * @brief       This function initializes the BM internal data structure.  It must be called
 *              once when the software system is started and before any other function in the
 *              buffer manager is called. The number of data buffer blocks is statically configured.
 *
 * @param[in]
 *
 * @return      None.
 ***************************************************************************************************/
void BM_init(void)
{
   memb_init(&BM_BUF);

   // init mutx lock
   pthread_mutex_init(&mutexBufAPI, NULL);
}


/**************************************************************************************************//**
 *
 * @brief       This function is used to allocate one buffer from buffer manager.  If the
 *              allocation is success, the buffer manager will return a pointer to BM_BUF_t
 *              data structure. If there is no buffer left in the buffer manager pool, it will
 *              return NULL. It is caller responsibility to check the returned value after
 *              calling this API.
 *
 * @param[in]
 *
 *
 * @return      The pointer to BM_BUF_t data structure. If the allocation operation is failure,
 *              the returned value will be NULL
 ***************************************************************************************************/
BM_BUF_t* BM_alloc(void)
{
   BM_BUF_t* pbuf;
   uint32_t int_st;

   // enter critical section
   int_st = BM_spinLock();

   pbuf = memb_alloc(&BM_BUF);

   // exit critical section
   BM_spinUnlock(int_st);

   if(!pbuf)
   {
      return NULL;
   }

   // set data structure to default value
   pbuf->datalen = 0;
   pbuf->dataoffset = 0;

   return pbuf;
}

/**************************************************************************************************//**
 *
 * @brief       This function is used to free the previously allocated buffer and returned
 *              the buffer back to buffer manager. After this function call, the previously
 *              allocated buffer is no longer valid.
 *
 * @param[in]   pbuf: previous allocated BM_BUF_t pointer
 *
 * @return      BM_SUCCESS  if the free operation is success.
 *              BM_ERROR_POINTER  if the input buffer pointer has some problems.
 ***************************************************************************************************/
BM_STATUS_t BM_free(BM_BUF_t* pbuf)
{
   int rtn;
   uint32_t int_st;

   // enter critical section
   int_st = BM_spinLock();

   rtn = memb_free(&BM_BUF, pbuf);

   // exit critical section
   BM_spinUnlock(int_st);

   if (rtn == -1 )
   {
      return BM_ERROR_POINTER;
   }

   return BM_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This function is used to reset the buffer data structure to default value.
 *
 * @param[in]
 *
 * @return      The pointer to BM_BUF_t data structure. If the allocation operation is failure,
 *              the returned value will be NULL
 ***************************************************************************************************/
BM_STATUS_t BM_reset(BM_BUF_t *pbuf)
{

   // set data structure to default value
   pbuf->datalen = 0;
   pbuf->dataoffset = 0;

   return BM_SUCCESS;
}

/**************************************************************************************************//**
 *
 * @brief       This function is used to copy data to BM buffer from other memory location.
 *              The data is copied to buffer memory which starts from dataptr. If the specified
 *              data size is greater than the size which BM buffer can support. It will only
 *              copy up to its capacity and return the number of copied data bytes.
 *
 *
 * @param[in]   pbuf: BM_BUF_t pointer
 *              pSrc: pointer to source data memory
 *              len: number of bytes from the source memory
 *
 *
 * @return      The number of byte actually copied.
 ***************************************************************************************************/
uint16_t BM_copyfrom(BM_BUF_t* pbuf, void *pSrc, const uint16_t len)
{
   uint16_t copy_len;
   uint8_t *pDstAddr;

   copy_len = BM_BUFFER_SIZE - pbuf->dataoffset;

   if (copy_len >= len)
   {
      copy_len = len;
   }

   pDstAddr  = (uint8_t *)pbuf->buf_aligned;
   pDstAddr += pbuf->dataoffset;

   memcpy(pDstAddr, pSrc, copy_len);

   // set the data length field
   pbuf->datalen = copy_len;

   // return number of bytes copied
   return copy_len;
}

/**************************************************************************************************//**
 *
 * @brief       This function is used to copy data from BM buffer to other memory location.
 *              The data is copied from BM buffer memory which starts from dataptr. We assume
 *              user specified memory has enough memory to hold the BM buffer's data. It is caller's
 *              responsibility to provide memory with enough space.
 *
 * @param[in]   pbuf: BM_BUF_t pointer
 *              pDst: pointer to destination data memory
 *
 *
 * @return      The number of byte actually copied.
 ***************************************************************************************************/
uint16_t BM_copyto(BM_BUF_t* pbuf, void *pDst)
{
   uint16_t copy_len;
   uint8_t *pSrcAddr;

   copy_len =  pbuf->datalen;

   // source address in uint8
   pSrcAddr =  (uint8_t *)pbuf->buf_aligned;
   pSrcAddr += pbuf->dataoffset;

   memcpy(pDst, pSrcAddr, copy_len);

   // return number of bytes copied
   return copy_len;
}


/**************************************************************************************************//**
 *
 * @brief       This function sets the BM buffer data length. It is help function and
 *              implemented as inline function.
 *
 * @param[in]   pBuf: BM_BUF_t pointer
 *              len: number of bytes in data length
 *
 *
 * @return      BM_SUCCESS  if the  operation is success.
 *              BM_ERROR_INVALID_DATA_LEN if the user specified data length is
 *              beyond the BM buffer length.
 ***************************************************************************************************/
BM_STATUS_t BM_setDataLen (BM_BUF_t *pBuf, const uint16_t len)
{
   // check make sure there is enough space
   if ( (BM_BUFFER_SIZE - pBuf->dataoffset) >= len)
   {
      pBuf->datalen = len;
      return BM_SUCCESS;
   }

   return BM_ERROR_INVALID_DATA_LEN;
}

/**************************************************************************************************//**
 *
 * @brief       This function returns the data length of the buffer. It is help function and
 *              implemented as inline function.
 *
 * @param[in]   pBuf: BM_BUF_t pointer
 *
 * @return      number of bytes in data length
 ***************************************************************************************************/
uint16_t BM_getDataLen (BM_BUF_t *pBuf)
{
   return pBuf->datalen ;
}

/**************************************************************************************************//**
 *
 * @brief       This function returns the data offset of the buffer. It is help function and
 *              implemented as inline function.
 *
 * @param[in]   pBuf: BM_BUF_t pointer
 *
 * @return      data offset
 ***************************************************************************************************/
uint8_t BM_getDataOffset (BM_BUF_t *pBuf)
{
   return pBuf->dataoffset;
}

/**************************************************************************************************//**
 *
 * @brief       This function returns the buffer pointer  (bufptr) of BM buffer.
 *              It is help function and implemented as inline function.
 *
 * @param[in]   pBuf: BM_BUF_t pointer
 *
 * @return      : pointer to BM buffer
 ***************************************************************************************************/
uint8_t *  BM_getBufPtr (BM_BUF_t *pBuf)
{
   return (uint8_t *)(&pBuf->buf_aligned[0]);
}

/**************************************************************************************************//**
 *
 * @brief       This function returns the data pointer (databufptr) of BM buffer.
 *              It is help function and implemented as inline function.
 *
 * @param[in]   pBuf: BM_BUF_t pointer
 *
 *
 * @return      : The databufptr in the BM buffer data structure.
 ***************************************************************************************************/
uint8_t *  BM_getDataPtr (BM_BUF_t *pBuf)
{
   return ((uint8_t *)(&pBuf->buf_aligned) + pBuf->dataoffset);
}
