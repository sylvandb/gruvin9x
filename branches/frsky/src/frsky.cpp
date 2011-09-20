/*
 * Authors (alpahbetical order)
 * - Bertrand Songis <bsongis@gmail.com>
 * - Bryan J. Rentoul (Gruvin) <gruvin@gmail.com>
 *
 * Original contributors
 * - Philip Moss Adapted first frsky functions from jeti.cpp code by 
 * - Karl Szmutny <shadow@privy.de> 

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
#include "ff.h"
#include <stdlib.h>

// Enumerate FrSky packet codes
#define LINKPKT         0xfe
#define USRPKT          0xfd
#define A11PKT          0xfc
#define A12PKT          0xfb
#define A21PKT          0xfa
#define A22PKT          0xf9
#define ALRM_REQUEST    0xf8
#define RSSI1PKT        0xf7
#define RSSI2PKT        0xf6

#define START_STOP      0x7e
#define BYTESTUFF       0x7d
#define STUFF_MASK      0x20

uint16_t frskyComputeVolts(uint8_t rawADC, uint16_t ratio/* max cirecuit designed input voltage */, uint8_t decimals/* 1 or 2. defaults to 1 */)
{
  uint16_t val;
  val = (uint32_t)rawADC * ratio / ((decimals == 2) ? 255 : 2550); // result is naturally rounded and will in fact always be <= 16 bits
  return val;
}

/*
   Displays either a voltage or raw formatted value, given raw ADC, calibrartion ratio and optionally type, print mode and decimals
   type defaults to 0 -- volts
   mode defaults to 0 -- no special display attributes
   decimals can be either 1 or 2 and defaults to 1
  */
void frskyPutAValue(uint8_t x, uint8_t y, uint8_t channel, uint8_t value, uint8_t mode)
{
  if (g_model.frsky.channels[channel].type == 0/*volts*/)
  {
    uint16_t val = frskyComputeVolts(value, g_model.frsky.channels[channel].ratio, (mode & PREC2) ? 2 : 1);
    lcd_outdezNAtt(x, y, val, mode | (mode&PREC2 ? PREC2 : PREC1));
    lcd_putcAtt(lcd_lastPos, y, 'v', mode);
  }
  else /* assume raw */
  {
    lcd_outdezNAtt(x, y, value, mode&(~(PREC1|PREC2)), 3|LEADING0);
  }
}

// Fr-Sky Single Packet receive buffer. 9 bytes (full packet), worst case 18 bytes with byte-stuffing (+1). Ditto for tx buffer
uint8_t frskyRxPacketBuf[FRSKY_RX_PACKET_SIZE];
uint8_t frskyTxPacketBuf[FRSKY_TX_PACKET_SIZE];
uint8_t frskyTxPacketCount = 0;
uint8_t FrskyRxPacketReady = 0;
uint8_t frskyStreaming = 0;
uint8_t frskyTxISRIndex = 0;

FrskyData frskyTelemetry[2];
FrskyData frskyRSSI[2];

struct FrskyAlarm {
  uint8_t level;    // The alarm's 'urgency' level. 0=disabled, 1=yellow, 2=orange, 3=red
  uint8_t greater;  // 1 = 'if greater than'. 0 = 'if less than'
  uint8_t value;    // The threshold above or below which the alarm will sound
};
struct FrskyAlarm frskyAlarms[4];

// Fr-Sky User Data receive and user data buffers. (Circular)
#define FRSKY_BUFFER_SIZE 32
class CircularBuffer
{
public:
  uint8_t in;     // input / write index
  uint8_t out;    // output / read index
  char queue[FRSKY_BUFFER_SIZE];

  CircularBuffer() // constructor
  {
    in = 0;
    out = 0;
  }

  inline void put(uint8_t byte)
  {
    if (((in + 1) % FRSKY_BUFFER_SIZE) != out) // if queue not full
    {
      queue[in] = byte;
      in++;
      in %= FRSKY_BUFFER_SIZE;
    }
  }

  inline uint8_t get()
  {
    uint8_t data = 0;
    if (out != in) // if queue not empty
    {
      data = queue[out];
      out++;
      out %= FRSKY_BUFFER_SIZE;
    }
    return data;
  }

