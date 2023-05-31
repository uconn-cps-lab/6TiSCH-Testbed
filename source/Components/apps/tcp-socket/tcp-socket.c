/*
 * Copyright (c) 2012, Thingsquare, http://www.thingsquare.com/.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "net/uip.h"
#include "uip_rpl_process.h"

#include "lib/list.h"

#include "tcp-socket.h"
#include "net/uip-ds6.h"
#include <stdio.h>
#include <string.h>

// LED functions
#include "GPIO.h"
#include "Board.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*
  TI RTOS general porting
  1. continue to use PT_THREAD to change function flow so that
      we don't need to use the FSM
   2. comment out
       PROCESS_CONTEXT_BEGIN(&tcp_socket_process);
       PROCESS_CONTEXT_END();
       replace the function by provoding the mailbox

*/
// Robert printf do nothing
// later, we can replace this function by LOG_INFO
#define printf(...)


//Mailbox_Handle ptcp_socket_mb;
//ADDED Contiki 2.7: Changed Mailbox for process object
process_obj_t   tcp_socket_process_obj;

extern process_event_t tcpip_event;
extern const Mailbox_Handle socketNotifyMb;
extern const Mailbox_Handle tcpsocket_mailbox;
/*
  tcp socket and buffer pool
*/
struct tcp_socket tcp_socket_pool[MAX_NUM_TCP_SOCKET];
TCP_SOCKET_BUF_s  tcp_socket_buf_pool[MAX_NUM_TCP_SOCKET];

TCP_SOCKET_DBG_s TCP_SOCKET_Dbg;
LIST(socketlist);


#if 0
/*---------------------------------------------------------------------------*/
PROCESS(tcp_socket_process, "TCP socket process");
/*---------------------------------------------------------------------------*/
#endif
static void
call_event(struct tcp_socket *s, tcp_socket_event_t event)
{
  SOCKET_NOTIFY_MSG_s msg;
  Bool mbSt;
  uint16_t post_msg;
#if 0
  if(s != NULL && s->event_callback != NULL) {
    s->event_callback(s, s->ptr, event);
  }
#endif

  if ( ( s->tcp_open_pending == 0 ) &&
       ( s->tcp_send_pending == 0 ) &&
       ( s->tcp_recv_pending == 0 ) &&
       ( s->tcp_close_pending == 0 ) )
  { // no pending request
    return ;
  }


  post_msg = 1;
  if (s->tcp_open_pending)
  {
    msg.event = TCP_SOCKET_CONNECTED;
    if (event == TCP_SOCKET_CONNECTED)
    {
      msg.status = 0;
      TCP_SOCKET_Dbg.post_tcp_open++;
    }
    else if ( (event == TCP_SOCKET_CLOSED) ||
              (event == TCP_SOCKET_TIMEDOUT) ||
              (event == TCP_SOCKET_ABORTED) )
    {
      msg.status = -1;
      TCP_SOCKET_Dbg.post_tcp_open_err++;
    }
    else
    {
      post_msg = 0;
    }

  }
  else if (s->tcp_send_pending)
  {
    msg.event = TCP_SOCKET_DATA_SENT;
    if (event == TCP_SOCKET_DATA_SENT)
    {
      msg.status = 0;
      TCP_SOCKET_Dbg.post_tcp_send++;
    }
    else if ( (event == TCP_SOCKET_CLOSED) ||
              (event == TCP_SOCKET_TIMEDOUT) ||
               (event == TCP_SOCKET_ABORTED) )
    {
      msg.status = -1;
      TCP_SOCKET_Dbg.post_tcp_send_err++;
    }
    else
    {
      post_msg = 0;
    }

  }
  else if (s->tcp_recv_pending)
  {
    msg.event = TCP_SOCKET_DATA_RECEIVE;
    if (event == TCP_SOCKET_DATA_RECEIVE)
    {
      TCP_SOCKET_Dbg.post_tcp_recv++;
      msg.status = 0;
    }
    else if ( (event == TCP_SOCKET_CLOSED) ||
              (event == TCP_SOCKET_TIMEDOUT) ||
              (event == TCP_SOCKET_ABORTED) )
    {
      TCP_SOCKET_Dbg.post_tcp_recv_err++;
      msg.status = -1;
    }
    else
    {
      post_msg = 0;
    }

  }
  else if (s->tcp_close_pending)
  {
    msg.event = TCP_SOCKET_CLOSED;
    if (event == TCP_SOCKET_CLOSED)
    {
      msg.status = 0;
      TCP_SOCKET_Dbg.post_tcp_close++;
    }
    else if ( (event == TCP_SOCKET_TIMEDOUT) ||
              (event == TCP_SOCKET_ABORTED) )
    {
      msg.status = -1;
      TCP_SOCKET_Dbg.post_tcp_close_err++;
    }
    else
    {
      post_msg = 0;
    }

  }

  if (post_msg == 0)
  {
    return;
  }

  mbSt = Mailbox_post(s->mboxNotify,&msg,BIOS_NO_WAIT);
  if (mbSt == FALSE)
  {
    TCP_SOCKET_Dbg.mbx_full++;
  }

}

