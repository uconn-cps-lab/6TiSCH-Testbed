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
#include "mqtt_common.h"
#include "mqtt_client.h"

//#define ENABLE_IBM_USERNAME_PASSWORD

#ifndef __SL_MQTT_H__
#define __SL_MQTT_H__

#ifdef __cplusplus
extern "C"
{
#endif
        /*!
          \mainpage SimpleLink MQTT Client Layer
          
          \section intro_sec Introduction

          The SimpleLink MQTT Client Layer provides an easy-to-use API(s) to enable
          constrained and deeply embedded microcontroller based products to interact
          with cloud or network based server for telemetery. The users of SL MQTT
          Client services, while benefiting from the abstraction of the MQTT protocol
          would find them suitable for varied deployments of MQTT subscribers and / or
          publishers.

          The following figure outlines the composition of the SL MQTT Client Layer.

          @image html ./image/sl_mqtt_client_view.png
          
          \section descrypt_sec Description

          The SL MQTT Client Layer, in addition to providing services to the application,
          encompasses a RTOS task to handle the incoming messages from the server. Such
          a dedicated context to process the messages from the server facilitates the
          apps to receive data (i.e. PUBLISH messages) even when they are blocked, whilst
          awaiting ACK for a previous transaction with the server. The receive task in
          the SL MQTT Layer can not be disabled anytime, however its system wide priority
          is configurable and can be set.

          Some of the salient features of the SL MQTT Layer are 
          
          - Easy-to-use, intuitive and small set of MQTT API
          - A single in-flight message to the server 
          - App can indicate its choice to await ACK for a message transaction
          - Supports MQTT 3.1 protocol
          
          \note An app that has chosen not to await an ACK from the server for an
          scheduled transaction can benefit from the availability of control to
          pursue other activities to make overall progress in the system. However,
          an attempt to schedule another transaction with the server, while the
          previous one is still active, will cause the application to block. The
          app will unblock, once the previous trasaction is completed.
          

          \subsection seq_subsec Typical Sequences:
          
          - Publishers:  Init --> CONNECT --> PUBLISH (TX) --> DISCONNECT
          - Subscribers: Init --> CONNECT --> SUBSCRIBE --> PUBLISH (RX) --> DISCONNECT
          
        */

        /** @defgroup sl_mqtt_cl_api SL MQTT Client API
            @{
        */
  
        /** @defgroup sl_mqtt_cl_evt SL MQTT Client Events
            @{ 
        */
#define SL_MQTT_CL_EVT_PUBACK   0x01  /**< PUBACK has been received from the server */
#define SL_MQTT_CL_EVT_SUBACK   0x02  /**< SUBACK has been received from the server */
#define SL_MQTT_CL_EVT_UNSUBACK 0x03  /**< UNSUBACK has been received from the server */
#define SL_MQTT_CL_EVT_NOCONN   0x04  /**< Client has lost network connection with server */
        /** @} */ /* End Client events */
        
    /* Define server structure which holds , server address and port number. 
      These values are set by the sl_MqttSet API and retrieved by sl_MqttGet API*/
        
        /** Callbacks Routines
            The routines are invoked by SL Implementation onto Client application
         */
        typedef struct {
                
                /** Callback routine to receive a PUBLISH from the server.
                    The client app must provide this routine for the instances where it has
                    subscribed to certain set of topics from the server. The callback is
                    invoked in the context of the internal SL Receive Task.
                    
                    \param[in] topstr name of topic published by the server. Not NUL terminated.
                    \param[in] toplen length of the topic name published by the server.
                    \param[in] payload refers to payload published by the server.
                    \param[in] pay_len length of the payload.
                    \param[in] retain asserted to indicate that a retained message has been
                    published
                    \param[in] dup assert to indicate that it is re-send by the server
                    \param[in] qoS quality of service of the published message
                    \return none.
                */
                void (*sl_ExtLib_mqtt_Recv)(unsigned char *topstr, int toplen,
                                            void *payload, int pay_len,
                                            bool retain, bool dup, unsigned char qos);
                
                /** Indication of event either from the server or implementation generated.
                    These events are notified as part of the processing carried out by the
                    internal recv task of the SL implementation. The application must populate
                    the callback to receive events about the progress made by the SL Mqtt layer.
                    
                    This handler is used by the SL Mqtt Layer to report acknowledgements from the
                    server, in case, the application has chosen not to block the service invokes
                    till the arrival of the corresponding ACK.
                    
                    \param[in] evt identifier to the reported event. Refer to @ref sl_mqtt_cl_evt
                    \param[in] buf points to buffer
                    \param[in] len length of buffer
                    
                    \note
                */
                void (*sl_ExtLib_mqtt_Event)(int evt, void *buf, unsigned int len);
                
        } SlMqttClientCbs_t;
 
        /** Secure Socket Parameters to open a secure connection */
        typedef struct {

#define SL_MQTT_NETCONN_IP6  0x01  /**< Assert for IPv6 connection, otherwise  IPv4 */
#define SL_MQTT_NETCONN_URL  0x02  /**< Server address is an URL and not IP address */
#define SL_MQTT_NETCONN_SEC  0x04  /**< Connection to server  must  be secure (TLS) */

                unsigned int         netconn_flags; /**< Enumerate connection type  */
                char                *server_addr;   /**< Server Address: URL or IP  */
                unsigned short       port_number;   /**< Port number of MQTT server */
                unsigned char        method;
                unsigned int         cipher;
                //SlSockSecureFiles_t  secure_files;
                
        } SlMqttServer_t;
        
        /** MQTT Lib structure which holds Initialization Data */
        typedef struct
        {
                SlMqttServer_t  server_info;      /**< Server information */
                unsigned int    resp_time;        /**< Reasonable response time (seconds) from server */
                unsigned long   rx_tsk_priority;  /**< Priority of the receive task */

        } SlMqttClientCfg_t;
    
        /** Initialize the SL MQTT Implementation.
            A caller must initialize the MQTT implementation prior to using its services.
            
            \param[in] cfg refers to configuration parameters
            \param[in] cbs callbacks into application 
            \return Success (0) or Failure (-1)
        */
        void sl_ExtLib_mqtt_Init(SlMqttClientCfg_t  *cfg, 
                                 SlMqttClientCbs_t  *cbs);
        
        /** @defgroup sl_mqtt_cl_params SL MQTT Oper Paramters
            @{
        */
#define SL_MQTT_PARAM_CLIENT_ID  0x01   /**< Refers to Client ID */
#define SL_MQTT_PARAM_USER_NAME  0x02   /**< User name of client */
#define SL_MQTT_PARAM_PASS_WORD  0x03   /**< Pass-word of client */
#define SL_MQTT_PARAM_TOPIC_QOS1 0x04   /**< Set a QoS1 SUB topic */
        /** @} */
        
        /** Set parameters in SL MQTT implementation.
            The caller must configure these paramters prior to invoking any MQTT
            transaction.
            
            \note The implementation does not copy the contents referred. Therefore,
            the caller must ensure that contents are persistent in the memory.
            
            \param[in] param identifies parameter to set. Refer to @ref sl_mqtt_cl_params
            \param[in] value refers to the plach-holder of value to be set
            \param[in] len length of the value of the parameter
            \return Success (0) or Failure (-1)          
        */
        int sl_ExtLib_mqtt_Set(int param, void *value, unsigned int len);
        
        /*\brief None defined at the moment
        */
        int sl_ExtLib_mqtt_Get(int param, void *value, unsigned int len);
  
  
        /** CONNECT to the server. 
            This routine establishes a connection with the server for MQTT transactions.
            The caller should specify a time-period with-in which the implementation
            should send a message to the server to keep-alive the connection.
            
            \param[in] clean assert to make a clean start and purge the previous session
            \param[in] keep_alive_time the maximum time within which client should send
            a message to server. The unit of the interval is in seconds.
            \return success(0) or failure (-1)
        */
        int sl_ExtLib_mqtt_Connect(bool clean, unsigned short keep_alive_time);
        
        /** DISCONNECT from the server.
            The caller must use this service to close the connection with the 
            server. 
         
            \return none
        */
        void sl_ExtLib_mqtt_Disconnect(void);
        
        /** @defgroup sl_mqtt_cl_cmdflags SL MQTT Command Flags
            @{
        */
#define SL_MQTT_CL_CMD_BLOCK  0x00000001  /**< Block to receive a corresponding ACK */
        /** @} */
        
        /** SUBSCRIBE a set of topics.
            To receive data about a set of topics from the server, the app through
            this routine must subscribe to those topic names with the server. The
            caller can indicate whether the routine should block until a time, the
            message has been acknowledged by the server.

            In case, the app has chosen not to await for the ACK from the server, 
            the SL MQTT implementation will notify the app about the subscription
            through the callback routine.
            
            \param[in] topics set of topic names to subscribe. It is an array of
            pointers to NUL terminated strings.
            \param[in,out] qos array of qos values for each topic in the same order
            of the topic array. If configured to await for SUB-ACK from server, the
            array will contain qos responses for topics from the server.
            \param[in] count number of such topics
            \param[in] flags Command flag. Refer to @ref sl_mqtt_cl_cmdflags           
            \return Success(0) or Failure(-1)
        */
        int sl_ExtLib_mqtt_Sub(char **topics, unsigned char *qos,
                               int count, unsigned int flags);

        
        /** UNSUBSCRIBE a set of topics.
            The app should use this service to stop receiving data for the named
            topics from the server. The caller can indicate whether the routine
            should block until a time, the message has been acknowleged by the
            server.

            In case, the app has chosen not to await for the ACK from the server, 
            the SL MQTT implementation will notify the app about the subscription
            through the callback routine.
            
            \param[in] topics set of topics to be unsubscribed. It is an array of
            pointers to NUL terminated strings.
            \param[in] count number of topics to be unsubscribed
            \param[in] flags Command flag. Refer to @ref sl_mqtt_cl_cmdflags
            \return success(0) or failure.(-1) 
        */
        int sl_ExtLib_mqtt_Unsub(char **topics, int count, unsigned int flags);

        
        
        /** PUBLISH a named message to the server.
            In addition to the PUBLISH specific parameters, the caller can indicate
            whether the routine should block until the time, the message has been
            acknowleged by the server. This is applicable only for non-QoS0 messages.
            
            In case, the app has chosen not to await for the ACK from the server, 
            the SL MQTT implementation will notify the app about the subscription
            through the callback routine.

            \note Only QoS0 and QoS1 messages are supported.
            
            \param[in] topic  topic of the data to be published. It is NUL terminated.
            \param[in] data   binary data to be published
            \param[in] len    length of the data
            \param[in] qos    QoS for the publish message 
            \param[in] retain assert if server should retain the message
            \param[in] flags Command flag. Refer to @ref sl_mqtt_cl_cmdflags
            \return Success(0) or Failure(-1).
        */
        int sl_ExtLib_mqtt_ClientSend(unsigned char *topic,  void *data,
                                      int len, int qos, bool retain, 
                                      unsigned int flags);
        
        /** @} */ /* End Client API */
 
#ifdef __cplusplus  
}
#endif  



#endif // __SL_MQTT_H__

