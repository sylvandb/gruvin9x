#include "menus.h"

enum MainViews {
  e_outputValues,
  e_outputBars,
  e_inputs1,
  e_inputs2,
  e_inputs3,
  e_timer2,
#ifdef FRSKY
  e_telemetry,
#endif
  MAX_VIEWS
};

void menuMainView(uint8_t event)
{
  static uint8_t trimSwLock;
  uint8_t view = g_eeGeneral.view & 0xf;

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
#ifdef FRSKY
    case EVT_KEY_BREAK(KEY_RIGHT):
      if(view == e_telemetry) {
        g_eeGeneral.view = (g_eeGeneral.view + 0x10) % 0x20;
        eeDirty(EE_GENERAL);
        beepKey();
      }
      break;
    case EVT_KEY_BREAK(KEY_LEFT):
      if(view == e_telemetry) {
        g_eeGeneral.view = (g_eeGeneral.view - 0x10) % 0x20;
        eeDirty(EE_GENERAL);
        beepKey();
      }
      break;
#endif
    case EVT_KEY_LONG(KEY_RIGHT):
      pushMenu(menuProcModelSelect);//menuProcExpoAll);
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

  if (g_eeGeneral.view < 0x10) {
    uint8_t att = (g_vbat100mV < g_eeGeneral.vBatWarn ? BLINK : 0) | DBLSIZE;
    for(uint8_t i=0;i<sizeof(g_model.name);i++)
      lcd_putcAtt(2*FW+i*2*FW-i-2, 0*FH, g_model.name[i], DBLSIZE);

    putsVBat(6*FW+2, 2*FH, att|NO_UNIT);
    lcd_putc(6*FW+2, 3*FH, 'V');

    if(s_timerState != TMR_OFF) {
      uint8_t att = DBLSIZE | (s_timerState==TMR_BEEPING ? BLINK : 0);
      putsTime(16*FW-2, FH*2, s_timerVal, att,att);
      putsTmrMode(s_timerVal >= 0 ? 9*FW-FW/2+5 : 9*FW-FW/2-2, FH*3, 0);
    }

    uint8_t phase = getFlightPhase();
    putsFlightPhase(6*FW+2, 2*FH, phase+1, 0);

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
  else {
    lcd_putsnAtt(0, 0, g_model.name, sizeof(g_model.name), BSS|INVERS);
    uint8_t att = (g_vbat100mV < g_eeGeneral.vBatWarn ? INVERS|BLINK : INVERS);
    putsVBat(14*FW,0,att);
    if(s_timerState != TMR_OFF){
      att = (s_timerState==TMR_BEEPING ? INVERS|BLINK : INVERS);
      putsTime(18*FW+3, 0, s_timerVal, att, att);
    }
  }

  if(view<e_inputs1) {
    for(uint8_t i=0; i<8; i++)
    {
      uint8_t x0,y0;
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
      if (g_eeGeneral.view & 0x10) {
        if (g_model.frsky.channels[0].ratio || g_model.frsky.channels[1].ratio) {
          x0 = 0;
          for (int i=0; i<2; i++) {
            if (g_model.frsky.channels[i].ratio) {
              blink = (alarmRaised[i] ? INVERS : 0);
              lcd_puts_P(x0, 3*FH, PSTR("A ="));
              lcd_putc(x0+FW, 3*FH, '1'+i);
              x0 += 3*FW;
              val = ((uint16_t)staticTelemetry[i]+g_model.frsky.channels[i].offset)*g_model.frsky.channels[i].ratio / 255;
              putsTelemetry(x0-2, 2*FH, val, g_model.frsky.channels[i].type, blink|DBLSIZE|LEFT);
              val = ((int16_t)frskyTelemetry[i].min+g_model.frsky.channels[i].offset)*g_model.frsky.channels[i].ratio / 255;
              putsTelemetry(x0+FW, 4*FH, val, g_model.frsky.channels[i].type, 0);
              val = ((int16_t)frskyTelemetry[i].max+g_model.frsky.channels[i].offset)*g_model.frsky.channels[i].ratio / 255;
              putsTelemetry(x0+3*FW, 4*FH, val, g_model.frsky.channels[i].type, LEFT);
              x0 = 11*FW-2;
            }
          }
        }

#if 0
        // Display RX Batt Volts only if a valid channel (A1/A2) has been selected
        if (g_eeFrsky.rxVoltsChannel >0)
        {
          y+=FH; lcd_puts_P(2*FW, y, PSTR("Rx Batt:"));
          // Rx batt voltage bar frame

          // Minimum voltage
          lcd_vline(3, 58, 6);  // marker

          y = 6*FH;
          putsVolts(1, y, g_eeFrsky.rxVoltsBarMin, LEFT);
          uint8_t middleVolts = g_eeFrsky.rxVoltsBarMin+(g_eeFrsky.rxVoltsBarMax - g_eeFrsky.rxVoltsBarMin)/2;
          putsVolts(64-FW, y, middleVolts, LEFT);
          lcd_vline(64, 58, 6);  // marker
          putsVolts(128-FW, y, g_eeFrsky.rxVoltsBarMax, 0);
          lcd_vline(125, 58, 6); // marker

          // Rx Batt: volts (255 == g_eefrsky.rxVoltsMax)
          uint16_t centaVolts = (voltsVal > 0) ? (10 * (uint16_t)g_eeFrsky.rxVoltsMax * (uint32_t)(voltsVal) / 255) + g_eeFrsky.rxVoltsOfs : 0;
          lcd_outdezAtt(13*FW, 4*FH, centaVolts, 0|PREC2);
          lcd_putc(13*FW, 4*FH, 'v');

          // draw the actual voltage bar
          uint16_t centaVoltsMin = 10 * g_eeFrsky.rxVoltsBarMin;
          if (centaVolts >= centaVoltsMin)
          {
            uint8_t vbarLen = (centaVolts - (10 * (uint16_t)g_eeFrsky.rxVoltsBarMin))  * 12
                                / (g_eeFrsky.rxVoltsBarMax - g_eeFrsky.rxVoltsBarMin);
            for (uint8_t i = 59; i < 63; i++) // Bar 4 pixels thick (high)
              lcd_hline(4, i, (vbarLen > 120) ? 120 : vbarLen);
          }
        }
#endif

        lcd_puts_P(0, 6*FH, PSTR("Rx="));
        lcd_outdezAtt(3 * FW - 2, 5*FH+2, staticRSSI[0], DBLSIZE|LEFT);
        lcd_outdezAtt(4 * FW, 7*FH, frskyRSSI[0].min, 0);
        lcd_outdezAtt(6 * FW, 7*FH, frskyRSSI[0].max, LEFT);
        lcd_puts_P(11 * FW - 2, 6*FH, PSTR("Tx="));
        lcd_outdezAtt(14 * FW - 4, 5*FH+2, staticRSSI[1], DBLSIZE|LEFT);
        lcd_outdezAtt(15 * FW - 2, 7*FH, frskyRSSI[1].min, 0);
        lcd_outdezAtt(17 * FW - 2, 7*FH, frskyRSSI[1].max, LEFT);
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
            val = ((int16_t)staticTelemetry[i]+g_model.frsky.channels[i].offset)*g_model.frsky.channels[i].ratio / 255;
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

    int8_t a = (view == e_inputs1) ? 0 : 9+(view-3)*6;
    int8_t b = (view == e_inputs1) ? 6 : 12+(view-3)*6;
    for(int8_t i=a; i<(a+3); i++) lcd_putsnAtt(2*FW-2 ,(i-a)*FH+4*FH,get_switches_string()+3*i,3,getSwitch(i+1, 0) ? INVERS : 0);
    for(int8_t i=b; i<(b+3); i++) lcd_putsnAtt(17*FW-1,(i-b)*FH+4*FH,get_switches_string()+3*i,3,getSwitch(i+1, 0) ? INVERS : 0);
  }
  else  // New Timer2 display
  {
    putsTime(30+5*FW, FH*5, timer2, DBLSIZE, DBLSIZE);
  }
}
