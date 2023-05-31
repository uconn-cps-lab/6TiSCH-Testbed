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
#include "sl_mqtt.h"
#include "tcp-socket/net_dev_services.h"

//TODO: need to use different malloc, free functions.
//#include "zip_osport.h" //zipMalloc, zipFree


#ifdef TIRTOS
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#endif /* TIRTOS */
/*Defining Events*/

#define MQTT_EVENT_ERR 0x01 /*Indicating an error occured */
#define MQTT_EVENT_ACK 0x02 /* Indicating an ack is received*/

/*Defining Flags */

#define RETAIN   0x02 /*Retain Flag*/
#if 0
#define BUF_LEN 200
#define MAX_MQP 2
#else
#define BUF_LEN 300
#define MAX_MQP 1
#endif

/*Defining Event Messages*/
#define MQTT_ACK "Ack Received"
#define MQTT_ERROR "Connection Error"
//*****************************************************************************
// global variables used
//*****************************************************************************
#ifdef TIRTOS
//Char recvTaskStack[612];
Char recvTaskStack[512];
#endif /* TIRTOS */

/* synchronisation object. This is used in blocking mode to ensure ack is received*/
#ifdef TIRTOS
Semaphore_Handle SyncObj;
#else
_SlSyncObj_t SyncObj;
#endif /* TIRTOS */

/* synchronisation object. This is used to synchronise background receive task 
with connect and disconnect*/

Semaphore_Handle RxSyncObj;


/*Lock Object used to ensure only one inflight message*/

Semaphore_Handle g_LockObj,LockObj;


/*utf8 string into which client id will be stored*/
struct utf8_string client;

#if 0
/*utf8 string into which username will be stored*/
struct utf8_string *user=NULL;

/*utf8 string into which password will be stored*/
struct utf8_string *password=NULL;
#else
/*utf8 string into which username will be stored*/
struct utf8_string user;

/*utf8 string into which password will be stored*/
struct utf8_string password;

#endif

/* g_sRxMQP =>  mqtt packet structure which can be used by sl_ExtLib_mqtt_Receive API */
struct mqtt_packet g_sRxMQP;

/*Buffer associated with the receive packet*/
uint8_t rx_buf[BUF_LEN];

/*Dummy debug print function to be passed into Mqtt Core lib*/
int Mqtt_Print(const char *pcFormat, ...);

/* Client configurations are stored within cfg. Server Address and Port Number will be
   updated within the structure cfg
*/
struct mqtt_client_lib_cfg client_cfg =
        { true,
          0,
          0,
          0,
          Mqtt_Print, 
          NULL, //osi_EnterCritical,    /* Atomic Enable  */
          NULL //osi_ExitCritical     /* Atomic Disable */
        }; 

/* Network Services specific to cc3200*/
struct device_net_services net={tcp_open,tcp_send,tcp_recv,close_tcp,rtc_secs};

/* Call backs structure*/
struct mqtt_client_msg_cbs cbs = {NULL, NULL};

/*SlHost callbacks structure*/
SlMqttClientCbs_t Sl_cbs={NULL,NULL};

/*Flag indicating ack await stage */
uint8_t cAwaitedAck;

/*Flag indicating Connection status*/
volatile bool bConnect=false;

/*Flag indicating Blocking or Non Blocking*/
bool g_bBlocking=false;

/* Creating a pool of MQTT coonstructs that can be used by the MQTT Lib
     mqp_vec =>pointer to a pool of the mqtt packet constructs
      buf_vec => the buffer area which is attached with each of mqp_vec*/
DEFINE_MQP_BUF_VEC(MAX_MQP, mqp_vec, BUF_LEN, buf_vec);

/*Task Priority and Response Time*/
#ifndef TIRTOS
uint32_t g_TaskPriority;
#endif /* TIRTOS */

/* Response Time */
uint32_t g_wait_secs;

