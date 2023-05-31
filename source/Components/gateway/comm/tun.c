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
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <errno.h>
#include <proj_assert.h>
#include <project.h>
#include <semaphore.h>

#include <typedefs.h>
#include <util.h>
#include <pltfrm_mbx.h>

#include <tun.h>
#include <comm.h>

#include "net/uip.h"

/* -----------------------------------------------------------------------------
*                                         DEFINE
* -----------------------------------------------------------------------------
*/
#define UIP_IP_BUF ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_UDP_BUF ((struct uip_udp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])

#define IPv6_HDR_LEN  40
#define IPv6_ADDR_LEN  16

#define TUN_RX_BUFF_LEN  2048
#define TUN_TX_BUFF_LEN  2048

#define MAX_NUMBER_STORED_DEST_ADDR         (64)

/* -----------------------------------------------------------------------------
*                                         Global Variables
* -----------------------------------------------------------------------------
*/

pthread_t TUN_rxThreadInfo;

char tun_addr[128] = "2001:db8:1234:ffff::2/64";
char sysCmdBuff[128];

SINT8 TUN_dev[32] = "tun0";
UINT8 tun_ip_prefix[IPv6_ADDR_LEN / 2] = {0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0xff, 0xff};
UINT8 TUN_rxBuff[TUN_RX_BUFF_LEN];
UINT8 TUN_txBuff[TUN_TX_BUFF_LEN];
/*
* The IPv6 link-local address [RFC4291] for an IEEE 802.15.4 interface
* is formed by appending the Interface Identifier to the prefix FE80::/64.
*/
UINT8 IPv6_linkLocalPrefix[ ] = {0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


UINT16 txPktDstAddr[MAX_NUMBER_STORED_DEST_ADDR];
uint16_t TUN_rxBuff_outOffset = 0;
uint16_t TUN_rxBuff_inOffset = 0;

extern volatile uint8_t NetworkReadyFlag;
/* -----------------------------------------------------------------------------
*                                         Local Variables
* -----------------------------------------------------------------------------
*/
static SINT32 TUN_fd;

/***************************************************************************//**
 * @fn          ssystem
 *
 * @brief       Run shell command
 *
 * @param[in]   fmt - commands
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
int ssystem(const char *fmt, ...)
{
  char cmd[128];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(cmd, sizeof(cmd), fmt, ap);
  va_end(ap);
  printf("%s\n", cmd);
  fflush(stdout);
  return system(cmd);
}

/***************************************************************************//**
 * @fn          TUN_tun_alloc
 *
 * @brief       Allocate tun interface
 *
 * @param[in]   none
 *
 * @param[out]  dev - tun device
 *
 * @return      none
 *
 ******************************************************************************/
int TUN_tun_alloc(char *dev)
{
  struct ifreq ifr;
  int fd, err;
  
  if( (fd = open("/dev/net/tun", O_RDWR)) < 0 )
  {
    return -1;
  }
  
  memset(&ifr, 0, sizeof(ifr));
  
  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
  *        IFF_TAP   - TAP device
  *
  *        IFF_NO_PI - Do not provide packet information
  */
  ifr.ifr_flags = (IFF_TUN) | IFF_NO_PI;
  
  if(*dev != 0)
  {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }
  
  if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 )
  {
    close(fd);
    return err;
  }
  
  strcpy(dev, ifr.ifr_name);
  return fd;
}

