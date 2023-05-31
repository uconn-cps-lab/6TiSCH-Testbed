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
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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

#ifndef __UART_H__
#define __UART_H__

#include <project.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <typedefs.h>

typedef struct
{
   UINT8 *commDevName_p;
   SINT32 baudRate;
   SINT32 commDevFd;
   struct termios dcb;
} UART_cntxt_s;

#define UART_flush(uartCntxt_p)  tcflush((uartCntxt_p)->commDevFd, TCIFLUSH)

extern SINT32 UART_connectPort(UART_cntxt_s *uartCntxt_p);
extern SINT32 UART_readPort(UART_cntxt_s *uartCntxt_p, UINT8 *buff_p, UINT32 len);
extern SINT32 UART_writePort(UART_cntxt_s *uartCntxt_p, UINT8 *buff_p, UINT32 cnt);
#endif