#ifndef TIRTOS
extern TcpSecSocOpts_t g_SecSocOpts;
extern SlSockSecureFiles_t	g_sockSecureFiles;
#endif /* TIRTOS */


/* MQTT Quality of Service */
static const enum mqtt_qos qos[] ={
        
        MQTT_QOS0,
        MQTT_QOS1,
        MQTT_QOS2
};


/* Defining Retain Flag. Getting retain flag bit from fixed header*/
#define MQP_PUB_RETAIN(mqp)    ( ((mqp->fh_byte1 ) & 0x01)?true:false )

/*Defining Duplicate flag. Getting Duplicate flag bit from fixed header*/
#define MQP_PUB_DUP(mqp)        (((mqp->fh_byte1>>3) & 0x01)?true:false )

/*Defining QOS value. Getting QOS value from fixed header*/
#define MQP_PUB_QOS(mqp)        ((mqp->fh_byte1 >>1)  &0x03)

//*****************************************************************************
// process_rx_pubd 
//*****************************************************************************
static void
process_rx_pub(struct mqtt_packet *mqp)
{
  
  //
  // Invokes the event handler with topic,topic length,payload and payload length values
  //
  Sl_cbs.sl_ExtLib_mqtt_Recv(MQP_PUB_TOP_BUF(mqp), MQP_PUB_TOP_LEN(mqp),MQP_PUB_PAY_BUF(mqp),MQP_PUB_PAY_LEN(mqp),MQP_PUB_RETAIN(mqp),MQP_PUB_DUP(mqp),MQP_PUB_QOS(mqp));
}

//*****************************************************************************
// process_rx_pkt 
//*****************************************************************************
static void 
process_rx_pkt(struct mqtt_packet *mqp)
{
    //
    // Incoming messages are either pub or ack.
    //
    if(MQTT_PUBLISH == mqp->msg_type)
        process_rx_pub(mqp); 
    else 
    {   
        //
        // If the client is in blocking mode and we got ack from broker
        //
        
        Semaphore_pend( LockObj,  BIOS_WAIT_FOREVER );
        

        if(g_bBlocking && (cAwaitedAck == mqp->msg_type))
        {
            //
            // update the Client status
            //
            g_bBlocking=false;
           
            Semaphore_post( LockObj );
            
            //  
            // Signal the sync object
            //
            
            Semaphore_post( SyncObj );
            
        }   
        // Client is not in blocking mode. we got the ack
        else if(cAwaitedAck == mqp->msg_type)
        {            
            Semaphore_post( LockObj );
            
            Sl_cbs.sl_ExtLib_mqtt_Event(MQTT_EVENT_ACK,MQTT_ACK,strlen(MQTT_ACK)); 
            //
            // Unlock the object which has been taken
            //
           
            Semaphore_post( g_LockObj );            
        }
        else
        {          
          Semaphore_post( LockObj );
          
        }
    }
  return ;
}

//*****************************************************************************
// Receive Task. Invokes in the context of a task 
//*****************************************************************************
#ifdef TIRTOS
void VMqttRecvTask(UArg a0, UArg a1)
#else
void 
VMqttRecvTask(void *pArgs)  
#endif /* TIRTOS */
{
    do
    {
        //
        // wait for the sync object. signaled on connection success
        //
        
        (void)Semaphore_pend( RxSyncObj, BIOS_WAIT_FOREVER );
        
        while(1) 
        {
            //
            // awaiting any type of message
            //
            int Ret = mqtt_client_recv(&g_sRxMQP, g_wait_secs);
            //
            // bConnect flag indicates connection status. It will be updated by
            // sl_MqttConnect and sl_MqttDisconnect. If user calls sl_MqttDisconnect
            // this flag will be set as false
            //
         
            (void)Semaphore_pend( LockObj, BIOS_WAIT_FOREVER );
            
            if(bConnect==false)
            {                
              Semaphore_post( LockObj );
              Semaphore_post( SyncObj );
              
              break;
            }
         
            Semaphore_post( LockObj );
            
            if(Ret > 0)
            {
                process_rx_pkt(&g_sRxMQP);
            } 
            else if(MQP_ERR_TIMEOUT == Ret)
            {
                // Time out happens. Since Timeout don t indicate broker connection 
                // lost. Loop back to mqtt_recv() call
            }
                
            else 
            {
               //
               //Socket Error or Network Error or Unknown Error. Recv task exits from recv loop and waits on
               // next mqtt_Connect to happen
               //
               Sl_cbs.sl_ExtLib_mqtt_Event(SL_MQTT_CL_EVT_NOCONN,MQTT_ERROR,strlen(MQTT_ERROR));
               break;
            }
            
               
        }
    }while(1);
    
}

