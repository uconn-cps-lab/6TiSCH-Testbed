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


#ifndef __MQTT_CLIENT_H__
#define __MQTT_CLIENT_H__

/**@file mqtt_client.h
   This file describes the interface / API(s) of the MQTT Client Library.

   This is a light-weight library to provision the services of the MQTT protocol
   for a client app (that would typically run on a constrained system).

   The key consideration in the design of small footprint library has been to
   abstract the low level details of message transactions with the server and
   yet, provide rudimentary API(s), such that, the capabilities & features of
   the protocol are availalbe to be utilized by existing and new applications
   in an un-restrictive manner.
   
   The MQTT Client library is a highly portable software and implies a very
   limited set of dependencies on a platform. Importantly, such dependencies,
   even though, limited in scope, can be easily adapted to target platform
   through plug-ins.

   The services of the library are multi-task safe. Platform specific atomicity
   constructs are used, through abstractions, by the library to maintain data 
   coherency and synchronization. In addition, the library can configured to
   support several in-flight messages.

   The library is targeted to conform to MQTT 3.1.1 specification. 

   Non-QoS0 packets are held in the library for re-sends to the server, if ACK
   for those are not received with in the specified reasonable time.

   Any future extensions & development must follow the following guidelines.
   
   A new API or an extension to the existing API
   a) must be rudimentary
   b) must not imply a rule or policy (including a state machine)
   b) must ensure simple design and implementation
   
*/

#include "mqtt_common.h"


/*------------------------------------------------------------------------------
 *  MQTT Client Messaging Routines / Services
 *------------------------------------------------------------------------------
 */

/* 
   Provides a new MSG Identifier for a packet dispatch to server.
   
   @returns MSG / PKT Transaction identifier
*/
u16 mqtt_client_new_msg_id(void);

/** Ascertain whether connection / session with the server is active
    Prior to sending out any information any message to server, the client app
    can use this routine to check the status of the connection. If connection
    does not exist, then client should first CONNECT to the broker. 
    
    A connection to server can be closed either due to keep alive time-out or 
    due to error in RX and TX transactions.
    
    @note this API does not refer to network connection or session
    
    @return true if connection is active otherwise false.
*/
bool mqtt_client_is_connected(void);

/** Send the CONNECT message to the server (and don't wait for CONNACK)
    This routine accomplishes multiple sequences. As a first step, it tries
    to establish a network connection with the server. Then, it populates
    an internaly allocated packet buffer with all the previously provided
    payload data information, prepares the requisite headers and finally,
    dispatches the constructed message to the server.
    
    The client app must invoke an appropriate library routine to receive
    CONNACK from the broker.
    
    The client app must register the payload contents prior to invoking
    this service of connecting to the server. The address of the server
    forms the part of the library initialization.
    
    @param[in] clean_session asserted to delete references to previous session
    at both server and client
    @param[in] ka_secs Keep Alive Time
    @param[in] wait_secs Maximum time to wait for a CONNACK.
    @return number of bytes sent or LIB defined errors @ref lib_err_group
*/
i32 mqtt_connect_msg_send(bool clean_session, u16 ka_secs);

/** Send the CONNECT message to the server (and wait for CONNACK).
    Blocking version of the mqtt_connect_msg_send( ). The routine blocks till the
    time there is either a CONNACK packet from the server or the time to wait for
    the CONNACK message has expired. 
    
    @param[in] clean_session asserted to delete references to previous session
    at both server and client
    @param[in] ka_secs Keep Alive Time
    @param[in] wait_secs Maximum time to wait for a CONNACK.
    @return 2 byte codes received in CONNACK (0, +ve) or LIB defined errors
    @ref lib_err_group
*/
i32 mqtt_connect(bool clean_session, u16 ka_secs, u32 wait_secs);

