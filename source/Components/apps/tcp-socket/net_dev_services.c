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

#include "net_dev_services.h" 
 
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>

#include "tcp-socket.h"
#include "sys/clock.h"

// LED functions
#include "GPIO.h"
#include "Board.h"

/*
 TSCH Network Services Implementation
*/

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MQTT_SVR_ADDR 0x26867E4B

#define TCP_SEND_WAIT_DATA_SENT_COMPLETE

// API timeout value should be bigger than TCPIP stack
#define TCP_SOCKET_CLOSE_API_TIMEOUT_WAIT             (CLOCK_SECOND * 10 )
#define TCP_SOCKET_SEND_API_TIMEOUT                   (CLOCK_SECOND * 300 )


extern TCP_SOCKET_DBG_s TCP_SOCKET_Dbg;
extern uint32_t tcpip_timeout_value;

//*****************************************************************************
//                      GLOBAL VARIABLES
//*****************************************************************************

/*-----------------------------------------------------------------------------
global variables needed for the functions in this file
These values are to be populated before the functions are called
-----------------------------------------------------------------------------*/

// TcpSecSocOpts_t g_SecSocOpts;

// SlSockSecureFiles_t	g_sockSecureFiles;


//*****************************************************************************
//                      STATIC FUNCTIONS
//*****************************************************************************

#if 0
static i32 buf_printf(const u8 *buf, u32 len, u32 idt)
{
	i32 i = 0;
	for(i = 0; i < len; i++)
	{
		PRINTF("%02x ", *buf++);

		if(0x03 == (i & 0x03))
			PRINTF(" ");

		if(0x0F == (i & 0x0F))
		{
			i32 j = 0;
			PRINTF("\n\r");

			for(j = 0; j < idt; j++)
				PRINTF(" ");
		}
	}

	PRINTF("\n\r");

	return len;
}


/*-----------------------------------------------------------------------------
Open a TCP socket and modify its properties i.e security options if req.
Socket properties modified in this function are based on the options set
outside the scope of this function.
Returns a valid handle on success, otherwise a negative number.
-----------------------------------------------------------------------------*/

static i32 create_socket(void)
{
	i32 MqttSocketFd, Status;

	//If TLS is required
	if (g_SecSocOpts.SecurityMethod <= 5)
	{
		MqttSocketFd = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_SEC_SOCKET);  
		if (MqttSocketFd < 0)
		{
			return(MqttSocketFd);
		}

		//u8 method = g_SecSocOpts.SecurityMethod;
		//u32 cipher = g_SecSocOpts.SecurityCypher;

		//Set Socket Options that were just defined
		Status = sl_SetSockOpt(MqttSocketFd, SL_SOL_SOCKET, SL_SO_SECMETHOD,
								&g_SecSocOpts.SecurityMethod, sizeof(g_SecSocOpts.SecurityMethod));
		if (Status < 0)
		{
			sl_Close(MqttSocketFd);
			return (Status);
		}

		Status = sl_SetSockOpt(MqttSocketFd, SL_SOL_SOCKET, SL_SO_SECURE_MASK,
								&g_SecSocOpts.SecurityCypher, sizeof(g_SecSocOpts.SecurityCypher));
		if (Status < 0)
		{
			sl_Close(MqttSocketFd);
			return (Status);
		}

		Status = sl_SetSockOpt(MqttSocketFd, SL_SOL_SOCKET, SL_SO_SECURE_FILES,
								g_sockSecureFiles.secureFiles, sizeof(g_sockSecureFiles));
		if (Status < 0)
		{
			sl_Close(MqttSocketFd);
			return (Status);
		}
	}
	// If no TLS required
	else
	{
		MqttSocketFd = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SL_IPPROTO_TCP); // consider putting 0 in place of SL_IPPROTO_TCP
	}

	return(MqttSocketFd);

}// end of function


/*-----------------------------------------------------------------------------
This function takes an ipv4 address in dot format i.e "a.b.c.d" and returns the
ip address in Network byte Order, which can be used in connect call
-----------------------------------------------------------------------------*/

