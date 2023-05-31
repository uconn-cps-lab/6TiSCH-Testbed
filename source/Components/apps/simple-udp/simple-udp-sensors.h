#ifndef SIMPLE_UDP_SENSOR_H
#define SIMPLE_UDP_SENSOR_H

// if you want to fragmentation in sample UDP, please use SIZE 120
#define SIZE 18
//#define SIZE 120
enum
{
  BATTERY,
  TEMPERATURE,
  HUMIDITY,
  LIGHT,
  PRESSURE,
  MOTIONX,
  MOTIONY,
  MOTIONZ,
  NUM_SENSORS,
};

typedef struct
{
    uint16_t seqno;
    uint16_t size;
    uint16_t lastTxRxChannel;
    uint16_t txTimestamp[2];
    uint16_t sensors[NUM_SENSORS];
    uint16_t reserve[SIZE];
} SampleMsg_t;

void sample_msg_create(SampleMsg_t *pmsg);

#endif
