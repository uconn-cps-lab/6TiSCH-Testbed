/**
  @file  cc26xxsensortag_host_app.c
  @brief CC26xx sensor tag host application

  <!--
  Copyright 2014 Texas Instruments Incorporated. All rights reserved.

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
  PROVIDED ``AS IS'' WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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
  -->
*/

/*******************************************************************************
 * INCLUDES
 */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>

#include "sys/clock.h"

#if 0
#include <ti/sysbios/family/arm/cc26xx/Power.h>
#include <ti/sysbios/family/arm/cc26xx/PowerCC2650.h>
#include <ti/drivers/AES.h>
#include <ti/drivers/aes/AESTiva.h>
#include <Board.h>
#endif
#include <string.h>

#include "sl_mqtt.h"
#include "webapp/ti_web_sensor_msg.h"

extern uint8_t joinedDODAG;

// ==============================================
// SPECIFY the MQTT broker IP address
//
// ==============================================


#define MQTT_BROKER_SERVER_ADDR_LOCAL_BBB
//#define MMQT_BROKER_SERVER_ADDR_IBM
//#define MQTT_BROKER_SERVER_ADDR_AMAZON
//#define MQTT_BROKER_SERVER_ADDR_TEST_MOSQUITTO
//#define MQTT_BROKER_SERVER_ADDR_RIOT_TI



// ==============================================
// SPECIFY the MQTT message format
//
// ==============================================

#define MQTT_MSG_FORMAT_TI_GUI     
//#define MQTT_MSG_FORMAT_IBM         


// every two seconds
// default =20 seconds
#define MQTT_APP_DATA_POLL_PERIOD       (4)
#define MQTT_TSK_CONNECT_FAIL_SLEEP     (3)

/*******************************************************************************
 * MACROS
 */

/**
 * Macro to check if the extended address is valid
 */
#define EXT_ADDR_VALID( extAddr )                                              \
  ( !(                                                                         \
       ((extAddr)[0] == 0xFF) &&                                               \
       ((extAddr)[1] == 0xFF) &&                                               \
       ((extAddr)[2] == 0xFF) &&                                               \
       ((extAddr)[3] == 0xFF) &&                                               \
       ((extAddr)[4] == 0xFF) &&                                               \
       ((extAddr)[5] == 0xFF) &&                                               \
       ((extAddr)[6] == 0xFF) &&                                               \
       ((extAddr)[7] == 0xFF)                                                  \
     )                                                                         \
  )
/**
 * Copies extended address
 */
#define COPY_EXT_ADDR( dstPtr, srcPtr )                                        \
  (dstPtr)[0] = (srcPtr)[0];                                                   \
  (dstPtr)[1] = (srcPtr)[1];                                                   \
  (dstPtr)[2] = (srcPtr)[2];                                                   \
  (dstPtr)[3] = (srcPtr)[3];                                                   \
  (dstPtr)[4] = (srcPtr)[4];                                                   \
  (dstPtr)[5] = (srcPtr)[5];                                                   \
  (dstPtr)[6] = (srcPtr)[6];                                                   \
  (dstPtr)[7] = (srcPtr)[7];

#define COPY_EXT_ADDR_BE( dstPtr, srcPtr )                                        \
  (dstPtr)[0] = (srcPtr)[7];                                                   \
  (dstPtr)[1] = (srcPtr)[6];                                                   \
  (dstPtr)[2] = (srcPtr)[5];                                                   \
  (dstPtr)[3] = (srcPtr)[4];                                                   \
  (dstPtr)[4] = (srcPtr)[3];                                                   \
  (dstPtr)[5] = (srcPtr)[2];                                                   \
  (dstPtr)[6] = (srcPtr)[1];                                                   \
  (dstPtr)[7] = (srcPtr)[0];
  
  
/*******************************************************************************
 * CONSTANTS
 */
/**
 * MQTT port number
 */
