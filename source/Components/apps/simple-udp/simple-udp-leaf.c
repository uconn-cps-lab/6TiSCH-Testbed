/*******************************************************************************
Filename:       simple-udp-leaf.c
Revised:
Revision:

Description:    This file contains main functions and application task to send
data periodically.


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
#include "net/queuebuf.h"
#include "net/udp-simple-socket.h"
#include "rpl/rpl.h"
#include "nhl_mgmt.h"
#include "mac_config.h"
#include "nhl_setup.h"
#include "simple-udp/simple-udp-sensors.h"

#include "uip_rpl_process.h"
#include "bsp_aes.h"
#include "lib/random.h"

#if SENSORTAG || I3_MOTE
#include "SensorI2C.h"
#include "SensorBatMon.h"
#include "SensorTmp007.h"
#include "SensorHDC1000.h"
#include "SensorOpt3001.h"
#include "SensorBmp280.h"
#include "SensorMpu9250.h"
#include "SensorLIS2HH12.h"
#endif

/* -----------------------------------------------------------------------------
 *                                      Global Variables
 * -----------------------------------------------------------------------------
 */
uint16_t APP_respSeqNo =0; //resp_seqno = 0;

/* -----------------------------------------------------------------------------
*                                         Local Variables
* -----------------------------------------------------------------------------
*/
static Semaphore_Handle semHandle;
static UART_Handle     handle;

static uip_buf_t app_rx_aligned_buf;
static struct udp_simple_socket udp_socket;

/* -----------------------------------------------------------------------------
*                                           Defines
* -----------------------------------------------------------------------------
*/
#define app_rx_buf (app_rx_aligned_buf.c)
#define UIP_IP_BUF           ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define APP_IP_BUF ((struct uip_udpip_hdr *)&app_rx_buf[UIP_LLH_LEN])
#define APP_PAYLOAD_BUF ((struct uip_ip_hdr *)&app_rx_buf[UIP_LLH_LEN+UIP_IPUDPH_LEN])

//#define USE_BBB_UDP_SERVER

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
                          uint16_t source_port, const uip_ipaddr_t *dest_addr,
                          uint16_t dest_port, const char *data, uint16_t datalen)
{
    APP_respSeqNo++;
}

/*******************************************************************************
* @fn          app_send
*
* @brief       Post function to application task when periodic clock expires.
*
* @param       none
*
* @return      none
********************************************************************************
*/
void app_send(void)
{
    Semaphore_post(semHandle);
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
void main()
{
    PIN_init(BoardGpioInitTable);
    LED_init();

#ifdef FEATURE_MAC_SECURITY
    bspAesInit(0);
#endif


#ifdef ENABLE_HCT
    UART_init();
    app_uart_open();
#endif

    set_defaultTschMacConfig();

    BIOS_start();
}

/*******************************************************************************
* @fn          sample_udp_open_conn
*
* @brief       Open UDP connection.
*
* @param       none
*
* @return      none
********************************************************************************
*/
void sample_udp_open_conn(void)
{
  udp_simple_socket_open(&udp_socket, NULL, UDP_SERVER_PORT, UDP_CLIENT_PORT, NULL, udp_receive_callback);
}



/*******************************************************************************
* @fn          sample_udp_send
*
* @brief       Create sample message and send to uIP
*
* @param       *pmsg - pointer to data message
*
* @return      none
********************************************************************************
*/
void sample_udp_send(SampleMsg_t *pmsg)
{
    sample_msg_create(pmsg);

    rpl_instance_t* default_instance = rpl_get_default_instance();

    if (default_instance != NULL)
    {
#ifdef USE_BBB_UDP_SERVER
        uip_ipaddr_t server_ipaddr;

        memcpy(&server_ipaddr,&default_instance->current_dag->dag_id,sizeof(server_ipaddr));

        server_ipaddr.u16[4] = 0x0000;
        server_ipaddr.u16[5] = 0x0000;
        server_ipaddr.u16[6] = 0x0000;
        server_ipaddr.u16[7] = 0x0200;

        uip_udp_packet_send_post(&udp_socket, pmsg, sizeof(SampleMsg_t), &server_ipaddr, UDP_SERVER_PORT);
#else
        uip_udp_packet_send_post(&udp_socket, pmsg, sizeof(SampleMsg_t), &default_instance->current_dag->dag_id, UDP_SERVER_PORT);
#endif

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
    BM_BUF_t *pbuf;
    SampleMsg_t *pudp_msg;
    uint32_t timeout;
    Clock_Params clockParams;
    NHL_MlmeScanReq_t nhlMlmeScanReq;

    uip_setup_appConnCallback(sample_udp_open_conn);

#if SENSORTAG
   bool status;
   status = SensorI2C_open();
   status = SensorBatmon_init();
   status = SensorTmp007_init();
   status = SensorOpt3001_init();
   status = SensorHdc1000_init();
   status = SensorBmp280_init();
   status = SensorMpu9250_init();
#elif I3_MOTE
   bool status;
   status = SensorI2C_open();
   status = SensorBatmon_init();
   status = SensorTmp007_init();
   status = SensorOpt3001_init();
   status = SensorHdc1000_init();
   status = SensorBmp280_init();
   status = SensorLis2hh12_init();
#endif

  
    Semaphore_Params semParams;
    Semaphore_Params_init(&semParams);
    semParams.mode = Semaphore_Mode_BINARY;
    semHandle = Semaphore_create(0, &semParams, NULL);

     /* Periodic application clock */
    Clock_Params_init(&clockParams);
    /* Default sysbios clock tick is 10 us */
    clockParams.period = 30*CLOCK_SECOND;
    clockParams.startFlag = true;

    timeout = (random_rand()%15)*CLOCK_SECOND;
    Clock_create((Clock_FuncPtr)app_send,timeout, &clockParams, NULL);

    Task_sleep(CLOCK_SECOND / 4);

    NHL_setupScanArg(&nhlMlmeScanReq);
    NHL_startScan(&nhlMlmeScanReq,NULL);
    

    pbuf = BM_alloc(9);
    if (pbuf == 0)
    {
        exceptionHandler();
    }
    pudp_msg = (SampleMsg_t *) pbuf;

    while (1)
    {
        Semaphore_pend(semHandle,BIOS_WAIT_FOREVER);

        if (!TSCH_checkConnectionStatus())
        {
            continue;
        }
        else
        {
            sample_udp_send(pudp_msg);
        }

    }
}