//*****************************************************************************
// sl_MqttInit 
//*****************************************************************************

void 
sl_ExtLib_mqtt_Init(SlMqttClientCfg_t  *cfg,SlMqttClientCbs_t  *cbs)
{
 
    //
    // initialize brokers ip address and port number
    //
    if((cfg->server_info.netconn_flags & SL_MQTT_NETCONN_URL))
    { 
      //Get IP address host by name
    }
    else  
    {
      client_cfg.server_addr=cfg->server_info.server_addr;
    }
    client_cfg.port_number=cfg->server_info.port_number;
    
    //
    //initialize wait seconds
    //
    g_wait_secs=cfg->resp_time;
    
    //
    // initialize SLHost Call Backs
    //
    Sl_cbs.sl_ExtLib_mqtt_Recv=cbs->sl_ExtLib_mqtt_Recv;
    Sl_cbs.sl_ExtLib_mqtt_Event=cbs->sl_ExtLib_mqtt_Event;
    if(cfg->server_info.netconn_flags & SL_MQTT_NETCONN_SEC)
    {
        //
        // initialize secure socket parameters
        //
       // g_SecSocOpts.SecurityCypher=  cfg->server_info.cipher;
       // g_SecSocOpts.SecurityMethod= cfg->server_info.method;
       // g_sockSecureFiles=cfg->server_info.secure_files;
        
    }
    else
    {
       // g_SecSocOpts.SecurityCypher=0;
       // g_SecSocOpts.SecurityMethod=6;
    }  //
    //
    //initialize task priority
    //
   
    //
    // Initialize client library with broker configuration{no: of retries,server address etc} and call backs
    //
    mqtt_client_lib_init(&client_cfg, NULL);
    //
    // provide MQTT Lib information to create a pool of MQTT constructs. 
    //
    mqtt_client_register_buffers(MAX_MQP,mqp_vec,BUF_LEN,&buf_vec[0][0]);
    //
    // setup the receive packet
    //  
    mqp_buffer_attach(&g_sRxMQP, rx_buf, BUF_LEN,0);
    //
    // Register network services speicific to CC3200
    //
    mqtt_client_register_net_svc(&net);
    //
    // create and clear the sync objects 
    //
    
    {
        Semaphore_Params params;
        Task_Params taskParams;

        Semaphore_Params_init(&params);
        params.mode = Semaphore_Mode_BINARY;
        //RxSyncObj = Semaphore_create( 1, &params, NULL );
        // make sure RxSync is flase initially
        RxSyncObj = Semaphore_create( 0, &params, NULL );
        SyncObj = Semaphore_create( 1, &params, NULL );
        g_LockObj = Semaphore_create( 1, &params, NULL );
        LockObj = Semaphore_create( 1, &params, NULL );

#if 1
          // Configure task.
        Task_Params_init(&taskParams);
        taskParams.stack = recvTaskStack;
        taskParams.stackSize = sizeof(recvTaskStack);
        taskParams.priority = 1;

        Task_create( VMqttRecvTask, &taskParams, NULL); 
#endif        
    }

    return;
}