static u32 svr_addr_NB_order(i8 *svr_addr_str)
{
   u8 addr[4];
   i8 i = 0;
   i8 *token;
   u32 svr_addr;

   /* get the first token */
   token = strtok(svr_addr_str, ".");

   /* walk through other tokens */
   while( token != NULL )
   {
      addr[i++] = atoi(token);
      token = strtok(NULL, ".");
   }

   svr_addr = *((u32 *)&addr);
wi
   return(svr_addr);
}
#endif // if 0


//*****************************************************************************
//                      Network Services functions
//*****************************************************************************


/*-----------------------------------------------------------------------------
Open a TCP socket with required properties
Also connect to the server.
Returns a valid handle on success, NULL on failure.
-----------------------------------------------------------------------------*/
void *tcp_open(u32 nwconn_info, i8 *server_addr, u16 port_number)
{
  struct tcp_socket *pSocket;
  int st;
  Bool mbSt;
  SOCKET_NOTIFY_MSG_s evMsg;
  UInt key; 
  
  // ingore nwconn_info
  (void)nwconn_info;

  TCP_SOCKET_Dbg.tcp_api_open++;
  pSocket = tcp_socket_create();

  if (pSocket == NULL)
  {
    TCP_SOCKET_Dbg.tcp_api_open_create_socket_error++;
    return NULL;
  }       
  st = tcp_socket_connect(pSocket,(uip_ipaddr_t *)server_addr,port_number);
  if (st == 0 )
  {
    tcp_socket_remove(pSocket);
    return NULL;
  }

  // wait the mailbox to notify the event
  // enter critical section
  key = Task_disable(); 

  pSocket->tcp_open_pending = 1;

  // exit critical section
  Task_restore(key);
    
  while (1)
  {
    mbSt = Mailbox_pend(pSocket->mboxNotify,&evMsg,BIOS_WAIT_FOREVER);

    if (evMsg.event == TCP_SOCKET_CONNECTED)
    { // the socket conenction is established      
      TCP_SOCKET_Dbg.tcp_open_api_rx_connected++;

      // enter critical section
      key = Task_disable(); 
  
      pSocket->tcp_open_pending = 0;

      // exit critical section
      Task_restore(key);

      if (evMsg.status == 0)
      { // toggle LED
#ifdef TCP_LED_ENABLE

#ifdef CC2650_SENSORTAG
        //GPIO_toggle(SENSORTAG_CC2650_LED2);
        LED_toggle(Board_LED2);
#else
        //GPIO_toggle(SRF06EB_CC2650_LED4);
        LED_toggle( Board_LED4);
#endif  
#endif
        return pSocket;      
      }
    }
    else 
    { // unexpected event
      TCP_SOCKET_Dbg.tcp_open_api_unexpected_ev++;
      // post message back to mailbox??
    }
    
    close_tcp(pSocket);
    return NULL;
    
  }

  //return NULL;

}// end of function