/*---------------------------------------------------------------------------*/
static void
retrans_timeout(struct tcp_socket *s)
{
  UInt key;

  // the current connection has been aborted due to too many
  // retransmission
  // enter critical section
  key = Task_disable();

  // we need to clean up output buffer
  s->output_data_len = 0;
  s->output_data_send_nxt = 0;

  // exit critical section
  Task_restore(key);
}

/*---------------------------------------------------------------------------*/
static void
senddata(struct tcp_socket *s)
{
  int len;
  UInt key;

  // enter critical section
  key = Task_disable();

  if(s->output_data_len > 0) {
    len = MIN(s->output_data_len, uip_mss());
    s->output_data_send_nxt = len;
    uip_send(s->output_data_ptr, len);
  }
  else
  {
    TCP_SOCKET_Dbg.senddata_no_data++;
  }

  // exit critical section
  Task_restore(key);

}
/*---------------------------------------------------------------------------*/
static void
acked(struct tcp_socket *s)
{
  UInt key;

  if(s->output_data_len > 0) {
    /* Copy the data in the outputbuf down and update outputbufptr and
       outputbuf_lastsent */

    // enter critical section
    key = Task_disable();

    if(s->output_data_send_nxt > 0) {
      memcpy(&s->output_data_ptr[0],
	     &s->output_data_ptr[s->output_data_send_nxt],
	     s->output_data_maxlen - s->output_data_send_nxt);

	     TCP_SOCKET_Dbg.tx_byte += s->output_data_send_nxt;

    }
    if(s->output_data_len < s->output_data_send_nxt) {
      printf("tcp: acked assertion failed s->output_data_len (%d) < s->output_data_send_nxt (%d)\n",
	     s->output_data_len,
	     s->output_data_send_nxt);
	     TCP_SOCKET_Dbg.tcp_send_ack_len_error++;
    }
    s->output_data_len -= s->output_data_send_nxt;
    s->output_data_send_nxt = 0;

    // exit critical section
    Task_restore(key);

    call_event(s, TCP_SOCKET_DATA_SENT);
  }
}
/*---------------------------------------------------------------------------*/
static void
newdata(struct tcp_socket *s)
{
  uint16_t len, copylen, bytesleft;
  uint8_t *dataptr;
  len = uip_datalen();
  dataptr = uip_appdata;

#if 0
  /* We have a segment with data coming in. We copy as much data as
     possible into the input buffer and call the input callback
     function. The input callback returns the number of bytes that
     should be retained in the buffer, or zero if all data should be
     consumed. If there is data to be retained, the highest bytes of
     data are copied down into the input buffer. */
  do {
    copylen = MIN(len, s->input_data_maxlen);
    memcpy(s->input_data_ptr, dataptr, copylen);
    if(s->input_callback) {
      bytesleft = s->input_callback(s, s->ptr,
				    s->input_data_ptr, copylen);
    } else {
      bytesleft = 0;
    }
    if(bytesleft > 0) {
      printf("tcp: newdata, bytesleft > 0 (%d) not implemented\n", bytesleft);
    }
    dataptr += copylen;
    len -= copylen;

  } while(len > 0);
#endif

  if (len == 0)
  {
    TCP_SOCKET_Dbg.tcp_rx_data_zero_len++;
    return;
  }

  // enter critical section

  // find the free space in the socket input buffer
  bytesleft = s->input_data_maxlen - s->input_data_len;

  if (bytesleft >= len)
  { // copy data to socket input  buffer
    copylen = len;
    memcpy(&(s->input_data_ptr[s->input_data_len]),dataptr,copylen);

    // update the variable
    s->input_data_len += len;

    TCP_SOCKET_Dbg.rx_byte +=copylen;
  }
  else
  { // input buffer is full
    copylen =0;
    TCP_SOCKET_Dbg.tcp_rx_data_drop++;
  }

  // exit critical section

  if (copylen)
  {
    call_event(s, TCP_SOCKET_DATA_RECEIVE);
  }
}

