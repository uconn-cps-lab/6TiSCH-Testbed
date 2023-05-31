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

#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include "net/uip.h"
#include <ti/sysbios/knl/Mailbox.h> 

struct tcp_socket;

#define MAX_NUM_TCP_SOCKET          (1)

typedef enum {
  TCP_SOCKET_CONNECTED,
  TCP_SOCKET_CLOSED,
  TCP_SOCKET_TIMEDOUT,
  TCP_SOCKET_ABORTED,
  TCP_SOCKET_DATA_SENT,
  TCP_SOCKET_DATA_RECEIVE
} tcp_socket_event_t;

typedef enum {
  TCP_STATE_CONNECTED =0,
  TCP_STATE_ABORTED,
  TCP_STATE_CLOSED,
  TCP_STATE_TIMEOUT,

} tcp_internal_error_t;

/**
 * \brief      TCP input callback function
 * \param s    A pointer to a TCP socket
 * \param ptr  A user-defined pointer
 * \param input_data_ptr A pointer to the incoming data
 * \param input_data_len The length of the incoming data
 * \return     The function should return the number of bytes to leave in the input buffer
 *
 *             The TCP socket input callback function gets
 *             called whenever there is new data on the socket. The
 *             function can choose to either consume the data
 *             directly, or leave it in the buffer for later. The
 *             function must return the amount of data to leave in the
 *             buffer. I.e., if the callback function consumes all
 *             incoming data, it should return 0.
 */

typedef int (* tcp_socket_input_callback_t)(struct tcp_socket *s,
					    void *ptr,
					    const uint8_t *input_data_ptr,
					    int input_data_len);


/**
 * \brief      TCP event callback function
 * \param s    A pointer to a TCP socket
 * \param ptr  A user-defined pointer
 * \param event The event number
 *
 *             The TCP socket event callback function gets
 *             called whenever there is an event on a socket, such as
 *             the socket getting connected or closed.
 */
typedef void (* tcp_socket_event_callback_t)(struct tcp_socket *s,
                                             void *ptr,
                                             tcp_socket_event_t event);

#define TCP_SOCKET_INPUT_BUFF_SIZE        128
#define TCP_SOCKET_OUTPUT_BUFF_SIZE       256

/*
  define TCP socket data buffer
  

*/
typedef struct __tcp_socket_buf__
{
  /* Incoming data related */
  uint8_t in_buffer[TCP_SOCKET_INPUT_BUFF_SIZE];
  /* outgoing data buffer */
  uint8_t out_buffer[TCP_SOCKET_OUTPUT_BUFF_SIZE];   
   
} TCP_SOCKET_BUF_s;

#define MAX_NUM_TCP_NOTIFY_MSG      (8)

typedef struct __socket_notify_msg_
{
  uint16_t  event;
  int16_t   status;
} SOCKET_NOTIFY_MSG_s;

typedef struct __tcp_socket_debug__
{
  uint32_t  rx_byte;
  uint32_t  tx_byte;
  // APPCALL releated events
  uint32_t  appcall_event;
  uint16_t  appcall_connected;
  uint16_t  appcall_mark_connected;
  uint16_t  appcall_rx_data_in_coonected;
  uint16_t  appcall_send_in_connected; 
  uint16_t  appcall_aborted_in_connected;
  uint16_t  appcall_timeout;
  uint16_t  appcall_aborted;
  uint16_t  appcall_aborted_null;
  uint16_t  appcall_acked;
  uint16_t  appcall_rx_data;
  uint16_t  appcall_send_in_rexmit_new_acked;
  uint16_t  appcall_send_in_uip_poll;
  uint16_t  appcall_closing;
  uint16_t  appcall_closed;
  
  // API
  uint16_t  tcp_api_open;
  uint16_t  tcp_api_open_create_socket_error;
  uint16_t  tcp_open_api_rx_connected;
  uint16_t  tcp_open_api_unexpected_ev;
  
  uint16_t  tcp_send_api;
  uint16_t  tcp_send_api_timeout;
  uint16_t  tcp_send_ack_len_error;
  
  uint16_t  tcp_api_recv;
  uint16_t  tcp_api_close;
  
  uint16_t  senddata_no_data;    
  
  uint16_t  tcp_rx_data_drop;
  uint16_t  tcp_rx_data_zero_len;

  uint16_t  tcp_close_timeout;
  uint16_t  tcp_close_ok;
  
  uint16_t  post_tcp_open;
  uint16_t  post_tcp_open_err;
  uint16_t  post_tcp_send;
  uint16_t  post_tcp_send_err;
  uint16_t  post_tcp_recv;
  uint16_t  post_tcp_recv_err;  
  uint16_t  post_tcp_close;
  uint16_t  post_tcp_close_err;
  
  uint16_t  mbx_full;

  
  uint16_t  tcp_recv_api_ok;
  uint16_t  tcp_recv_api_err;
  uint16_t  tcp_recv_api_timeout;
  
  
} TCP_SOCKET_DBG_s;

struct tcp_socket {
  struct tcp_socket *next;

  tcp_socket_input_callback_t input_callback;
  tcp_socket_event_callback_t event_callback;
  void *ptr;  // for event callback to carry back connection handler

  //struct process *p;

  uint8_t *input_data_ptr;
  uint8_t *output_data_ptr;

  uint16_t input_data_maxlen;
  uint16_t input_data_len;
  uint16_t output_data_maxlen;
  uint16_t output_data_len;
  uint16_t output_data_send_nxt;

  uint16_t listen_port;
  uint8_t flags;
  
  // extra control flags
  uint8_t valid;          // 0: it is not used, 1: it is valid
  uint8_t tcp_open_pending;
  uint8_t tcp_send_pending;
  uint8_t tcp_recv_pending;
  uint8_t tcp_close_pending;

  // TCP connection internal state flag
  uint8_t tcp_internal_state_flag;
  
  // notification mailbox
  //  here we assume the tcp send/recv APIs are called from the single task
  //  if they are called from from two different tasks, we need to use two different mailboxes
  //  
  Mailbox_Handle mboxNotify;
  
};

enum {
  TCP_SOCKET_FLAGS_NONE      = 0x00,
  TCP_SOCKET_FLAGS_LISTENING = 0x01,
  TCP_SOCKET_FLAGS_CLOSING   = 0x02,
};

struct tcp_socket * tcp_socket_create(void);
void tcp_socket_init(void);
void  tcp_socket_remove(struct tcp_socket *pSocket);

void tcp_socket_register(struct tcp_socket *s, void *ptr,
                         uint8_t *input_databuf, int input_databuf_len,
                         uint8_t *output_databuf, int output_databuf_len,
                         tcp_socket_input_callback_t input_callback,
                         tcp_socket_event_callback_t event_callback);

int tcp_socket_connect(struct tcp_socket *s,
                       uip_ipaddr_t *ipaddr,
                       uint16_t port);

int tcp_socket_listen(struct tcp_socket *s,
                      uint16_t port);

int tcp_socket_unlisten(struct tcp_socket *s);

int tcp_socket_send(struct tcp_socket *s,
                    const uint8_t *dataptr,
                    int datalen);

int tcp_socket_send_str(struct tcp_socket *s,
                        const char *strptr);

int tcp_socket_close(struct tcp_socket *s);

#endif /* TCP_SOCKET_H */
