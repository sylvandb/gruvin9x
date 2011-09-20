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

#if defined (PCBV3) /* ATmega64A just doesn't have enough RAM */
#define FRSKY_RX_BUFFER_SIZE 64
#else
#define FRSKY_RX_BUFFER_SIZE 20
#endif

#define FRSKY_RX_PACKET_SIZE 19
#define FRSKY_TX_PACKET_SIZE 12

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

extern uint16_t frskyComputeVolts(uint8_t rawADC, uint16_t ratio, uint8_t decimals=1); 
extern void frskyPutAValue(uint8_t x, uint8_t y, uint8_t channel, uint8_t value, uint8_t mode = 0);

void FRSKY_Init(void);
void FRSKY10mspoll(void);

inline void FRSKY_setModelAlarms(void)
{
  FrskyAlarmSendState = 4;
}

bool FRSKY_alarmRaised(uint8_t idx);

void resetTelemetry();


void frskyParseRxData();

#ifdef DISPLAY_USER_DATA
uint8_t frskyGetUserData(char *buffer, uint8_t bufSize);
#endif

#define TELEM_PKT_SIZE 3
extern uint8_t telemPacket[TELEM_PKT_SIZE];
extern void parseTelemHubData();

extern uint16_t gTelem_GPSaltitude[2];   // before,after decimal
extern uint16_t gTelem_GPSspeed[2];      // before,after decimal
extern uint16_t gTelem_GPSlongitude[2];  // before,after decimal
extern uint8_t  gTelem_GPSlongitudeEW;   // East West
extern uint16_t gTelem_GPSlatitude[2];   // before,after decimal
extern uint8_t  gTelem_GPSlatitudeNS;    // North/South
extern uint16_t gTelem_GPScourse[2];     // before.after (0..359 deg. -- unknown precision)
extern uint8_t  gTelem_GPSyear;
extern uint8_t  gTelem_GPSmonth;
extern uint8_t  gTelem_GPSday;
extern uint8_t  gTelem_GPShour;
extern uint8_t  gTelem_GPSmin;
extern uint8_t  gTelem_GPSsec;
extern int16_t  gTelem_AccelX;           // 1/256th gram (-8g ~ +8g)
extern int16_t  gTelem_AccelY;           // 1/256th gram (-8g ~ +8g)
extern int16_t  gTelem_AccelZ;           // 1/256th gram (-8g ~ +8g)
extern int16_t  gTelem_Temperature1;     // -20 .. 250 deg. celcius
extern uint16_t gTelem_RPM;              // 0..60,000 revs. per minute
extern uint8_t  gTelem_FuelLevel;        // 0, 25, 50, 75, 100 percent
extern int16_t  gTelem_Temperature2;     // -20 .. 250 deg. celcius
extern uint16_t gTelem_Volts;            // 1/500V increments (0..4.2V)
extern uint16_t gTelem_baroAltitude;     // 0..9,999 meters

#endif