/*---------------------------------------------------------------------------*/
static void
relisten(struct tcp_socket *s)
{
  if(s != NULL && s->listen_port != 0) {
    s->flags |= TCP_SOCKET_FLAGS_LISTENING;
  }
}
/*---------------------------------------------------------------------------*/
static void
appcall(void *state)
{
  struct tcp_socket *s = state;

  TCP_SOCKET_Dbg.appcall_event++;

  if(uip_connected()) {
    /* Check if this connection originated in a local listen
       socket. We do this by checking the state pointer - if NULL,
       this is an incoming listen connection. If so, we need to
       connect the socket to the uip_conn and call the event
       function. */
    if(s == NULL) {
      for(s = list_head(socketlist); s != NULL; s = list_item_next(s))
      {
	      if((s->flags & TCP_SOCKET_FLAGS_LISTENING) != 0 &&
	          s->listen_port != 0 &&
	          s->listen_port == uip_htons(uip_conn->lport)) {
    	  s->flags &= ~TCP_SOCKET_FLAGS_LISTENING;
    	  tcp_markconn(uip_conn, s,&tcp_socket_process_obj);
    	  TCP_SOCKET_Dbg.appcall_mark_connected++;
    	  s->tcp_internal_state_flag = TCP_STATE_CONNECTED;
    	  call_event(s, TCP_SOCKET_CONNECTED);
    	  break;
    	  }
      }
    } else {
      TCP_SOCKET_Dbg.appcall_connected++;
      s->tcp_internal_state_flag = TCP_STATE_CONNECTED;
      call_event(s, TCP_SOCKET_CONNECTED);
    }

    if(s == NULL) {
      uip_abort();
      TCP_SOCKET_Dbg.appcall_aborted_in_connected++;
    } else {
      if(uip_newdata()) {
        TCP_SOCKET_Dbg.appcall_rx_data_in_coonected++;
        newdata(s);
      }
      TCP_SOCKET_Dbg.appcall_send_in_connected++;
      senddata(s);
    }
    return;
  }

  // check the socket pointer first
  if(s == NULL) {
    uip_abort();
    TCP_SOCKET_Dbg.appcall_aborted_null++;
    return;
  }

  if(uip_timedout()) {
    TCP_SOCKET_Dbg.appcall_timeout++;
    s->tcp_internal_state_flag = TCP_STATE_TIMEOUT;
    retrans_timeout(s);
    call_event(s, TCP_SOCKET_TIMEDOUT);
    relisten(s);
  }

  if(uip_aborted()) {
    TCP_SOCKET_Dbg.appcall_aborted++;
    s->tcp_internal_state_flag = TCP_STATE_ABORTED;
    call_event(s, TCP_SOCKET_ABORTED);
    relisten(s);
  }

  if(uip_acked()) {
    TCP_SOCKET_Dbg.appcall_acked++;
    acked(s);
  }
  if(uip_newdata()) {
    TCP_SOCKET_Dbg.appcall_rx_data++;
    newdata(s);
  }

  if(uip_rexmit() ||
     uip_newdata() ||
     uip_acked()) {
    TCP_SOCKET_Dbg.appcall_send_in_rexmit_new_acked++;
    senddata(s);
  } else if(uip_poll()) {
    TCP_SOCKET_Dbg.appcall_send_in_uip_poll++;
    senddata(s);
  }

  if(s->output_data_len == 0 && s->flags & TCP_SOCKET_FLAGS_CLOSING) {
    s->flags &= ~TCP_SOCKET_FLAGS_CLOSING;
    uip_close();
    tcp_markconn(uip_conn, NULL,&tcp_socket_process_obj);
    TCP_SOCKET_Dbg.appcall_closing++;
    s->tcp_internal_state_flag = TCP_STATE_CLOSED;
    call_event(s, TCP_SOCKET_CLOSED);
    relisten(s);
  }

  if(uip_closed()) {
    tcp_markconn(uip_conn, NULL,&tcp_socket_process_obj);
    TCP_SOCKET_Dbg.appcall_closed++;
    s->tcp_internal_state_flag = TCP_STATE_CLOSED;
    call_event(s, TCP_SOCKET_CLOSED);
    relisten(s);
  }
}
#if 0
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tcp_socket_process, ev, data)
{
  PROCESS_BEGIN();
  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == tcpip_event) {
      appcall(data);
    }
  }
  PROCESS_END();
}
#else