//*****************************************************************************
// sl_MqttDeInit 
//*****************************************************************************
void 
sl_ExtLib_mqtt_DeInit()
{

  Semaphore_delete( &SyncObj );
  Semaphore_delete( &RxSyncObj );
  Semaphore_delete( &g_LockObj );
  Semaphore_delete( &LockObj );
    
}

//*****************************************************************************
// sl_MqttSet 
//*****************************************************************************
int
sl_ExtLib_mqtt_Set(int cfg,void *ptr,unsigned int len)
{
 
    switch(cfg)
    {
        case SL_MQTT_PARAM_CLIENT_ID:
          //
          // Get Client information from the user
          //
          client.buffer =(uint8_t*)ptr;
          client.length=strlen((char*)client.buffer);
          
          break;
        case SL_MQTT_PARAM_USER_NAME:
          //
          // Get Username and Password  from the user
          //
#if 0          
          user->buffer =(uint8_t*)ptr;
          user->length=strlen((char*)user->buffer);
#else
          user.buffer = (uint8_t*)ptr;
          if (user.buffer != NULL)
            user.length = strlen((char*)user.buffer);
          else
            user.length = 0;
#endif

          // Provide info to lib. Since username and password leads to connection refusal its disabled now
          break;
        
        case SL_MQTT_PARAM_PASS_WORD:
          //
          // Get Username and Password  from the user
          //
#if 0          
          password->buffer =(uint8_t*)ptr;
          password->length=strlen((char*)password->buffer);
#else
          password.buffer =(uint8_t*)ptr;
          if (password.buffer != NULL )            
            password.length = strlen((char*)password.buffer);
          else
            password.length = 0;
#endif
          // Provide info to lib. Since username and password leads to connection refusal its disabled now
          break;
  
    case SL_MQTT_PARAM_TOPIC_QOS1:
          //
          // Set a qos1 Sub topic
          //
          break;
        default:
          break;
   }
   return 0;
   
}

//*****************************************************************************
// sl_MqttGet 
//*****************************************************************************
int
sl_ExtLib_mqtt_Get(int cfg,void *value, unsigned int len)
{
  // Not enabled now
#if 0
    switch(cfg)
    {
        case SL_MQTT_PARAM_CLIENT_ID:
            //
            // return pointer to "client"  which stores client id
            //
            value= (void*)client.buffer;
            len=client.length;
            break;
  
        case SL_MQTT_PARAM_USER_NAME:
            //
            // return pointer to username 
            //
            value= (void*)user->buffer;
            len=user->length;
            break;
            
        case SL_MQTT_PARAM_PASS_WORD:
            //
            // return pointer to username 
            //
            value= (void*)password->buffer;
            len=password->length;
            break;
            
    case SL_MQTT_PARAM_TOPIC_QOS1:
            
          break;
        default:
            
            break;
    }
#endif
return 0;
}

static void
updateConnectionStatus(bool bStatus)
{

  Semaphore_pend( LockObj, BIOS_WAIT_FOREVER );

  bConnect = bStatus;
  
  Semaphore_post( LockObj );
    
    
}
//*****************************************************************************
// sl_MqttConnect 
//*****************************************************************************
int
sl_ExtLib_mqtt_Connect(bool bClean,unsigned short kepp_alive_time)
{
  int32_t iRet=0;
  //
  // Client CONNECT to Broker
  //
  
  
  Semaphore_pend( g_LockObj, BIOS_WAIT_FOREVER );
  
  //
  // Provide Client ID into MQTT Library 
  //
  
  //mqtt_client_register_info(&client,user,password); 
  // no user name and password
#ifndef ENABLE_IBM_USERNAME_PASSWORD  
  mqtt_client_register_info(&client,NULL,NULL); 
#else
  mqtt_client_register_info(&client,&user,&password); 
#endif
  iRet = mqtt_connect(bClean, kepp_alive_time,g_wait_secs);
  
  Semaphore_post( g_LockObj );
  
  if(iRet < 0) 
  {
    //
    // Return Error
    //
    return iRet;
  }
  //
  // Set the connection flag
  //
  updateConnectionStatus(true);
  //
  // Signal the semaphore. Receive Task will be waiting on this semaphore
  //

  // BLOCK the RX task
  //Semaphore_post(RxSyncObj);    
 
  return iRet;
}

