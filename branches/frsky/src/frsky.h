/*
 * Author - Philip Moss
 * Adapted from jeti.h code by Karl Szmutny <shadow@privy.de>
 * 
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

#ifndef frsky_h
#define frsky_h


#include "gruvin9x.h"

// .20 seconds
#define FRSKY_TIMEOUT10ms 20

extern uint8_t frskyBuffer[19]; // 9 bytes (full packet), worst case 18 bytes with byte-stuffing (+1)
extern uint8_t FrskyBufferReady;

// Global Fr-Sky telemetry data variables
extern uint8_t frskyA1;
extern uint8_t frskyA2;
extern uint8_t frskyRSSI; // RSSI (virtual 10 slot) running average
struct struct_frskyAlarm {
  uint8_t level;    // The alarm''s 'urgency' level. 0=disabled, 1=yellow, 2=orange, 3=red
  uint8_t greater;  // 1 = 'if greater than'. 0 = 'if less than'
  uint8_t value;    // The threshold above or below which the alarm will sound
};
extern struct struct_frskyAlarm frskyAlarms[4];
extern uint8_t frskyStreaming; // >0 (true) == data is streaming in. 0 = nodata detected for some time


void processFrskyPacket(uint8_t *packet);

void FRSKY_Init(void);
void FRSKY_DisableTXD (void);
void FRSKY_EnableTXD (void);
void FRSKY_DisableRXD (void);
void FRSKY_EnableRXD (void);


#endif