void tcp_socket_process_post_func(process_event_t event, process_data_t data){
  	TCPIP_cmdmsg_t  msg;

        msg.event_type = event;
	msg.data = data;

        if (Mailbox_post(tcpsocket_mailbox, &msg, BIOS_NO_WAIT) != TRUE){
            TCPIP_Dbg.tcpip_post_app_err++;
        }
}

void tcp_socket_process(UArg arg0, UArg arg1)
{
  TCPIP_cmdmsg_t tcp_socket_msg;
  uint16_t msg_size1, msg_size2;

  /*
      init the process
  */
  //ptcp_socket_mb = tcpsocket_mailbox;
  tcp_socket_init();

  // check and varify the mailbox size
  msg_size1 = sizeof(tcp_socket_msg);
  msg_size2 = Mailbox_getMsgSize(tcpsocket_mailbox);

  if (msg_size1 != msg_size2)
  {
    for(;;);
  }

  while (1)
  {
    // wait for new message
    Mailbox_pend(tcpsocket_mailbox, &tcp_socket_msg, BIOS_WAIT_FOREVER);

    if (tcp_socket_msg.event_type == tcpip_event)
    {
      appcall(tcp_socket_msg.data);
    }
  }

}
#endif
/*---------------------------------------------------------------------------*/
void
tcp_socket_init(void)
{
  static uint8_t inited = 0;
  if(!inited) {
    //ADDED Contiki 2.7
    process_obj_init(&tcp_socket_process_obj, tcp_socket_process_post_func);

    list_init(socketlist);
    // init tcp socket pool
    memset(tcp_socket_pool,0,sizeof(tcp_socket_pool));
    //process_start(&tcp_socket_process, NULL);
    inited = 1;
  }

}
/*---------------------------------------------------------------------------*/
struct tcp_socket * tcp_socket_create(void)
{
  uint8_t i,found;
  struct tcp_socket * pSocket;

  found =0;
  // get the free the TCP_SOCKET data buffer from the pool
  for (i=0;i<MAX_NUM_TCP_SOCKET;i++)
  {
    pSocket = &tcp_socket_pool[i];

    if (pSocket->valid == 0)
    {
      found =1;
      break;
    }
  }

  if (found ==0)
    return NULL;

  // here we found the first free TCP SOCKET data buffer
  memset(pSocket,0x0,sizeof(struct tcp_socket));

  //  try to init the data structure
  pSocket->valid =1;	// mark it is valid

  pSocket->input_data_ptr = tcp_socket_buf_pool[i].in_buffer;
  pSocket->input_data_maxlen = TCP_SOCKET_INPUT_BUFF_SIZE;
  pSocket->output_data_ptr = tcp_socket_buf_pool[i].out_buffer;
  pSocket->output_data_maxlen = TCP_SOCKET_OUTPUT_BUFF_SIZE;
  // add to linked list
  list_add(socketlist, pSocket);
  pSocket->listen_port = 0;
  pSocket->flags = TCP_SOCKET_FLAGS_NONE;

#if 0
  // create mailbox for notify
  pSocket->mboxNotify = Mailbox_create(MAX_NUM_TCP_NOTIFY_MSG,
                              sizeof(SOCKET_NOTIFY_MSG_s),
                              NULL, // from heap
                              NULL);
  if (pSocket == NULL)
  { // we don't have space to allocate the mailbox

    pSocket->valid =0;	// mark it is invalid
    // remove from linked list
    list_remove(socketlist, pSocket);
    return NULL;
  }
#endif