int tcp_send(void *comm, const u8 *buf, u32 datalen)
{
  struct tcp_socket *pSocket;  
  int len,remaining_len;
  Bool mbSt;
  SOCKET_NOTIFY_MSG_s evMsg;

  TCP_SOCKET_Dbg.tcp_send_api++;
  
  pSocket = (struct tcp_socket *)comm;
  //make sure it is valid
  if ( (pSocket->valid == 0) || (datalen == 0 ) ||
       (pSocket->tcp_internal_state_flag != TCP_STATE_CONNECTED) )
    // in case TCP connection has error, we need to quit and let
    // host API reopen TCP connection
    return (-1);

  remaining_len = datalen;

  while (remaining_len)
  {
    UInt key; 

    // enter critical section
    key = Task_disable(); 
    
    
    // copy user data buf to output buffer
    len = MIN(remaining_len, pSocket->output_data_maxlen - pSocket->output_data_len);

    memcpy(&pSocket->output_data_ptr[pSocket->output_data_len], buf, len);
    pSocket->output_data_len += len;

    // exit critical section
    Task_restore(key);
    
    remaining_len -=len;
#ifndef TCP_SEND_WAIT_DATA_SENT_COMPLETE
    // don't return until all data is sent
    if (remaining_len == 0)
    {
      return datalen;
    }
#endif
    // wait for data sent event
    // enter critical section
    key = Task_disable(); 
    
    pSocket->tcp_send_pending = 1;

    // exit critical section
    Task_restore(key);
    
    while (1)
    {
      mbSt = Mailbox_pend(pSocket->mboxNotify,&evMsg,TCP_SOCKET_SEND_API_TIMEOUT);

      // enter critical section
      key = Task_disable(); 
  
      pSocket->tcp_send_pending = 0;
      
      // exit critical section
      Task_restore(key);
      
      if (mbSt == FALSE)
      { // mailbox is timeout
                        
        TCP_SOCKET_Dbg.tcp_send_api_timeout++;

        return (-1);
      }
      
      if (evMsg.event == TCP_SOCKET_DATA_SENT)
      { // the socket conenction is established
                        
        if (evMsg.status == 0)
        {       
#ifdef TCP_SEND_WAIT_DATA_SENT_COMPLETE        
          // return to caller
          if (remaining_len == 0)
          {
            return datalen;
          }
#endif          
          break;
        }
        else
        { // connection is close or error
          
          return (-1);
        }
      }
    }
  }
  return datalen;

}// end of function

#if 0
static void tcp_readnotify( long so, void *arg )
{
  Semaphore_post( tcpRead_sem );
}

static UInt32 msec2biostick( uint32_t msecs)
{
  UInt32 biosticks;

  if (msecs == 0)
  {
    biosticks = BIOS_NO_WAIT;
  }
  else if (msecs == 0xfffffffful)
  {
    /* Type conversion is required to suppress potential compiler warnings
     * as the SYS/BIOS does not provide type safe convention. */
    biosticks = (UInt32) BIOS_WAIT_FOREVER;
  }
  else
  {
#if 0  
    uint_fast64_t intermediate = msecs;
    intermediate *= 1000;
    intermediate /= Clock_tickPeriod;
    if (intermediate >= ((uint_fast64_t) 1 << (sizeof(UInt)*8 - 1)))
    {
      /* Out of range. Cannot block for the full timeout period */
      intermediate = ((uint_fast64_t) 1 << (sizeof(UInt)*8 - 1)) - 1;
    }
    biosticks = (UInt) intermediate;

#endif
  }
  return biosticks;
}
#endif

static UInt32 sec2biostick( uint32_t secs)
{
  UInt32 biosticks;

  if (secs == 0)
  {
    biosticks = BIOS_NO_WAIT;
  }
  else if (secs == 0xfffffffful)
  {
    /* Type conversion is required to suppress potential compiler warnings
     * as the SYS/BIOS does not provide type safe convention. */
    biosticks = (UInt32) BIOS_WAIT_FOREVER;
  }
  else
  {
#if 0
    uint_fast64_t intermediate = msecs;
    intermediate *= 1000;
    intermediate /= Clock_tickPeriod;
    if (intermediate >= ((uint_fast64_t) 1 << (sizeof(UInt)*8 - 1)))
    {
      /* Out of range. Cannot block for the full timeout period */
      intermediate = ((uint_fast64_t) 1 << (sizeof(UInt)*8 - 1)) - 1;
    }
    biosticks = (UInt) intermediate;
#else
    // TSCH porting
    //  CLOCK_SECOND
    biosticks = CLOCK_SECOND * (secs);
#endif
  }
  return biosticks;
}
 
