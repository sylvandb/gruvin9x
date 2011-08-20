/*
 * Authors (alphabetical order)
 * - Bertrand Songis <bsongis@gmail.com>
 * - Bryan J. Rentoul (Gruvin) <gruvin@gmail.com>
 *
 * gruvin9x is based on code named er9x by
 * Author - Erez Raviv <erezraviv@gmail.com>, which is in turn
 * was based on the original (and ongoing) project by Thomas Husterer,
 * th9x -- http://code.google.com/p/th9x/
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

#include "menus.h"

#define ALTERNATE 0x10

enum MainViews {
  e_outputValues,
  e_outputBars,
  e_inputs,
  e_timer2,
#ifdef FRSKY
  e_telemetry,
#endif
  MAX_VIEWS
};

uint8_t tabViews[] = {
  1, /*e_outputValues*/
  1, /*e_outputBars*/
  3, /*e_inputs*/
  1, /*e_timer2*/
#ifdef FRSKY
  4, /*e_telemetry*/
#endif
};

void menuMainView(uint8_t event)
{
  static uint8_t trimSwLock;
  uint8_t view = g_eeGeneral.view & 0x0f;

  switch(event)
  {
    case EVT_KEY_BREAK(KEY_MENU):
      if (view == e_timer2) {
        Timer2_running = !Timer2_running;
        beepKey();
      }
    break;
    case EVT_KEY_LONG(KEY_MENU):// go to last menu
      pushMenu(lastPopMenu());
      killEvents(event);
      break;
    case EVT_KEY_BREAK(KEY_RIGHT):
    case EVT_KEY_BREAK(KEY_LEFT):
      g_eeGeneral.view = (g_eeGeneral.view + (event == EVT_KEY_BREAK(KEY_RIGHT) ? 
            ALTERNATE : tabViews[view]*ALTERNATE-ALTERNATE)) % (tabViews[view]*ALTERNATE);
      eeDirty(EE_GENERAL);
      beepKey();
      break;
    case EVT_KEY_LONG(KEY_RIGHT):
      pushMenu(menuProcModelSelect);
      killEvents(event);
      break;
    case EVT_KEY_LONG(KEY_LEFT):
      pushMenu(menuProcSetup);
      killEvents(event);
      break;
    case EVT_KEY_BREAK(KEY_UP):
      g_eeGeneral.view = view+1;
      if(g_eeGeneral.view>=MAX_VIEWS) g_eeGeneral.view=0;
      eeDirty(EE_GENERAL);
      beepKey();
      break;
    case EVT_KEY_BREAK(KEY_DOWN):
      if(view>0)
        g_eeGeneral.view = view - 1;
      else
        g_eeGeneral.view = MAX_VIEWS-1;
      eeDirty(EE_GENERAL);
      beepKey();
      break;
    case EVT_KEY_LONG(KEY_UP):
      chainMenu(menuProcStatistic);
      killEvents(event);
      break;
    case EVT_KEY_LONG(KEY_DOWN):
#if defined(JETI)
      JETI_EnableRXD(); // enable JETI-Telemetry reception
      chainMenu(menuProcJeti);
#else
      chainMenu(menuProcStatistic2);
#endif
      killEvents(event);
      break;
    case EVT_KEY_FIRST(KEY_EXIT):
      if(s_timerState==TMR_BEEPING) {
        s_timerState = TMR_STOPPED;
        beepKey();
      }
      else if (view == e_timer2) {
       resetTimer2();
       beepKey();
      }
#ifdef FRSKY
      else if (view == e_telemetry) {
        resetTelemetry();
        beepKey();
      }
#endif
      else {
        resetTimer1();
      }
      break;
    case EVT_KEY_LONG(KEY_EXIT):
      resetTimer1();
      resetTimer2();
#ifdef FRSKY
      resetTelemetry();
#endif
      beepKey();
      break;
    case EVT_ENTRY:
      killEvents(KEY_EXIT);
      killEvents(KEY_UP);
      killEvents(KEY_DOWN);
      trimSwLock = true;
      break;
  }

  if(getSwitch(g_model.phaseData[0].swtch, 0) && !trimSwLock) setStickCenter();
  trimSwLock = getSwitch(g_model.phaseData[0].swtch,0);

  ///////////////////////////////////////////////////////////////////////
  /// Upper Section of Display common to all but telemetry alt. views ///
  if (g_eeGeneral.view >= e_telemetry+ALTERNATE) {
    lcd_putsnAtt(0, 0, g_model.name, sizeof(g_model.name), ZCHAR|INVERS);
    uint8_t att = (g_vbat100mV < g_eeGeneral.vBatWarn ? INVERS|BLINK : INVERS);
    putsVBat(14*FW,0,att);
    if(s_timerState != TMR_OFF){
      att = (s_timerState==TMR_BEEPING ? INVERS|BLINK : INVERS);
      putsTime(18*FW+3, 0, s_timerVal, att, att);
    }
  }
  else {
    uint8_t att = (g_vbat100mV < g_eeGeneral.vBatWarn ? BLINK : 0) | DBLSIZE;
    lcd_putsnAtt(2*FW-2, 0*FH, g_model.name, sizeof(g_model.name), ZCHAR|DBLSIZE);
    putsVBat(4*FW+2, 2*FH, att, NO_UNIT);
    lcd_putc(6*FW+2, 3*FH, 'V');

    if(s_timerState != TMR_OFF) {
      uint8_t att = DBLSIZE | (s_timerState==TMR_BEEPING ? BLINK : 0);
      putsTime(16*FW-2, FH*2, s_timerVal, att,att);
      putsTmrMode(s_timerVal >= 0 ? 9*FW-FW/2+5 : 9*FW-FW/2-2, FH*3, 0);
    }

    uint8_t phase = getFlightPhase();
    lcd_putsnAtt(6*FW+2, 2*FH, g_model.phaseData[phase].name, sizeof(g_model.phaseData[phase].name), ZCHAR);

    // trim sliders
    for(uint8_t i=0; i<4; i++)
    {
#define TL 27
      //                        LH LV RV RH
      static uint8_t x[4]    = {128*1/4+2, 4, 128-4, 128*3/4-2};
      static uint8_t vert[4] = {0,1,1,0};
      uint8_t xm,ym;
      xm=x[i];
      int8_t val = max((int8_t)-(TL+1),min((int8_t)(TL+1),(int8_t)(phaseaddress(getTrimFlightPhase(i, phase))->trim[i]/4)));
      if(vert[i]){
        ym=31;
        lcd_vline(xm, ym-TL, TL*2);

        if(((g_eeGeneral.stickMode&1) != (i&1)) || !(g_model.thrTrim)){
          lcd_vline(xm-1, ym-1,  3);
          lcd_vline(xm+1, ym-1,  3);
        }
        ym -= val;
      }else{
        ym=60;
        lcd_hline(xm-TL, ym, TL*2);
        lcd_hline(xm-1, ym-1,  3);
        lcd_hline(xm-1, ym+1,  3);
        xm += val;
      }
      lcd_square(xm-3, ym-3, 7);
    }
  }
  /// Upper Section of Display common to all but telemetry alt. views ///
  ///////////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////////
  /// Lower section of display                                        ///
  if(view < e_inputs) {
    for(uint8_t i=0; i<8; i++)
    {
      uint8_t x0, y0;
      int16_t val = g_chans512[i];
      //val += g_model.limitData[i].revert ? g_model.limitData[i].offset : -g_model.limitData[i].offset;
      switch(view)
      {
        case e_outputValues:
          x0 = (i%4*9+3)*FW/2;
          y0 = i/4*FH+40;
          // *1000/1024 = x - x/32 + x/128
#define GPERC(x)  (x - x/32 + x/128)
#if defined (DECIMALS_DISPLAYED)
          lcd_outdezAtt( x0+4*FW , y0, GPERC(val), PREC1 );
#else
          lcd_outdezAtt( x0+4*FW , y0, GPERC(val)/10, 0); // G: Don't like the decimal part*
#endif
          break;
        case e_outputBars:
#define WBAR2 (50/2)
          x0       = i<4 ? 128/4+2 : 128*3/4-2;
          y0       = 38+(i%4)*5;
          int8_t l = (abs(val) * WBAR2 + 512) / 1024;
          if(l>WBAR2)  l =  WBAR2;  // prevent bars from going over the end - comment for debugging

          lcd_hlineStip(x0-WBAR2,y0,WBAR2*2+1,0x55);
          lcd_vline(x0,y0-2,5);
          if(val>0){
            x0+=1;
          }else{
            x0-=l;
          }
          lcd_hline(x0,y0+1,l);
          lcd_hline(x0,y0-1,l);
          break;
      }
    }
  }
#ifdef FRSKY
  else if(view == e_telemetry) {
    static uint8_t displayCount = 0;
    static uint8_t staticTelemetry[2];
    static uint8_t staticRSSI[2];
    static bool alarmRaised[2];

    if (frskyStreaming) {
      uint8_t y0, x0, val, blink;
      if (!displayCount) {
        for (int i=0; i<2; i++) {
          staticTelemetry[i] = frskyTelemetry[i].value;
          staticRSSI[i] = frskyRSSI[i].value;
          alarmRaised[i] = FRSKY_alarmRaised(i);
        }
      }
      displayCount = (displayCount+1) % 50;
      if (g_eeGeneral.view == e_telemetry+ALTERNATE) { // if on first alternate telemetry view

        // TODO -- this screen line buffer is currently always empty
        // Write user data characters along line, scrolling horizontally
        // Not much use for telemetry. Maybe do hex chars instead or get 
        // rid of it altogether XXX
        for (uint8_t ii=0; ii < TELEM_SCREEN_BUFFER_SIZE; ii++)
        {
          char c = userDataDisplayBuf[ii];
          if (c) lcd_putc(ii*FW, 1*FH, c);
        }
        //////////////////////////////////////////

        if (g_model.frsky.channels[0].ratio || g_model.frsky.channels[1].ratio) {
          x0 = 0;
          for (int i=0; i<2; i++) {
            if (g_model.frsky.channels[i].ratio) {
              blink = (alarmRaised[i] ? INVERS : 0);
              lcd_puts_P(x0, 3*FH, PSTR("A ="));
              lcd_putc(x0+FW, 3*FH, '1'+i);
              x0 += 3*FW;
              val = (uint16_t)staticTelemetry[i]*(g_model.frsky.channels[i].ratio+g_model.frsky.channels[i].offset/10)/ 255;
              putsTelemetry(x0, 2*FH, val, g_model.frsky.channels[i].type, blink|DBLSIZE|LEFT);
              val = (int16_t)frskyTelemetry[i].min*(g_model.frsky.channels[i].ratio+g_model.frsky.channels[i].offset/10)/ 255;
              putsTelemetry(x0, 4*FH, val, g_model.frsky.channels[i].type, 0);
              val = (int16_t)frskyTelemetry[i].max*(g_model.frsky.channels[i].ratio+g_model.frsky.channels[i].offset/10)/ 255;
              putsTelemetry(x0+3*FW, 4*FH, val, g_model.frsky.channels[i].type, LEFT);
              x0 = 11*FW-2;
            }
          }
        }

        lcd_puts_P(0, 6*FH, PSTR("Rx="));
        lcd_outdezAtt(3 * FW - 2, 5*FH+2, staticRSSI[0], DBLSIZE|LEFT);
        lcd_outdezAtt(4 * FW, 7*FH, frskyRSSI[0].min, 0);
        lcd_outdezAtt(6 * FW, 7*FH, frskyRSSI[0].max, LEFT);
        lcd_puts_P(11 * FW - 2, 6*FH, PSTR("Tx="));
        lcd_outdezAtt(14 * FW - 4, 5*FH+2, staticRSSI[1], DBLSIZE|LEFT);
        lcd_outdezAtt(15 * FW - 2, 7*FH, frskyRSSI[1].min, 0);
        lcd_outdezAtt(17 * FW - 2, 7*FH, frskyRSSI[1].max, LEFT);
      }
      else if (g_eeGeneral.view == e_telemetry+2*ALTERNATE) { //e_telemetry+ALTERNATE+1) { // if on second alternate telemetry view {
        lcd_putsAtt(19*FW-4, 0, PSTR("GPS"), INVERS); // XXX TEMP

        // date XXX: day and month seem to always be zero. Bug with the Fr-Sky hub?
        lcd_outdezNAtt(1*FW, 1*FH, gTelem_GPSyear+2000, LEFT, 4);
        lcd_putc(lcd_lastPos, 1*FH, '-');
        lcd_outdezNAtt(lcd_lastPos, 1*FH, gTelem_GPSmonth, LEFT, 2|LEADING0);
        lcd_putc(lcd_lastPos, 1*FH, '-');
        lcd_outdezNAtt(lcd_lastPos, 1*FH, gTelem_GPSday, LEFT, 2|LEADING0);

        // time
        lcd_outdezNAtt(FW*10+8, 1*FH, gTelem_GPShour, LEFT, 2|LEADING0);
        lcd_putc(lcd_lastPos, 1*FH, ':');
        lcd_outdezNAtt(lcd_lastPos, 1*FH, gTelem_GPSmin, LEFT, 2|LEADING0);
        lcd_putc(lcd_lastPos, 1*FH, ':');
        lcd_outdezNAtt(lcd_lastPos, 1*FH, gTelem_GPSsec, LEFT, 2|LEADING0);

        // Longitude
        lcd_outdezAtt(FW*3-2, 3*FH,  gTelem_GPSlongitude[0] / 100, 0); // ddd before '.'
        lcd_putc(lcd_lastPos, 3*FH, '@');
        uint8_t mn = gTelem_GPSlongitude[0] % 100;
        lcd_outdezNAtt(lcd_lastPos, 3*FH, mn, LEFT, 2|LEADING0); // mm before '.'
        lcd_plot(lcd_lastPos, 4*FH-2, 0); // small decimal point
        lcd_outdezNAtt(lcd_lastPos+2, 3*FH, gTelem_GPSlongitude[1], LEFT|UNSIGN, 4|TRAILING0); // after '.'
        lcd_putc(lcd_lastPos+1, 3*FH, gTelem_GPSlongitudeEW ? 'E' : 'W'); 

        // Latitude
        lcd_outdezAtt(lcd_lastPos+3*FW+3, 3*FH,  gTelem_GPSlatitude[0] / 100, 0); // ddd before '.'
        lcd_putc(lcd_lastPos, 3*FH, '@');
        mn = gTelem_GPSlatitude[0] % 100;
        lcd_outdezNAtt(lcd_lastPos, 3*FH, mn, LEFT, 2|LEADING0); // mm before '.'
        lcd_plot(lcd_lastPos, 4*FH-2, 0); // small decimal point
        lcd_outdezNAtt(lcd_lastPos+2, 3*FH, gTelem_GPSlatitude[1], LEFT|UNSIGN, 4|TRAILING0); // after '.'
        lcd_putc(lcd_lastPos+1, 3*FH, gTelem_GPSlatitudeNS ? 'S' : 'N'); 

        // Course / Heading
        lcd_puts_P(5, 5*FH, PSTR("Hdg:"));
        lcd_outdezNAtt(lcd_lastPos, 5*FH, gTelem_GPScourse[0], LEFT, 3|LEADING0); // before '.'
        lcd_plot(lcd_lastPos, 6*FH-2, 0); // small decimal point
        lcd_outdezAtt(lcd_lastPos+2, 5*FH, gTelem_GPScourse[1], LEFT); // after '.'
        lcd_putc(lcd_lastPos, 5*FH, '@');

        // Speed
        lcd_puts_P(76, 5*FH, PSTR("Spd:"));
        lcd_outdezAtt(lcd_lastPos, 5*FH, gTelem_GPSspeed[0], LEFT); // before '.'
        lcd_plot(lcd_lastPos, 6*FH-2, 0); // small decimal point
        lcd_outdezAtt(lcd_lastPos+2, 5*FH, gTelem_GPSspeed[1], LEFT|UNSIGN); // after '.'

        // Altititude 
        lcd_puts_P(7*FW, 7*FH, PSTR("Alt:"));
        lcd_outdezNAtt(lcd_lastPos, 7*FH, gTelem_GPSaltitude[0], LEFT, 3|LEADING0); // before '.'
        lcd_plot(lcd_lastPos, 8*FH-2, 0); // small decimal point
        lcd_outdezAtt(lcd_lastPos+2, 7*FH, gTelem_GPSaltitude[1], LEFT|UNSIGN); // after '.'
        lcd_putc(lcd_lastPos, 7*FH, 'm');

      }
      else if (g_eeGeneral.view == e_telemetry+3*ALTERNATE) { //e_telemetry+ALTERNATE+1) { // if on second alternate telemetry view {
        lcd_puts_P(0,1*FH, PSTR("GPS TWO")); // XXX TEMP
      }
      else {

        y0 = 5*FH;
        //lcd_puts_P(2*FW-3, y0, PSTR("Tele:"));
        x0 = 4*FW-3;
        for (int i=0; i<2; i++) {
          if (g_model.frsky.channels[i].ratio) {
            blink = (alarmRaised[i] ? INVERS+BLINK : 0)|LEFT;
            lcd_puts_P(x0, y0, PSTR("A ="));
            lcd_putc(x0+FW, y0, '1'+i);
            val = (int16_t)staticTelemetry[i]*(g_model.frsky.channels[i].ratio+g_model.frsky.channels[i].offset/10) / 255;
            putsTelemetry(x0+3*FW, y0, val, g_model.frsky.channels[i].type, blink);
            x0 = 13*FW-3;
          }
        }
        y0+=FH;
        //lcd_puts_P(2*FW-3, y0, PSTR("RSSI:"));
        lcd_puts_P(4*FW-3, y0, PSTR("Rx="));
        lcd_outdezAtt(7*FW-3, y0, staticRSSI[0], LEFT);
        lcd_puts_P(13*FW-3, y0, PSTR("Tx="));
        lcd_outdezAtt(16*FW-3, y0, staticRSSI[1], LEFT);
      }
    }
    else {
      
      if (g_eeGeneral.view == e_telemetry+ALTERNATE) // if on first alternate telemetry view
        lcd_putsAtt(0, FH*2, g_logFilename, BSS); // Show log filename (or error msg)
      lcd_putsAtt(22, 40, PSTR("NO DATA"), DBLSIZE);
    }
  }
#endif
  else if(view<e_timer2) {
    #define BOX_WIDTH     23
    #define BAR_HEIGHT    (BOX_WIDTH-1l)
    #define MARKER_WIDTH  5
    #define SCREEN_WIDTH  128
    #define SCREEN_HEIGHT 64
    #define BOX_LIMIT     (BOX_WIDTH-MARKER_WIDTH)
    #define LBOX_CENTERX  (  SCREEN_WIDTH/4 + 10)
    #define LBOX_CENTERY  (SCREEN_HEIGHT-9-BOX_WIDTH/2)
    #define RBOX_CENTERX  (3*SCREEN_WIDTH/4 - 10)
    #define RBOX_CENTERY  (SCREEN_HEIGHT-9-BOX_WIDTH/2)

    lcd_square(LBOX_CENTERX-BOX_WIDTH/2, LBOX_CENTERY-BOX_WIDTH/2, BOX_WIDTH);
    lcd_square(RBOX_CENTERX-BOX_WIDTH/2, RBOX_CENTERY-BOX_WIDTH/2, BOX_WIDTH);

    DO_CROSS(LBOX_CENTERX,LBOX_CENTERY,3)
    DO_CROSS(RBOX_CENTERX,RBOX_CENTERY,3)

    lcd_square(LBOX_CENTERX+(calibratedStick[0]*BOX_LIMIT/(2*RESX))-MARKER_WIDTH/2, LBOX_CENTERY-(calibratedStick[1]*BOX_LIMIT/(2*RESX))-MARKER_WIDTH/2, MARKER_WIDTH);
    lcd_square(RBOX_CENTERX+(calibratedStick[3]*BOX_LIMIT/(2*RESX))-MARKER_WIDTH/2, RBOX_CENTERY-(calibratedStick[2]*BOX_LIMIT/(2*RESX))-MARKER_WIDTH/2, MARKER_WIDTH);

    // Optimization by Mike Blandford
    {
        uint8_t x, y, len ;         // declare temporary variables
        for( x = -5, y = 4 ; y < 7 ; x += 5, y += 1 )
        {
            len = ((calibratedStick[y]+RESX)*BAR_HEIGHT/(RESX*2))+1l ;  // calculate once per loop
            V_BAR(SCREEN_WIDTH/2+x,SCREEN_HEIGHT-10, len )
        }
    }

    int8_t a = (g_eeGeneral.view == e_inputs) ? 0 : 3+(g_eeGeneral.view/ALTERNATE)*6;
    int8_t b = (g_eeGeneral.view == e_inputs) ? 6 : 6+(g_eeGeneral.view/ALTERNATE)*6;
    for(int8_t i=a; i<(a+3); i++) lcd_putsnAtt(2*FW-2 ,(i-a)*FH+4*FH,get_switches_string()+3*i,3,getSwitch(i+1, 0) ? INVERS : 0);
    for(int8_t i=b; i<(b+3); i++) lcd_putsnAtt(17*FW-1,(i-b)*FH+4*FH,get_switches_string()+3*i,3,getSwitch(i+1, 0) ? INVERS : 0);
  }
  else  // New Timer2 display
  {
    putsTime(30+5*FW, FH*5, timer2, DBLSIZE, DBLSIZE);
  }
  /// Lower section of display                                        ///
  ///////////////////////////////////////////////////////////////////////
}