/***************************************************************************//**
 * @fn          TUN_ifconf
 *
 * @brief       Tun interface configuration 
 *
 * @param[in]   tundev - tun device 
*  @param[in]   ipaddr-  tun ip address 
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void TUN_ifconf(const char *tundev, const char *ipaddr)
{
  ssystem("ifconfig %s inet `hostname` up", tundev);  
  ssystem("ifconfig %s add %s", tundev, ipaddr);
  ssystem("ifconfig %s inet 172.16.0.1 pointopoint 172.16.0.2", tundev);
  
  /* radvd needs a link local address for routing */
  
  /* Generate a link local address a la sixxs/aiccu */
  /* First a full parse, stripping off the prefix length */
  {
    char lladdr[40];
    char c, *ptr = (char *)ipaddr;
    uint16_t digit, ai, a[8], cc, scc, i;
    
    for(ai = 0; ai < 8; ai++)
    {
      a[ai] = 0;
    }
    
    ai = 0;
    cc = scc = 0;
    
    while(c = *ptr++)
    {
      if(c == '/')
      {
        break;
      }
      
      if(c == ':')
      {
        if(cc)
        {
          scc = ai;
        }
        
        cc = 1;
        
        if(++ai > 7)
        {
          break;
        }
      }
      else
      {
        cc = 0;
        digit = c - '0';
        
        if (digit > 9)
        {
          digit = 10 + (c & 0xdf) - 'A';
        }
        
        a[ai] = (a[ai] << 4) + digit;
      }
    }
    
    // save the tun IPv6 prefix
    for (i = 0; i < 4; i++)
    {
      tun_ip_prefix[2 * i]     = a[i] >> 8;
      tun_ip_prefix[2 * i + 1]  = a[i] & 0xFF;
    }
    
    printf("Tun IP Prefix 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
           tun_ip_prefix[0], tun_ip_prefix[1], tun_ip_prefix[2], tun_ip_prefix[3],
           tun_ip_prefix[4], tun_ip_prefix[5], tun_ip_prefix[6], tun_ip_prefix[7]);

    
    /* Get # elided and shift what's after to the end */
    cc = 8 - ai;
    
    for(i = 0; i < cc; i++)
    {
      if ((8 - i - cc) <= scc)
      {
        a[7 - i] = 0;
      }
      else
      {
        a[7 - i] = a[8 - i - cc];
        a[8 - i - cc] = 0;
      }
    }
    
    sprintf(lladdr, "fe80::%x:%x:%x:%x", a[1] & 0xfefd, a[2], a[3], a[7]);
    ssystem("ifconfig %s add %s/64", tundev, lladdr);
  }
  
  ssystem("ifconfig %s\n", tundev);
}

/***************************************************************************//**
 * @fn          TUN_readTunIntf
 *
 * @brief       TUN read function
 *
 * @param[in]   none
 *
 * @param[out]  none
 *
 * @return      Read length
 *
 ******************************************************************************/
SINT32 TUN_readTunIntf(void)
{
  SINT32 rc;
  
  rc = read(TUN_fd, TUN_rxBuff + TUN_rxBuff_inOffset, TUN_RX_BUFF_LEN - TUN_rxBuff_inOffset);
  
  if (rc <= 0)
  {
     printf("Read from tun failed with error code %d\n",rc);
  }
  
  return rc;
}
/***************************************************************************//**
 * @fn          TUN_write
 *
 * @brief       TUN write function
 *
 * @param[in]   buff_p - buffer pointer
 * @param[in]    cnt - buffer length
 *
 * @param[out]  none
 *
 * @return      Write status
 *
 ******************************************************************************/
SINT32 TUN_write(UINT8 *buff_p, SINT32 cnt)
{
  SINT32 rc, bytesLeft = cnt, bytesWritten = 0;
  
  while (bytesLeft > 0)
  {
    rc = write(TUN_fd, buff_p + bytesWritten, bytesLeft);
    
    if (rc <= 0)
    {
      printf("Write to tun failed with error code %d. %d bytes written, %d bytes left\n",rc,bytesWritten,bytesLeft);
      return rc;
    }
    else
    {
      bytesLeft -= rc;
      bytesWritten += rc;
    }
  }
  
  return bytesWritten;
}


/***************************************************************************//**
 * @fn          TUN_pktCheck
 *
 * @brief       Check if received packet is valid or not.
 *
 * @param[in]    pktBuff_p - buffer pointer
 * @param[in]    pktLen - buffer length
 *
 * @param[out]  none
 *
 * @return      Write status
 *
 ******************************************************************************/
BOOL TUN_pktCheck(UINT8 *pktBuff_p, SINT32 pktLen)
{
  if (pktLen <= IPv6_HDR_LEN)
  {
    return FALSE;
  }
  
  return TRUE;
}

/*****************************************************************************/
#if FEATURE_DTLS_TEST
#include "../../apps/coap/coap.h"
#include "../../dtls/dtls_client.h"

enum enumTestClientMsgId
{
  DTLS_TESTCLIENT_MSG_TX,
  DTLS_TESTCLIENT_MSG_RX,
  DTLS_TESTCLIENT_MSG_TMR,
};

struct structPacket
{
  uint8_t *data_p;
  uint16_t len;
};

typedef struct
{
  UINT8 id;
} DTLS_testCleint_msg_s;

UINT8 DTLS_testClientIntfMbxMsgBuff[sizeof(DTLS_testCleint_msg_s)];
pthread_t DTLS_testClientThreadInfo;
pltfrm_mbx_t DTLS_testClientIntfMbx;