  inline uint8_t isFull()
  {
    return (((in + 1) % FRSKY_BUFFER_SIZE) == out);
  }

  inline uint8_t isEmpty()
  {
    return (out == in);
  }
};

CircularBuffer frskyUserData;

// inherit and extend CircularBuffer to make parsing data more efficient
class FrskyRxParser : public CircularBuffer {

  uint8_t numPktBytes;
  uint8_t dataState;

public:
// packet parser machine states
#define frskyDataIdle    0
#define frskyDataStart   1
#define frskyDataInFrame 2
#define frskyDataXOR     3
  FrskyRxParser() : CircularBuffer() 
  {
    numPktBytes = 0;
    dataState = frskyDataIdle;
  };

  /*
    Fr-Sky telemtry packet parser, called from perMain() for each incoming serial byte
    (State machine)
    When a full packet has been collected, it is processed immediately from here with 
    a call to processFrskyPacket()
  */
  void parseData()
  {
    while (out != in) // if queue not empty
    {
      uint8_t data = get(); // get() is inline to save stack etc

      if (FrskyRxPacketReady == 0) // can't get more data if the buffer hasn't been cleared
      {
        switch (dataState) 
        {
          case frskyDataStart:
            if (data == START_STOP) break; // Remain in userDataStart if possible 0x7e,0x7e doublet found.

            frskyRxPacketBuf[numPktBytes++] = data;
            dataState = frskyDataInFrame;
            break;

          case frskyDataXOR:
            data ^= STUFF_MASK;
            // drop through

          case frskyDataInFrame:
            if (dataState == frskyDataXOR)
              dataState = frskyDataInFrame;
            else if (data == BYTESTUFF)
            { 
                dataState = frskyDataXOR; // XOR next byte
                break; 
            }

            if (data == START_STOP) // end of frame detected
            {
              // Process Fr-Sky Packet(uint8_t *frskyRxPacketBuf)

              switch (frskyRxPacketBuf[0]) // What type of frskyRxPacketBuf?
              {
                case A22PKT:
                case A21PKT:
                case A12PKT:
                case A11PKT:
                  {
                    struct FrskyAlarm *alarmptr ;
                    alarmptr = &frskyAlarms[(frskyRxPacketBuf[0]-A22PKT)] ;
                    alarmptr->value = frskyRxPacketBuf[1];
                    alarmptr->greater = frskyRxPacketBuf[2] & 0x01;
                    alarmptr->level = frskyRxPacketBuf[3] & 0x03;
                  }
                  break;

                case LINKPKT: // A1/A2/RSSI values
                  frskyTelemetry[0].set(frskyRxPacketBuf[1]);
                  frskyTelemetry[1].set(frskyRxPacketBuf[2]);
                  frskyRSSI[0].set(frskyRxPacketBuf[3]);
                  frskyRSSI[1].set(frskyRxPacketBuf[4] / 2);
                  break;

                case USRPKT: // User Data frskyRxPacketBuf
                  uint8_t numBytes = frskyRxPacketBuf[1] & 0x07; // sanitize in case of data corruption leading to buffer overflow
                  for(uint8_t i=0; (i < numBytes) && (!frskyUserData.isFull()); i++)
                      frskyUserData.put(frskyRxPacketBuf[3+i]);
                  break;
              }

              FrskyRxPacketReady = 0;
              frskyStreaming = FRSKY_TIMEOUT10ms; // reset counter only if valid frsky frskyRxPacketBufs are being detected

              dataState = frskyDataIdle;
              break;
            }
            else // not START_STOP
            {
              if (numPktBytes < FRSKY_RX_PACKET_SIZE)
                frskyRxPacketBuf[numPktBytes++] = data;
            }
            break;

          case frskyDataIdle:
            if (data == START_STOP)
            {
              numPktBytes = 0;
              dataState = frskyDataStart;
            }
            break;

        } // switch
      } // if (FrskyRxPacketReady == 0)
    } // while
  }; // parseData method
};
FrskyRxParser frskyRxParser;