/** Send a PUBLISH message to the server.
    This routine creates a PUBLISH message in an internally allocated packet 
    buffer by embedding the 'topic' and 'data' contents, then prepares the 
    packet header and finally, dispatches the message to the server. 
    
    After the packet has been sent to the server, if the associated QoS of the
    dispatched packet is ether level 1 or 2, then the library will retain the
    packet until the time, a corresponding PUB-ACK (for QoS1) or PUB-REC (QoS2)
    message is received from the server.
    
    If the client app asserts the 'clean_session' parameter in the next iteration
    of the CONNECT operation, then the references to all stored PUBLISH messages
    are dropped.

    @param[in] topic UTF8 based Topic Name for which data is being published.
    @param[in] data_buf The binary data that is being published for the topic.
    @param[in] data_len The length of the binary data.
    @param[in] qos Quality of service of the message
    @param[in] retain Should the client retain the message.
    @return A valid message or packet id on success or LIB defined errors
    @ref lib_err_group
*/
i32 mqtt_client_pub_msg_send(const  struct utf8_string *topic,
                             const u8 *data_buf, u32 data_len,
                             enum mqtt_qos qos,  bool retain);

/** Dispatch app constructed PUBLISH message to the server. 
    Prior to sending the message to the server, this routine will prepare a fixed
    header to account for the size of the contents and the flags that have been
    indicated by the caller. 
    
    If the associated QoS of the dispatched packet is either level 1 or 2, then
    the library will retain the packet until the time, a corresponding PUB-ACK
    (for QoS1) or PUB-REC (Qos2) message is received from the server. 
    
    If the client app asserts the 'clean_session' parameter in the next iteration
    of the CONNECT operation, then the references to all stored PUBLISH messages
    are dropped.
    
    The caller must populate the payload information with topic and data before
    invoking this service. 
    
    This service facilitates direct writing of topic and (real-time) payload data
    into the buffer, thereby, avoiding power consuming and wasteful intermediate
    data copies.
    
    In case, the routine returns an error, the caller is responsbile for freeing
    up or re-using the packet buffer. For all other cases, the client library
    will manage the return of the packet buffer to the pool.
    
    @param[in] mqp app created PUBLISH message without the fixed header
    @param[in] qos QoS with which the message needs to send to server
    @param[in] retain Asserted if the message is to be retained by server.
    @return on success, the transaction Message ID, otherwise LIB defined errors
    @ref lib_err_group
*/
i32 mqtt_client_pub_dispatch(struct mqtt_packet *mqp, 
                             enum mqtt_qos qos, bool retain);

/** Send a SUBSCRIBE message to the server.
    This routine creates a SUBSCRIBE message in an internally allocated packet
    buffer by embedding the 'qos_topics', then prepares the message header and
    finally, dispatches the packet to the server. 
    
    After the packet has been dispatched to the server, then the library, when set
    to be operate in the MQTT 3.1 mode, will retain the packet until the time, a 
    corresponding SUB-ACK is received from the server. 
    
    The library, when set to operate in the MQTT 3.1.1 mode, does not store the
    SUBCRIBE packet to await the SUB-ACK.
    
    @param[in] qos_topics an array of topic along-with its qos
    @param[in] count the number of elements in the array
    @return on success, the transaction Message ID, otherwise Lib defined errors
    @ref lib_err_group
*/
i32 mqtt_sub_msg_send(const struct utf8_strqos *qos_topics, u32 count);

/** Dispatch app constructed SUSBSCRIBE message to the server.
    Prior to sending the message to the server, this routine will prepare a fixed
    header to account for the size of the size of the contents. 
    
    After the packet has been dispatched to the server, then the library, when set
    to be operate in the MQTT 3.1 mode, will retain the packet until the time, a 
    corresponding SUB-ACK is received from the server. 
    
    The library, when set to operate in the MQTT 3.1.1 mode, does not store the
    SUBCRIBE packet to await the SUB-ACK.
    
    If the client app asserts the 'clean_session' parameter in the next iteration
    of the CONNECT operation, then the references to all stored SUBSCRIBE packets
    are dropped. This is applicable only for the MQTT 3.1 mode of operation.
    
    The caller must populate the payload information of topic along with qos
    before invoking this service. 
    
    This service facilitates direct writing of topic and (real-time) payload data
    into the buffer, thereby, avoiding power consuming and wasteful intermediate
    data copies.
    
    In case, the routine returns an error, the caller is responsbile for freeing
    up or re-using the packet buffer. For all other cases, the client library
    will manage the return of the packet buffer to the pool.
    
    @param[in] mqp app created SUBSCRIBE message without the fixed header.
    @return, on success, the transaction Message ID, otherwise Lib defined errors
    @ref lib_err_group
*/
i32 mqtt_sub_dispatch(struct mqtt_packet *mqp);

