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
#ifndef PLTFRM_LIB_H_
#define PLTFRM_LIB_H_

#include "plat-conf.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#define __access_byte(ptr,idx)   (((char*)ptr)[idx])

#include <stdio.h>
#include <string.h>

#include "pltfrm_timer.h"
#include "comm.h"
//DEBUGGING MESSAGES

//#define pltfrm_debug(...) pltfrm_debug_print(__VA_ARGS__)
#define pltfrm_debug_address(addr)  uip_debug_ipaddr_print(addr)

#define pltfrm_debug_lladdr(lladdr) uip_debug_lladdr_print(lladdr)

#define PLTFRM_DEBUG_ALWAYS_FLUSH   1 // 0: if we do not want to flush each message debugged (use IP6_flush() for flushing). In other case, each message is directly flushed

#ifdef LINUX_GATEWAY
#define pltfrm_debug(...) printf(__VA_ARGS__)
#endif
/******************************************************************************
*  FUNCTION NAME: pltfrm_debug_init
*
*  DESCRIPTION:
*        This function initializes the debugging method if needed.
*
*  Return value:      none
*
*  Input Parameters:  none
*
*  Output Parameters: none
*
*  Functions Called:
*
*******************************************************************************/
void pltfrm_debug_init();


/******************************************************************************
*  FUNCTION NAME: pltfrm_debug_hex
*
*  DESCRIPTION:
*        This function prints a value in hexadecimal mode
*
*  Return value:      none
*
*  Input Parameters:
*        int b: the value to print
*
*  Output Parameters: none
*
*  Functions Called:
*        System_printf (for SYS_BIOS)
*
*******************************************************************************/
void pltfrm_debug_hex(int b);

/******************************************************************************
*  FUNCTION NAME: pltfrm_debug_dec
*
*  DESCRIPTION:
*        This function prints a value in decimal mode
*
*  Return value:      none
*
*  Input Parameters:
*        long b: the value to print
*
*  Output Parameters: none
*
*  Functions Called:
*        System_printf (for SYS_BIOS)
*
*******************************************************************************/
void pltfrm_debug_dec(long b);

/******************************************************************************
*  FUNCTION NAME: pltfrm_debug_flush
*
*  DESCRIPTION:
*        This function flushes the buffer that contains the debugging messages to the console
*
*  Return value:        none
*
*  Input Parameters: none
*
*  Output Parameters:   none
*
*  Functions Called:
*        System_flush (for SYS_BIOS)
*
*******************************************************************************/
void pltfrm_debug_flush(void);

//BYTE ACCESS:

//IMPORTANT: All these functions will be defined as macros for performance purposes (better footprint)

#define pltfrm_byte_memcpy(dst,offset_dst_bytes,src,offset_src_bytes,num_bytes)     memcpy(((char*)(dst))+(offset_dst_bytes),((char*)(src))+(offset_src_bytes),num_bytes)

#define pltfrm_byte_memset(dst,offset_dst_bytes,value,num_bytes)        memset(((char*)(dst))+(offset_dst_bytes),value,num_bytes)

#define pltfrm_byte_memcmp(ptr1,offset_ptr1_bytes,ptr2,offset_ptr2_bytes,num_bytes)    memcmp(((char*)(ptr1))+(offset_ptr1_bytes),((char*)(ptr2))+(offset_ptr2_bytes),num_bytes)

#define pltfrm_byte_set(ptr,offset_bytes,value)         ((char*)ptr)[offset_bytes]=(value)

#define pltfrm_byte_get(ptr,offset_bytes)               ((char*)ptr)[offset_bytes]

//Initializes detination with dest_len zeros and then copies src_len bytes from source to destination starting from the end of source, to the beginning of destination (dest_len must be higher than src_len)
void pltfrm_byte_memcpy_swapped(void* destination, int dest_len, void* source, int src_len);

//TIMESTAMPS

//#define pltfrm_getTimeStamp()  clock_time()
uint32_t pltfrm_getTimeStamp(void);


//RANDOM NUMBERS

#define pltfrm_initRandom(seed)

#define pltfrm_getRandom()      rand()

//*--------------------------------------------*//
//TIMERS

typedef void (*pltfrm_timer_function_t)(void*);

typedef struct pltfrm_timer
{
   void*          handle;
   pltfrm_timer_function_t         func;
   void*          params;
} pltfrm_timer_t;

//Init timer structure
void pltfrm_timer_init(pltfrm_timer_t* timer);

//Reserve memory and create the timer with the corresponding function and parameters and timeout
void pltfrm_timer_create(pltfrm_timer_t* timer, pltfrm_timer_function_t func, void* func_params, unsigned int timeout);
bool pltfrm_timer_isCreated(pltfrm_timer_t* timer);
void pltfrm_timer_restart(pltfrm_timer_t* timer, unsigned int timeout);
void pltfrm_timer_start(pltfrm_timer_t* timer);
void pltfrm_timer_stop(pltfrm_timer_t* timer);
int pltfrm_timer_isActive(pltfrm_timer_t* timer);



#define pltfrm_timer_getTimeout(timer)

//Delete the platform timer (free memory is necessary)
void pltfrm_timer_delete(pltfrm_timer_t* timer);

#endif /* PLTFRM_UTIL_H_ */