static int dtlsInitialized = 0;
static struct uip_udp_conn udp_conn;
static struct udp_simple_socket test_dtls_socket;
static process_obj_t test_dtls_timer_handler_obj;
static struct structPacket decodedPacket;
uint8_t testDtlsUdpIpHeader[UIP_IPH_LEN + UIP_UDPH_LEN];
uint8_t DTLS_txPacket[512];
uint8_t DTLS_txPacketLen;

/*****************************************************************************
*  FUNCTION:  test_dtls_data_handler
*
*  DESCRIPTION: This is the callback function called by the DTLS test client to pass
*  a decrypted application data to the host through the TUN interface.
*
*  PARAMETERS:
*  decodedPacek_p: Not used.  The decrypted packet is contained in the buffer
*  appRxFifo[appRxFifo_ReadIdx].c.
*
*  RETURNS:
*
*****************************************************************************/
void test_dtls_data_handler(void *decodedPacek_p)
{
  uint16_t len = appRxLenFifo[appRxFifo_ReadIdx];
  struct uip_ip_hdr *uip_hdr_p = ((struct uip_ip_hdr *)&(appRxFifo[appRxFifo_ReadIdx].c[UIP_LLH_LEN]));
  struct uip_udp_hdr *udp_hdr_p =  ((struct uip_udp_hdr *)&(appRxFifo[appRxFifo_ReadIdx].c[UIP_LLIPH_LEN]));
  
  uip_hdr_p->len0 = ((len + UIP_UDPH_LEN) >> 8);
  uip_hdr_p->len1 = ((len + UIP_UDPH_LEN) & 0xff);
  udp_hdr_p->udplen = UIP_HTONS(len + UIP_UDPH_LEN);
  udp_hdr_p->udpchksum = 0;
  udp_hdr_p->srcport = UIP_HTONS(COAP_DEFAULT_PORT);
  
  /* Calculate UDP checksum. */
  udp_hdr_p->udpchksum = udp_chksum(appRxFifo[appRxFifo_ReadIdx].c);
  
  TUN_write(appRxFifo[appRxFifo_ReadIdx].c, UIP_IPH_LEN + UIP_UDPH_LEN + len);
}