/** Send an UNSUBSCRIBE message to the server.
    This routine creates an UNSUBSCRIBE message in an internally allocated packet
    buffer by embedding the 'topics', then prepares the message header and 
    finally, dispatches the packet to the server. 
    
    After the packet has been sent to the server, then the library, when set to
    operate in the MQTT 3.1 mode, will retain the packet until the time, a 
    corresponding UNSUB-ACK is received from the server. 
    
    The library, when set to operate in the MQTT 3.1.1 mode, does not store the
    UNSUBCRIBE packet to await the UNSUB-ACK.
    
    @param[in] topics an array of topic to unsubscribe
    @param[in] count the number of elements in the array
    @return on success, the transaction Message ID, otherwise Lib defined errors
    @ref lib_err_group
*/
i32 mqtt_unsub_msg_send(const struct utf8_string *topics, u32 count);

/** Dispatch app constructed UNSUSBSCRIBE message to the server. 
    Prior to sending the message to the server, this routine will prepare a fixed
    header to account for the size of the size of the contents. 
    
    After the packet has been dispatched to the server, then the library, when 
        set to be operate in the MQTT 3.1 mode, will retain the packet until the
    time, a corresponding UNSUB-ACK is received from the server. 
    
    The library, when set to operate in the MQTT 3.1.1 mode, does not store the
    UNSUBCRIBE packet to await the SUB-ACK.
    
    If the client app asserts the 'clean_session' parameter in the next iteration
    of the CONNECT operation, then the references to all stored SUBSCRIBE packets
    are dropped. This is applicable only for the MQTT 3.1 mode of operation.
    
    The caller must populate the payload information of topics before invoking
    this service. 
    
    This service facilitates direct writing of topic and (real-time) payload data
    into the buffer, thereby, avoiding power consuming and wasteful intermediate
    data copies.
    
    In case, the routine returns an error, the caller is responsbile for freeing
    up or re-using the packet buffer. For all other cases, the client library
    will manage the return of the packet buffer to the pool.
    
    @param[in] Packet Buffer that holds UNSUBSCRIBE message without a fixed header
    @return on success, the transaction Message ID, otherwise Lib defined errors
    @ref lib_err_group
*/
i32 mqtt_unsub_dispatch(struct mqtt_packet *mqp);

/** Send a PINGREQ message to the server
    @return number of bytes sent or Lib define errors @ref lib_err_group
*/

i32 mqtt_pingreq_send(void);

/* Send a DISCONNECT message to the server
   @return number of bytes sent or Lib define errors @ref lib_err_group
*/
i32 mqtt_disconn_send(void);

/** Await or block to receive the specified message within the certain time.
    This service is valid only for the set-up, where the app has not  provided
    the callbacks to the MQTT client library. The caller must provide a packet
    buffer of adequate size to hold the expected message from the server.
    
    The wait time implies the maximum intermediate duration between the reception
    of two successive messages from the server. If no message is received before
    the expiry of the wait time, the routine returns. However, the routine would
    continue to block, in case, messages are being received within the successive
    period of wait time, however these messages are not the one that client is
    awaiting.
    
    @param[in] msg_type Message type to receive. A value of 0 would imply that
    caller is ready to receive the next message, whatsoever, from the  server.
    @param[in, out] mqp Packet Buffer to hold the message received from the
    server.
    @param[in] wait_secs: Maximum Time to wait for a message from the server.
    @return on success, number of bytes received for 'msg_type' from server,
    otherwise LIB defined error values @ref lib_err_group
*/
i32 mqtt_client_await_msg(u8 msg_type, struct mqtt_packet *mqp, u32 wait_secs);

