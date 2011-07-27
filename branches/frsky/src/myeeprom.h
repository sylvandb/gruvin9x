/*
 * Author - Erez Raviv <erezraviv@gmail.com>
 *
 * Based on th9x -> http://code.google.com/p/th9x/
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
#ifndef eeprom_h
#define eeprom_h

#include <inttypes.h>

#define WARN_THR_BIT  0x01
#define WARN_BEP_BIT  0x80
#define WARN_SW_BIT   0x02
#define WARN_MEM_BIT  0x04
#define WARN_BVAL_BIT 0x38

#define WARN_THR     (!(g_eeGeneral.warnOpts & WARN_THR_BIT))
#define WARN_BEP     (!(g_eeGeneral.warnOpts & WARN_BEP_BIT))
#define WARN_SW      (!(g_eeGeneral.warnOpts & WARN_SW_BIT))
#define WARN_MEM     (!(g_eeGeneral.warnOpts & WARN_MEM_BIT))
#define BEEP_VAL     ( (g_eeGeneral.warnOpts & WARN_BVAL_BIT) >>3 )

#define EEPROM_ER9X_VER  4
#define EEPROM_ER9X_r751 9

#define EEPROM_VER_r584  3
#define EEPROM_VER_r751  5
#define EEPROM_VER       101

typedef struct t_TrainerMix {
  uint8_t srcChn:3; //0-7 = ch1-8
  int8_t  swtch:5;
  int8_t  studWeight:6;
  uint8_t mode:2;   //off,add-mode,subst-mode
} __attribute__((packed)) TrainerMix; //

typedef struct t_TrainerData {
  int16_t        calib[4];
  TrainerMix     mix[4];
} __attribute__((packed)) TrainerData;

typedef struct t_FrSkyRSSIAlarm {
  uint8_t       level:2;
  int8_t        value:6;
} __attribute__((packed)) FrSkyRSSIAlarm;

typedef struct t_EEGeneral {
  uint8_t   myVers;
  int16_t   calibMid[7];
  int16_t   calibSpanNeg[7];
  int16_t   calibSpanPos[7];
  uint16_t  chkSum;
  uint8_t   currModel; //0..15
  uint8_t   contrast;
  uint8_t   vBatWarn;
  int8_t    vBatCalib;
  int8_t    lightSw;
  TrainerData trainer;
  uint8_t   view;      //index of subview in main scrren
  uint8_t   disableThrottleWarning:1;
  int8_t    switchWarning:2; // -1=down, 0=off, 1=up
  uint8_t   beeperVal:3;
  uint8_t   disableMemoryWarning:1;
  uint8_t   disableAlarmWarning:1;
  uint8_t   stickMode;
  uint8_t   inactivityTimer;
  uint8_t   throttleReversed:1;
  uint8_t   minuteBeep:1;
  uint8_t   preBeep:1;
  uint8_t   flashBeep:1;
  uint8_t   disableSplashScreen:1;
  uint8_t   enableTelemetryWarning:1;   // 0=no, 1=yes (Sound alarm when there's no telem. data coming in)
  uint8_t   spare:2;
  uint8_t   filterInput;
  uint8_t   lightAutoOff;
  uint8_t   templateSetup;  //RETA order according to chout_ar array 
  int8_t    PPM_Multiplier;
  FrSkyRSSIAlarm frskyRssiAlarms[2];
} __attribute__((packed)) EEGeneral;

// eeprom modelspec

typedef struct t_ExpoData {
  uint8_t mode:2;         // 0=end, 1=pos, 2=neg, 3=both
  uint8_t chn:2;
  uint8_t curve:4;        // 0=no curve, 1-6=std curves, 7-10=CV1-CV4, 11-15=CV9-CV13
  int8_t  swtch:5;
  uint8_t phase:3;        // if negPhase is 0: 0=normal, 5=FP4    if negPhase is 1: 5=!FP4
  uint8_t negPhase:1;
  uint8_t weight:7;
  int8_t  expo;
} __attribute__((packed)) ExpoData;

typedef struct t_LimitData {
  int8_t  min;
  int8_t  max;
  bool    revert;
  int16_t  offset;
} __attribute__((packed)) LimitData;

typedef struct t_MixData {
  uint8_t destCh:5;          // 0, 1..NUM_CHNOUT
  uint8_t mixWarn:3;         // mixer warning
#define MIX_P1    5
#define MIX_P2    6
#define MIX_P3    7
#define MIX_MAX   8
#define MIX_FULL  9
#define MIX_CYC1  10
#define MIX_CYC2  11
#define MIX_CYC3  12
  uint8_t srcRaw;            //
  int8_t  weight;
  int8_t  swtch;
  uint8_t curve;             // 0=symmetrisch, 1=no neg, 2=no pos
  uint8_t delayUp:4;
  uint8_t delayDown:4;
  uint8_t speedUp:4;         // Servogeschwindigkeit aus Tabelle (10ms Cycle)
  uint8_t speedDown:4;       // 0 nichts
  uint8_t carryTrim:1;
#define MLTPX_ADD  0
#define MLTPX_MUL  1
#define MLTPX_REP  2
  uint8_t mltpx:3;           // multiplex method 0=+ 1=* 2=replace
  int8_t  phase:4;           // -5=!FP4, 0=normal, 5=FP4
  int8_t  sOffset;
} __attribute__((packed)) MixData;

typedef struct t_CustomSwData { // Custom Switches data
  int8_t  v1; //input
  int8_t  v2; //offset
  uint8_t func;
} __attribute__((packed)) CustomSwData;

typedef struct t_SafetySwData { // Safety Switches data
  int8_t  swtch;
  int8_t  val;
} __attribute__((packed)) SafetySwData;

typedef struct t_FrSkyChannelData {
  uint8_t   ratio;              // 0.0 means not used, 0.1V steps EG. 6.6 Volts = 66. 25.1V = 251, etc.
  uint8_t   type:4;             // channel unit (0=volts, ...)
  int8_t    offset:4;           // calibration offset. Signed 0.1V steps. EG. -4 to substract 0.4V
  uint8_t   alarms_value[2];    // 0.1V steps EG. 6.6 Volts = 66. 25.1V = 251, etc. 
  uint8_t   alarms_level:4;
  uint8_t   alarms_greater:2;   // 0=LT(<), 1=GT(>)
  uint8_t   spare:2;
  int8_t    barMin;             // minimum for bar display
  uint8_t   barMax;             // ditto for max display (would usually = ratio)
} __attribute__((packed)) FrSkyChannelData;

typedef struct t_FrSkyData {
  FrSkyChannelData channels[2];
} __attribute__((packed)) FrSkyData;

typedef struct t_SwashRingData { // Swash Ring data
  uint8_t   invertELE:1;
  uint8_t   invertAIL:1;
  uint8_t   invertCOL:1;
  uint8_t   type:5;  
  uint8_t   collectiveSource;
  uint8_t   value;

/* TODO BSS everything is in comments in menus.cpp, how should it be used?
  uint8_t lim;   // 0 mean off 100 full deflection
  uint8_t chX; // 2 channels to limit
  uint8_t chY; // 2 channels to limit */
} __attribute__((packed)) SwashRingData;