#ifdef DISPLAY_USER_DATA
/*
  Copies all available bytes (up to max bufsize) from frskyUserData circular 
  buffer into supplied *buffer. Returns number of bytes copied (or zero)
*/
uint8_t frskyGetUserData(char *buffer, uint8_t bufSize)
{
  uint8_t i = 0;
  while (!frskyUserData.isEmpty())
  {
    buffer[i] = frskyUserData.get();
    i++;
  }
  return i;
}
#endif


///////////////////////////////////////
/// Fr-Sky Telemetry Hub Processing ///
///////////////////////////////////////

// Globbal vars to hold telemetry data for logging and display

uint16_t gTelem_GPSaltitude[2];   // before.after
uint16_t gTelem_GPSspeed[2];      // before.after
uint16_t gTelem_GPSlongitude[2];  // before.after
uint8_t  gTelem_GPSlongitudeEW;   // East West
uint16_t gTelem_GPSlatitude[2];   // before.after
uint8_t  gTelem_GPSlatitudeNS;    // North/South
uint16_t gTelem_GPScourse[2];     // before.after (0..359.99 deg. -- seemingly 2-decimal precision)
uint8_t  gTelem_GPSyear;
uint8_t  gTelem_GPSmonth;
uint8_t  gTelem_GPSday;
uint8_t  gTelem_GPShour;
uint8_t  gTelem_GPSmin;
uint8_t  gTelem_GPSsec;
int16_t  gTelem_AccelX;           // 1/256th gram (-8g ~ +8g)
int16_t  gTelem_AccelY;           // 1/256th gram (-8g ~ +8g)
int16_t  gTelem_AccelZ;           // 1/256th gram (-8g ~ +8g)
int16_t  gTelem_Temperature1;     // -20 .. 250 deg. celcius
uint16_t gTelem_RPM;              // 0..60,000 revs. per minute
uint8_t  gTelem_FuelLevel;        // 0, 25, 50, 75, 100 percent
int16_t  gTelem_Temperature2;     // -20 .. 250 deg. celcius
uint16_t gTelem_Volts;            // 1/500V increments (0..4.2V)
uint16_t gTelem_baroAltitude;     // 0..9,999 meters

uint8_t telemPacket[TELEM_PKT_SIZE];
inline void processTelemPacket(void)
{
  switch (telemPacket[0])
  { 
    case 0x02:    // Temperature 1 (deg. C ~ -20..250)
      gTelem_Temperature1 = telemPacket[1] | (telemPacket[2] << 8);
      break;

    case 0x03:    // RPM (0..60000)
      gTelem_RPM = telemPacket[1] | (telemPacket[2] << 8);
      break;

    case 0x04:    // Fuel Level (percentage 0, 25, 50, 75, 100)
      gTelem_FuelLevel = telemPacket[1];
      break;

    case 0x05:    // Temperature 2 (deg. C ~ -20..250)
      gTelem_Temperature2 = telemPacket[1] | (telemPacket[2] << 8);
      break;

    case 0x06:    // Volts (1/500v ~ 0..4.2v)
      gTelem_Volts = (telemPacket[1] | (telemPacket[2] << 8)) / 5; // PREC2
      break;

    case 0x10:    // Barometric Altitude
      gTelem_baroAltitude = telemPacket[1] | (telemPacket[2] << 8);
      break;

    // GPS altitude
    case 0x01:    // before '.'
      gTelem_GPSaltitude[0] = telemPacket[1] | (telemPacket[2] << 8);
      break;
    case 0x01+8:  // after '.'
      gTelem_GPSaltitude[1] = telemPacket[1] | (telemPacket[2] << 8);
      break;

    // GPS speed
    case 0x11:    // before '.'
      gTelem_GPSspeed[0] = telemPacket[1] | (telemPacket[2] << 8);
      break;
    case 0x11+8:  // after '.'
      gTelem_GPSspeed[1] = telemPacket[1] | (telemPacket[2] << 8);
      break;

    // GPS longitude
    case 0x12:    // before '.'
      gTelem_GPSlongitude[0] = telemPacket[1] | (telemPacket[2] << 8);
      break;
    case 0x12+8:  // after '.'
      gTelem_GPSlongitude[1] = telemPacket[1] | (telemPacket[2] << 8);
      break;
    case 0x1a+8:  // East/West
      gTelem_GPSlongitudeEW = telemPacket[1];
      break;

    // GPS latitude
    case 0x13:    // before '.'
      gTelem_GPSlatitude[0] = telemPacket[1] | (telemPacket[2] << 8);
      break;
    case 0x13+8:  // after '.'
      gTelem_GPSlatitude[1] = telemPacket[1] | (telemPacket[2] << 8);
      break;
    case 0x1b+8:  // North/South
      gTelem_GPSlatitudeNS = telemPacket[1];
      break;

    // GPS course
    case 0x14:    // before '.'
      gTelem_GPScourse[0] = telemPacket[1] | (telemPacket[2] << 8);
      break;
    case 0x14+8:  // after '.'
      gTelem_GPScourse[1] = telemPacket[1] | (telemPacket[2] << 8);
      break;

    // GPS date
    case 0x15:    // day/month
      gTelem_GPSday = telemPacket[1];
      gTelem_GPSmonth = telemPacket[2];
      break;
    case 0x16:    // year
      gTelem_GPSyear = telemPacket[1];
      break;
    case 0x17:    // hour/minute
      gTelem_GPShour = telemPacket[1];
      gTelem_GPSmin = telemPacket[2];
      break;
    case 0x18:    // second
      gTelem_GPSsec = telemPacket[1];
      break;

    // Accelerometer g-forces (leave raw / un-scaled)
    case 0x24:    // x-axis
      gTelem_AccelX = telemPacket[1] | (telemPacket[2] << 8);
      break;
    case 0x25:    // y-axis
      gTelem_AccelY = telemPacket[1] | (telemPacket[2] << 8);
      break;
    case 0x26:    // z-axis
      gTelem_AccelZ = telemPacket[1] | (telemPacket[2] << 8);
      break;
  }
}