//*****************************************************************************
// sl_MqttDisconnect 
//*****************************************************************************
void
sl_ExtLib_mqtt_Disconnect()
{
   
  Semaphore_pend( g_LockObj, BIOS_WAIT_FOREVER );
  
  //
  //update bConnect variable which will be read by receive task also.
  // when this variable is set to false, receive task signal the semaphore
  //
  updateConnectionStatus(false);
  //
  //To ensure that Recv Task has given up socket 
  //
 
  Semaphore_pend( SyncObj, BIOS_WAIT_FOREVER );
  
  //
  //send the disconnect command
  //
  
  mqtt_disconn_send();   
  
  Semaphore_post( g_LockObj );
    
}


static void
updateSharedVariables(uint8_t cAck,bool bBlocking)
{
    
  Semaphore_pend( LockObj, BIOS_WAIT_FOREVER );
 
  //
  // wait for ack
  //
  cAwaitedAck=cAck;
  //
  // update the Client status
  //
  g_bBlocking=bBlocking;
  
  Semaphore_post( LockObj );
    
}

#if 0
//*****************************************************************************
// sl_MqttSub 
//*****************************************************************************

int
sl_ExtLib_mqtt_Sub( char **topics,unsigned char *iQos,int count,unsigned int Flags)
{
    int32_t Ret = -1;
    int32_t i;
    //
    // Allocate an array of qos_topic structure
    //
    struct utf8_strqos *qos_topics=(struct utf8_strqos*)zipMalloc(sizeof(struct utf8_strqos)*count);
   
    if(!(mqtt_client_is_connected()))
            goto mqtt_sub_exit1;  // client not connected
    
    #ifdef TIRTOS 
    Semaphore_pend( g_LockObj,  BIOS_WAIT_FOREVER );
    #else
    sl_LockObjLock(&g_LockObj,SL_OS_WAIT_FOREVER); //Lock entry
    #endif /* TIRTOS */

    //
    // populate the values
    //
    for(i=0;i<count;i++)
    {
        qos_topics[i].buffer=(uint8_t*)topics[i];
        qos_topics[i].qosreq=qos[iQos[i]];
        qos_topics[i].length=strlen((char*)topics[i]);
          
    }
   // Next revision, need better synchronization between RX Task and this context

    // Set up all variables to receive an ACK for the message to be sent
    updateSharedVariables(MQTT_SUBACK, (Flags & SL_MQTT_CL_CMD_BLOCK)? true : false);
    if(mqtt_sub_msg_send(qos_topics,count) < 0)
    {
        #ifdef TIRTOS 
        Semaphore_post( g_LockObj );
        #else
        sl_LockObjUnlock(&g_LockObj); // send failed
        #endif /* TIRTOS */
        goto mqtt_sub_exit2;
    }

    if(Flags & SL_MQTT_CL_CMD_BLOCK)
    {      
        #ifdef TIRTOS
        Semaphore_pend( SyncObj, (UInt32) BIOS_WAIT_FOREVER );
        Semaphore_post( g_LockObj );
        #else
        sl_SyncObjWait(&SyncObj,SL_OS_WAIT_FOREVER);
        sl_LockObjUnlock(&g_LockObj);
        #endif /* TIRTOS */
    }

    Ret = 0;

 mqtt_sub_exit2:
    updateSharedVariables(0x00, false);

 mqtt_sub_exit1:
    zipFree(qos_topics);
    return Ret;
}



