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

#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <typedefs.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <err.h>
#include <ctype.h>

#include "uart.h"
#include "util.h"

/* -----------------------------------------------------------------------------
*                                         DEFINE
* -----------------------------------------------------------------------------
*/
#define UART_RX_BUFFER_SIZE (8192)

/* -----------------------------------------------------------------------------
*                                         Global Variables
* -----------------------------------------------------------------------------
*/
UINT8 uart_rx_buffer[UART_RX_BUFFER_SIZE];
SINT32 uart_data_len;

/***************************************************************************//**
 * @fn          UART_readPort
 *
 * @brief       UART read
 *
 * @param[in]   uartCntxt_p - UART context
 * @param[in]   buff_p - buffer pointer
 * @param[in]  len - length to read
 *
 * @param[out]  none
 *
 * @return      read length
 *
 ******************************************************************************/
SINT32 UART_readPort(UART_cntxt_s *uartCntxt_p, UINT8 *buff_p, UINT32 len)
{
  SINT32 rdLen, readLeft = len, totRead = 0; 
  
  /* Pend infinite for the 1st byte*/
  uartCntxt_p->dcb.c_cc[VTIME] = 0;  
  uartCntxt_p->dcb.c_cc[VMIN] = 1;
  
  if ((tcsetattr(uartCntxt_p->commDevFd, TCSANOW, &(uartCntxt_p->dcb))) != 0)
  {
    printf("%s: tcsetattr errorno: %d\n",__FUNCTION__,errno);
    return 0;
  }
  
  while (readLeft > 0)
  {
    rdLen = read(uartCntxt_p->commDevFd, buff_p + totRead, readLeft);
    
    /* 0.1 second timeout for inter byte arrival*/
    uartCntxt_p->dcb.c_cc[VTIME] = 1; 
    uartCntxt_p->dcb.c_cc[VMIN] = 0;
    
    if ((tcsetattr(uartCntxt_p->commDevFd, TCSANOW, &(uartCntxt_p->dcb))) != 0)
    {
        printf("%s: tcsetattr errorno: %d\n",__FUNCTION__,errno);
        return 0;
    }
    
    if (rdLen > 0)
    {
      totRead += rdLen;
      readLeft -= rdLen;
    }
    else
    {
      return rdLen;
    }
  }

  return totRead;
}
/***************************************************************************//**
 * @fn          UART_writePort
 *
 * @brief       UART write
 *
 * @param[in]   uartCntxt_p - UART context
 * @param[in]   buff_p - buffer pointer
 * @param[in]  len - length to write
 *
 * @param[out]  none
 *
 * @return      write status
 *
 ******************************************************************************/
SINT32 UART_writePort(UART_cntxt_s *uartCntxt_p, UINT8 *buff_p, UINT32 cnt)
{
  SINT32 rc, bytesLeft = cnt, bytesWritten = 0;
  
  while (bytesLeft > 0)
  {
    rc = write(uartCntxt_p->commDevFd, buff_p + bytesWritten, bytesLeft);
    
    if (rc <= 0)
    {
      return -1;
    }
    else
    {
      bytesLeft -= rc;
      bytesWritten += rc;
    }
  }
  
  usleep(5000);//5ms
  return 1;
}

/***************************************************************************//**
 * @fn          UART_connectPort
 *
 * @brief       UART Setup
 *
 * @param[in]   uartCntxt_p - UART context
 *
 * @param[out]  none
 *
 * @return      Setup status
 *
 ******************************************************************************/