typedef enum {
  TS_IDLE = 0,// waiting for 0x5e frame marker
  TS_START,   // reset and start receiving data
  TS_DATA,    // receive data byte
  TS_FRAME,   // frame complete. Precess it. (Not used as there's no end of packet market to swallow)
  TS_XOR      // decode stuffed byte
} TS_STATE;

void parseTelemHubData()
{
  static uint8_t telemIndex;
  static TS_STATE telemState = TS_IDLE;

  // Process all available bytes in frsky user data Buffer
  while (!frskyUserData.isEmpty())
  {
    uint8_t data = frskyUserData.get();
    
    switch (telemState)
    {
      case TS_XOR:
        data = data ^ 0x60;
        // drop through
        
      case TS_DATA:

        if (telemState == TS_XOR)
          telemState = TS_DATA;
        else if (data == 0x5d) // discard this byte and decode byte-stuff on next
        {
          telemState = TS_XOR;
          break;
        }

        if (telemIndex < TELEM_PKT_SIZE)
        {
          telemPacket[telemIndex] = data;
          telemIndex++;

          if (telemIndex == TELEM_PKT_SIZE)
          {
            // process this packet
            processTelemPacket(); // inline function just to make this code section easier to read
            telemState = TS_IDLE;
          }
        }
        break;

      case TS_START:
        if (data == 0x5e)
          break; // swallow any 0x5e doublet, which DOES happen between frames (not packets)
        
        telemPacket[0] = data;
        telemIndex = 1;
        telemState = TS_DATA;
        break;

      case TS_IDLE:
      default:
        if (data == 0x5e)
          telemState = TS_START;
    }
  }
}
  
/*
  Empties any current USART0 receiver buffer content into the Fr-Sky packet
  parser state machine. (Called from main loop.)
*/
char telemRxByteBuf[FRSKY_RX_BUFFER_SIZE];
void frskyParseRxData()
{
  UCSR0B &= ~(1 << RXCIE0); // disable USART RX interrupt

  frskyRxParser.parseData();

  UCSR0B |= (1 << RXCIE0); // enable USART RX interrupt
}