#define PORT_NUMBER               1883

/**
 * Receive time out for the MQTT Receive task.
 */
 
//#define RCV_TIMEOUT               10
#define RCV_TIMEOUT          789

/**
 * Task priority for the MQTT receive task.
 */
#define TASK_PRIORITY             3

/**
 * MQTT Keep Alive Timer value.
 */
//#define KEEP_ALIVE_TIMER          25
#define KEEP_ALIVE_TIMER          999

/**
 * MQTT Clean session flag. 
 */
#define CLEAN_SESSION             true

/**
 * MQTT Retain Flag. Used in publish message.
 */
#define RETAIN                    1

/**
 * MQTT QOS levels
 */
#define QOS0                      0
#define QOS1                      1
#define QOS2                      2

/**
 * Extended address len in bytes. 
 */
#define EXT_ADDR_LEN              8     
  
/**
 * FCFG Base.
 */
#define FCFG1_BASE                0x50001000
  
/**
 * Flash page size
 */
#define FLASH_PAGE_SIZE           4096
/**
 * Location where the flash size is written.
 */
#define FLASH_SIZE_OFFSET         0x2B1

/**
 * ExtADDR Flash Address Offset in CCA 
 */
#define EXTADDR_PAGE_OFFSET       0xFC8

/**
 * Extended Address offset in FCFG
 */
#define EXTADDR_OFFSET            0x2F0     // in FCFG; LSB..MSB


// IEEE address location, factory config page 
#define FCFG_FLASH_IEEE_ADDR      0x500012F0
// IEEE address location, customer config page 
#define CCFG_FLASH_IEEE_ADDR      0x50003FC8

/** 
 * Note that AES is not defined in SensorTag board definition,
 * hence they are defined here instead.
 */
#define Board_AES                 0
#define Board_AES_COUNT           1


/*******************************************************************************
 * GLOBAL VARIABLES
 */

/** 
 * Semaphore for the manager task.
 */ 
Semaphore_Handle mgr_sem;

/** 
 * Semaphore for the mqttApp task.
 */ 
Semaphore_Handle mqttApp_sem;

/** 
 * Semaphore for MQTT tcp read operation synchronization.
 */ 
Semaphore_Handle tcpRead_sem;

/**
 * The MQTT broker IP address. 
 * The server IPv4 address is 184.172.124.189
 */

