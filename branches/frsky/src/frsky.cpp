/*
 *
 * gruvin9x Author Bryan J.Rentoul (Gruvin) <gruvin@gmail.com>
 *
 * frsky.cpp original author - Philip Moss Adapted from jeti.cpp code by Karl
 * Szmutny <shadow@privy.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include "frsky.h"
#include "gruvin9x.h"

uint8_t frskyBuffer[19]; // 9 bytes (full packet), worst case 18 bytes with byte-stuffing (+1)
uint8_t FrskyBufferReady = 0;

uint8_t frskyA1;
uint8_t frskyA2;
uint8_t frskyRSSI; 
uint8_t frskyStreaming = 0;
struct struct_frskyAlarm frskyAlarms[4];

/*
   Called from somewhere in the main loop or a low prioirty interrupt
   routine perhaps. This funtcion processes Fr-Sky telemetry data packets
   assembled byt he USART0_RX_vect) ISR function (below) and stores
   extracted data in global variables for use by other parts of the program.

   Packets can be any of the following:

    - A1/A2/RSSI telemtry data
    - Alarm level/mode/threshold settings for Ch1A, Ch1B, Ch2A, Ch2B
    - User Data packets

   User Data packets are not yet implementedi (they are simply ignored), 
   but will likely one day contain the likes of GPS long/lat/alt/speed, 
   AoA, airspeed, etc.
*/
void processFrskyPacket(uint8_t *packet)
{
  static uint8_t lastRSSI = 0; // for running average calculation

  // What type of packet?
  switch (packet[0])
  {
    case 0xf9:
    case 0xfa:
    case 0xfb:
    case 0xfc:
      frskyAlarms[(packet[0]-0xf9)].level = packet[1];
      frskyAlarms[(packet[0]-0xf9)].greater = packet[2];
      frskyAlarms[(packet[0]-0xf9)].value = packet[3];
      break;
    case 0xfe: // A1/A2/RSSI values
      frskyA1 = packet[1];
      frskyA2 = packet[2];

      if (lastRSSI == 0)
        frskyRSSI = packet[3];
      else
        frskyRSSI = ((uint16_t)packet[3] + ((uint16_t)lastRSSI * 15)) >> 4; // >>4 = divide by 16 the fast way
      lastRSSI = frskyRSSI;
      break;

    case 0xfd: // User Data packet not yet implemented
      break;

  }

  FrskyBufferReady = 0;
  frskyStreaming = FRSKY_TIMEOUT10ms; // reset counter only if valid frsky packets are being detected
}

// Receive buffer state machine state defs
#define frskyDataIdle    0
#define frskyDataStart   1
#define frskyDataInFrame 2
#define frskyDataXOR     3
/*
   Receive serial (RS-232) characters, detecting and storing each Fr-Sky 
   0x7e-framed packet as it arrives.  When a complete packet has been 
   received, process its data into storage variables.  NOTE: This is an 
   interrupt routine and should not get too lengthy. 
   
   Note however that, if a constant stream of telemetry data is coming in 
   at 9600 baud -- 960 bytes a second -- then complete FrSky packets will be
   assembled in only just over 10ms. The menus/cpp function only gets called
   once every 10ms -- so there exists a probability that longer byte-stuffed
   frsky packets could get skipped if packets are processed from there. Thus, 
   packet processing should be called from within the perMain() function.
*/
ISR(USART0_RX_vect)
{
  uint8_t stat;
  uint8_t data;
  
  static uint8_t numPktBytes = 0;
  static uint8_t dataState = frskyDataIdle;

  stat = UCSR0A; // USART control and Status Register 0 A

    /*
              bit      7      6      5      4      3      2      1      0
                      RxC0  TxC0  UDRE0    FE0   DOR0   UPE0   U2X0  MPCM0
             
              RxC0:   Receive complete
              TXC0:   Transmit Complete
              UDRE0:  USART Data Register Empty
              FE0:    Frame Error
              DOR0:   Data OverRun
              UPE0:   USART Parity Error
              U2X0:   Double Tx Speed
              PCM0:   MultiProcessor Comms Mode
    */
    // rh = UCSR0B; //USART control and Status Register 0 B

    /*
              bit      7      6      5      4      3      2      1      0
                   RXCIE0 TxCIE0 UDRIE0  RXEN0  TXEN0 UCSZ02  RXB80  TXB80
             
              RxCIE0:   Receive Complete int enable
              TXCIE0:   Transmit Complete int enable
              UDRIE0:   USART Data Register Empty int enable
              RXEN0:    Rx Enable
              TXEN0:    Tx Enable
              UCSZ02:   Character Size bit 2
              RXB80:    Rx data bit 8
              TXB80:    Tx data bit 8
    */

  data = UDR0; // USART data register 0

  if (stat & ((1 << FE0) | (1 << DOR0) | (1 << UPE0)))
  { // discard buffer and start fresh on any comms error
    FrskyBufferReady = 0;
    numPktBytes = 0;
  } 
  else
  {
    if (FrskyBufferReady == 0) // can't get more data if the buffer hasn't been cleared
    {
      switch (dataState) 
      {
        case frskyDataStart:
          if (data == 0x7e) break; // Remain in userDataStart if possible 0x7e,0x7e doublet found. 

          if (numPktBytes < 19)
            frskyBuffer[numPktBytes++] = data;
          dataState = frskyDataInFrame;
          break;

        case frskyDataInFrame:
          if (data == 0x7d) 
          { 
              dataState = frskyDataXOR; // XOR next byte
              break; 
          }
          if (data == 0x7e) // end of frame detected
          {
            FrskyBufferReady = 1;
            dataState = frskyDataIdle;
            break;
          }
          frskyBuffer[numPktBytes++] = data;
          break;

        case frskyDataXOR:
          if (numPktBytes < 19)
            frskyBuffer[numPktBytes++] = data ^ 0x20;
          dataState = frskyDataInFrame;
          break;

        case frskyDataIdle:
          if (data == 0x7e)
          {
            numPktBytes = 0;
            dataState = frskyDataStart;
          }
          break;

      } // switch
    } // if (FrskyBufferReady == 0)
  }
}

void FRSKY_Init(void)
{
  DDRE &= ~(1 << DDE0);    // set RXD0 pin as input
  PORTE &= ~(1 << PORTE0); // disable pullup on RXD0 pin

#undef BAUD
#define BAUD 9600
#include <util/setbaud.h>

  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;
  UCSR0A &= ~(1 << U2X0); // disable double speed operation

  // set 8 N1
  UCSR0B = 0 | (0 << RXCIE0) | (0 << TXCIE0) | (0 << UDRIE0) | (0 << RXEN0) | (0 << TXEN0) | (0 << UCSZ02);
  UCSR0C = 0 | (1 << UCSZ01) | (1 << UCSZ00);


  //flush receive buffer
  while (UCSR0A & (1 << RXC0))
    UDR0;

  // clear alarm variables
  memset(frskyAlarms, 0, sizeof(frskyAlarms));
}


void FRSKY_DisableTXD(void)
{
  UCSR0B &= ~(1 << TXEN0); // disable TX
}


void FRSKY_EnableTXD(void)
{

  UCSR0B |= (1 << TXEN0); // enable TX
}


void FRSKY_DisableRXD(void)
{
  UCSR0B &= ~(1 << RXEN0);  // disable RX
  UCSR0B &= ~(1 << RXCIE0); // disable Interrupt
}


void FRSKY_EnableRXD(void)
{

  UCSR0B |= (1 << RXEN0);  // enable RX
  UCSR0B |= (1 << RXCIE0); // enable Interrupt
}