/*
   Receive USART0 data bytes, loading them into circular buffer
   'frskyRxBuffer'. Swallows incoming bytes if buffer is full.
*/
#ifndef SIMU
ISR(USART0_RX_vect)
{
  UCSR0B &= ~(1 << RXCIE0); // disable (ONLY) USART RX interrupt

  // Must read data byte regardless, to clear interrupt flag
  char data = UDR0;  // USART0 received byte register

  sei(); // un-block other interrupts. VITAL to allow TIMER1_COMPA (PPM_out) int to fire from here

  // Ignore this byte if (frame | overrun | partiy) error
  if ((UCSR0A & ((1 << FE0) | (1 << DOR0) | (1 << UPE0))) == 0)
    frskyRxParser.put(data);

  UCSR0B |= (1 << RXCIE0); // enable (ONLY) USART RX interrupt
  cli(); // As much as I hate to have this here for PPM_out's sake -- I really don't have a choice.
}

/*
   USART0 Transmit Data Register Emtpy ISR
   Used to transmit FrSky data packets
*/
ISR(USART0_UDRE_vect)
{
  if (frskyTxPacketCount > 0) {
    UDR0 = frskyTxPacketBuf[frskyTxISRIndex++];
    frskyTxPacketCount--;
  }
  else {
    UCSR0B &= ~(1 << UDRIE0); // disable UDRE0 interrupt
  }
}
#endif

/******************************************/

void frskyPushValue(uint8_t & i, uint8_t value);

void frskyTransmitBuffer()
{
  frskyTxISRIndex = 0;
  UCSR0B |= (1 << UDRIE0); // enable  UDRE0 (data register empty) interrupt
}


uint8_t FrskyAlarmSendState = 0 ; // Which channel and alarm slot (1/2) to send
                                  // Set to 4 to cause all four alarm setting to be sent to module

void FRSKY10mspoll(void) // called from drivers.cpp if FrskyAlarmSendState > 0
{

    if (frskyTxPacketCount) // we only have one buffer. If it's in use, then we can't send yet.
      return;               // drivers.cpp will have us back here in 50ms if needed.

    uint8_t sendAlarmState = FrskyAlarmSendState - 1;
    // Now send a packet
    {
      uint8_t channel = 1 - (sendAlarmState / 2);
      uint8_t alarm = 1 - (sendAlarmState % 2);

      uint8_t i = 0;
      frskyTxPacketBuf[i++] = START_STOP; // Start of packet
      frskyTxPacketBuf[i++] = (A22PKT + sendAlarmState); // fc - fb - fa - f9
      frskyPushValue(i, g_model.frsky.channels[channel].alarms_value[alarm]);
      {
        frskyTxPacketBuf[i++] = ALARM_GREATER(channel, alarm);
        frskyTxPacketBuf[i++] = ALARM_LEVEL(channel, alarm);
        frskyTxPacketBuf[i++] = 0x00 ;
        frskyTxPacketBuf[i++] = 0x00 ;
        frskyTxPacketBuf[i++] = 0x00 ;
        frskyTxPacketBuf[i++] = 0x00 ;
        frskyTxPacketBuf[i++] = 0x00 ;
        frskyTxPacketBuf[i++] = START_STOP;        // End of packet
      }
      frskyTxPacketCount = i;
      frskyTransmitBuffer(); 
    }
    FrskyAlarmSendState--;
}

bool FRSKY_alarmRaised(uint8_t idx)
{
  for (int i=0; i<2; i++) {
    if (ALARM_LEVEL(idx, i) != alarm_off) {
      if (ALARM_GREATER(idx, i)) {
        if (frskyTelemetry[idx].value > g_model.frsky.channels[idx].alarms_value[i])
          return true;
      }
      else {
        if (frskyTelemetry[idx].value < g_model.frsky.channels[idx].alarms_value[i])
          return true;
      }
    }
  }
  return false;
}

inline void FRSKY_EnableTXD(void)
{
  frskyTxPacketCount = 0;
  UCSR0B |= (1 << TXEN0) | (1 << UDRIE0); // enable TX and TX interrupt
}

inline void FRSKY_EnableRXD(void)
{
  UCSR0B |= (1 << RXEN0);  // enable RX
  UCSR0B |= (1 << RXCIE0); // enable Interrupt
}

