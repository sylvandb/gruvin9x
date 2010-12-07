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

#include "gruvin9x.h"
#include "frsky.h"

uint8_t frskyBuffer[19]; // 9 bytes (full packet), worst case 18 bytes with byte-stuffing (+1)
uint8_t FrskyBufferReady = 0;

uint8_t frskyA1;
uint8_t frskyA2;
uint8_t frskyRSSI; 
uint8_t frskyStreaming = 0;
struct FrskyAlarm frskyAlarms[4];

/*
   Write out an alarm settings programming packet for given alarm slot (0-4)
*/
void frskyWriteAlarm(uint8_t slot)
{
  return; // do nothing yet
}

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

/*
   FRSKY MENUS BEGIN HERE
   FRSKY MENUS BEGIN HERE
   FRSKY MENUS BEGIN HERE
*/

uint8_t hex2dec(uint16_t number, uint8_t multiplier)
{
	uint16_t value = 0;
	
	switch (multiplier)
	{
		case 1:
			value = number%100;
			value = value%10;
			break;
			
		case 10:
			value = number%100;
			value = value /10;
			break;
			
		case 100:
			value = number/100;
			break;
			
		default:
			
			break;
	}
	
	value += 48; // convert to ASCII digit
	return value;
	
}

MenuFuncP_PROGMEM APM menuTabFrsky[] = {
  menuProcFrsky,
  menuProcFrskyAlarms
};

// FRSKY menu
void menuProcFrsky(uint8_t event)
{
  static uint8_t blinkCount = 0; // let's blink the data on and off if there's no data stream
  static MState2 mstate2;
  TITLE("FRSKY");
  MSTATE_CHECK_V(1,menuTabFrsky,1); // curr,menuTab,numRows(including the page counter [1/2] etc)
  // int8_t  sub    = mstate2.m_posVert; // alias sub to m_posvert. Clever Mr. TH :-D

  if (frskyStreaming == 0)
  {
    lcd_putsAtt(30, 0, PSTR("NO"), DBLSIZE);
    lcd_putsAtt(62, 0, PSTR("DATA"), DBLSIZE);
  }

  // Data labels
  lcd_puts_P(2*FW, 2*FH, PSTR("A1:"));
  lcd_puts_P(11*FW, 2*FH, PSTR("A2:"));
  lcd_puts_P(2*FW, 3*FH, PSTR("Rx RSSI:   dB"));
  lcd_puts_P(2*FW, 4*FH, PSTR("Rx Batt:"));
  lcd_putc(11*FW,4*FH, '.');

  // Rx batt voltage bar frame
  lcd_puts_P(0, FH*6, PSTR("4.2V"));
  lcd_vline(3, 58, 6);
  lcd_vline(64, 58, 6);
  lcd_puts_P(64-(FW*2), FH*6, PSTR("5.4V"));
  lcd_vline(125, 58, 6);
  lcd_puts_P(128-(FW*4), FH*6, PSTR("6.6V"));

  // blinking if no data stream
  if (frskyStreaming || ((blinkCount++ % 128) > 25)) // 50:255 off:on ratio at double speed
  {

    // A1 raw value, zero padded
    lcd_putc(5*FW,2*FH,hex2dec(frskyA1, 100));
    lcd_putc(6*FW,2*FH,hex2dec(frskyA1, 10));
    lcd_putc(7*FW,2*FH,hex2dec(frskyA1, 1));

    // A2 raw value, zero padded
    lcd_putc(14*FW,2*FH,hex2dec(frskyA2, 100));
    lcd_putc(15*FW,2*FH,hex2dec(frskyA2, 10));
    lcd_putc(16*FW,2*FH,hex2dec(frskyA2, 1));

    // RSSI value 
    lcd_putc(10*FW,3*FH,(frskyRSSI > 99) ? hex2dec(frskyRSSI, 100) : ' ');
    lcd_putc(11*FW,3*FH,(frskyRSSI > 9) ? hex2dec(frskyRSSI, 10) : ' ');
    lcd_putc(12*FW,3*FH,hex2dec(frskyRSSI, 1));

    // Rx Batt: volts (255 = 6.6V) -10 to calibrate XXX FIX THIS
    uint16_t centaVolts = (frskyA1 > 0) ? (660 * (uint32_t)(frskyA1) / 255) - 10 : 0;
    lcd_putc(10*FW,4*FH, hex2dec(centaVolts, 100));
    lcd_putc(12*FW,4*FH, hex2dec(centaVolts, 10));
    lcd_putc(13*FW,4*FH, hex2dec(centaVolts, 1));
    
    // draw the actual voltage bar
    if (centaVolts > 419)
    {
      uint8_t vbarLen = (centaVolts - 420) >> 1;
      for (uint8_t i = 59; i < 63; i++) // Bar 4 pixels thick (high)
      lcd_hline(4, i, vbarLen);
    }
  } // if data streaming / blink choice
    
}

// FRSKY Alarms menu
void menuProcFrskyAlarms(uint8_t event)
{
  static MState2 mstate2;
  TITLE("FRSKY ALARMS");
  MSTATE_TAB = {1,4}; // horizontal column count for MSTAT_CHECK_VxH
  MSTATE_CHECK_VxH(2,menuTabFrsky,5); // current page=2, 5 rows of settings including page counter top/right

  int8_t  sub    = mstate2.m_posVert - 1; // vertical position (1 = page counter, top/right)
  uint8_t subSub = mstate2.m_posHorz + 1; // horizontal position

  switch(event)
  {
    case EVT_ENTRY:
      s_editMode = false;
      break;
    case EVT_KEY_FIRST(KEY_MENU):
      if(sub>=0) s_editMode = !s_editMode;
      break;
    case EVT_KEY_FIRST(KEY_EXIT):
      if(s_editMode)
      {
        s_editMode = false;
        killEvents(event);
      }
      break;
  }

  lcd_puts_P(0, 2*FH,PSTR("Slot Level > < Value"));
  for(uint8_t i=0; i<4; i++) // 4 alarm slots
  {
    uint8_t y=(i+3)*FH;
    FrskyAlarm *ad = &frskyAlarms[i];
    for(uint8_t j=0; j<4;j++) // 4 settings each slot
    {
      uint8_t attr = ((sub==i && subSub==j) ? (s_editMode ? BLINK : INVERS) : 0);
      switch(j)
      {
        case 0:
          lcd_putsnAtt(0,y,PSTR("A1a""A1b""A2a""A2b")+i*3,3, (sub==i) ? INVERS : 0);
          break;
        case 1:
          lcd_putsnAtt(5*FW,y,PSTR("---""Yel""Org""Red")+ad->level*3,3, attr);
          if(attr && (s_editMode || p1valdiff)) // p1valdiff is analog user input via Pot 1 (rear/left)
            if(checkIncDecGen2( event, &ad->level, 0, 3, _FL_UNSIGNED8)) frskyWriteAlarm(i);
          break;
        case 2:
          lcd_putsnAtt(11*FW,y,PSTR("LT<""GT>")+ad->greater*3,3, attr);
          if(attr && s_editMode)
            if(checkIncDecGen2( event, &ad->greater, 0, 1, _FL_UNSIGNED8)) frskyWriteAlarm(i);
          break;
        case 3:
          lcd_outdezAtt(19*FW,y, ad->value, attr);
          if(attr && (s_editMode || p1valdiff))
            if(checkIncDecGen2( event, &ad->value, 0, 255, _FL_UNSIGNED8)) frskyWriteAlarm(i);
          break;
      }
    }
  }
}