/** Helper function to receive any message from the server.
    Refer to mqtt_client_await_msg for the details.
    @see mqtt_client_await_msg
*/
static inline i32 mqtt_client_recv(struct mqtt_packet *mqp, u32 wait_secs)
{
        /* Receive next and any MQTT Message from the broker */
        return mqtt_client_await_msg(0x00, mqp, wait_secs);
}

/* 
   Run MQTT Lib until there is a 'connected' network link with the server.

   This service is valid only for the set-up, where the app has populated the
   callbacks that can be invoked by the MQTT client library

   This routine yields the control back to the app after a certain duration of
   wait time. Such an arrangement enable the app make overall progress to meet
   it intended functionality. 

   The wait time implies the maximum intermediate duration between the reception
   of two successive messages from the server. If no message is received before
   the expiry of the wait time, the routine returns. However, the routine would
   continue to block, in case, messages are being received within the successive
   period of wait time.

   @param[in] wait_secs maximum time to wait for a message from the server
   @return on connection close by client app, number of bytes received for the
   last msg from broker, otherwise LIB defined error values.
*/
i32 mqtt_client_run(u32 wait_secs);

/*------------------------------------------------------------------------------
 *  MQTT Client Library: Packet Buffer Pool and its management
 *------------------------------------------------------------------------------
 */

/** Allocates a free MQTT Packet Buffer.  
    The pool from which a free MQP buffer is allocated by the library has to 
    be configured (i.e. registered) a-priori by the app.
    
    The parameter 'offset' is used to specify the number of bytes that are
    reserved for the header in the buffer
    
    @param[in] msg_type Message Type for which MQP buffer is being assigned.
    @param[in] offset Number of bytes to be reserved for MQTT headers.
    @return a NULL on error, otherwise a reference to a valid packet holder.
    
    @see mqtt_client_register_buffers
*/
struct mqtt_packet *mqp_client_alloc(u8 msg_type, u8 offset);

/** Helper function to allocate a MQTT Packet Buffer for a message to be
    dispatched to server.
    
    @see to mqp_client_alloc( ) for details.
*/
static inline struct mqtt_packet *mqp_client_send_alloc(u8 msg_type)
{
        return mqp_client_alloc(msg_type, MAX_FH_LEN);
}

/** Helper function to allocate a MQTT Packet Buffer for a message to be
    received from the server.
    
    @see to mqp_client_alloc( ) for details.
*/
static inline struct mqtt_packet *mqp_client_recv_alloc(u8 msg_type)
{
        return mqp_client_alloc(msg_type, 0);
}

/** Create a pool of MQTT Packet Buffers for the client library. 
    This routine creates a pool of free MQTT Packet Buffers by attaching a buffer
    (buf) to a packet holder (mqp). The count of mqp elements and buf elements in
    the routine are same. And the size of the buffer in constant across all the
    elements.
    
    The MQTT Packet Buffer pool should support (a) certain number of in-flight and
    stored packets that await ACK(s) from the server (b) certain number of packets
    from server whose processing would be deferred by the client app (to another
    context) (c) a packet to create a CONNECT message to re-establish transaction
    with the server.
    
    A meaningful size of the pool is very much application specific and depends
    on the target functionality of the application. For example, an app that
    intends to have only one in-flight message to the server and does not intend
    to defer the processing of the received packet from server would need atmost
    three MQP buffers (1 for TX, 1 for RX and 1 for CONNECT message). If this app
    does not send any non-QoS0 messages to server, then the number of MQP buffers
    would reduce to two (i.e. 1 Tx to support CONNECT / PUB out and 1 RX)

    @param[in] num_mqp Number or count of elements in mqp_vec and buf_vec.
    @param[in] mqp_vec An array of MQTT Packet Holder without a buffer.
    @param[in] buf_len The size or length of the buffer element in the 'buf_vec'
    @param[in] buf_vec An array of buffers.
    @retun  0 on success otherwise -1 on error.
    
    @note The parameters mqp_vec and buf_vec should be peristent entities.
    
    @see mqtt_client_await_msg
    @see mqtt_client_run
*/
i32 mqtt_client_register_buffers(u32 num_mqp, struct mqtt_packet *mqp_vec,
                                 u32 buf_len, u8 *buf_vec);