void FRSKY_Init(void)
{
  // clear frsky variables
  memset(frskyAlarms, 0, sizeof(frskyAlarms));
  resetTelemetry();

  DDRE &= ~(1 << DDE0);    // set RXD0 pin as input
  PORTE &= ~(1 << PORTE0); // disable pullup on RXD0 pin

#undef BAUD
#define BAUD 9600
#ifndef SIMU
#include <util/setbaud.h>

  UBRR0H = UBRRH_VALUE;
  UBRR0L = UBRRL_VALUE;
  UCSR0A &= ~(1 << U2X0); // disable double speed operation.

  // set 8N1
  UCSR0B = 0 | (0 << RXCIE0) | (0 << TXCIE0) | (0 << UDRIE0) | (0 << RXEN0) | (0 << TXEN0) | (0 << UCSZ02);
  UCSR0C = 0 | (1 << UCSZ01) | (1 << UCSZ00);

  
  while (UCSR0A & (1 << RXC0)) UDR0; // flush receive buffer

#endif

  // These should be running right from power up on a FrSky enabled '9X.
  FRSKY_EnableTXD(); // enable FrSky-Telemetry reception
  FRSKY_EnableRXD(); // enable FrSky-Telemetry reception
}

void frskyPushValue(uint8_t & i, uint8_t value)
{
  // byte stuff the only byte than might need it
  if (value == START_STOP) {
    frskyTxPacketBuf[i++] = BYTESTUFF;
    frskyTxPacketBuf[i++] = 0x5e;
  }
  else if (value == BYTESTUFF) {
    frskyTxPacketBuf[i++] = BYTESTUFF;
    frskyTxPacketBuf[i++] = 0x5d;
  }
  else {
    frskyTxPacketBuf[i++] = value;
  }
}

void FrskyData::set(uint8_t value)
{
   this->value = value;
   if (!max || max < value)
     max = value;
   if (!min || min > value)
     min = value;
 }

#if defined (PCBV3)
char g_logFilename[21]; //  "/G9XLOGS/M00_000.TXT\0" max required length = 21
// These global so we can close any open file from anywhere
FATFS FATFS_Obj;
FIL g_oLogFile;
#endif
void resetTelemetry()
{
  memset(frskyTelemetry, 0, sizeof(frskyTelemetry));
  memset(frskyRSSI, 0, sizeof(frskyRSSI));

#if defined (PCBV3)

  // Determine and set log file filename
  
  FRESULT result;

  // close any file left open. E.G. Changing models with log switch still on.
  if (g_oLogFile.fs) f_close(&g_oLogFile); 

  strcpy(g_logFilename, "/G9XLOGS/M00_000.TXT");

  uint8_t num = g_eeGeneral.currModel + 1;
  char *n = &g_logFilename[11];
  *n = (char)((num % 10) + '0');
  *(--n) = (char)((num / 10) + '0');

  result = f_mount(0, &FATFS_Obj);
  if (result!=FR_OK)
  {
    strcpy(g_logFilename, "FILE SYSTEM ERROR");
  }
  else
  {
    // Skip over any existing log files ... _000, _001, etc. (or find first gap in numbering)
    while (1)
    {
      result = f_open(&g_oLogFile, g_logFilename, FA_OPEN_EXISTING | FA_READ);

      if (result == FR_OK)
      {
        f_close(&g_oLogFile);

        // bump log file counter (file extension)
        n = &g_logFilename[15];
        if (++*n > '9')
        {
          *n='0';
          n--;
          if (++*n > '9')
          {
            *n='0';
            n--;
            if (++*n > '9')
            {
              *n='0';
              break; // Wow. We looped back around past 999 to 000! abort loop
            }
          }
        }
      }
      else if (result == FR_NO_PATH)
      {
        if (f_mkdir("/G9XLOGS") != FR_OK)
        {
          result = FR_NO_PATH;
          break;
        }
        else
          continue;
      }
      else
        break;
    }

    switch (result)
    {
      case FR_NO_PATH:
        strcpy(g_logFilename, "Check /G9XLOGS folder");
        break;
      case FR_NOT_READY:
        strcpy(g_logFilename, "DATA CARD NOT PRESENT");
        break;

      default:
        break;
    }
  }

  // g_logFilename should now be set appropriately.

#endif

}