#if defined ( MQTT_BROKER_SERVER_ADDR_LOCAL_BBB) 
// for local BBB: 
//  old prefix:   0xAA, 0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
//  new prefix: 0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0xff, 0xff, 
unsigned char svrAddr[16] = { 0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0xff, 0xff, 
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
                              
#elif defined (MMQT_BROKER_SERVER_ADDR_IBM)
// for IBM quickstart 184.172.124.189 (b8.ac.7c.bd)
// for IBM registered 23.246.232.210 (17.f6.e8.d2)

#ifndef ENABLE_IBM_USERNAME_PASSWORD  
// quick start service
unsigned char svrAddr[16] = { 0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0xff, 0xff, 
                              0x00, 0x00, 0x00, 0x00, 0xb8, 0xac, 0x7c, 0xbd};

#else
// for registered service
unsigned char svrAddr[16] = { 0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0xff, 0xff, 
                              0x00, 0x00, 0x00, 0x00, 0x17, 0xf6, 0xe8, 0xd2};

#endif

#elif defined (MQTT_BROKER_SERVER_ADDR_AMAZON)
// for amzaon (54.69.202.101)  (36.45.ca.65)
unsigned char svrAddr[16] = { 0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0xff, 0xff, 
                              0x00, 0x00, 0x00, 0x00, 0x36, 0x45, 0xca, 0x65};
                              
#elif defined (MQTT_BROKER_SERVER_ADDR_TEST_MOSQUITTO)
// test Mosquitto: 85.119.83.194 (hex: 55:77:53:c2)
unsigned char svrAddr[16] = { 0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0xff, 0xff, 
                              0x00, 0x00, 0x00, 0x00, 0x55, 0x77, 0x53, 0xc2};
                              
#elif defined (MQTT_BROKER_SERVER_ADDR_RIOT_TI)
// IPv4: RIOT.itg.ti.com :128.247.102.33 (0x80.F7.66.21)
unsigned char svrAddr[16] = { 0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0xff, 0xff, 
                              0x00, 0x00, 0x00, 0x00, 0x80, 0xf7, 0x66, 0x21};
                            
#else
// error: need to specify the MQTT broker
#error "please specify the MQTT broker IP address "
#endif


/** 
 * MQTT Client ID 
 */ 
//unsigned char *clientId = "quickstart:00124b000002";
char clientId[48] = "quickstart:00124b000002";

// define for user name and password
char client_user[32];
char client_password[32];

/** 
 * MQTT Publish topic 
 */ 
//unsigned char *pubTopic = "iot-1/d/00124b000002/evt/titag-quickstart/json";
//unsigned char pubTopic[128] = "ABCDEFGHIJKLMNOPQURTUVWXYZABCDEFGHIJKLMNO";
char pubTopic[128];


uint32_t data_count=0;

#if 0
/** 
 * AES objects 
 */ 
AESTiva_Object aesTivaObjects[Board_AES_COUNT];

/** 
 * I2C configuration structure, describing which pins are to be used 
 */ 
const AESTiva_HWAttrs aesTivaHWAttrs[Board_AES_COUNT] = {
    {CRYPTO_BASE, INT_CRYPTO, PERIPH_CRYPTO}
};

const AES_Config AES_config[] = {
    {&AESTiva_fxnTable, &aesTivaObjects[0], &aesTivaHWAttrs[0]},
    {NULL, NULL, NULL}
};
#endif

/**
 * Initialization structure to be used with sl_ExtMqtt_Init API
 */
SlMqttClientCfg_t mqttClient = 
{
  {
    SL_MQTT_NETCONN_IP6,
    (char *)svrAddr,
    PORT_NUMBER,
    0,
    //0,
    NULL
    }
    ,
  RCV_TIMEOUT,
  TASK_PRIORITY
};

typedef struct __mqtt_app_dbg__
{
  uint16_t num_new_conn;
  uint16_t num_message;
  uint16_t num_message_error;

} MQTT_APP_DBG_s;

MQTT_APP_DBG_s MQTT_APP_Dbg;
int init_msg=0;
#if 0
/*******************************************************************************
 * FUNCTIONS
 */

/**
 * Sets the extended address into the Low PAN interface.
 * It first tries to read the extended address from the customer
 * configuration area, if not set there, reads it from the
 * factory configuration area and even if not found there,
 * creates an extended address with the last 4 bytes random.
 */
static void setExtAddr( void )
{
  uint8_t extAddr[EXT_ADDR_LEN];

  // fetch ExtAddr from CCA
  COPY_EXT_ADDR_BE( extAddr,
                   (uint8_t *)(((*((uint8_t *)(FCFG1_BASE + FLASH_SIZE_OFFSET)) - 1) *
                   FLASH_PAGE_SIZE) + EXTADDR_PAGE_OFFSET) );

  // check if ExtAddr is valid
  if ( EXT_ADDR_VALID( extAddr ) == FALSE )
  {
    // it isn't, so use ExtAddr from FCFG, valid or not
    COPY_EXT_ADDR_BE( extAddr,
                   (uint8_t *)(FCFG1_BASE + EXTADDR_OFFSET) );
  }

  //TODO: Remove the temporary code once we know that all chips coming from factory
  // are programmed with the default eui64.
  if ( EXT_ADDR_VALID( extAddr ) == FALSE )
  {
    uint8_t tempExtAddr[ EXT_ADDR_LEN ] =  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    //Generate extended address
    memcpy( extAddr, tempExtAddr, EXT_ADDR_LEN );
    *(uint32_t*)&extAddr[4] = zipRandom();
  }
  
  zipLoWPANSetEUI64((unsigned char*)extAddr);
} /* setExtAddr() */
#endif
/**
 * Defines Mqtt_Pub_Message_Receive event handler.                         
 * Client App needs to register this event handler with sl_ExtLib_mqtt_Init
 * API. Background receive task invokes this handler whenever MQTT Client  
 * receives a Publish Message from the broker.                             
 * 
 * @param topstr    pointer to topic of the message             
 * @param toplen    topic length                                
 * @param payload   pointer to payload                         
 * @param pay_len   payload length                             
 * @param bRetain   Tells whether its a Retained message or not
 * @param bDup      Tells whether its a duplicate message or not
 * @param Qos       Tells the Qos level                          
 */
void 
mqttRecv( unsigned char *topstr, int toplen, void *payload,
          int pay_len,bool bRetain,bool bDup,unsigned char Qos )
{
  //TODO: Handle subscribed data if any.
  return;
}

/**
 * Mqtt client event handler. Client App needs to register this
 * event handler with sl_ExtLib_mqtt_Init API. Background 
 * receive task invokes this handler whenever MQTT Client 
 * receives an ack(whenever user is in non-blocking mode) or 
 * encounters an error. 
 * 
 * @param evt Event that invokes the handler. Event can be of
 *            the following types: MQTT_ACK - Ack Received
 *            MQTT_ERROR - unknown error
 *  
 * @param buf points to buffer
 * @param len buffer length
 */
void 
mqttEvtHdlr(int evt, void *buf,unsigned int len)
{
  //TODO: handle events from mqtt client lib
}

/**
 *  Callbacks
 */
SlMqttClientCbs_t mqttCallBacks =
{
  mqttRecv,
  mqttEvtHdlr
};

// Robert ???
extern unsigned char mqtt_pub_data[256];

/* demo node manager callback functions */
void NodeMgrCback(void)
{
  Semaphore_post( mqttApp_sem );
}

/**
 * Mqtt application task entry point.
 * 
 * @param a0 **Not used**
 * @param a1 **Not used**
 * 
 * @returns  none.
 */
Void mqttApp_task(UArg a0, UArg a1)
{
  int status = 0;
  

  Semaphore_Params params;
  /* Create a semaphore for mqttApp synchronization */
  Semaphore_Params_init(&params);
  params.mode = Semaphore_Mode_BINARY;
  mqttApp_sem = Semaphore_create(0, &params, NULL);

  //ADDED Contiki 2.7
  while(!tcpip_isInitialized()){
      Task_sleep(2*CLOCK_SECOND);//Until the uIP stack is initialized, we do not do anything. When using 16 bit MAC addresses, this IS NEEDED
  }
  
  ti_web_topic_msg_init();
  

  // init client ID
  ti_web_ibm_client_id(clientId);

#if 1
  // set up user name and passowrd
  ti_web_ibm_setup_user_password(client_user,client_password);
#endif


  /* Initialze MQTT client lib */
  sl_ExtLib_mqtt_Init( &mqttClient,&mqttCallBacks );

  /* Set Client ID */
  sl_ExtLib_mqtt_Set( SL_MQTT_PARAM_CLIENT_ID, clientId, strlen((char*)clientId) );

  // set up user name and passowrd 
#ifndef ENABLE_IBM_USERNAME_PASSWORD  
  /* no user name and password  */
  sl_ExtLib_mqtt_Set( SL_MQTT_PARAM_USER_NAME, NULL,0 );
  
  /* set client password  */
  sl_ExtLib_mqtt_Set( SL_MQTT_PARAM_PASS_WORD, NULL,0 );
#else
  // set user name
  sl_ExtLib_mqtt_Set( SL_MQTT_PARAM_USER_NAME,client_user,strlen((char*)client_user) );
  // set user password
  sl_ExtLib_mqtt_Set( SL_MQTT_PARAM_PASS_WORD,client_password,strlen((char*)client_password) );
#endif

  // wait the node is associated with parent
#if 1
  //Semaphore_pend( mqttApp_sem, (UInt32)BIOS_WAIT_FOREVER );

  while (1)
  {
    /* Delay the reconnection to the server.*/
    Semaphore_pend(mqttApp_sem, CLOCK_SECOND); 

    // check if node is connect to root
    if (joinedDODAG == 1)
      break;
  }
#endif
  Task_sleep(3*CLOCK_SECOND);
  
  do
  {

    MQTT_APP_Dbg.num_new_conn++;
    
    /* Loop until the connection is established to the server. */
    do
    {
      status = sl_ExtLib_mqtt_Connect(CLEAN_SESSION, KEEP_ALIVE_TIMER);
      if (status < 0)
      { // delay some and close connection
        Task_sleep(MQTT_TSK_CONNECT_FAIL_SLEEP*CLOCK_SECOND);
      }
    } while (status < 0);
    init_msg =0;
    
    for ( ;; )
    {

#if defined (MQTT_MSG_FORMAT_TI_GUI)
      if (init_msg ==0 )
      {
        ti_web_node_init_topic(pubTopic);
        ti_web_node_init_msg(mqtt_pub_data);
      }        
#elif defined(MQTT_MSG_FORMAT_IBM)
      if (init_msg ==0 )
      {
        ti_web_ibm_topic(pubTopic);
        ti_web_ibm_message(mqtt_pub_data);
      }  
#else
#error "please specify the message format"
#endif      
      /**
       * Publish data.
       * If error or not successful in sending, break from the for loop.
       */
    if (init_msg ==0)
    {
      if ( 0 != sl_ExtLib_mqtt_ClientSend( pubTopic, mqtt_pub_data,
                                          strlen((char*)mqtt_pub_data),
                                          //QOS0, RETAIN, SL_MQTT_CL_CMD_BLOCK) )
                                          QOS0, 0, SL_MQTT_CL_CMD_BLOCK) )
      {
        MQTT_APP_Dbg.num_message_error++;
        break;
      }
    }
    init_msg = 1;
    
    MQTT_APP_Dbg.num_message++;
    
      /* Control the Publish interval.*/
      //Semaphore_pend(mqttApp_sem, 300000); 
      // 60 seconds
      Semaphore_pend(mqttApp_sem, CLOCK_SECOND * MQTT_APP_DATA_POLL_PERIOD); 


#if defined(MQTT_MSG_FORMAT_TI_GUI)
      ti_web_node_update_topic(pubTopic);
      ti_web_node_update_msg(mqtt_pub_data);
#elif defined ( MQTT_MSG_FORMAT_IBM)
      
      ti_web_ibm_topic(pubTopic);
      ti_web_ibm_message(mqtt_pub_data);

#else
#error "please specify the message format"
#endif      
      if ( 0 != sl_ExtLib_mqtt_ClientSend( pubTopic, mqtt_pub_data,
                                          strlen((char*)mqtt_pub_data),
                                          //QOS0, RETAIN, SL_MQTT_CL_CMD_BLOCK) )
                                          QOS0, 0, SL_MQTT_CL_CMD_BLOCK) )
      {
        MQTT_APP_Dbg.num_message_error++;
        break;
      }

      MQTT_APP_Dbg.num_message++;
      
      /* Control the Publish interval.*/
      //Semaphore_pend(mqttApp_sem, 300000); 
      // 60 seconds
      Semaphore_pend(mqttApp_sem, CLOCK_SECOND * MQTT_APP_DATA_POLL_PERIOD); 
      
    }

    /* Delay the reconnection to the server.*/
    // wait 20 second
    //Semaphore_pend(mqttApp_sem, 10000); 
    Semaphore_pend(mqttApp_sem,CLOCK_SECOND * 20); 

  } while(1);
} /* mqttApp_task() */

#if 0  
/**
 * Manager task entry point.
 * 
 * @param a0 ** not used **
 * @param a1 ** not used **
 * 
 * @returns none.
 */
Void mgr_task(UArg a0, UArg a1)
{
  extern void nodeMgrInit(void);
  extern zipU32_t nodeMgrProcess(void);

  /* Disallow shutting down JTAG, VIMS, SYSBUS during idle state
   * since TIMAC requires SYSBUS during idle. */
  Power_setConstraint(Power_IDLE_PD_DISALLOW);
#if 0
  /* TODO: Remove the following temporary code to disable power management */
  Power_setConstraint(Power_PD_DISALLOW);
  Power_setConstraint(Power_SD_DISALLOW);
  Power_setConstraint(Power_SB_DISALLOW);
#endif

#ifdef FEATURE_MAC_SECURITY
  AES_Params AESparams;
  extern AES_Handle AEShandle;
  AES_Params_init(&AESparams);

  AEShandle = AES_open(Board_AES, &AESparams);
  if (!AEShandle)
  {
    Task_exit();
  }
#endif

  /* Create a semaphore for thread synchronization */
  {
    Semaphore_Params params;
    Semaphore_Params_init(&params);
    params.mode = Semaphore_Mode_BINARY;
    mgr_sem = Semaphore_create(0, &params, NULL);
  }

  /* Kickoff stack */
  if (CLIPBin_main(5))
  {
    /* Stack starts up failed */
    zipAbort(NULL);
  }

#if 0  
  /* Program EUI-64. Note that EUI-64 must be programmed before invoking
   * node manager. Otherwise, stack might not be ready to be called with
   * security key programming.
   */
  {
    extern uint8_t addr64[];
    zipLoWPANSetEUI64(addr64);
  }
#else
  setExtAddr();
#endif //if 0  

  /* Kick off node manager */
  nodeMgrInit();

  // and away we go!
  for (;;)
  {
    zipU32_t sleepDur = 0xfffffffful;

    {
      zipU32_t tmp = connmgrTaskEntry();
      if (tmp < sleepDur)
      {
        sleepDur = tmp;
      }
    }

    {
      zipU32_t tmp = nodeMgrProcess();
      if (tmp < sleepDur)
      {
        sleepDur = tmp;
      }
    }

    if (sleepDur == 0)
    {
      continue;
    }

    if (sleepDur == 0xFFFFFFFFul)
    {
      sleepDur = BIOS_WAIT_FOREVER;
    }
    else
    {
      uint_fast64_t intermediate = sleepDur;
      intermediate *= 1000;
      intermediate /= Clock_tickPeriod;
      if (intermediate >= ((uint_fast64_t) 1 << (sizeof(UInt)*8 - 1)))
      {
        intermediate = ((uint_fast64_t) 1 << (sizeof(UInt)*8-1)) - 1;
      }
      sleepDur = (UInt) intermediate;
    }

    /* Block till an event happens */
    Semaphore_pend(mgr_sem, sleepDur);

    /* TODO: Investigate to replace zipTick() mechanism with dynamic clock */
    zipTick();
  }
}

/**
 * Resumes node manager task.
 */
void nodeMgrTaskResume( void )
{
  Semaphore_post(mgr_sem);
}

/**
 * Resumes connectivity manager task
 */
void connmgrTaskResume(void)
{
  Semaphore_post(mgr_sem);
}

/* demo node manager callback functions */
void demoNodeMgrBootupCback(void)
{
  Semaphore_post( mqttApp_sem );
}

/**
 *
 * Demo node manager (connectivity manager) callback function.
 * 
 * @param status ** not used **
 */
void demoNodeMgrConnMgrCback(unsigned char status)
{
  (void) status;
}
#endif