/*------------------------------------------------------------------------------
 *  MQTT Client Library: Register app, platform information and services.
 *------------------------------------------------------------------------------
 */

/** Register app information and its credentials with the client library.
    This routine registers information for all the specificed parameters,
    therefore, an upate to single element would imply re-specification of
    the other paramters, as well.
    
    @note: contents embedded in the parameters is not copied by the routine, 
    and instead a reference to the listed constructs is retained. Therefore,
    the app must enable the parameter contents for persistency.
    
    @param[in] client_id MQTT UTF8 identifier of the client. If set to NULL,
    then the client will be treated as zero length entity.
    @param[in] user_name MQTT UTF8 user name for the client. If not used,
    set it to NULL. If used, then it can't be of zero length.
    @param[in] pass_word MQTT UTF8 pass word for the client. If not used, set
    it to NULL, If used, then it can't be of zero length.
    @return 0 on success otherwise -1
    
    User name without a pass word is a valid configuration. A pass word won't
    be processed if it not associated with a valid user name.
*/
i32 mqtt_client_register_info(const struct utf8_string *client_id,
                              const struct utf8_string *user_name,
                              const struct utf8_string *pass_word);

/** Register WILL information of the client app.
    This routine registers information for all the specificed parameters,
    therefore, an update to single element would imply re-specification
    of the other paramters, as well.
    
    @note: contents embedded in the parameters is not copied by the routine,
    and instead a reference to the listed constructs is retained. Therefore,
    the app must enable the parameter contents for persistency.
    
    @param[in] will_top UTF8 WILL Topic on which WILL message is to be published.
    @param[in] will_msg UTF8 WILL message.
    @param[in] will_qos QOS for the WILL message
    @param[in] retain asserted to indicate that published WILL must be retained
    @return 0 on success otherwise -1.

    Either both will_top and will_msg should be present or both should be NULL.
    will_qos and retain are relevant only for a valid Topic and Message combo.
*/
i32 mqtt_client_register_will(const struct utf8_string  *will_top,
                              const struct utf8_string  *will_msg,
                              enum mqtt_qos will_qos, bool retain);

/** Abstraction for the device specific network services
    Network services for communication with the server

    @param[in] net references to network services supported by the platform 
    @ref net_ops_group
    @return on success, 0, otherwise -1
    
    @note all entries in net must be supported by the platform.
*/ 
i32 mqtt_client_register_net_svc(const struct device_net_services *net);

struct mqtt_client_lib_cfg {

        bool  mqtt_mode31;  /* Operate LIB in MQTT 3.1 mode; default is  3.1.1 */

        /* App specific config to guide implementation about NW connection */
        u32   nwconn_info;  /* Type of connection: TLS, Address Type, .... */
        i8   *server_addr;  /* Reference to '\0' terminated address string */
        u16   port_number;  /* Network Listening Port number of the server */

        i32  (*dbg_print)(const i8 *format, ...);      /* Debug, mandatory */

        void (*atomic_enbl)(void);/* Optional, needed for multitasking app */
        void (*atomic_dsbl)(void);
};

/*
 * Helper functions & macros to derive 16 bit CONNACK Return Code from broker.
 */
#define VHB_CONNACK_RC(vh_buf) (vh_buf[1])
#define MQP_CONNACK_RC(mqp)    (mqp->buffer[3])