//*****************************************************************************
// sl_MqttUnsub 
//*****************************************************************************

int
sl_ExtLib_mqtt_Unsub(char **topics,int count,unsigned int Flags)
{
    int32_t Ret = -1;
    int32_t i;

    //
    //generate array of structures of utf8_string type and store the details
    //
    struct utf8_string *sTopics=(struct utf8_string*)zipMalloc(sizeof(struct utf8_string)*count);
    if(!(mqtt_client_is_connected()))
            goto mqtt_unsub_exit1; // client not connected

    #ifdef TIRTOS 
    Semaphore_pend( g_LockObj, BIOS_WAIT_FOREVER );
    #else
    sl_LockObjLock(&g_LockObj,SL_OS_WAIT_FOREVER); // Lock entry
    #endif /* TIRTOS */
    // Next revision, need better synchronization between RX Task and this context
    //
    //populate the values
    //
    for(i=0;i<count;i++)
    {
        sTopics[i].buffer=(uint8_t*)topics[i];
        sTopics[i].length=strlen((char*)topics[i]);
    }
    // Set up all variables to receive an ACK for the message to be sent
    updateSharedVariables(MQTT_UNSUBACK, (Flags & SL_MQTT_CL_CMD_BLOCK)? true : false);
    if(mqtt_unsub_msg_send(sTopics,count) < 0)
    {
        #ifdef TIRTOS 
        Semaphore_post( g_LockObj );
        #else
        sl_LockObjUnlock(&g_LockObj); // send failed
        #endif /* TIRTOS */
        goto mqtt_unsub_exit2;
    }
    if(Flags & SL_MQTT_CL_CMD_BLOCK)
    {
        #ifdef TIRTOS
        Semaphore_pend( SyncObj, BIOS_WAIT_FOREVER );
        Semaphore_post( g_LockObj );
        #else
        sl_SyncObjWait(&SyncObj,SL_OS_WAIT_FOREVER);
        sl_LockObjUnlock(&g_LockObj);
        #endif /* TIRTOS */
    }

    Ret = 0;

 mqtt_unsub_exit2:
    updateSharedVariables(0x00, false);
    
 mqtt_unsub_exit1:
    zipFree(sTopics);
    return Ret; 
}
#endif
//*****************************************************************************
// sl_MqttSend 
//*****************************************************************************
int 
sl_ExtLib_mqtt_ClientSend(unsigned char *topic,void *data,int iDataLen,int iQos,bool retain,unsigned int Flags )
{
  int32_t Ret = -1;
  struct utf8_string sTopic={NULL};
  //
  // Check whether Client is connected or not?
  //
  if(!(mqtt_client_is_connected()))
  {
      return Ret;
  }

  Semaphore_pend( g_LockObj, BIOS_WAIT_FOREVER );
 

  // Next revision, need better synchronization between RX Task and this context
  sTopic.buffer = topic;
  sTopic.length=strlen((char*)topic);
  if(MQTT_QOS0 != qos[iQos])
          updateSharedVariables(MQTT_PUBACK, (Flags & SL_MQTT_CL_CMD_BLOCK)? true : false); 

  if(mqtt_client_pub_msg_send(&sTopic, data, iDataLen,
                              qos[iQos], retain) < 0) 
  {
      goto mqtt_pub_exit1;
  }
 
  if(MQTT_QOS1==qos[iQos])
  {
      if(Flags & SL_MQTT_CL_CMD_BLOCK) 
      {
         
        //Semaphore_pend( SyncObj, (UInt32) BIOS_WAIT_FOREVER );
      	Semaphore_pend( SyncObj, BIOS_WAIT_FOREVER );
          
      }
  }

  Ret = 0;

 mqtt_pub_exit1:
  updateSharedVariables(0, false);
 
  Semaphore_post( g_LockObj );
  

  return Ret;
}

int 
Mqtt_Print(const char *pcFormat, ...)
{
  return 0;
}