/*****************************************************************************
*  FUNCTION:  test_dtls_timer_handler
*
*  DESCRIPTION: Callback function when the DTLS retransmit timer times out.
*
*  PARAMETERS:
*
*  RETURNS:
*
*****************************************************************************/
void test_dtls_timer_handler(process_event_t event, process_data_t data)
{
  DTLS_testCleint_msg_s msg;
  
  msg.id = DTLS_TESTCLIENT_MSG_TMR;
  pltfrm_mbx_send(&DTLS_testClientIntfMbx, (UINT8 *) &msg, PLTFRM_MBX_OPN_NO_WAIT);
}
/*****************************************************************************
*  FUNCTION: dtls_test_client_tx
*
*  DESCRIPTION: This is the function called to handle a packet received from host through the
*  TUN interface
*
*  PARAMETERS:
*
*  RETURNS:
*
*****************************************************************************/
void dtls_test_client_tx()
{
  session_t session;
  dtls_peer_t *peer;
  struct uip_ip_hdr *uip_ip_hdr =  (struct uip_ip_hdr *)DTLS_txPacket;
  struct uip_udp_hdr *uip_udp_hdr = (struct uip_udp_hdr *)(DTLS_txPacket + UIP_IPH_LEN);
  uint8_t sendWithoutDtls = 0;
  
  memset(&session, 0, sizeof(session));
  uip_ipaddr_copy(&(session.addr), &(uip_ip_hdr->destipaddr));
  session.port = UIP_NTOHS(uip_udp_hdr->destport);
  session.size = sizeof(session.addr) + sizeof(session.port);
  
  if (session.port == COAP_DEFAULT_PORT)
  {
    session.port = COAP_DTLS_UDP_PORT;
    
    if (!dtlsInitialized)
    {
      memset(&udp_conn, 0, sizeof(udp_conn));
      udp_conn.lport = COAP_DTLS_UDP_PORT;
      udp_conn.ttl = 32;
      memset(&test_dtls_socket, 0, sizeof(test_dtls_socket));
      test_dtls_socket.udp_conn = &udp_conn;
      test_dtls_timer_handler_obj.process_post = test_dtls_timer_handler;
      
      init_dtls(&test_dtls_socket, &test_dtls_timer_handler_obj, test_dtls_data_handler, &decodedPacket);
      dtlsInitialized = 1;
    }
    
    peer = dtls_get_peer(dtls_context[0], &session);
    
    if (peer == NULL)
    {
      printf("Start DTLS connect\n");
      memcpy(testDtlsUdpIpHeader, DTLS_txPacket, UIP_IPH_LEN + UIP_UDPH_LEN);
      dtls_connect(dtls_context[0], &session);
      sendWithoutDtls = 1;
    }
    else if (peer->state == DTLS_STATE_CONNECTED)
    {
      memcpy(testDtlsUdpIpHeader, DTLS_txPacket, UIP_IPH_LEN + UIP_UDPH_LEN);
      dtls_write(dtls_context[0], &session, DTLS_txPacket + UIP_IPH_LEN + UIP_UDPH_LEN, DTLS_txPacketLen - UIP_IPH_LEN - UIP_UDPH_LEN);
    }
    else
    {
      sendWithoutDtls = 1;
    }
  }
  else
  {
    sendWithoutDtls = 1;
  }
  
  if (sendWithoutDtls)
  {
    if (uip_input_len != 0)
    {
      //FIFO full
      uipTxPacketLossCount++;
    }
    else
    {
      uipBuffer_LockWriteBuffer(); //fengmo debug GW packet loss
      memcpy(&uip_input_buf[UIP_LLH_LEN], DTLS_txPacket, DTLS_txPacketLen); //fengmo debug GW packet loss
      uip_input_len = DTLS_txPacketLen; //fengmo debug GW packet loss
      uipBufferIndex_inc(&uipBuffer_WriteIdx); //fengmo debug GW packet loss
      uipBuffer_UnlockWriteBuffer(); //fengmo debug GW packet loss
      tcpip_input(NULL);
    }
  }
}
/*****************************************************************************
*  FUNCTION: dtls_test_client_rx
*
*  DESCRIPTION:  This is the function called to handle a packet received from the
*  sensor node (the DTLS server)
*
*  PARAMETERS:
*
*  RETURNS:
*
*****************************************************************************/
void dtls_test_client_rx()
{
  session_t session;
  dtls_peer_t *peer;
  struct uip_ip_hdr *uip_ip_hdr;
  struct uip_udp_hdr *uip_udp_hdr;
  uint16_t DTLS_rxPacketLen;
  
  appRxFifoIndex_inc(&appRxFifo_ReadIdx);
  
  uip_ip_hdr =  (struct uip_ip_hdr *)(appRxFifo[appRxFifo_ReadIdx].c);
  uip_udp_hdr = (struct uip_udp_hdr *)(appRxFifo[appRxFifo_ReadIdx].c + UIP_IPH_LEN);
  DTLS_rxPacketLen = appRxLenFifo[appRxFifo_ReadIdx];
  
  memset(&session, 0, sizeof(session));
  uip_ipaddr_copy(&(session.addr), &(uip_ip_hdr->srcipaddr));
  session.port = UIP_NTOHS(uip_udp_hdr->srcport);
  session.size = sizeof(session.addr) + sizeof(session.port);
  
  if (session.port == COAP_DTLS_UDP_PORT)
  {
    peer = dtls_get_peer(dtls_context[0], &session);
    
    if (peer == NULL)
    {
      TUN_write(appRxFifo[appRxFifo_ReadIdx].c, DTLS_rxPacketLen);
    }
    else
    {
      dtls_handle_message(dtls_context[0], &session, appRxFifo[appRxFifo_ReadIdx].c + UIP_IPH_LEN + UIP_UDPH_LEN,
                          DTLS_rxPacketLen - UIP_IPH_LEN - UIP_UDPH_LEN);
    }
  }
  else
  {
    TUN_write(appRxFifo[appRxFifo_ReadIdx].c, DTLS_rxPacketLen);
  }
  
  appRxLenFifo[appRxFifo_ReadIdx] = 0;
}
/*****************************************************************************
*  FUNCTION:  dtls_test_client_send
*
*  DESCRIPTION:  This is the messenger function called by the TUN thread
*  to inform the DTLS test client thread of a packet received from the host through the TUN interface
*
*  PARAMETERS: ipUdpPacket_p: the packet received from the host
*       ipUdpPacket_p: the length of the host packet
*
*  RETURNS:
*
*****************************************************************************/
void dtls_test_client_send(uint8_t *ipUdpPacket_p, int pLen)
{
  DTLS_testCleint_msg_s msg;
  
  memcpy(DTLS_txPacket, ipUdpPacket_p, pLen);
  DTLS_txPacketLen = pLen;
  
  msg.id = DTLS_TESTCLIENT_MSG_TX;
  pltfrm_mbx_send(&DTLS_testClientIntfMbx, (UINT8 *) &msg, PLTFRM_MBX_OPN_NO_WAIT);
}
/*****************************************************************************
*  FUNCTION:  dtls_test_client_receive
*
*  DESCRIPTION: This is the messenger function called by the network thread
*  to inform the DTLS test client thread of a packet received from the sensor node
*
*  PARAMETERS:
*
*  RETURNS:
*
*****************************************************************************/
void dtls_test_client_receive(uint8_t *ipUdpPacket_p, int pLen)
{
  DTLS_testCleint_msg_s msg;
  
  memcpy(appRxFifo[appRxFifo_WriteIdx].c, ipUdpPacket_p, pLen);
  appRxLenFifo[appRxFifo_WriteIdx] = pLen;
  appRxFifoIndex_inc(&appRxFifo_WriteIdx);
  
  msg.id = DTLS_TESTCLIENT_MSG_RX;
  pltfrm_mbx_send(&DTLS_testClientIntfMbx, (UINT8 *) &msg, PLTFRM_MBX_OPN_NO_WAIT);
}

