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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <err.h>
#include "comm.h"
#include "tun.h"
#include "uip_rpl_process.h"
#include "bm_api.h"
#include "nm.h"
#include "coap/coap_gw_app.h"
#include "sys/clock.h"
#include "net/tcpip.h"
#include "nv_params.h"

volatile uint8_t NetworkReadyFlag;


extern char uartName[32];
extern uint16_t tsch_slot_frame_size;
extern uint16_t tsch_number_shared_slots;
extern uint16_t beacon_freq;

uint8_t globalTermination  = 0;   

int main(int argc, char **argv)
{
   int c, parent;
   const char *siodev = NULL;
   const char *host = NULL;
   const char *port = NULL;
   const char *prog;
   char *pEnd;

   printf("\nGateway 6LP/IP/RPL Stack Application\n\n");

   prog = argv[0];
   setvbuf(stdout, NULL, _IOLBF, 0); /* Line buffered output. */

   while ((c = getopt(argc, argv, "s:a:f:n:p:b:k:u:d:")) != -1)
   {
      switch (c)
      {
         case 's':
            siodev = optarg;
            // copy name to device name
            strcpy(uartName, optarg);
            break;
         case 'f':
            tsch_slot_frame_size = strtol(optarg, &pEnd, 0);
            break;
         case 'n':
            tsch_number_shared_slots = strtol(optarg, &pEnd, 0);
            break;
         case 'b':
            beacon_freq = strtol(optarg, &pEnd, 0);
            break;
         case '?':
         case 'h':
         default:
            fprintf(stderr, "usage:  %s [options] \n", prog);
            fprintf(stderr, "Options are:\n");
            fprintf(stderr, " -s serial dev for UART (default /dev/ttyUSB1)\n");
            fprintf(stderr, " -f frame size of slot (default =127)\n");
            fprintf(stderr, " -n number of shared slot (default = 1)\n");
            fprintf(stderr, " -b beacon freq (default = 4)\n");

            exit(1);
            return(1);
      }
   }

   //init syslog
   openlog("sn-gw", 0, LOG_DAEMON);
   setlogmask(LOG_ERR);

   BM_init();
   printf("Initializing tunnel interface...\n");

   if (TUN_init() < 1)
   {
      return EXIT_FAILURE;
   }

   printf("Initializing Network Manager...\n");

   if (NM_init() < 1)
   {
      return EXIT_FAILURE;
   }

   printf("Initializing UART port...\n");

   if (COMM_init() < 1)
   {
      return EXIT_FAILURE;
   }

   printf("Initializing timer\n");

   if (pltfrm_TIMER_init() < 1)
   {
      return EXIT_FAILURE;
   }
   
   NVM_init();
   
   printf("Initializing 6LP/uIP/RPL...\n");

   if (UIP_init() < 1)
   {
      return EXIT_FAILURE;
   }

   clock_init();

   NetworkReadyFlag = 1;

   while(!tcpip_isInitialized())
   {
      sleep(1);
   }

   coap_task();

   while (1)
   {
      sleep(1);
      if (globalTermination)  
      {
         sleep(5);
         break;
      }
   }

   return EXIT_SUCCESS;
}