typedef struct t_PhaseData {
  int8_t trim[4];     // -125..125 => trim value, 127 => use trim of phase 0, -128, -127, -126 => use trim of phases 1|2|3|4 instead
  int8_t swtch;       // swtch of phase[0] is the trimSw
  char name[6];
  uint8_t speedUp:4;
  uint8_t speedDown:4;
} __attribute__((packed)) PhaseData;

#define MAX_MODELS 16
#define MAX_PHASES 5
#define MAX_MIXERS 32
#define MAX_EXPOS  14
#define MAX_CURVE5 8
#define MAX_CURVE9 8

#define NUM_CHNOUT   16 // number of real output channels CH1-CH16
#define NUM_CSW      12  // number of custom switches

typedef struct t_ModelData {
  char      name[10];             // 10 must be first for eeLoadModelName
  int8_t    tmrMode:7;            // timer trigger source -> off, abs, stk, stk%, sw/!sw, !m_sw/!m_sw
  uint8_t   tmrDir:1;             // 0=>Count Down, 1=>Count Up
  uint16_t  tmrVal;
  uint8_t   protocol:3;
  int8_t    ppmNCH:3;
  uint8_t   thrTrim:1;            // Enable Throttle Trim
  uint8_t   thrExpo:1;            // Enable Throttle Expo
  uint8_t   trimInc:3;            // Trim Increments
  uint8_t   traineron:1;          // 0 disable trainer, 1 allow trainer
  uint8_t   pulsePol:1;
  uint8_t   extendedLimits:1;
  uint8_t   spare:2;
  int8_t    ppmDelay;
  uint8_t   beepANACenter;        // 1<<0->A1.. 1<<6->A7
  int8_t    tmr2Mode:7;           // timer trigger source -> off, abs, stk, stk%, sw/!sw, !m_sw/!m_sw
  uint8_t   tmr2Dir:1;            // 0=>Count Down, 1=>Count Up
  uint16_t  tmr2Val;
  MixData   mixData[MAX_MIXERS];
  LimitData limitData[NUM_CHNOUT];
  ExpoData  expoData[MAX_EXPOS];
  int8_t    curves5[MAX_CURVE5][5];
  int8_t    curves9[MAX_CURVE9][9];
  CustomSwData  customSw[NUM_CSW];
  SafetySwData  safetySw[NUM_CHNOUT];
  SwashRingData swashR;
  PhaseData phaseData[MAX_PHASES];
  FrSkyData     frsky;
} __attribute__((packed)) ModelData;

extern EEGeneral g_eeGeneral;
extern ModelData g_model;

#define TOTAL_EEPROM_USAGE (sizeof(ModelData)*MAX_MODELS + sizeof(EEGeneral))

#endif
/*eof*/