/*****************************************************************************
*  FUNCTION:  DTLS_testClientEntryFn
*
*  DESCRIPTION:  Thread function for the DTLS test client thread
*
*  PARAMETERS:
*
*  RETURNS:
*
*****************************************************************************/
void* DTLS_testClientEntryFn(void *param_p)
{
  while (1)
  {
    int rc = pltfrm_mbx_pend(&DTLS_testClientIntfMbx, DTLS_testClientIntfMbxMsgBuff,
                             PLTFRM_MBX_OPN_WAIT_FOREVER);
    
    if (rc)
    {
      DTLS_testCleint_msg_s *reqMsg_p = (DTLS_testCleint_msg_s *) DTLS_testClientIntfMbxMsgBuff;
      
      switch (reqMsg_p->id)
      {
      case DTLS_TESTCLIENT_MSG_TX:
        dtls_test_client_tx();
        break;
        
      case DTLS_TESTCLIENT_MSG_RX:
        dtls_test_client_rx();
        break;
        
      case DTLS_TESTCLIENT_MSG_TMR:
        dtls_handle_retransmit(dtls_context[0]);
        break;
        
      default:
        break;
      }
    }
  }
}

#endif  //FEATURE_DTLS_TEST

/***************************************************************************//**
 * @fn          TUN_rxThreadEntryFn
 *
 * @brief       TUN RX Thread Function
 *
 * @param[in]   param_p - function parameter
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void* TUN_rxThreadEntryFn(void *param_p)
{
  TUN_rxBuff_inOffset = 0;
  
  while (1)
  {
    SINT32 pktLen = TUN_readTunIntf();
    
    if (pktLen > 0 && pktLen <= UIP_BUFSIZE)
    {
      if (NetworkReadyFlag == 0x0)
      {
        continue;
      }
      
      pktLen += TUN_rxBuff_inOffset;
      
      TUN_rxBuff_outOffset = 0;
      
      while(TUN_pktCheck(TUN_rxBuff + TUN_rxBuff_outOffset, pktLen - TUN_rxBuff_outOffset))
      {
        uint8_t *uip_hdr_p = (TUN_rxBuff + TUN_rxBuff_outOffset);
        uint16_t ip_pkt_len = ((uint16_t)(uip_hdr_p[4]) << 8) + uip_hdr_p[5] + UIP_IPH_LEN;
        uint16_t rxDataLen = pktLen - TUN_rxBuff_outOffset;
        
        if (uip_hdr_p[6] == 0x3A)
        {
          //ICMPv6 packet, "dest unreachable", drop it since the node doesn't care
          //printf("Tun forwarding failed. Dropping the packet\n");
          TUN_rxBuff_outOffset += ip_pkt_len;
          continue;
        }
        if (ip_pkt_len > (UIP_BUFSIZE - UIP_LLH_LEN))
        {
          printf("%s: buffer overflow, TUN IP packet too large: %d\n", __FUNCTION__, ip_pkt_len);
          for (int i = 0; i < rxDataLen; i++)
          {
            printf("IP[%d] = %x\n", i, uip_hdr_p[i]);
          }
          continue;
        }
        
        if (ip_pkt_len <= rxDataLen)
        {
#if FEATURE_DTLS_TEST
          dtls_test_client_send(TUN_rxBuff + TUN_rxBuff_outOffset, ip_pkt_len);
#else
          COMM_flowControl();

          uipBuffer_LockWriteBuffer(); 
          if (uip_input_len != 0)
          {
            uipTxPacketLossCount++;
            uipBuffer_UnlockWriteBuffer(); 
            printf("%s: UIP TX overflow: %d. uipBuffer_WriteIdx=%d \n", __FUNCTION__, uipTxPacketLossCount,uipBuffer_WriteIdx);
          }
          else
          {
            // copy data from rx buf to uip buffer and set packet length
            memcpy(&uip_input_buf[UIP_LLH_LEN], TUN_rxBuff + TUN_rxBuff_outOffset, ip_pkt_len);  
            uip_input_len = ip_pkt_len + UIP_LLH_LEN;  
            uipBufferIndex_inc(&uipBuffer_WriteIdx); 
            uipBuffer_UnlockWriteBuffer(); 
            
            tcpip_input(NULL);
          }
          TUN_rxBuff_outOffset += ip_pkt_len;
#endif
        }
        else
        {
          //A partial IP packet has been received
          break;
        }
      }
      
      if (TUN_rxBuff_outOffset > 0 && TUN_rxBuff_outOffset < pktLen)
      {
        //Move the partial packet down to the start of the buffer
        int i;
        for (i = 0; i < pktLen - TUN_rxBuff_outOffset; i++)
        {
          TUN_rxBuff[i] = TUN_rxBuff[i+TUN_rxBuff_outOffset];
        }
        TUN_rxBuff_inOffset = i;
      }
      else
      {
        TUN_rxBuff_inOffset = 0;
      }
    }
    else
    {
      break;
    }
  }
  
  return NULL;
}

/***************************************************************************//**
 * @fn          TUN_init
 *
 * @brief       Init
 *
 * @param[in]   none
 *
 * @param[out]  none
 *
 * @return      Init status
 *
 ******************************************************************************/
