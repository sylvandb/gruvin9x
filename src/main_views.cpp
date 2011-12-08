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

#undef ALTERNATE
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
#if defined(FRSKY_HUB)
  4, /*e_telemetry*/
#elif defined(FRSKY)
  2, /*e_telemetry*/
#endif
};

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

void doMainScreenGrphics()
{
  lcd_square(LBOX_CENTERX-BOX_WIDTH/2, LBOX_CENTERY-BOX_WIDTH/2, BOX_WIDTH);
  lcd_square(RBOX_CENTERX-BOX_WIDTH/2, RBOX_CENTERY-BOX_WIDTH/2, BOX_WIDTH);

  DO_CROSS(LBOX_CENTERX,LBOX_CENTERY,3)
  DO_CROSS(RBOX_CENTERX,RBOX_CENTERY,3)

  lcd_square(LBOX_CENTERX+(calibratedStick[0]*BOX_LIMIT/(2*RESX))-MARKER_WIDTH/2, LBOX_CENTERY-(calibratedStick[1]*BOX_LIMIT/(2*RESX))-MARKER_WIDTH/2, MARKER_WIDTH);
  lcd_square(RBOX_CENTERX+(calibratedStick[3]*BOX_LIMIT/(2*RESX))-MARKER_WIDTH/2, RBOX_CENTERY-(calibratedStick[2]*BOX_LIMIT/(2*RESX))-MARKER_WIDTH/2, MARKER_WIDTH);

  // Optimization by Mike Blandford
  {
    uint8_t x, y, len ;  // declare temporary variables
    for( x = -5, y = 4 ; y < 7 ; x += 5, y += 1 )
    {
      len = ((calibratedStick[y]+RESX)*BAR_HEIGHT/(RESX*2))+1l;  // calculate once per loop
      V_BAR(SCREEN_WIDTH/2+x,SCREEN_HEIGHT-8, len)
    }
  }
}

void menuMainView(uint8_t event)
{
  static bool instantTrimSwLock;
  static bool trim2OfsSwLock;
  
  uint8_t view = g_eeGeneral.view & 0x0f; // mask out ALTERNATE views

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
      instantTrimSwLock = true;
      trim2OfsSwLock = true;
      break;
  }

  bool trimSw = isFunctionActive(FUNC_INSTANT_TRIM);
  if (!instantTrimSwLock && trimSw) instantTrim();
  instantTrimSwLock = trimSw;

  trimSw = isFunctionActive(FUNC_TRIMS_2_OFS);
  if (!trim2OfsSwLock && trimSw) moveTrimsToOffsets();
  trim2OfsSwLock = trimSw;
  
  ///////////////////////////////////////////////////////////////////////
  /// Upper Section of Display common to all but telemetry alt. views ///
#if defined (FRSKY)
  if (view == e_telemetry && ((g_eeGeneral.view & 0xf0) >= ALTERNATE)) { // If view is a telemetry ALTERNATE view
    putsModelName(0, 0, g_model.name, g_eeGeneral.currModel, 0);
    uint8_t att = (g_vbat100mV < g_eeGeneral.vBatWarn ? BLINK : 0);
    putsVBat(14*FW,0,att);
    if(s_timerState != TMR_OFF){
      att = (s_timerState==TMR_BEEPING ? BLINK : 0);
      putsTime(17*FW, 0, s_timerVal, att, att);
    }
    // The timer is in the way ... but more important than a screen title
    else {
      lcd_putsnAtt(17*FW-4, 0, PSTR(" MAIN  GPSOTHER") + 5 * (g_eeGeneral.view - e_telemetry) / ALTERNATE - 5, 5, 0);
    }
    lcd_filled_rect(0, 0, DISPLAY_W, 8);
  }
  else 
