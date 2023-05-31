/*******************************************************************************
Filename:       simple-udp-root.c
Revised:
Revision:

Description:   This file contains main functions and application data receive
handler function.


Copyright 2012-2016 Texas Instruments Incorporated. All rights reserved.

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
*******************************************************************************/

/*******************************************************************************
*                                            INCLUDES
*******************************************************************************/

#include "stdio.h"
#include <string.h>

#include <ti/drivers/UART.h>

#include "Board.h"
#include "led.h"

#include "hal_api.h"
#include "net/uip.h"
#include "net/udp-simple-socket.h"
#include "simple-udp/simple-udp-sensors.h"

#include "uip_rpl_process.h"
#include "net/uip-ds6.h"
#include "mac_config.h"
#include "nhl_api.h"
#include "nhl_setup.h"
#include "bsp_aes.h"

/* -----------------------------------------------------------------------------
 *                                         Local Variables
 * -----------------------------------------------------------------------------
 */
static uip_buf_t app_rx_aligned_buf;
static UART_Handle     handle;
static struct udp_simple_socket udp_socket;

/* -----------------------------------------------------------------------------
 *                                           Defines
 * -----------------------------------------------------------------------------
 */
#define app_rx_buf (app_rx_aligned_buf.c)

#define UIP_IP_BUF           ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define APP_IP_BUF ((struct uip_ip_hdr *)&app_rx_buf[UIP_LLH_LEN])
#define APP_PAYLOAD_BUF ((struct uip_ip_hdr *)&app_rx_buf[UIP_LLH_LEN+UIP_IPUDPH_LEN])

#define SERVER_REPLY_RATE 0

/*******************************************************************************
 * @fn         exceptionHandler
 *
 * @brief      Default exception handler function register in .cfg
 *
 * @param       none
 *
 * @return      none
 *******************************************************************************
 */
void exceptionHandler()
{
  while(1){}
}

/*******************************************************************************
 * @fn          app_uart_open
 *
 * @brief       Set UART parameters and open UART
 *
 * @param       none
 *
 * @return      none
 *******************************************************************************
 */
void app_uart_open(void)
{
    UART_Params     params;
    UART_Params_init(&params);
    params.baudRate      = 115200;
    params.writeMode     = UART_MODE_BLOCKING;
    params.readMode     = UART_MODE_BLOCKING;
    params.writeDataMode = UART_DATA_BINARY;
    params.readDataMode  = UART_DATA_BINARY;
    params.readEcho      = UART_ECHO_OFF;

    handle = UART_open(0, &params);
    if (!handle)
    {
        Task_exit();
    }
}

/*******************************************************************************
 * @fn          print_recv
 *
 * @brief       Print received message to UART console
 *
 * @param       none
 *
 * @return      none
 *******************************************************************************
 */
void print_recv(const  rimeaddr_t *originator, uint8_t hops,
                    const uint8_t *payload, uint16_t payload_len)
{
  unsigned long time;
  uint16_t data,addr;
  uint8_t i,size;
  char msg1[]="\r\n";
  char msg2[20];

  time = clock_seconds();
  addr = originator->u8[0] + (originator->u8[1] << 8);
  sprintf(msg2,"%04x %04x %02d",addr, time, hops);
  UART_write(handle, msg2, strlen(msg2));

  size = *(payload+2);

  for(i = 0; i < size/2; i++)
  {
    memcpy(&data, payload, sizeof(data));
    payload += sizeof(data);
    sprintf(msg2," %04x", data);
    UART_write(handle, msg2, strlen(msg2));
  }
    UART_write(handle, msg1, strlen(msg1));
}

/*******************************************************************************
 * @fn          tcpip_handler
 *
 * @brief       Handler function for received meesage
 *
 * @param       none
 *
 * @return      none
 *******************************************************************************
 */
void tcpip_handler(void)
{

    uint8_t *appdata;
    rimeaddr_t sender;
    static uint8_t pre_seqno=0;
    uint8_t seqno;
    uint8_t hops;

#if SERVER_REPLY_RATE
    uip_ipaddr_t *sender_ipaddr;
#endif

    appdata = (uint8_t *)APP_PAYLOAD_BUF;
    sender.u8[0] = APP_IP_BUF->srcipaddr.u8[15];
    sender.u8[1] = APP_IP_BUF->srcipaddr.u8[14];

    seqno = *appdata;

    hops = uip_ds6_if.cur_hop_limit - APP_IP_BUF->ttl + 1;

    print_recv(&sender, hops, appdata, sizeof(SampleMsg_t) / sizeof(uint8_t) );
    if ((pre_seqno !=0) && ((seqno-pre_seqno)!=1)){
                  Log_info0("lost packet");
    }
    pre_seqno = seqno;

#if SERVER_REPLY_RATE
    sender_ipaddr = &APP_IP_BUF->srcipaddr;
    uip_udp_packet_send_post(&udp_socket, appdata,  sizeof(SampleMsg_t),
                             sender_ipaddr, UDP_CLIENT_PORT);
#endif

}

/*******************************************************************************
 * @fn          main
 *
 * @brief       Start of application.
 *
 * @param       none
 *
 * @return      none
 *******************************************************************************
 */
void main(void)
{
    PIN_init(BoardGpioInitTable);
    LED_init();

#ifdef FEATURE_MAC_SECURITY
    bspAesInit(0);
#endif

    UART_init();
    app_uart_open();

    set_defaultTschMacConfig();

     /* Enable interrupts and start sysbios */
    BIOS_start();
}


/*******************************************************************************
* @fn          udp_receive_callback
*
* @brief       UDP Connection Callback Function
*
* @param       *c - udp connection socket pointer
*              *source_addr - source IP address
*              source_port - UDP source port
*              *dest_addr - destination IP address
*              dest_port - UDP desintation port
*              *data - pointer to data message
*              dataLen - data message length
*
* @return      none
********************************************************************************
*/
void udp_receive_callback(struct udp_simple_socket *c, void *ptr,
                                             const uip_ipaddr_t *source_addr,
                                             uint16_t source_port,
                                             const uip_ipaddr_t *dest_addr,
                                             uint16_t dest_port,
                                             const char *data,
                                             uint16_t datalen)
{
  	TCPIP_cmdmsg_t  msg;

        memcpy(APP_IP_BUF,UIP_IP_BUF, UIP_BUFSIZE - UIP_LLH_LEN);

        if (Mailbox_post(apptask_mailbox, &msg, BIOS_NO_WAIT) != TRUE)
        {
            TCPIP_Dbg.tcpip_post_app_err++;
        }
}


/*******************************************************************************
* @fn          app_task
*
* @brief       Application task
*
* @param       arg0 - meaningless
*              arg1 - meaningless
*
* @return      none
********************************************************************************
*/
void app_task(UArg arg0, UArg arg1)
{

    uip_udp_appstate_t ts;
    NHL_MlmeStartPAN_t startArg;
    
    NHL_setupStartArg(&startArg);
    NHL_startPAN(&startArg,NULL);
  
    while(!tcpip_isInitialized())
    {
      Task_sleep(2*CLOCK_SECOND);
    }

    udp_simple_socket_open(&udp_socket, NULL, UDP_CLIENT_PORT, UDP_SERVER_PORT,
                           NULL, udp_receive_callback);

    while (1)
    {
          Mailbox_pend(apptask_mailbox, &ts, BIOS_WAIT_FOREVER);
          tcpip_handler();
    }
}