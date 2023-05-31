//#############################################################################
//
//! \file simple-udp-sensor.c
//!
//! \brief Creation of udp packet with sensor data
//
// (C) Copyright 2015, Texas Instruments, Inc.
//#############################################################################

/*******************************************************************************
*                                            INCLUDES
*******************************************************************************/

#include "simple-udp-sensors.h"
#include "Board.h"

#include "sys/clock.h"
#include <ti/drivers/I2C.h>

#include "tsch-acm.h"
#include "ulpsmac.h"
#include "mac_config.h"

#include "SensorBatMon.h"

#if SENSORTAG || I3_MOTE
#include "SensorI2C.h"
#include "SensorTmp007.h"
#include "SensorHDC1000.h"
#include "SensorHDC2010.h"
#include "SensorOpt3001.h"
#include "SensorBmp280.h"
#include "SensorMpu9250.h"
#include "SensorLIS2HH12.h"
#include "SensorLIS2DW12.h"
#endif

/*******************************************************************************
*                                            DEFINES
*******************************************************************************/

/*******************************************************************************
*                                            LOCAL VARIABLES
*******************************************************************************/

/***************************************************************************//**
 * @fn          read_sensors
 *
 * @brief       This will read sensor data and populate the message
 *
 * @param[in]   msg - SampleMsg_t pointer to the message that is being populated
 *
 * @return      none
 *
 ******************************************************************************/
void read_sensors(SampleMsg_t *msg)
{
  bool success;
  SENSOR_BatData_t cc26xxBat;
  
  success = SensorBatmon_oneShotRead(&cc26xxBat);
  msg->sensors[BATTERY]  = cc26xxBat.bat;
  
#if SENSORTAG 
  SENSOR_InfraredTempData_t tempData;
  SENSOR_HumData_t humData;
  SENSOR_LightData_t lightData;
  SENSOR_PressData_t pressData;
  SENSOR_MotionData_t motionData;

  success = SensorTmp007_oneShotRead(&tempData);
  msg->sensors[TEMPERATURE] = tempData.temp;

  success = SensorHdc1000_oneShotRead(&humData);
  msg->sensors[HUMIDITY]  = humData.hum;
 
  success = SensorOpt3001_oneShotRead(&lightData);
  msg->sensors[LIGHT]   = lightData.lux;

  success = SensorBmp280_oneShotRead(&pressData);
  msg->sensors[PRESSURE]  = pressData.press;
  
  success = SensorMpu9250_oneShotRead(&motionData);
  msg->sensors[MOTIONX]  = motionData.accelx ;
  msg->sensors[MOTIONY]  = motionData.accely;
  msg->sensors[MOTIONZ]  = motionData.accelz;
#elif I3_MOTE  
  SENSOR_HumData_t humData;
  SENSOR_LightData_t lightData;
  SENSOR_PressData_t pressData;
  SENSOR_MotionData_t motionData;
  
  success = SensorBmp280_oneShotRead(&pressData);
  msg->sensors[PRESSURE]  = pressData.press;
  
    success = SensorOpt3001_oneShotRead(&lightData);
  msg->sensors[LIGHT]   = lightData.lux;
#if CC13XX_DEVICES
  success = SensorHdc2010_oneShotRead(&humData);
  msg->sensors[HUMIDITY]  = humData.hum;
  
  success = SensorLis2DW12_oneShotRead(&motionData);
  msg->sensors[MOTIONX]  = motionData.accelx ;
  msg->sensors[MOTIONY]  = motionData.accely;
  msg->sensors[MOTIONZ]  = motionData.accelz;
  
#elif CC26XX_DEVICES
  success = SensorHdc1000_oneShotRead(&humData);
  msg->sensors[HUMIDITY]  = humData.hum;
  
  success = SensorLis2hh12_oneShotRead(&motionData);
  msg->sensors[MOTIONX]  = motionData.accelx ;
  msg->sensors[MOTIONY]  = motionData.accely;
  msg->sensors[MOTIONZ]  = motionData.accelz;
#endif
#else
  msg->sensors[BATTERY]  = 300;
  msg->sensors[TEMPERATURE] = 25;
  msg->sensors[HUMIDITY]  = 47;
  msg->sensors[LIGHT]   = 260;
  msg->sensors[PRESSURE]  = 999;
  msg->sensors[MOTIONX]  = 0 ;
  msg->sensors[MOTIONY]  = 0;
  msg->sensors[MOTIONZ]  = 0;
#endif
}

/***************************************************************************//**
 * @fn          sample_msg_create
 *
 * @brief       This will populate the msg with additional useful data
 *
 * @param[in]   pmsg - pointer to the SampleMsg_t
 *
 * @return      none
 *
 ******************************************************************************/
void sample_msg_create(SampleMsg_t *pmsg)
{
    static uint16_t seqno=0;
    uint16_t len;
    uint32_t ts;

    len = sizeof(SampleMsg_t);

    memset(pmsg, 0, len);
    seqno++;
    if(seqno == 0)
    {
        /* Wrap to 128 to identify restarts */
        seqno = 128;
    }

    pmsg->seqno = seqno;
    pmsg->size =  sizeof(SampleMsg_t) / sizeof(uint8_t);
    pmsg->lastTxRxChannel = ((link_hopping_info[2].channel)<<8) +
                              link_hopping_info[3].channel;
    ts = clock_seconds();
    memcpy(pmsg->txTimestamp,&ts,4);

    memset( pmsg->reserve,0x0,sizeof(pmsg->reserve));

    read_sensors(pmsg);
}

/*---------------------------------------------------------------------------*/