#endif
  { // not in a telemetry ALTERNATE view
    uint8_t phase = getFlightPhase();
    lcd_putsnAtt(6*FW+4, 2*FH, g_model.phaseData[phase].name, sizeof(g_model.phaseData[phase].name), ZCHAR);

    uint8_t att = (g_vbat100mV < g_eeGeneral.vBatWarn ? BLINK : 0) | DBLSIZE;
    putsModelName(2*FW-2, 0*FH, g_model.name, g_eeGeneral.currModel, DBLSIZE);
    putsVBat(6*FW+3, 2*FH, att|NO_UNIT);
    lcd_putc(6*FW+4, 3*FH, 'V');

    if (s_timerState != TMR_OFF) {
      uint8_t att = DBLSIZE | (s_timerState==TMR_BEEPING ? BLINK : 0);
      putsTime(12*FW+3, FH*2, s_timerVal, att, att);
      putsTmrMode(s_timerVal >= 0 ? 9*FW-FW/2+5 : 9*FW-FW/2-2, FH*3, 0);
    }

    // trim sliders
    for(uint8_t i=0; i<4; i++)
    {
#define TL 27
      //                        LH LV RV RH
      static uint8_t x[4]    = {128*1/4+2, 4, 128-4, 128*3/4-2};
      static uint8_t vert[4] = {0,1,1,0};
      uint8_t xm, ym;
      xm = x[i];

      uint8_t att = 0;
      int16_t val = getTrimValue(getTrimFlightPhase(i, phase), i);;

      if (val < -125 || val > 125)
        att = BLINK;

      if (val < -(TL+1)*4)
        val = -(TL+1);
      else if (val > (TL+1)*4)
        val = TL+1;
      else
        val /= 4;

      if (vert[i]) {
        ym = 31;
        lcd_vline(xm, ym-TL, TL*2);
        if(((g_eeGeneral.stickMode&1) != (i&1)) || !(g_model.thrTrim)){
          lcd_vline(xm-1, ym-1,  3);
          lcd_vline(xm+1, ym-1,  3);
        }
        ym -= val;
      }
      else {
        ym = 60;
        lcd_hline(xm-TL, ym, TL*2);
        lcd_hline(xm-1, ym-1,  3);
        lcd_hline(xm-1, ym+1,  3);
        xm += val;
      }
      lcd_square(xm-3, ym-3, 7, att);
    }
  }
  /// Upper Section of Display common to all but telemetry alt. views ///
  ///////////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////////
  /// Lower section of display                                        ///
  if(view < e_inputs) { // values and bars
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
          lcd_outdezAtt( x0+4*FW , y0, GPERC(val), PREC1);
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
  else if (view == e_inputs) {
    doMainScreenGrphics();
    int8_t a = (g_eeGeneral.view == e_inputs) ? 1 : 4+(g_eeGeneral.view/ALTERNATE)*6;
    int8_t b = (g_eeGeneral.view == e_inputs) ? 7 : 7+(g_eeGeneral.view/ALTERNATE)*6;
    for(int8_t i=a; i<(a+3); i++) putsSwitches(2*FW-2,  (i-a)*FH+4*FH, i, getSwitch(i, 0) ? INVERS : 0);
    for(int8_t i=b; i<(b+3); i++) putsSwitches(17*FW-1, (i-b)*FH+4*FH, i, getSwitch(i, 0) ? INVERS : 0);
  }

#if defined(FRSKY)
  else if (view == e_telemetry) {
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

#ifdef DISPLAY_USER_DATA
        // this screen line buffer is currently always empty
        // Write user data characters along line, scrolling horizontally
        // Not much use for telemetry. Maybe do hex chars instead or get 
        // rid of it altogether XXX
        for (uint8_t ii=0; ii < TELEM_SCREEN_BUFFER_SIZE; ii++)
        {
          char c = userDataDisplayBuf[ii];
          if (c) lcd_putc(ii*FW, 1*FH, c);
        }
        //////////////////////////////////////////
#endif

        if (g_model.frsky.channels[0].ratio || g_model.frsky.channels[1].ratio) {
          x0 = 0;
          for (int i=0; i<2; i++) {
            if (g_model.frsky.channels[i].ratio) {
              blink = (alarmRaised[i] ? INVERS : 0);
              lcd_puts_P(x0, 3*FH, PSTR("A ="));
              lcd_putc(x0+FW, 3*FH, '1'+i);
              x0 += 3*FW;
              val = frskyComputeVolts(staticTelemetry[i], g_model.frsky.channels[i].ratio);
              putsTelemetry(x0, 2*FH, val, g_model.frsky.channels[i].type, blink|DBLSIZE|LEFT);
              val = frskyComputeVolts(frskyTelemetry[i].min, g_model.frsky.channels[i].ratio);
              putsTelemetry(x0, 4*FH, val, g_model.frsky.channels[i].type, 0);
              val = frskyComputeVolts(frskyTelemetry[i].max, g_model.frsky.channels[i].ratio);
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
#ifdef FRSKY_HUB
      else if (g_eeGeneral.view == e_telemetry+2*ALTERNATE) { // if on second alternate telemetry view
        // Date
        lcd_outdezNAtt(1*FW, 1*FH, frskyHubData.year+2000, LEFT, 4);
        lcd_putc(lcd_lastPos, 1*FH, '-');
        lcd_outdezNAtt(lcd_lastPos+FW, 1*FH, frskyHubData.month, LEFT|LEADING0, 2);
        lcd_putc(lcd_lastPos, 1*FH, '-');
        lcd_outdezNAtt(lcd_lastPos+FW, 1*FH, frskyHubData.day, LEFT|LEADING0, 2);

        // Time
        lcd_outdezNAtt(FW*10+8, 1*FH, frskyHubData.hour, LEFT|LEADING0, 2);
        lcd_putc(lcd_lastPos, 1*FH, ':');
        lcd_outdezNAtt(lcd_lastPos+FW, 1*FH, frskyHubData.min, LEFT|LEADING0, 2);
        lcd_putc(lcd_lastPos, 1*FH, ':');
        lcd_outdezNAtt(lcd_lastPos+FW, 1*FH, frskyHubData.sec, LEFT|LEADING0, 2);

        // Longitude
        lcd_outdezAtt(FW*3-2, 3*FH,  frskyHubData.gpsLongitude_bp / 100, 0); // ddd before '.'
        lcd_putc(lcd_lastPos, 3*FH, '@');
        uint8_t mn = frskyHubData.gpsLongitude_bp % 100;
        lcd_outdezNAtt(lcd_lastPos+FW, 3*FH, mn, LEFT|LEADING0, 2); // mm before '.'
        lcd_plot(lcd_lastPos, 4*FH-2, 0); // small decimal point
        lcd_outdezNAtt(lcd_lastPos+2, 3*FH, frskyHubData.gpsLongitude_ap, LEFT|UNSIGN|LEADING0, 4); // after '.'
        lcd_putc(lcd_lastPos+1, 3*FH, frskyHubData.gpsLongitudeEW ? frskyHubData.gpsLongitudeEW : '-');

        // Latitude
        lcd_outdezAtt(lcd_lastPos+3*FW+3, 3*FH,  frskyHubData.gpsLatitude_bp / 100, 0); // ddd before '.'
        lcd_putc(lcd_lastPos, 3*FH, '@');
        mn = frskyHubData.gpsLatitude_bp % 100;
        lcd_outdezNAtt(lcd_lastPos+FW, 3*FH, mn, LEFT|LEADING0, 2); // mm before '.'
        lcd_plot(lcd_lastPos, 4*FH-2, 0); // small decimal point
        lcd_outdezNAtt(lcd_lastPos+2, 3*FH, frskyHubData.gpsLatitude_ap, LEFT|UNSIGN|LEADING0, 4); // after '.'
        lcd_putc(lcd_lastPos+1, 3*FH, frskyHubData.gpsLatitudeNS ? frskyHubData.gpsLatitudeNS : '-');

        // Course / Heading
        lcd_puts_P(5, 5*FH, PSTR("Hdg:"));
        lcd_outdezNAtt(lcd_lastPos, 5*FH, frskyHubData.gpsCourse_bp, LEFT|LEADING0, 3); // before '.'
        lcd_plot(lcd_lastPos, 6*FH-2, 0); // small decimal point
        lcd_outdezAtt(lcd_lastPos+2, 5*FH, frskyHubData.gpsCourse_ap, LEFT); // after '.'
        lcd_putc(lcd_lastPos, 5*FH, '@');

        // Speed
        lcd_puts_P(76, 5*FH, PSTR("Spd:"));
        lcd_outdezAtt(lcd_lastPos, 5*FH, frskyHubData.gpsSpeed_bp, LEFT); // before '.'
        lcd_plot(lcd_lastPos, 6*FH-2, 0); // small decimal point
        lcd_outdezAtt(lcd_lastPos+2, 5*FH, frskyHubData.gpsSpeed_ap, LEFT|UNSIGN); // after '.'

        // Altitude
        lcd_puts_P(7*FW, 7*FH, PSTR("Alt:"));
        lcd_outdezNAtt(lcd_lastPos, 7*FH, frskyHubData.gpsAltitude_bp, LEFT|LEADING0, 3); // before '.'
        lcd_plot(lcd_lastPos, 8*FH-2, 0); // small decimal point
        lcd_outdezAtt(lcd_lastPos+2, 7*FH, frskyHubData.gpsAltitude_ap, LEFT|UNSIGN); // after '.'
        lcd_putc(lcd_lastPos, 7*FH, 'm');
      }
      else if (g_eeGeneral.view == e_telemetry+3*ALTERNATE) { // if on second alternate telemetry view

        uint8_t y = 2*FH;

        // Temperature 1
        lcd_puts_P(0, y, PSTR("Temp1:"));
        lcd_outdezNAtt(lcd_lastPos, y, frskyHubData.temperature1, LEFT);
        lcd_puts_P(lcd_lastPos, y, PSTR("@C"));
        y += FH;

        // Temperature 2
        lcd_puts_P(0, y, PSTR("Temp2:"));
        lcd_outdezNAtt(lcd_lastPos, y, frskyHubData.temperature2, LEFT);
        lcd_puts_P(lcd_lastPos, y, PSTR("@C"));

        y += 2*FH;

        // RPM
        lcd_puts_P(0, y, PSTR("RPM:"));
        lcd_outdezNAtt(lcd_lastPos, y, frskyHubData.rpm, LEFT);
        y += FH;

        // Fuel
        lcd_puts_P(0, y, PSTR("Fuel:"));
        lcd_outdezNAtt(lcd_lastPos, y, frskyHubData.fuelLevel, LEFT);
        lcd_putc(lcd_lastPos, y, '%');
        y += FH;

        // Volts
        lcd_puts_P(0, y, PSTR("Volts:"));
        lcd_outdezNAtt(lcd_lastPos, y, frskyHubData.volts, LEFT);
        lcd_putc(lcd_lastPos, y, 'V');

        y = 2*FH;
        // Altitude (barometric)
        lcd_puts_P(12*FW, y, PSTR("Alt:"));
        lcd_outdezNAtt(lcd_lastPos, y, frskyHubData.baroAltitude, LEFT|UNSIGN);
        lcd_putc(lcd_lastPos, y, 'm');

        y += 2*FH;

        // Acceleromter
        lcd_puts_P(11*FW, y, PSTR("Accel"));
        y += FH;
        lcd_puts_P(11*FW, y, PSTR("x="));
        lcd_outdezNAtt(FW*17, y, (int32_t)frskyHubData.accelX * 100 / 256, PREC2);
        lcd_putc(lcd_lastPos, y, 'g');
        y += FH;
        lcd_puts_P(11*FW, y, PSTR("y="));
        lcd_outdezNAtt(FW*17, y, (int32_t)frskyHubData.accelY * 100 / 256, PREC2);
        lcd_putc(lcd_lastPos, y, 'g');
        y += FH;
        lcd_puts_P(11*FW, y, PSTR("z="));
        lcd_outdezNAtt(FW*17, y, (int32_t)frskyHubData.accelZ * 100 / 256, PREC2);
        lcd_putc(lcd_lastPos, y, 'g');
      }
#endif
      else {
        y0 = 5*FH;
        //lcd_puts_P(2*FW-3, y0, PSTR("Tele:"));
        x0 = 4*FW-3;
        for (int i=0; i<2; i++) {
          if (g_model.frsky.channels[i].ratio) {
            blink = (alarmRaised[i] ? INVERS+BLINK : 0)|LEFT;
            lcd_puts_P(x0, y0, PSTR("A ="));
            lcd_putc(x0+FW, y0, '1'+i);
            val = frskyComputeVolts(staticTelemetry[i], g_model.frsky.channels[i].ratio);
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
#if defined (PCBV3)
      if (g_eeGeneral.view == e_telemetry+ALTERNATE) // if on first alternate telemetry view
      {
        lcd_putsAtt(0, FH*2, g_logFilename, BSS); // Show log filename (or error msg)
        
        /*** DEBUG XXX TODO REMOVE ME ***/
#if defined (PCBV4)
        lcd_outdezNAtt(FW*5, FH*3, g_rotenc1);
        lcd_outdezNAtt(FW*10, FH*3, g_rotenc2);
        if (~PIND & 0b00100000) lcd_putc(FW*5, FH*3,'X');
        if (~PIND & 0b00010000) lcd_putc(FW*10, FH*3,'X');
#endif
        /*************/
      }

#endif
      lcd_putsAtt(22, 40, PSTR("NO DATA"), DBLSIZE);
    }
  }
#endif /* defined(FRSKY) */

  else  // timer2
  {
    putsTime(33+FW+2, FH*5, timer2, DBLSIZE, DBLSIZE);
  }

  theFile.DisplayProgressBar(20*FW+1);
}