SINT32 UART_connectPort(UART_cntxt_s *uartCntxt_p)
{
  int len;
  char altCommDevName[256];
  
  if (uartCntxt_p->commDevFd > 0)
  {
     printf("FD for %s = is not 0. (%d) \n",uartCntxt_p->commDevName_p, uartCntxt_p->commDevFd);
     if (close(uartCntxt_p->commDevFd) <0)
       printf("Close for FD = %d is failed\n",uartCntxt_p->commDevFd);
     else
       printf("Close for FD = %d is succesfful\n",uartCntxt_p->commDevFd);
     uartCntxt_p->commDevFd = -1;
   }
  
  strncpy(altCommDevName, uartCntxt_p->commDevName_p, 256);
  len = strlen(altCommDevName);
  
  if (isdigit(altCommDevName[len-1]))
  {
    (altCommDevName[len-1])++;
  }
  
  uart_data_len = 0;
  
  uartCntxt_p->commDevFd = open((SINT8 *)uartCntxt_p->commDevName_p, O_RDWR | O_NOCTTY );
  
  //printf("FD = %d for %s\n", uartCntxt_p->commDevFd, uartCntxt_p->commDevName_p);
  
  if (uartCntxt_p->commDevFd < 0)
  {
    if (altCommDevName[len-1] != uartCntxt_p->commDevName_p[len-1])
    {
      printf("Open %s failed, try %s\n", uartCntxt_p->commDevName_p, altCommDevName);
      uartCntxt_p->commDevFd = open(altCommDevName, O_RDWR | O_NOCTTY );
    }
    
    if (uartCntxt_p->commDevFd < 0)
    {
      printf("Open %s also failed\n", altCommDevName);
      return -1;
    }
    else
    {
       printf("Open %s successed with FD %d \n", altCommDevName, uartCntxt_p->commDevFd);
    }
  }
 
  // Zero out port status flags
  if (fcntl(uartCntxt_p->commDevFd, F_SETFL, 0) != 0x0)
  {
    return -1;
  }
  
  bzero(&(uartCntxt_p->dcb), sizeof(uartCntxt_p->dcb));
  
  uartCntxt_p->dcb.c_cflag |= uartCntxt_p->baudRate;  // Set baud rate first time
  uartCntxt_p->dcb.c_cflag |= CLOCAL;  // local - don't change owner of port
  uartCntxt_p->dcb.c_cflag |= CREAD;  // enable receiver
  
  // Set to 8N1
  uartCntxt_p->dcb.c_cflag &= ~PARENB;  // no parity bit
  uartCntxt_p->dcb.c_cflag &= ~CSTOPB;  // 1 stop bit
  uartCntxt_p->dcb.c_cflag &= ~CSIZE;  // mask character size bits
  uartCntxt_p->dcb.c_cflag |= CS8;  // 8 data bits
  
  // Set output mode to 0
  uartCntxt_p->dcb.c_oflag = 0;
  
  uartCntxt_p->dcb.c_lflag &= ~ICANON;  // disable canonical mode
  uartCntxt_p->dcb.c_lflag &= ~ECHO;  // disable echoing of input characters
  uartCntxt_p->dcb.c_lflag &= ~ECHOE;
  
  // Set baud rate
  cfsetispeed(&uartCntxt_p->dcb, uartCntxt_p->baudRate);
  cfsetospeed(&uartCntxt_p->dcb, uartCntxt_p->baudRate);
  
  uartCntxt_p->dcb.c_cc[VTIME] = 0;  // timeout = 0.1 sec
  uartCntxt_p->dcb.c_cc[VMIN] = 1;
  
  if ((tcsetattr(uartCntxt_p->commDevFd, TCSANOW, &(uartCntxt_p->dcb))) != 0)
  {
    printf("%s: tcsetattr errorno: %d\n",__FUNCTION__,errno);
    close(uartCntxt_p->commDevFd);
    return -1;
  }
  
  // set up DTR
  {
    int i = TIOCM_DTR;
    if(ioctl(uartCntxt_p->commDevFd, TIOCMBIS, &i) == -1)
    {
      err(1, "ioctl");
    }
  }
  // flush received data
  tcflush(uartCntxt_p->commDevFd, TCIFLUSH);
  tcflush(uartCntxt_p->commDevFd, TCOFLUSH);
  
  return 1;
}