int tcp_recv(void *comm, u8 *buf, u32 datalen, u32 wait_secs, bool *timed_out)
{
  struct tcp_socket *pSocket;  
  int len,remaining_len,copylen;
  Bool mbSt;
  SOCKET_NOTIFY_MSG_s evMsg;
  u32 timeout_val;
  UInt key; 

  TCP_SOCKET_Dbg.tcp_api_recv++;
  
  pSocket = (struct tcp_socket *)comm;
  //make sure it is valid
  if ( (pSocket->valid == 0) || (datalen == 0 ) ||
       (pSocket->tcp_internal_state_flag != TCP_STATE_CONNECTED) )
    return (-1);

  timeout_val = sec2biostick(wait_secs);
   
  remaining_len = datalen;
  copylen = 0;
  while (remaining_len)
  {
    // enter critical section
    key = Task_disable(); 

    len = MIN(remaining_len,pSocket->input_data_len);
       
    // copy data to user buffer from input buffer
    memcpy(&buf[copylen],&(pSocket->input_data_ptr[0]),len);

    // move the data in input buffer to beginning of buffer
    memcpy(&pSocket->input_data_ptr[0],
	     &pSocket->input_data_ptr[len],pSocket->input_data_len - len);
	    
    copylen += len;
    remaining_len -= len;

    pSocket->input_data_len -= len;
    
    // exit critical section
    Task_restore(key);
    
    if (remaining_len == 0)
    { // no timeout
      *timed_out = false;

      return copylen;
    }

    // enter critical section
    key = Task_disable(); 

    pSocket->tcp_recv_pending = 1;

    // exit critical section
    Task_restore(key);
    
    while (1)
    {
      mbSt = Mailbox_pend(pSocket->mboxNotify,&evMsg,timeout_val);

      // enter critical section
      key = Task_disable(); 

      pSocket->tcp_recv_pending = 0;

      // exit critical section
      Task_restore(key);
      // toggle LED
#ifdef TCP_LED_ENABLE

#ifdef CC2650_SENSORTAG
      
#else      
      //GPIO_toggle(SRF06EB_CC2650_LED1);
      LED_toggle( Board_LED2);
#endif      
#endif

      if (mbSt == FALSE)
      { // mailbox is timeout
                
        *timed_out = true;
        TCP_SOCKET_Dbg.tcp_recv_api_timeout++;

        return (-1);
      }

      if (evMsg.event == TCP_SOCKET_DATA_RECEIVE)
      { // the socket conenction is established
              
        if (evMsg.status != 0 )
        { // error happen, return
          *timed_out = false;
          TCP_SOCKET_Dbg.tcp_recv_api_err++;          
          
          return (-1);
        }

        TCP_SOCKET_Dbg.tcp_recv_api_ok++;
        break;
      }
    }

  }
  
  // never get here

  return (-1);
}// end of function


int close_tcp(void *comm)
{
  struct tcp_socket *pSocket; 
  Bool mbSt;
  SOCKET_NOTIFY_MSG_s evMsg;
  UInt key; 

  TCP_SOCKET_Dbg.tcp_api_close++;
  
  pSocket = (struct tcp_socket *)comm;
  if (pSocket->valid == 0) 
    return (-1);

  // for uIP, we need to yield the current APPCALL
  // in the next APPCALL, we can close connection
  // at lease 2 TCP poll period
  Task_sleep((TCP_PERIODIC_TIMER_TICKS)*2);
  
  tcp_socket_close(pSocket);

  // enter critical section
  key = Task_disable(); 
    
  // wait for data sent event
  pSocket->tcp_close_pending = 1;

  // exit critical section
  Task_restore(key);
  
  while (1)
  {
    mbSt = Mailbox_pend(pSocket->mboxNotify,&evMsg,TCP_SOCKET_CLOSE_API_TIMEOUT_WAIT);
    // enter critical section
    key = Task_disable(); 
    
    pSocket->tcp_close_pending = 0;    
      
    // exit critical section
    Task_restore(key);

    if (mbSt != TRUE)
      TCP_SOCKET_Dbg.tcp_close_timeout++;
    else
      TCP_SOCKET_Dbg.tcp_close_ok++;
    
    break;
    
  }
  tcp_socket_unlisten(pSocket);
  tcp_socket_remove(pSocket);

  return 0;
	
}// end of function


u32 rtc_secs(void)
{
#if 0
	uint64_t count;
	count = Timer_getCount64( NULL );

	return (u32)(count >> 32);
#endif
  return clock_seconds();

}


