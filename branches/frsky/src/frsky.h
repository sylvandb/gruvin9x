/*
 * Authors (alphabetical order)
 * - Bertrand Songis <bsongis@gmail.com>
 * - Bryan J. Rentoul (Gruvin) <gruvin@gmail.com>
 *
 * Original contributors
 * - Philip Moss Adapted first frsky functions from jeti.cpp code by 
 * - Karl Szmutny <shadow@privy.de> 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef FRSKY_H
#define FRSKY_H

#include <inttypes.h>

// .20 seconds
#define FRSKY_TIMEOUT10ms 20

#define FRSKY_RX_BUFFER_SIZE 80
#define FRSKY_RX_PACKET_SIZE 19
#define FRSKY_TX_PACKET_SIZE 19

enum AlarmLevel {
  alarm_off = 0,
  alarm_yellow = 1,
  alarm_orange = 2,
  alarm_red = 3
};

#define ALARM_GREATER(channel, alarm) ((g_model.frsky.channels[channel].alarms_greater >> alarm) & 1)
#define ALARM_LEVEL(channel, alarm) ((g_model.frsky.channels[channel].alarms_level >> (2*alarm)) & 3)

struct FrskyData {
  uint8_t value;
  uint8_t min;
  uint8_t max;
  void set(uint8_t value);
};

// Global Fr-Sky telemetry data variables
extern uint8_t frskyStreaming; // >0 (true) == data is streaming in. 0 = nodata detected for some time
extern uint8_t FrskyAlarmSendState;
extern FrskyData frskyTelemetry[2];
extern FrskyData frskyRSSI[2];
extern uint8_t frskyRxBufferIn;
extern uint8_t frskyRxBufferOut;
extern uint8_t frskyUserDataIn;
extern uint8_t frskyUserDataOut;


void FRSKY_Init(void);
void FRSKY10mspoll(void);

inline void FRSKY_setModelAlarms(void)
{
  FrskyAlarmSendState = 4 ;
}

bool FRSKY_alarmRaised(uint8_t idx);

void resetTelemetry();


void frskyParseRxData();

uint8_t frskyGetUserData(char *buffer, uint8_t bufSize);

#define TELEM_PKT_SIZE 3
extern char telemPacket[TELEM_PKT_SIZE];
extern void parseTelemHubData();

#endif