#define VHB_CONNACK_SP(vh_buf) (vh_buf[0])
#define MQP_CONNACK_SP(vh_buf) (mqp->buffer[2])

/* Callbacks to be invoked by MQTT Client library onto Client application */
struct mqtt_client_msg_cbs {

        /** Provides a PUBLISH message from server to client application.
            The app can utilize the associated set of helper macros to get
            references to the topic and the data information contained in the
            message. @ref rxpub_help_group

            Depending upon the QoS level of the message, the MQTT client library
            shall dispatch the correponding acknowlegement (PUBACK or PUBREC) to
            the broker, thereby, relieving app from this support.
            
            If the app completes the processing of the packet within the context
            and implementation of this callback routine, then a value of 'true'
            must be returned to the library. The library, in this case, does not
            handover the packet to app and instead, frees it up on return from
            this routine.
            
            If the app intends to defer the processing of the PUBLISH message to
            a different execution context, then it must takeover the owernship
            of the packet by returning a value of 'false' to the library. In this
            arrangement, the MQTT client library hands over the packet to the app.
            Now, the responsibility of storing, managing and eventually freeing
            up the packet back to the pool lies with the app. 
            
            @param[in] dup Asserted to indicate a DUPLICATE PUBLISH
            @param[in] qos Quality of Service of the PUBLISH message
            @param[in] retain Asserted to indicate message at new subscription
            @param[in] mqp Packet Buffer that holds the PUBLISH message
            
            @return true to indicate that processing of the packet has been
            completed and it can freed-up and returned back to the pool by
            the library. Otherwise, false.
        */
        bool (*publish_rx)(bool dup, enum mqtt_qos qos, bool retain, 
                           struct mqtt_packet *mqp);

        /** Notifies the client app about an ACK or a response from the server.
            Following are the messages that are notified by the client library
            to the app.
            
            CONNACK, PINGRSP, PUBACK, SUBACK, UNSUBACK
            
            @param[in] msg_type Type of the MQTT messsage 
            @param[in] msg_id transaction identity of the message
            @param[in] buf refers to contents of message and depends on msg_type
            @param[in] len length of the buf
            @return none
            
            @note The size of the buf parameter i.e len is non-zero for the
            SUBACK and CONNACK messages. For SUBACK the buf carries an array of
            QOS responses provided by the server. For CONNACK, the buf carries 
            variable header contents. Helper macro VHB_CONNACK_RC( ) and 
            VHB_CONNACK_SP( ) can be used to access contents provided by the 
            server. For all other messages, the value of len parameter is zero.
            
            @note The parameter msg_id is not relevant for the messages CONNACK
            and PINGRSP and is set to zero.
        */        
        void (*notify_ack)(u8 msg_type, u16 msg_id, u8 *buf, u32 len); 
};

/** Initialize the MQTT Client library.
    The MQTT Library can be operated in two modes:  (a) explicit send / receive
    mode and (b) the callback mode. Provisioning or the absence of the callback
    parameter in the initialization routine defines the mode of operation of the
    MQTT Library.
    
    Explicit Send / Receive mode is analogous to the socket programming which is
    supported by apps through the utilization of the send() / recv() calls. For
    MQTT, it is anticipated that apps which would make use of limited set of MQTT
    messages may find this mode of operation useful. Apps which intends to
    operate the library in this mode must not provision any callbacks.
    
    On the other hand, certain applications, may prefer an asynchronous mode of
    operation and would want the MQTT Library to raise callbacks into the app as
    and when packets arrive from the server. Such apps must provide the callback
    routines.
    
    @param[in] cfg Configuration information for the MQTT Library. 
    @param[in] cbs Callback routines. Must be set to NULL, if the app intends to
    operate the library in the explicit send and recv mode.
    @return 0 on success otherwise -1.
*/

i32 mqtt_client_lib_init(const struct mqtt_client_lib_cfg  *cfg,
                         const struct mqtt_client_msg_cbs  *cbs);


#endif