SINT32 TUN_init(void)
{
  SINT32 rc;
  
#if FEATURE_DTLS_TEST
  
  if (pltfrm_mbx_init(&DTLS_testClientIntfMbx, sizeof(DTLS_testCleint_msg_s), 8) < 0)
  {
    proj_assert(0);
  }
  
  rc = pthread_create(&DTLS_testClientThreadInfo, NULL, DTLS_testClientEntryFn, NULL);
#endif
  
  TUN_fd = TUN_tun_alloc(TUN_dev);
  
  if (TUN_fd < 0)
  {
    printf("tun-alloc device %s failed\n", TUN_dev);
    return(-1);
  }
  
  // configure the interface
  TUN_ifconf(TUN_dev, tun_addr);
  
  // Start rx thread
  rc = pthread_create(&TUN_rxThreadInfo, NULL, TUN_rxThreadEntryFn, NULL);
  
  if (rc != 0)
  {
    printf("Failed to create TUN RX thread\n");
    proj_assert(0);
  }
  
  return 1;
}

/***************************************************************************//**
 * @fn          TUN_fallback_init
 *
 * @brief       Dummy function
 *
 * @param[in]   none
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void TUN_fallback_init(void)
{
  return; 
}

/***************************************************************************//**
 * @fn          TUN_output
 *
 * @brief       TUN output
 *
 * @param[in]   none
 *
 * @param[out]  none
 *
 * @return      none
 *
 ******************************************************************************/
void TUN_output(void)  
{
  SINT32 pktLen;
  
  pktLen = uip_len;
  
  if (pktLen >= IPv6_HDR_LEN)
  {
    if (pktLen < TUN_TX_BUFF_LEN)
    {
#if FEATURE_DTLS_TEST
      dtls_test_client_receive((uint8_t *)UIP_IP_BUF, pktLen);
#else
      memcpy(TUN_txBuff, UIP_IP_BUF, pktLen);
      TUN_write(TUN_txBuff, pktLen);
#endif
    }
  }
}

const struct uip_fallback_interface tun_interface =
{
  TUN_fallback_init, TUN_output
};
