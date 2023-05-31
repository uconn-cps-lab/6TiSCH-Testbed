/******************************************************************************
*
*   Copyright (C) 2014 Texas Instruments Incorporated
*
*   All rights reserved. Property of Texas Instruments Incorporated.
*   Restricted rights to use, duplicate or disclose this code are
*   granted through contract.
*
*   The program may not be used without the written permission of
*   Texas Instruments Incorporated or against the terms and conditions
*   stipulated in the agreement under which this program has been supplied,
*   and under no circumstances can it be used with non-TI connectivity device.
*
******************************************************************************/

/*
  Network Services for General Purpose Linux environment
*/

#ifndef __NET_DEV_SERVICES_H__
#define __NET_DEV_SERVICES_H__

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "tcp-socket.h"

/* include a type file for u32, i32, .....*/
//nclude "mqtt_common.h"

typedef int            i32;
typedef unsigned int   u32;
typedef unsigned char   u8;
typedef char            i8;
typedef unsigned short u16;
typedef short          i16;

// This is TI RTOS Porting 
#define TIRTOS

//*****************************************************************************
//                      MACROS
//*****************************************************************************

// MACRO to include receive time out feature
#define SOC_RCV_TIMEOUT_OPT 	1


/*-----------------------------------------------------------------------------
definitions needed for the functions in this file
-----------------------------------------------------------------------------*/
typedef struct
{
	u8  SecurityMethod;
	u32  SecurityCypher;
}TcpSecSocOpts_t;


/*-----------------------------------------------------------------------------
prototypes of functions
-----------------------------------------------------------------------------*/

void *tcp_open(u32 nwconn_info, i8 *server_addr, u16 port_number);
i32 tcp_send(void *comm, const u8 *buf, u32 len);
i32 tcp_recv(void *comm, u8 *buf, u32 len, u32 wait_secs, bool *timed_out);
i32 close_tcp(void *comm);
u32 rtc_secs(void);

#ifdef DEBUG
//#define PRINTF(x,...)	Report(x,__VA_ARGS__)
#define PRINTF(x,...)
#else
#define PRINTF(x,...)
#endif

#endif