  // to save code and ram, we only support one socket and use
  // one dedicated statical created mailbox
   pSocket->mboxNotify = socketNotifyMb;
  return pSocket;
}
/*---------------------------------------------------------------------------*/
void  tcp_socket_remove(struct tcp_socket *pSocket)
{
#if 0
  // free the mailbox
  Mailbox_delete(&(pSocket->mboxNotify));
#endif
  // markk this socekt is invalid
  pSocket->valid =0;	// mark it is invalid
  // remove from linked list
  list_remove(socketlist, pSocket);
}


/*---------------------------------------------------------------------------*/
void
tcp_socket_register(struct tcp_socket *s, void *ptr,
		    uint8_t *input_databuf, int input_databuf_len,
		    uint8_t *output_databuf, int output_databuf_len,
		    tcp_socket_input_callback_t input_callback,
		    tcp_socket_event_callback_t event_callback)
{

  tcp_socket_init();

  s->ptr = ptr;
  s->input_data_ptr = input_databuf;
  s->input_data_maxlen = input_databuf_len;
  s->output_data_ptr = output_databuf;
  s->output_data_maxlen = output_databuf_len;
  s->input_callback = input_callback;
  s->event_callback = event_callback;
  list_add(socketlist, s);

  s->listen_port = 0;
  s->flags = TCP_SOCKET_FLAGS_NONE;
}

/*---------------------------------------------------------------------------*/
int
tcp_socket_connect(struct tcp_socket *s,
            uip_ipaddr_t *ipaddr,
            uint16_t port)
{
  struct uip_conn *c;
#if 0
  PROCESS_CONTEXT_BEGIN(&tcp_socket_process);
  c = tcp_connect(ipaddr, uip_htons(port), s);
  PROCESS_CONTEXT_END();
#else
  c = tcp_connect(ipaddr, uip_htons(port), s,&tcp_socket_process_obj);
#endif
  if(c == NULL) {
    return 0;
  } else {
    return 1;
  }
}

/*---------------------------------------------------------------------------*/
int
tcp_socket_listen(struct tcp_socket *s,
           uint16_t port)
{
  s->listen_port = port;
#if 0
  PROCESS_CONTEXT_BEGIN(&tcp_socket_process);
  tcp_listen(uip_htons(port));
  PROCESS_CONTEXT_END();
#else
  tcp_listen(uip_htons(port),&tcp_socket_process_obj);
#endif
  s->flags |= TCP_SOCKET_FLAGS_LISTENING;
  return 0;
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_unlisten(struct tcp_socket *s)
{
#if 0
  PROCESS_CONTEXT_BEGIN(&tcp_socket_process);
  tcp_unlisten(uip_htons(s->listen_port));
  PROCESS_CONTEXT_END();
#else
  tcp_unlisten(uip_htons(s->listen_port),&tcp_socket_process_obj);
#endif
  s->listen_port = 0;
  s->flags &= ~TCP_SOCKET_FLAGS_LISTENING;
  return 0;
}

/*---------------------------------------------------------------------------*/
int
tcp_socket_send(struct tcp_socket *s,
         const uint8_t *data, int datalen)
{
  int len;

  len = MIN(datalen, s->output_data_maxlen - s->output_data_len);

  memcpy(&s->output_data_ptr[s->output_data_len], data, len);
  s->output_data_len += len;
  return len;
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_send_str(struct tcp_socket *s,
             const char *str)
{
  return tcp_socket_send(s, (const uint8_t *)str, strlen(str));
}
/*---------------------------------------------------------------------------*/
int
tcp_socket_close(struct tcp_socket *s)
{
  // discard the remaining output data
  s->output_data_len = 0 ;

  s->flags |= TCP_SOCKET_FLAGS_CLOSING;
  return 1;
}
/*---------------------------------------------------------------------------*/

