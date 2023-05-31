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
#ifndef LINUX_BM_API_H
#define LINUX_BM_API_H

/* ------------------------------------------------------------------------------------------------
 *                                          Includes
 * ------------------------------------------------------------------------------------------------
 */

#if defined (__IAR_SYSTEMS_ICC__) || defined(__TI_COMPILER_VERSION__) || defined (linux)
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <stdio.h>
#include <string.h>
#endif
/* ------------------------------------------------------------------------------------------------
 *                                          Defines
 * ------------------------------------------------------------------------------------------------
 */



/* ------------------------------------------------------------------------------------------------
 *                                           Constants
 * ------------------------------------------------------------------------------------------------
 */

#define ULPSMACBUF_CONF_NUM                    (32)
#define BM_BUFFER_SIZE                         (256)

/* \brief   The number of the BM Buffer
            The default size is 8 buffers
*/
#ifdef ULPSMACBUF_CONF_NUM
#define BM_NUM_BUFFER  ULPSMACBUF_CONF_NUM
#else
#define BM_NUM_BUFFER  8
#endif

/* ------------------------------------------------------------------------------------------------
 *                                           Typedefs
 * ------------------------------------------------------------------------------------------------
 */



/* BM API status */

typedef enum
{
   BM_SUCCESS                                           = 0,
   BM_ERROR_POINTER                                     = 1,
   BM_ERROR_INVALID_DATA_LEN                            = 2,
   BM_ERROR_INVALID_DATA_OFFSET                         = 3,
   BM_ERROR_DEC_LENGTH                                  = 4,
   BM_ERROR_INC_LENGTH                                  = 5,

   BM_STATUS_MAX

} BM_STATUS_t;


typedef struct BM_BUF_t
{
   uint16_t datalen;          // data length in thting ae buffer
   uint8_t dataoffset;       // data starting address in the buffer

   uint16_t buf_aligned[(BM_BUFFER_SIZE) / 2 ];

} BM_BUF_t;

/* ------------------------------------------------------------------------------------------------
 *                                           Global Variables
 * ------------------------------------------------------------------------------------------------
 */


/* ------------------------------------------------------------------------------------------------
 *                                           Function Prototypes
 * ------------------------------------------------------------------------------------------------
 */


// we will use SYS/RTOS function
uint32_t BM_spinLock(void);
void BM_spinUnlock(uint32_t st);

void BM_init(void);
BM_BUF_t* BM_alloc(void);
BM_STATUS_t BM_free(BM_BUF_t* pbuf);
BM_STATUS_t BM_reset(BM_BUF_t *pBuf);


uint16_t BM_copyfrom(BM_BUF_t* pbuf, void *pSrc, const uint16_t len);
uint16_t BM_copyto(BM_BUF_t* pbuf, void *pDst);


// help functions
BM_STATUS_t BM_setDataLen (BM_BUF_t *pBuf, const uint16_t len);
uint16_t BM_getDataLen (BM_BUF_t *pBuf);
uint8_t BM_getDataOffset (BM_BUF_t *pBuf);
uint8_t *  BM_getBufPtr (BM_BUF_t *pBuf);
uint8_t * BM_getDataPtr (BM_BUF_t *pBuf);

/**************************************************************************************************
*/

#endif /* BM_API_H */


