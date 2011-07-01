/*
 * gruvin9x Author Bryan J.Rentoul (Gruvin) <gruvin@gmail.com>
 *
 * gruvin9x is based on code named er9x by
 * Author - Erez Raviv <erezraviv@gmail.com>, which is in turn
 * based on th9x -> http://code.google.com/p/th9x/
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

#include "gruvin9x.h"
#include "templates.h"
#if defined (FRSKY)
#include "frsky.h"
#endif
#include "menus.h"

#define GET_DR_STATE(x) (!getSwitch(g_model.expoData[x].drSw1,0) ?   \
                          DR_HIGH :                                  \
                          !getSwitch(g_model.expoData[x].drSw2,0)?   \
                          DR_MID : DR_LOW);

#define DO_SQUARE(xx,yy,ww)         \
    {uint8_t x,y,w ; x = xx; y = yy; w = ww ; \
    lcd_vline(x-w/2,y-w/2,w);  \
    lcd_hline(x-w/2,y+w/2,w);  \
    lcd_vline(x+w/2,y-w/2,w);  \
    lcd_hline(x-w/2,y-w/2,w);}

#define DO_CROSS(xx,yy,ww)          \
    lcd_vline(xx,yy-ww/2,ww);  \
    lcd_hline(xx-ww/2,yy,ww);  \

#define V_BAR(xx,yy,ll)       \
    lcd_vline(xx-1,yy-ll,ll); \
    lcd_vline(xx  ,yy-ll,ll); \
    lcd_vline(xx+1,yy-ll,ll); \
    
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

int16_t calibratedStick[NUM_STICKS+NUM_POTS];
int16_t ex_chans[NUM_CHNOUT];          // Outputs + intermidiates
uint8_t s_pgOfs;
uint8_t s_editMode;
uint8_t s_noHi;

int16_t g_chans512[NUM_CHNOUT];

#include "sticks.lbm"

extern MixData *mixaddress( uint8_t idx ) ;
extern LimitData *limitaddress( uint8_t idx ) ;

enum EnumTabModel {
  e_ModelSelect,
  e_Model,
#ifdef HELI
  e_Heli,
#endif
  e_ExpoAll,
  e_Mix,
  e_Limits,
  e_Curve,
  e_CustomSwitches,
  e_SafetySwitches,
#ifdef FRSKY
  e_Telemetry,
#endif
#ifdef TEMPLATES
  e_Templates
#endif
};

MenuFuncP_PROGMEM APM menuTabModel[] = {
  menuProcModelSelect,
  menuProcModel,
#ifdef HELI
  menuProcHeli,
#endif
  menuProcExpoAll,
  menuProcMix,
  menuProcLimits,
  menuProcCurve,
  menuProcCustomSwitches,
  menuProcSafetySwitches,
#ifdef FRSKY
  menuProcTelemetry,
#endif
#ifdef TEMPLATES
  menuProcTemplates
#endif
};

enum EnumTabDiag {
  e_Setup,
#if defined(PCBV3)
  e_FrskyTime,
#endif
  e_Trainer,
  e_Vers,
  e_Keys,
  e_Ana,
  e_Calib
};

MenuFuncP_PROGMEM APM menuTabDiag[] = {
  menuProcSetup,
#if defined(PCBV3)
  menuProcFrskyTime,
#endif
  menuProcTrainer,
  menuProcDiagVers,
  menuProcDiagKeys,
  menuProcDiagAna,
  menuProcDiagCalib
};

void menu_lcd_onoff( uint8_t x,uint8_t y, uint8_t value, uint8_t mode )
{
  lcd_putsnAtt( x, y, PSTR("OFFON ")+3*value,3,mode ? INVERS:0) ;
}

void menu_lcd_HYPHINV( uint8_t x,uint8_t y, uint8_t value, uint8_t mode )
{
  lcd_putsnAtt( x, y, PSTR("---INV")+3*value,3,mode ? INVERS:0) ;
}

void DisplayScreenIndex(uint8_t index, uint8_t count, uint8_t attr)
{
  lcd_outdezAtt(128,0,count,attr);
  lcd_putcAtt(1+128-FW*(count>9 ? 3 : 2),0,'/',attr);
  lcd_outdezAtt(1+128-FW*(count>9 ? 3 : 2),0,index+1,attr);
}

void MState2::check_simple(uint8_t event, uint8_t curr, MenuFuncP *menuTab, uint8_t menuTabSize, uint8_t maxrow)
{
  check(event, curr, menuTab, menuTabSize, 0, 0, maxrow);
}

void MState2::check_submenu_simple(uint8_t event, uint8_t maxrow)
{
  check_simple(event, 0, 0, 0, maxrow);
}

#define MAXCOL(row) (horTab ? pgm_read_byte(horTab+min(row, horTabMax)) : (const uint8_t)0)
#define INC(val,max) if(val<max) {val++;} else {val=0;}
#define DEC(val,max) if(val>0  ) {val--;} else {val=max;}
void MState2::check(uint8_t event, uint8_t curr, MenuFuncP *menuTab, uint8_t menuTabSize, prog_uint8_t *horTab, uint8_t horTabMax, uint8_t maxrow)
{
  if (menuTab) {
    uint8_t attr = m_posVert==0 ? INVERS : 0;

    if (m_posVert==0) {
      switch(event)
      {
        case EVT_KEY_FIRST(KEY_LEFT):
          if(curr>0){
            chainMenu((MenuFuncP)pgm_read_adr(&menuTab[curr-1]));
          }
          break;
        case EVT_KEY_FIRST(KEY_RIGHT):
          if(curr < (menuTabSize-1)){
            chainMenu((MenuFuncP)pgm_read_adr(&menuTab[curr+1]));
          }
          break;
      }
    }
    DisplayScreenIndex(curr, menuTabSize, attr);
  }

  uint8_t maxcol = MAXCOL(m_posVert);
  switch(event)
  {
    case EVT_ENTRY:
      //if(m_posVert>maxrow)
        init();
        s_editMode = false;
      //init();BLINK_SYNC;
      break;
    case EVT_KEY_FIRST(KEY_MENU):
      if (maxcol > 0)
        s_editMode = !s_editMode;
      break;
    case EVT_KEY_LONG(KEY_EXIT):
      s_editMode = false;
      //popMenu(true); //return to uppermost, beeps itself
      popMenu(false);
      break;
      //fallthrough
    case EVT_KEY_BREAK(KEY_EXIT):
      if(s_editMode) {
        s_editMode = false;
        break;
      }
      if(m_posVert==0 || !menuTab) {
        popMenu();  //beeps itself
      }
      else {
        beepKey();
        init();BLINK_SYNC;
      }
      break;

    case EVT_KEY_REPT(KEY_RIGHT):  //inc
      if(m_posHorz==maxcol) break;
    case EVT_KEY_FIRST(KEY_RIGHT)://inc
      if(!horTab || s_editMode)break;
      INC(m_posHorz,maxcol);
      BLINK_SYNC;
      break;

    case EVT_KEY_REPT(KEY_LEFT):  //dec
      if(m_posHorz==0) break;
    case EVT_KEY_FIRST(KEY_LEFT)://dec
      if(!horTab || s_editMode)break;
      DEC(m_posHorz,maxcol);
      BLINK_SYNC;
      break;

    case EVT_KEY_REPT(KEY_DOWN):  //inc
      if(m_posVert==maxrow) break;
    case EVT_KEY_FIRST(KEY_DOWN): //inc
      if(s_editMode)break;
      do {
        INC(m_posVert,maxrow);
      } while(MAXCOL(m_posVert) == (uint8_t)-1);
      m_posHorz = min(m_posHorz, MAXCOL(m_posVert));
      BLINK_SYNC;
      break;

    case EVT_KEY_REPT(KEY_UP):  //dec
      if(m_posVert==0) break;
    case EVT_KEY_FIRST(KEY_UP): //dec
      if(s_editMode)break;
      do {
        DEC(m_posVert,maxrow);
      } while(MAXCOL(m_posVert) == (uint8_t)-1);
      m_posHorz = min(m_posHorz, MAXCOL(m_posVert));
      BLINK_SYNC;
      break;
  }

  uint8_t max = menuTab ? 7 : 6;
  if(m_posVert<1) s_pgOfs=0;
  else if(m_posVert-s_pgOfs>max) s_pgOfs = m_posVert-max;
  else if(m_posVert-s_pgOfs<1) s_pgOfs = m_posVert-1;
}

static uint8_t s_curveChan;

#define XD X0-2

void menuProcCurveOne(uint8_t event)
{
  bool    cv9 = s_curveChan >= MAX_CURVE5;

  SUBMENU("CURVE", 2+(cv9 ? 9 : 5), { 9,0/*repeated...*/});
  lcd_outdezAtt(6*FW, 0,s_curveChan+1 ,INVERS);

  int8_t *crv = cv9 ? g_model.curves9[s_curveChan-MAX_CURVE5] : g_model.curves5[s_curveChan];

  int8_t  sub    = mstate2.m_posVert-1;
  int8_t  subSub = mstate2.m_posHorz;

  switch(event){
    case EVT_KEY_FIRST(KEY_EXIT):
      if(subSub!=0) {
        subSub = mstate2.m_posHorz = 0;
        killEvents(event);
      }
      break;
    case EVT_KEY_REPT(KEY_LEFT):
    case EVT_KEY_FIRST(KEY_LEFT):
      if(s_editMode && subSub>0) mstate2.m_posHorz--;
      break;
    case EVT_KEY_REPT(KEY_RIGHT):
    case EVT_KEY_FIRST(KEY_RIGHT):
      if(s_editMode && subSub<(cv9 ? 9 : 5)) mstate2.m_posHorz++;
      break;
  }
  
  s_editMode = mstate2.m_posHorz;

  for (uint8_t i = 0; i < 5; i++) {
    uint8_t y = i * FH + 16;
    uint8_t attr = sub == i ? INVERS : 0;
    lcd_outdezAtt(4 * FW, y, crv[i], attr);
  }
  if(cv9)
    for (uint8_t i = 0; i < 4; i++) {
      uint8_t y = i * FH + 16;
      uint8_t attr = sub == i + 5 ? INVERS : 0;
      lcd_outdezAtt(8 * FW, y, crv[i + 5], attr);
    }
  lcd_putsAtt( 2*FW, 1*FH,PSTR("EDIT->"),((sub == -1) && (subSub == 0)) ? INVERS : 0);
  lcd_putsAtt( 2*FW, 7*FH,PSTR("PRESET"),sub == (cv9 ? 9 : 5) ? INVERS : 0);

  static int8_t dfltCrv;
  if((sub<(cv9 ? 9 : 5)) && (sub>-1))  CHECK_INCDEC_H_MODELVAR( event, crv[sub], -100,100);
  else  if(sub>0){ //make sure we're not on "EDIT"
    dfltCrv = checkIncDec(event, dfltCrv, -4, 4, 0);
    if (checkIncDec_Ret) {
      if(cv9) for (uint8_t i = 0; i < 9; i++) crv[i] = (i-4)*dfltCrv* 100 / 16;
      else    for (uint8_t i = 0; i < 5; i++) crv[i] = (i-2)*dfltCrv* 100 /  8;
      eeDirty(EE_MODEL);
    }
  }

  if(s_editMode)
  {
    for(uint8_t i=0; i<(cv9 ? 9 : 5); i++)
    {
      uint8_t xx = XD-1-WCHART+i*WCHART/(cv9 ? 4 : 2);
      uint8_t yy = Y0-crv[i]*WCHART/100;


      if(subSub==(i+1))
      {
        if((yy-2)<WCHART*2) lcd_hline( xx-1, yy-2, 5); //do selection square
        if((yy-1)<WCHART*2) lcd_hline( xx-1, yy-1, 5);
        if(yy<WCHART*2)     lcd_hline( xx-1, yy  , 5);
        if((yy+1)<WCHART*2) lcd_hline( xx-1, yy+1, 5);
        if((yy+2)<WCHART*2) lcd_hline( xx-1, yy+2, 5);

        if(p1valdiff || event==EVT_KEY_FIRST(KEY_DOWN) || event==EVT_KEY_FIRST(KEY_UP) || event==EVT_KEY_REPT(KEY_DOWN) || event==EVT_KEY_REPT(KEY_UP))
           CHECK_INCDEC_H_MODELVAR( event, crv[i], -100,100);  // edit on up/down
      }
      else
      {
          if((yy-1)<WCHART*2) lcd_hline( xx, yy-1, 3); // do markup square
          if(yy<WCHART*2)     lcd_hline( xx, yy  , 3);
          if((yy+1)<WCHART*2) lcd_hline( xx, yy+1, 3);
      }
    }
  }

  for (uint8_t xv = 0; xv < WCHART * 2; xv++) {
    uint16_t yv = intpol(xv * (RESXu / WCHART) - RESXu, s_curveChan) / (RESXu
                                                                      / WCHART);
    lcd_plot(XD + xv - WCHART, Y0 - yv);
    if ((xv & 3) == 0) {
      lcd_plot(XD + xv - WCHART, Y0 + 0);
    }
  }
  lcd_vline(XD, Y0 - WCHART, WCHART * 2);
}



void menuProcCurve(uint8_t event)
{
  SIMPLE_MENU("CURVES", menuTabModel, e_Curve, 1+MAX_CURVE5+MAX_CURVE9);

  int8_t  sub    = mstate2.m_posVert - 1;

  switch (event) {
    case EVT_KEY_FIRST(KEY_RIGHT):
    case EVT_KEY_FIRST(KEY_MENU):
      if (sub >= 0) {
        s_curveChan = sub;
        pushMenu(menuProcCurveOne);
      }
      break;
  }

  uint8_t y    = 1*FH;
  uint8_t yd   = 1;
  uint8_t m    = 0;
  for (uint8_t i = 0; i < 7; i++) {
    uint8_t k = i + s_pgOfs;
    uint8_t attr = sub == k ? INVERS : 0;
    bool    cv9 = k >= MAX_CURVE5;

    if(cv9 && (yd>6)) break;
    if(yd>7) break;
    if(!m) m = attr;
    lcd_putsAtt(   FW*0, y,PSTR("CV"),attr);
    lcd_outdezAtt( (k<9) ? FW*3 : FW*4-1, y,k+1 ,attr);

    int8_t *crv = cv9 ? g_model.curves9[k-MAX_CURVE5] : g_model.curves5[k];
    for (uint8_t j = 0; j < (5); j++) {
      lcd_outdezAtt( j*(3*FW+3) + 7*FW, y, crv[j], 0);
    }
    y += FH;yd++;
    if(cv9){
      for (uint8_t j = 0; j < 4; j++) {
        lcd_outdezAtt( j*(3*FW+3) + 7*FW, y, crv[j+5], 0);
      }
      y += FH;yd++;
    }
  }

  if(!m) s_pgOfs++;
}

void setStickCenter() // copy state of 3 primary to subtrim
{
	int16_t zero_chans512_before[NUM_CHNOUT];
	int16_t zero_chans512_after[NUM_CHNOUT];

	perOut(zero_chans512_before, NO_TRAINER + NO_INPUT); // do output loop - zero input channels
	perOut(zero_chans512_after, NO_TRAINER); // do output loop - actual input channels

	for (uint8_t i = 0; i < NUM_CHNOUT; i++) {
		int16_t v = g_model.limitData[i].offset;
		v += g_model.limitData[i].revert ? (zero_chans512_before[i]
				- zero_chans512_after[i]) : (zero_chans512_after[i]
				- zero_chans512_before[i]);
		g_model.limitData[i].offset = max(min(v, 1000), -1000); // make sure the offset doesn't go haywire
	}

	for (uint8_t i = 0; i < 4; i++)
		if (!IS_THROTTLE(i))
			g_model.trim[0][i] = 0;// set trims to zero.
	STORE_MODELVARS;
	beepWarn1();
}

void menuProcLimits(uint8_t event)
{
  MENU("LIMITS", menuTabModel, e_Limits, NUM_CHNOUT+2, {0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 0});

  static bool swVal[NUM_CHNOUT];

  uint8_t y = 0;
  uint8_t k = 0;
  int8_t  sub    = mstate2.m_posVert - 1;
  uint8_t subSub = mstate2.m_posHorz;

  switch(event)
  {
    case EVT_KEY_LONG(KEY_MENU):
      if(sub>=0 && sub<NUM_CHNOUT) {
          int16_t v = g_chans512[sub - s_pgOfs];
          LimitData *ld = &g_model.limitData[sub];
          switch (subSub) {
          case 0:
              ld->offset = (ld->revert) ? -v : v;
              STORE_MODELVARS;
              break;
          }
      }
      break;
  }
  // TODO BSS #if defined (LABELS) lcd_puts_P( 4*FW, 1*FH,PSTR("subT min  max inv"));
  for(uint8_t i=0; i<7; i++){
    y=(i+1)*FH;
    k=i+s_pgOfs;
    if(k==NUM_CHNOUT) break;
    LimitData *ld = limitaddress( k ) ;
    int16_t v = (ld->revert) ? -ld->offset : ld->offset;
    if((g_chans512[k] - v) >  50) swVal[k] = (true==ld->revert);// Switch to raw inputs?  - remove trim!
    if((g_chans512[k] - v) < -50) swVal[k] = (false==ld->revert);
    putsChn(0,y,k+1,0);
    lcd_putcAtt(12*FW+FW/2, y, (swVal[k] ? 127 : 126),0); //'<' : '>'
    for(uint8_t j=0; j<4;j++){
      uint8_t attr = ((sub==k && subSub==j) ? (s_editMode ? BLINK : INVERS) : 0);
      switch(j)
      {
        case 0:
          lcd_outdezAtt(  8*FW, y,  ld->offset, attr|PREC1);
          if(attr && (s_editMode || p1valdiff)) {
            ld->offset = checkIncDec(event, ld->offset, -1000, 1000, EE_MODEL);
          }
          break;
        case 1:
          lcd_outdezAtt(  12*FW, y, (int8_t)(ld->min-100),   attr);
          if(attr && (s_editMode || p1valdiff)) {
            ld->min -=  100;
            // TODO BSS extended limits
            CHECK_INCDEC_H_MODELVAR( event, ld->min, -100,100);
            ld->min +=  100;
          }
          break;
        case 2:
          lcd_outdezAtt( 17*FW, y, (int8_t)(ld->max+100),    attr);
          if(attr && (s_editMode || p1valdiff)) {
            ld->max +=  100;
            CHECK_INCDEC_H_MODELVAR( event, ld->max, -100,100);
            ld->max -=  100;
          }
          break;
        case 3:
          lcd_putsnAtt(   18*FW, y, PSTR("---INV")+ld->revert*3,3,attr);
          if(attr && (s_editMode || p1valdiff)) {
            CHECK_INCDEC_H_MODELVAR(event, ld->revert, 0, 1);
          }
          break;
      }
    }
  }
  if(k==NUM_CHNOUT){
    //last line available - add the "copy trim menu" line
    uint8_t attr = (sub==NUM_CHNOUT) ? INVERS : 0;
    lcd_putsAtt(  3*FW,y,PSTR("COPY TRIM [MENU]"),s_noHi ? 0 : attr);
    if(attr && event==EVT_KEY_LONG(KEY_MENU)) {
      s_noHi = NO_HI_LEN;
      killEvents(event);
      setStickCenter(); //if highlighted and menu pressed - copy trims
    }
  }
}

#ifdef FRSKY
void menuProcTelemetry(uint8_t event)
{
  MENU("TELEMETRY", menuTabModel, e_Telemetry, 13, {0, -1, 1, 0, 1, 2, 2, -1, 1, 0, 1, 2/*, 2*/});

  int8_t  sub    = mstate2.m_posVert;
  uint8_t subSub = mstate2.m_posHorz;
  uint8_t blink;
  uint8_t y;

  switch(event){
    case EVT_KEY_BREAK(KEY_DOWN):
    case EVT_KEY_BREAK(KEY_UP):
    case EVT_KEY_BREAK(KEY_LEFT):
    case EVT_KEY_BREAK(KEY_RIGHT):
      if(s_editMode)
        FRSKY_setModelAlarms(); // update Fr-Sky module when edit mode exited
  }

  blink = s_editMode ? BLINK : INVERS ;
  uint8_t subN = 1;
  uint8_t t;
  int16_t val;

  for (int i=0; i<2; i++) {
    if(s_pgOfs<subN) {
      y=(subN-s_pgOfs)*FH;
      lcd_putsAtt(0, y, PSTR("A  channel"), 0);
      lcd_outdezAtt(2*FW, y, 1+i, 0);
    }
    subN++;

    if(s_pgOfs<subN) {
      y=(subN-s_pgOfs)*FH;
      lcd_putsAtt(4, y, PSTR("Max"), 0);
      putsTelemetry(8*FW, y, g_model.frsky.channels[i].ratio, g_model.frsky.channels[i].type, (sub==subN && subSub==0 ? blink:0)|NO_UNIT|LEFT);
      lcd_putsnAtt(lcd_lastPos, y, PSTR("v-")+g_model.frsky.channels[i].type, 1, (sub==subN && subSub==1 ? blink:0));
      if (sub==subN && (s_editMode || p1valdiff)) {
        if (subSub == 0)
          g_model.frsky.channels[i].ratio = checkIncDec(event, g_model.frsky.channels[i].ratio, 0, 255, EE_MODEL);
        else
          CHECK_INCDEC_H_MODELVAR(event, g_model.frsky.channels[i].type, 0, 1);
      }
    }
    subN++;

    if(s_pgOfs<subN) {
      y=(subN-s_pgOfs)*FH;
      lcd_putsAtt(4, y, PSTR("Calib"), 0);
      val = ((int16_t)frskyTelemetry[i].value+g_model.frsky.channels[i].offset)*g_model.frsky.channels[i].ratio / 255;
      putsTelemetry(8*FW, y, val, g_model.frsky.channels[i].type, (sub==subN ? blink:0)|LEFT);
      if(sub==subN) CHECK_INCDEC_H_MODELVAR(event, g_model.frsky.channels[i].offset, -127, 127);
    }
    subN++;

    if(s_pgOfs<subN) {
      y=(subN-s_pgOfs)*FH;
      lcd_puts_P(4, y, PSTR("Bar"));
      val = ((int16_t)g_model.frsky.channels[i].barMin+g_model.frsky.channels[i].offset)*g_model.frsky.channels[i].ratio / 255;
      putsTelemetry(8*FW, y, val, g_model.frsky.channels[i].type, (sub==subN && subSub==0 ? blink:0)|LEFT);
      val = ((int16_t)g_model.frsky.channels[i].barMax+g_model.frsky.channels[i].offset)*g_model.frsky.channels[i].ratio / 255;
      putsTelemetry(13*FW, y, val, g_model.frsky.channels[i].type, (sub==subN && subSub==1 ? blink:0)|LEFT);
      if(sub==subN && subSub==0 && (s_editMode || p1valdiff)) g_model.frsky.channels[i].barMin = checkIncDec(event, g_model.frsky.channels[i].barMin, 0, 255, EE_MODEL);
      if(sub==subN && subSub==1 && (s_editMode || p1valdiff)) g_model.frsky.channels[i].barMax = checkIncDec(event, g_model.frsky.channels[i].barMax, 0, 255, EE_MODEL);
    }
    subN++;

    for (int j=0; j<2; j++) {
      if(s_pgOfs<subN) {
        y=(subN-s_pgOfs)*FH;
        lcd_putsAtt(4, y, PSTR("Alarm"), 0);
        lcd_putsnAtt(8*FW, y, PSTR("---YelOrgRed")+3*ALARM_LEVEL(i, j),3,(sub==subN && subSub==0 ? blink:0));
        lcd_putsnAtt(13*FW, y, PSTR("<>")+ALARM_GREATER(i, j),1,(sub==subN && subSub==1 ? blink:0));
        uint8_t alarmValue = ((uint16_t)g_model.frsky.channels[i].alarms_value[j] * g_model.frsky.channels[i].ratio) / 255;
        putsTelemetry(17*FW, y, alarmValue, g_model.frsky.channels[i].type, (sub==subN && subSub==2 ? blink:0));

        if(sub==subN && (s_editMode || p1valdiff)) {
          switch (subSub) {
           case 0:
             t = ALARM_LEVEL(i, j);
             g_model.frsky.channels[i].alarms_level = (g_model.frsky.channels[i].alarms_level & ~(3<<(2*j))) + (checkIncDec(event, t, 0, 3, EE_MODEL) << (2*j));
             break;
           case 1:
             t = ALARM_GREATER(i, j);
             g_model.frsky.channels[i].alarms_greater = (g_model.frsky.channels[i].alarms_greater & ~(1<<j)) + (checkIncDec(event, t, 0, 1, EE_MODEL) << j);
             if(checkIncDec_Ret)
               FRSKY_setModelAlarms();
             break;
           case 2:
             g_model.frsky.channels[i].alarms_value[j] = checkIncDec(event, g_model.frsky.channels[i].alarms_value[j], 0, 255, EE_MODEL);
             break;
          }
        }
      }
      subN++;
    }
  }
}
#endif

#ifdef TEMPLATES
void menuProcTemplates(uint8_t event)
{
  SIMPLE_MENU("TEMPLATES", menuTabModel, e_Templates, NUM_TEMPLATES+3);

  uint8_t y = 0;
  uint8_t k = 0;
  int8_t  sub    = mstate2.m_posVert - 1;

  switch(event)
  {
    case EVT_KEY_LONG(KEY_MENU):
      killEvents(event);
      //apply mixes or delete
      s_noHi = NO_HI_LEN;
      if(sub==NUM_TEMPLATES+1)
        clearMixes();
      else if((sub>=0) && (sub<(int8_t)NUM_TEMPLATES))
        applyTemplate(sub);
      beepWarn1();
      break;
  }

  y=1*FH;
  for(uint8_t i=0; i<7; i++){
    k=i+s_pgOfs;
    if(k==NUM_TEMPLATES) break;

    //write mix names here
    lcd_outdezNAtt(3*FW, y, k+1, (sub==k ? INVERS : 0) + LEADING0,2);
    lcd_putsAtt(  4*FW, y, n_Templates[k],BSS | (s_noHi ? 0 : (sub==k ? INVERS  : 0)));
    y+=FH;
  }

  if(y>7*FH) return;
  uint8_t attr = s_noHi ? 0 : ((sub==NUM_TEMPLATES) ? INVERS : 0);
  lcd_puts_P( 1*FW, y,PSTR("Channel Order"));//   RAET->AETR

  {
    uint8_t i ;
    for ( i = 1 ; i <= 4 ; i += 1 ) {
      lcd_putsnAtt((14+i)*FW, y, PSTR(" RETA")+CHANNEL_ORDER(i),1,attr);
    }
  }

  if(attr) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.templateSetup, 0, 23);
  y+=FH;

  if(y>7*FH) return;
  attr = s_noHi ? 0 : ((sub==NUM_TEMPLATES+1) ? INVERS : 0);
  lcd_putsAtt(  1*FW,y,PSTR("CLEAR MIXES [MENU]"),attr);
  y+=FH;
}
#endif

void menuProcSafetySwitches(uint8_t event)
{
  MENU("SAFETY SWITCHES", menuTabModel, e_SafetySwitches, NUM_CHNOUT+1, {0, 2/*repeated*/});

  uint8_t y = 0;
  uint8_t k = 0;
  int8_t  sub    = mstate2.m_posVert - 1;
  uint8_t subSub = mstate2.m_posHorz;

//  lcd_puts_P( 0*FW, 1*FH,PSTR("ch    sw     val"));
  for(uint8_t i=0; i<7; i++){
    y=(i+1)*FH;
    k=i+s_pgOfs;
    if(k==NUM_CHNOUT) break;
    SafetySwData *sd = &g_model.safetySw[k];
    putsChn(0,y,k+1,0);
    for(uint8_t j=0; j<=2;j++){
      uint8_t attr = ((sub==k && subSub==j) ? (s_editMode ? BLINK : INVERS) : 0);
      switch(j)
      {
      case 0:
          putsDrSwitches(5*FW, y, sd->swtch  , attr);
          if(attr && (s_editMode || p1valdiff)) {
              CHECK_INCDEC_H_MODELVAR( event, sd->swtch, -MAX_DRSWITCH,MAX_DRSWITCH);
          }
          break;
      case 1:
          lcd_outdezAtt(  16*FW, y, sd->val,   attr);
          if(attr && (s_editMode || p1valdiff)) {
              CHECK_INCDEC_H_MODELVAR( event, sd->val, -125,125);
          }
          break;
      }
    }
  }

}

void menuProcCustomSwitches(uint8_t event)
{
  MENU("CUSTOM SWITCHES", menuTabModel, e_CustomSwitches, NUM_CSW+1, {0, 2/*repeated...*/});

  uint8_t y = 0;
  uint8_t k = 0;
  int8_t  sub    = mstate2.m_posVert - 1;
  uint8_t subSub = mstate2.m_posHorz;

  // TODO BSS if defined(LABELS) lcd_puts_P( 4*FW, 1*FH,PSTR("Function V1  V2"));
  for(uint8_t i=0; i<7; i++){
    y=(i+1)*FH;
    k=i+s_pgOfs;
    if(k==NUM_CSW) break;
    uint8_t attr = (sub==k ? (s_editMode ? BLINK : INVERS)  : 0);
    CustomSwData &cs = g_model.customSw[k];

    //write SW names here
    lcd_putsnAtt( 0*FW , y, PSTR("SW"),2,0);
    lcd_putc(  2*FW , y, k + (k>8 ? 'A'-9: '1'));
    lcd_putsnAtt( 4*FW , y, PSTR(CSWITCH_STR)+CSW_LEN_FUNC*cs.func,CSW_LEN_FUNC,subSub==0 ? attr : 0);

    uint8_t cstate = CS_STATE(cs.func);

    if(cstate == CS_VOFS)
    {
        putsChnRaw(    12*FW, y, cs.v1  ,subSub==1 ? attr : 0);
#if defined(FRSKY)
        if (cs.v1 > CHOUT_BASE+NUM_CHNOUT)
          lcd_outdezAtt( 20*FW, y, 125+cs.v2  ,subSub==2 ? attr : 0);
        else
#endif
        lcd_outdezAtt( 20*FW, y, cs.v2  ,subSub==2 ? attr : 0);
    }
    else if(cstate == CS_VBOOL)
    {
        putsDrSwitches(12*FW, y, cs.v1  ,subSub==1 ? attr : 0);
        putsDrSwitches(16*FW, y, cs.v2  ,subSub==2 ? attr : 0);
    }
    else // cstate == CS_COMP
    {
        putsChnRaw(    12*FW, y, cs.v1  ,subSub==1 ? attr : 0);
        putsChnRaw(    17*FW, y, cs.v2  ,subSub==2 ? attr : 0);
    }

    if((s_editMode || p1valdiff) && attr)
      switch (subSub) {
        case 0:
          CHECK_INCDEC_H_MODELVAR( event, cs.func, 0,CS_MAXF);
          if(cstate != CS_STATE(cs.func))
          {
              cs.v1  = 0;
              cs.v2 = 0;
          }
          break;
        case 1:
          switch (cstate) {
          case (CS_VOFS):
              CHECK_INCDEC_H_MODELVAR( event, cs.v1, 0,NUM_XCHNRAW);
              break;
          case (CS_VBOOL):
              CHECK_INCDEC_H_MODELVAR( event, cs.v1, -MAX_DRSWITCH,MAX_DRSWITCH);
              break;
          case (CS_VCOMP):
              CHECK_INCDEC_H_MODELVAR( event, cs.v1, 0,NUM_XCHNRAW);
              break;
          default:
              break;
          }
          break;
        case 2:
          switch (cstate) {
          case (CS_VOFS):
              CHECK_INCDEC_H_MODELVAR( event, cs.v2, -125,125);
              break;
          case (CS_VBOOL):
              CHECK_INCDEC_H_MODELVAR( event, cs.v2, -MAX_DRSWITCH,MAX_DRSWITCH);
              break;
          case (CS_VCOMP):
              CHECK_INCDEC_H_MODELVAR( event, cs.v2, 0,NUM_XCHNRAW);
              break;
          default:
              break;
          }
      }
  }
}

static int8_t s_currMixIdx;
static int8_t s_currDestCh;
static bool   s_currMixInsMode;


void deleteMix(uint8_t idx)
{
  memmove(&g_model.mixData[idx],&g_model.mixData[idx+1],
            (MAX_MIXERS-(idx+1))*sizeof(MixData));
  memset(&g_model.mixData[MAX_MIXERS-1],0,sizeof(MixData));
  STORE_MODELVARS;
}

void insertMix(uint8_t idx)
{
  memmove(&g_model.mixData[idx+1],&g_model.mixData[idx],
         (MAX_MIXERS-(idx+1))*sizeof(MixData) );
  memset(&g_model.mixData[idx],0,sizeof(MixData));
  g_model.mixData[idx].destCh      = s_currDestCh; //-s_mixTab[sub];
  g_model.mixData[idx].srcRaw      = s_currDestCh; //1;   //
  g_model.mixData[idx].weight      = 100;
  STORE_MODELVARS;
}

void menuProcMixOne(uint8_t event)
{
  SIMPLE_SUBMENU_NOTITLE(13);
  uint8_t x=TITLEP(s_currMixInsMode ? PSTR("INSERT MIX ") : PSTR("EDIT MIX "));
  MixData *md2 = mixaddress( s_currMixIdx ) ;
  putsChn(x+1*FW,0,md2->destCh,0);
  int8_t  sub    = mstate2.m_posVert;

  for(uint8_t k=0; k<7; k++)
  {
    uint8_t y = (k+1) * FH;
    uint8_t i = k + s_pgOfs;
    uint8_t attr = sub==i ? INVERS : 0;
    switch(i) {
      case 0:
        lcd_puts_P(  2*FW,y,PSTR("Source"));
        putsChnRaw(   FW*10,y,md2->srcRaw,attr);
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->srcRaw, 1,NUM_XCHNRAW);
        break;
      case 1:
        lcd_puts_P(  2*FW,y,PSTR("Weight"));
        lcd_outdezAtt(FW*10,y,md2->weight,attr|LEFT);
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->weight, -125,125);
        break;
      case 2:
        lcd_puts_P(  2*FW,y,PSTR("Offset"));
        lcd_outdezAtt(FW*10,y,md2->sOffset,attr|LEFT);
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->sOffset, -125,125);
        break;
      case 3:
        lcd_puts_P(  2*FW,y,PSTR("Trim"));
        lcd_putsnAtt(FW*10,y, PSTR("ON OFF")+3*md2->carryTrim,3,attr);  //default is 0=ON
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->carryTrim, 0,1);
        break;
      case 4:
        lcd_putsAtt(  2*FW,y,PSTR("Curves"),0);
        lcd_putsnAtt( FW*10,y,PSTR(CURV_STR)+md2->curve*3,3,attr);
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->curve, 0,MAX_CURVE5+MAX_CURVE9+7-1);
        if(attr && md2->curve>=CURVE_BASE && event==EVT_KEY_FIRST(KEY_MENU)){
          s_curveChan = md2->curve-CURVE_BASE;
          pushMenu(menuProcCurveOne);
        }
        break;
      case 5:
        lcd_puts_P(  2*FW,y,PSTR("Switch"));
        putsDrSwitches(9*FW,  y,md2->swtch,attr);
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->swtch, -MAX_DRSWITCH, MAX_DRSWITCH);
        break;
      case 6:
        lcd_puts_P(  2*FW,y,PSTR("Warning"));
        if(md2->mixWarn)
          lcd_outdezAtt(FW*10,y,md2->mixWarn,attr|LEFT);
        else
          lcd_putsAtt(  FW*10,y,PSTR("OFF"),attr);
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->mixWarn, 0,3);
        break;
      case 7:
        lcd_puts_P(  2*FW,y,PSTR("Multpx"));
        lcd_putsnAtt(10*FW, y,PSTR("Add     MultiplyReplace ")+8*md2->mltpx,8,attr);
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->mltpx, 0, 2);
        break;
      case 8:
        lcd_puts_P(  2*FW,y,PSTR("Delay Down"));
        lcd_outdezAtt(FW*16,y,md2->delayDown,attr);
        if(attr)  CHECK_INCDEC_H_MODELVAR( event, md2->delayDown, 0,15);
        break;
      case 9:
        lcd_puts_P(  2*FW,y,PSTR("Delay Up"));
        lcd_outdezAtt(FW*16,y,md2->delayUp,attr);
        if(attr)  CHECK_INCDEC_H_MODELVAR( event, md2->delayUp, 0,15);
        break;
      case 10:
        lcd_puts_P(  2*FW,y,PSTR("Slow  Down"));
        lcd_outdezAtt(FW*16,y,md2->speedDown,attr);
        if(attr)  CHECK_INCDEC_H_MODELVAR( event, md2->speedDown, 0,15);
        break;
      case 11:
        lcd_puts_P(  2*FW,y,PSTR("Slow  Up"));
        lcd_outdezAtt(FW*16,y,md2->speedUp,attr);
        if(attr)  CHECK_INCDEC_H_MODELVAR( event, md2->speedUp, 0,15);
        break;
      case 12:   lcd_putsAtt(  2*FW,y,PSTR("DELETE MIX [MENU]"),attr);
        if(attr && event==EVT_KEY_LONG(KEY_MENU)){
          killEvents(event);
          deleteMix(s_currMixIdx);
          beepWarn1();
          popMenu();
        }
        break;
    }
  }
}

struct MixTab{
  bool   showCh:1;// show the dest chn
  bool   hasDat:1;// show the data info
  int8_t chId;    //:4  1..NUM_XCHNOUT  dst chn id
  int8_t selCh;   //:5  1..MAX_MIXERS+NUM_XCHNOUT sel sequence
  int8_t selDat;  //:5  1..MAX_MIXERS+NUM_XCHNOUT sel sequence
  int8_t insIdx;  //:5  0..MAX_MIXERS-1        insert index into mix data tab
  int8_t editIdx; //:5  0..MAX_MIXERS-1        edit   index into mix data tab
} s_mixTab[MAX_MIXERS+NUM_XCHNOUT+1];
int8_t s_mixMaxSel;

void genMixTab()
{
  uint8_t maxDst  = 0;
  uint8_t mtIdx   = 0;
  uint8_t sel     = 1;
  memset(s_mixTab,0,sizeof(s_mixTab));

  MixData *md=g_model.mixData;

  for(uint8_t i=0; i<MAX_MIXERS; i++)
  {
    uint8_t destCh = md[i].destCh;
    if(destCh==0) destCh=NUM_XCHNOUT;
    if(destCh > maxDst){
      while(destCh > maxDst){ //ch-loop, hole alle channels auf
        maxDst++;
        s_mixTab[mtIdx].chId  = maxDst; //mark channel header
        s_mixTab[mtIdx].showCh = true;
        s_mixTab[mtIdx].selCh = sel++; //vorab vergeben, falls keine dat
        s_mixTab[mtIdx].insIdx= i;     //
        mtIdx++;
      }
      mtIdx--; //folding: letztes ch bekommt zusaetzlich dat
      s_mixMaxSel =sel;
      sel--; //letzte zeile hat dat, falls nicht ist selCh schon belegt
    }
    if(md[i].destCh==0) break; //letzter eintrag in mix data tab
    s_mixTab[mtIdx].chId    = destCh; //mark channel header
    s_mixTab[mtIdx].editIdx = i;
    s_mixTab[mtIdx].hasDat  = true;
    s_mixTab[mtIdx].selDat  = sel++;
    if(md[i].destCh == md[i+1].destCh){
      s_mixTab[mtIdx].selCh  = 0; //ueberschreibt letzte Zeile von ch-loop
      s_mixTab[mtIdx].insIdx = 0; //
    }
    else{
      s_mixTab[mtIdx].selCh  = sel++;
      s_mixTab[mtIdx].insIdx = i+1; //
    }
    s_mixMaxSel =sel;
    mtIdx++;
  }
}

uint8_t getMixerCount()
{
  uint8_t mixerCount = 0;
	uint8_t dch ;

  for(uint8_t i=0;i<MAX_MIXERS;i++)
  {
		dch = g_model.mixData[i].destCh ;
    if ((dch!=0) && (dch<=NUM_CHNOUT)) 
    {
      mixerCount++;
    }
  }
  return mixerCount;
}

void menuMixersLimit(uint8_t event)
{
  lcd_puts_P(0,2*FH, PSTR("Max mixers reach: "));
  lcd_outdezAtt(20*FW, 2*FH, getMixerCount(),0);

  lcd_puts_P(0,4*FH, PSTR("Press [EXIT] to abort"));
  switch(event)
  {
    case  EVT_KEY_FIRST(KEY_EXIT):
      killEvents(event);
      popMenu(true);
      pushMenu(menuProcMix);
    break;
  }
}

bool reachMixerCountLimit()
{
  // check mixers count limit
  if (getMixerCount() >= MAX_MIXERS)
  {
    pushMenu(menuMixersLimit);
    return true;
  }
  else
  {
    return false;
  }
}

static void memswap( void *a, void *b, uint8_t size )
{
  uint8_t *x ;
  uint8_t *y ;
  uint8_t temp ;

  x = (unsigned char *) a ;
  y = (unsigned char *) b ;
  while ( size-- ) {
    temp = *x ;
    *x++ = *y ;
    *y++ = temp ;
  }
}

void moveMix(uint8_t idx,uint8_t mcopy, uint8_t dir) //true=inc=down false=dec=up - Issue 49
{
  if(idx>MAX_MIXERS || (idx==0 && !dir) || (idx==MAX_MIXERS && dir)) return;
  uint8_t tdx = dir ? idx+1 : idx-1;
  MixData *src= mixaddress( idx ) ;
  MixData *tgt= mixaddress( tdx ) ;

  if((src->destCh==0) || (src->destCh>NUM_CHNOUT) || (tgt->destCh>NUM_CHNOUT)) return;

  if (mcopy) {
    if (reachMixerCountLimit()) return;
    memmove(&g_model.mixData[idx+1],&g_model.mixData[idx], (MAX_MIXERS-(idx+1))*sizeof(MixData) );
    return;
  }

  if(tgt->destCh!=src->destCh) {
    if ((dir)  && (src->destCh<NUM_CHNOUT)) src->destCh++;
    if ((!dir) && (src->destCh>0))          src->destCh--;
    return;
  }

  //flip between idx and tgt
  memswap( tgt, src, sizeof(MixData) ) ;
  STORE_MODELVARS;
}

void menuProcMix(uint8_t event)
{
  uint8_t sub = s_pgOfs; // keep the previous offset position
  SIMPLE_MENU("MIXER", menuTabModel, e_Mix, s_mixMaxSel);
  s_pgOfs = sub;

  sub    = mstate2.m_posVert;
  static uint8_t s_moveMode;
  static uint8_t s_sourceMoveMix;
  static uint8_t s_copyMix;
  if(sub==0) s_moveMode = false;

  MixData *md=g_model.mixData;
  switch(event)
  {
    case EVT_ENTRY:
      s_sourceMoveMix=0;
      s_copyMix=true;
      s_moveMode=false;
    case EVT_ENTRY_UP:
      genMixTab();
      break;
    case EVT_KEY_FIRST(KEY_MENU):
      if(sub>=1) s_moveMode = !s_moveMode;
      s_copyMix = s_moveMode;
      if(s_currMixInsMode) s_moveMode = false; //cant have edit mode if mix not selected
      break;
    case EVT_KEY_REPT(KEY_UP):
    case EVT_KEY_FIRST(KEY_UP):
      s_copyMix = false;  //only copy if going down
    case EVT_KEY_REPT(KEY_DOWN):
    case EVT_KEY_FIRST(KEY_DOWN):
      if(s_moveMode) {
        //killEvents(event);
        moveMix(s_currMixIdx,s_copyMix,event==EVT_KEY_FIRST(KEY_DOWN)); //true=inc=down false=dec=up
        if(s_copyMix && event==EVT_KEY_FIRST(KEY_UP)) sub--;
        s_copyMix = false;
        genMixTab();
      }
      break;
    case EVT_KEY_FIRST(KEY_EXIT):
      if(s_moveMode) {
        deleteMix(s_currMixIdx);
        s_moveMode = false;
        genMixTab();
        killEvents(event);
      }
      break;
    case EVT_KEY_LONG(KEY_MENU):
      if(sub<1) break;
      if(s_currMixInsMode && !reachMixerCountLimit())
        insertMix(s_currMixIdx);
      s_moveMode=false;
      pushMenu(menuProcMixOne);
      break;
  }

  int8_t markedIdx=-1;
  uint8_t i;
  int8_t minSel=99;
  int8_t maxSel=-1;
  // TODO if defined (LABELS) lcd_puts_P( 6*FW, 1*FH,PSTR("wt src  sw crv"));
  for(i=0; i<7; i++){
    uint8_t y = i * FH + FH;
    uint8_t k = i + s_pgOfs;
    if(!s_mixTab[k].showCh && !s_mixTab[k].hasDat ) break;

    if(s_mixTab[k].showCh){
      putsChn(0,y,s_mixTab[k].chId, 0); // show CHx
    }
    if(sub>0 && sub==s_mixTab[k].selCh) { //handle CHx is selected (other line)
      //if(BLINK_ON_PHASE)
      lcd_hline(0,y+7,FW*4);
      s_currMixIdx     = s_mixTab[k].insIdx;
      s_currDestCh     = s_mixTab[k].chId;
      s_currMixInsMode = true;
      markedIdx        = i;
    }

    if(s_mixTab[k].hasDat){ //show data
      MixData *md2=&md[s_mixTab[k].editIdx];
      uint8_t attr = sub==s_mixTab[k].selDat ? INVERS : 0;
      if(!s_mixTab[k].showCh)
        lcd_putsnAtt(   3*FW, y, PSTR("+*R")+1*md2->mltpx,1,s_moveMode ? attr : 0);
      lcd_outdezAtt(  7*FW+FW/2, y, md2->weight,attr);
      lcd_putcAtt(    7*FW+FW/2, y, '%',s_moveMode ? attr : 0);
      putsChnRaw(     9*FW, y, md2->srcRaw,s_moveMode ? attr : 0);
      if(md2->swtch)putsDrSwitches( 13*FW, y, md2->swtch,s_moveMode ? attr : 0);
      if(md2->curve)lcd_putsnAtt(   17*FW, y, PSTR(CURV_STR)+md2->curve*3,3,s_moveMode ? attr : 0);

      bool bs = (md2->speedDown || md2->speedUp);
      bool bd = (md2->delayUp || md2->delayDown);
      char cs = (bs && bd) ? '*' : (bs ? 'S' : 'D');
      if(bs || bd) lcd_putcAtt(20*FW+1, y, cs,s_editMode ? attr : 0);

      if(attr == INVERS){ //handle dat is selected
        CHECK_INCDEC_H_MODELVAR( event, md2->weight, -125,125);
        s_currMixIdx     = s_mixTab[k].editIdx;
        s_currDestCh     = s_mixTab[k].chId;
        s_currMixInsMode = false;
        markedIdx        = i;
        if(!s_moveMode) s_sourceMoveMix = k;
      }
    }
    //welche sub-indize liegen im sichtbaren bereich?
    if(s_mixTab[k].selCh){
      minSel = min(minSel,s_mixTab[k].selCh);
      maxSel = max(maxSel,s_mixTab[k].selCh);
    }
    if(s_mixTab[k].selDat){
      minSel = min(minSel,s_mixTab[k].selDat);
      maxSel = max(maxSel,s_mixTab[k].selDat);
    }

  } //for 7
  if( sub!=0 &&  markedIdx==-1) { //versuche die Marke zu finden
    if(sub < minSel) s_pgOfs = max(0,s_pgOfs-1);
    if(sub > maxSel) s_pgOfs++;
  }
  else if(markedIdx<0)          s_pgOfs = max(0,s_pgOfs-1);
  else if(markedIdx>=6 && i>=8) s_pgOfs++;

}



uint16_t expou(uint16_t x, uint16_t k)
{
  // k*x*x*x + (1-k)*x
  return ((unsigned long)x*x*x/0x10000*k/(RESXul*RESXul/0x10000) + (RESKul-k)*x+RESKul/2)/RESKul;
}
// expo-funktion:
// ---------------
// kmplot
// f(x,k)=exp(ln(x)*k/10) ;P[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20]
// f(x,k)=x*x*x*k/10 + x*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]
// f(x,k)=x*x*k/10 + x*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]
// f(x,k)=1+(x-1)*(x-1)*(x-1)*k/10 + (x-1)*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]

int16_t expo(int16_t x, int16_t k)
{
  if(k == 0) return x;
  int16_t   y;
  bool    neg =  x < 0;
  if(neg)   x = -x;
  if(k<0){
    y = RESXu-expou(RESXu-x,-k);
  }else{
    y = expou(x,k);
  }
  return neg? -y:y;
}


#ifdef EXTENDED_EXPO
/// expo with y-offset
class Expo
{
  uint16_t   c;
  int16_t    d,drx;
public:
  void     init(uint8_t k, int8_t yo);
  static int16_t  expou(uint16_t x,uint16_t c, int16_t d);
  int16_t  expo(int16_t x);
};
void    Expo::init(uint8_t k, int8_t yo)
{
  c = (uint16_t) k  * 256 / 100;
  d = (int16_t)  yo * 256 / 100;
  drx = d * ((uint16_t)RESXu/256);
}
int16_t Expo::expou(uint16_t x,uint16_t c, int16_t d)
{
  uint16_t a = 256 - c - d;
  if( (int16_t)a < 0 ) a = 0;
  // a x^3 + c x + d
  //                         9  18  27        11  20   18
  uint32_t res =  ((uint32_t)x * x * x / 0x10000 * a / (RESXul*RESXul/0x10000) +
                   (uint32_t)x                   * c
  ) / 256;
  return (int16_t)res;
}
int16_t  Expo::expo(int16_t x)
{
  if(c==256 && d==0) return x;
  if(x>=0) return expou(x,c,d) + drx;
  return -expou(-x,c,-d) + drx;
}
#endif

static uint8_t s_expoChan;

void editExpoVals(uint8_t event,uint8_t stopBlink,uint8_t editMode, uint8_t edit,uint8_t x, uint8_t y, uint8_t chn, uint8_t which, uint8_t exWt, uint8_t stkRL)
{
  uint8_t  invBlk = edit ? (editMode ? BLINK : INVERS) : 0;
  if(edit && stopBlink) invBlk = INVERS;

  if(which==DR_DRSW1) {
    putsDrSwitches(x,y,g_model.expoData[chn].drSw1,invBlk);
    if(edit && (editMode || p1valdiff)) CHECK_INCDEC_H_MODELVAR(event,g_model.expoData[chn].drSw1,-MAX_DRSWITCH,MAX_DRSWITCH);
  }
  else if(which==DR_DRSW2) {
    putsDrSwitches(x,y,g_model.expoData[chn].drSw2,invBlk);
    if(edit && (editMode || p1valdiff)) CHECK_INCDEC_H_MODELVAR(event,g_model.expoData[chn].drSw2,-MAX_DRSWITCH,MAX_DRSWITCH);
  }
  else
    if(exWt==DR_EXPO){
      lcd_outdezAtt(x, y, g_model.expoData[chn].expo[which][exWt][stkRL], invBlk);
      if(edit && (editMode || p1valdiff)) CHECK_INCDEC_H_MODELVAR(event,g_model.expoData[chn].expo[which][exWt][stkRL],-100, 100);
    }
    else {
      lcd_outdezAtt(x, y, g_model.expoData[chn].expo[which][exWt][stkRL]+100, invBlk);
      if(edit && (editMode || p1valdiff)) CHECK_INCDEC_H_MODELVAR(event,g_model.expoData[chn].expo[which][exWt][stkRL],-100, 0);
    }
}

void menuProcExpoOne(uint8_t event)
{
  SUBMENU("EXPO/DR", 4, {0});
  
  static uint8_t stkVal;  
  putsChnRaw(8*FW,0,s_expoChan+1,0);
  int8_t  sub    = mstate2.m_posVert;

  uint8_t expoDrOn = GET_DR_STATE(s_expoChan);
  uint8_t  y = 16;

  if(calibratedStick[s_expoChan]> 25) stkVal = DR_RIGHT;
  if(calibratedStick[s_expoChan]<-25) stkVal = DR_LEFT;
  if(IS_THROTTLE(s_expoChan) && g_model.thrExpo) stkVal = DR_RIGHT;

  lcd_puts_P(0,y,PSTR("Expo"));
  editExpoVals(event,true,true,sub==0,9*FW, y,s_expoChan, expoDrOn ,DR_EXPO,stkVal);
  y+=FH;
  lcd_puts_P(0,y,PSTR("Weight"));
  editExpoVals(event,true,true,sub==1,9*FW, y,s_expoChan, expoDrOn ,DR_WEIGHT,stkVal);
  y+=FH;
  lcd_puts_P(0,y,PSTR("DrSw1"));
  editExpoVals(event,true,true,sub==2,5*FW, y,s_expoChan, DR_DRSW1 , 0,0);
  y+=FH;
  lcd_puts_P(0,y,PSTR("DrSw2"));
  editExpoVals(event,true,true,sub==3,5*FW, y,s_expoChan, DR_DRSW2 , 0,0);
  y+=FH;
  switch (expoDrOn) {
    case DR_MID:
      lcd_puts_P(0,y,PSTR("DR Mid"));
      break;
    case DR_LOW:
      lcd_puts_P(0,y,PSTR("DR Low"));
      break;
    default: // DR_HIGH:
      lcd_puts_P(0,y,PSTR("DR High"));
      break;
  }
  y+=FH;


  int8_t   kViewR  = g_model.expoData[s_expoChan].expo[expoDrOn][DR_EXPO][DR_RIGHT];  //NormR;
  int8_t   kViewL  = g_model.expoData[s_expoChan].expo[expoDrOn][DR_EXPO][DR_LEFT];  //NormL;
  int8_t   wViewR  = g_model.expoData[s_expoChan].expo[expoDrOn][DR_WEIGHT][DR_RIGHT]+100;  //NormWeightR+100;
  int8_t   wViewL  = g_model.expoData[s_expoChan].expo[expoDrOn][DR_WEIGHT][DR_LEFT]+100;  //NormWeightL+100;

  if (IS_THROTTLE(s_expoChan) && g_model.thrExpo)
       for(uint8_t xv=0;xv<WCHARTl*2;xv++)
    {
      uint16_t yv=2*expo(xv*(RESXu/WCHARTl)/2,kViewR) / (RESXu/WCHARTl);
      yv = (yv * wViewR)/100;
      lcd_plot(X0l+xv-WCHARTl, 2*Y0l-yv);
      if((xv&3) == 0){
        lcd_plot(X0l+xv-WCHARTl, 2*Y0l-1);
        lcd_plot(X0l-WCHARTl   , Y0l+xv/2);
      }
    }
  else
    for(uint8_t xv=0;xv<WCHARTl;xv++)
    {
      uint16_t yv=expo(xv*(RESXu/WCHARTl),kViewR) / (RESXu/WCHARTl);
      yv = (yv * wViewR)/100;
      lcd_plot(X0l+xv, Y0l-yv);
      if((xv&3) == 0){
        lcd_plot(X0l+xv, Y0l+0);
        lcd_plot(X0l  , Y0l+xv);
      }

      yv=expo(xv*(RESXu/WCHARTl),kViewL) / (RESXu/WCHARTl);
      yv = (yv * wViewL)/100;
      lcd_plot(X0l-xv, Y0l+yv);
      if((xv&3) == 0){
        lcd_plot(X0l-xv, Y0l+0);
        lcd_plot(X0l  , Y0l-xv);
      }
    }

  int32_t x512  = calibratedStick[s_expoChan];
  lcd_vline(X0l+x512/(RESXu/WCHARTl), Y0l-WCHARTl,WCHARTl*2);

  int32_t y512 = 0;
  if (IS_THROTTLE(s_expoChan) && g_model.thrExpo) {
    y512  = 2*expo((x512+RESX)/2,kViewR);
    y512 = y512 * (wViewR / 4)/(100 / 4);
    lcd_hline(X0l-WCHARTl, 2*Y0l-y512/(RESXu/WCHARTl),WCHARTl*2);
    y512 /= 2;
  }
  else {
    y512  = expo(x512,(x512>0 ? kViewR : kViewL));
    y512 = y512 * ((x512>0 ? wViewR : wViewL) / 4)/(100 / 4);
    lcd_hline(X0l-WCHARTl, Y0l-y512/(RESXu/WCHARTl),WCHARTl*2);
  }

  lcd_outdezAtt( 19*FW, 6*FH,x512*25/((signed) RESXu/4), 0 );
  lcd_outdezAtt( 14*FW, 1*FH,y512*25/((signed) RESXu/4), 0 );
  //dy/dx
  int16_t dy  = x512>0 ? y512-expo(x512-20,(x512>0 ? kViewR : kViewL)) : expo(x512+20,(x512>0 ? kViewR : kViewL))-y512;
  lcd_outdezNAtt(14*FW, 2*FH,   dy*(100/20), PREC2,3);
}

void menuProcExpoAll(uint8_t event)
{
  MENU("EXPO/DR", menuTabModel, e_ExpoAll, 5, {0, 3/*repeated*/});

  static uint8_t stkVal[4];
  int8_t  sub    = mstate2.m_posVert - 1;
  int8_t  subHor = mstate2.m_posHorz;

  switch(event)
  {
    case EVT_KEY_LONG(KEY_MENU):
      if(sub>=0){
        s_expoChan = sub;
        pushMenu(menuProcExpoOne);
      }
      break;
  }

  lcd_puts_P( 4*FW-FW/2, 1*FH,PSTR("exp  %  sw1 sw2"));
  for(uint8_t i=0; i<4; i++)
  {
    uint8_t expoDrOn = GET_DR_STATE(i);
    uint8_t valsEqual = (g_model.expoData[i].expo[expoDrOn][DR_WEIGHT][DR_LEFT]==g_model.expoData[i].expo[expoDrOn][DR_WEIGHT][DR_RIGHT]) &&
                        (g_model.expoData[i].expo[expoDrOn][DR_EXPO][DR_LEFT]==g_model.expoData[i].expo[expoDrOn][DR_EXPO][DR_RIGHT]);
    uint8_t stickCentred = (abs(calibratedStick[i])<=25) && valsEqual;
    if(calibratedStick[i]> 25) stkVal[i] = DR_RIGHT;
    if(calibratedStick[i]<-25) stkVal[i] = DR_LEFT;
    if(IS_THROTTLE(i) && g_model.thrExpo) {
      stkVal[i] = DR_RIGHT;
      stickCentred = true;
    }

    uint8_t y=(i+2)*FH;
    putsChnRaw( 0, y,i+1,0);
    uint8_t stkOp = (stkVal[i] == DR_RIGHT) ? DR_LEFT : DR_RIGHT;

    uint8_t edtm = (s_editMode || p1valdiff);
    editExpoVals(event,false,edtm,sub==i && subHor==0, 7*FW-FW/2, y,i,expoDrOn,DR_EXPO,stkVal[i]);
    if(sub==i && subHor==0 && edtm && stickCentred)
      CHECK_INCDEC_H_MODELVAR(event,g_model.expoData[i].expo[expoDrOn][DR_EXPO][stkOp],-100, 100);

    editExpoVals(event,false,edtm,sub==i && subHor==1, 9*FW+FW/2, y,i,expoDrOn,DR_WEIGHT,stkVal[i]);
    if(sub==i && subHor==1 && edtm && stickCentred)
      CHECK_INCDEC_H_MODELVAR(event,g_model.expoData[i].expo[expoDrOn][DR_WEIGHT][stkOp],-100, 0);

    editExpoVals(event,false,edtm,sub==i && subHor==2,10*FW+FW/2, y,i,DR_DRSW1,0,0);
    editExpoVals(event,false,edtm,sub==i && subHor==3,14*FW+FW/2, y,i,DR_DRSW2,0,0);
    lcd_putc(9*FW+FW/2 + ((!stkVal[i] && !stickCentred) ? 2 : 1 ), y, stickCentred ? '-' : (stkVal[i] ? 127 : 126));//'|' : (stkVal[i] ? '<' : '>'),0);
    switch (expoDrOn) { /* TODO BSS switch to remove */
    case DR_MID:
      lcd_putc(19*FW+FW/2,y,'M');
      break;
    case DR_LOW:
      lcd_putc(19*FW+FW/2,y,'L');
      break;
    default: // DR_HIGH:
      lcd_putc(19*FW+FW/2,y,'H');
      break;
    }
  }
}
const prog_char APM s_charTab[]=" ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.";
#define NUMCHARS (sizeof(s_charTab)-1)

uint8_t char2idx(char c)
{
  for(int8_t ret=0;;ret++)
  {
    char cc= pgm_read_byte(s_charTab+ret);
    if(cc==c) return ret;
    if(cc==0) return 0;
  }
}
char idx2char(uint8_t idx)
{
  if(idx < NUMCHARS) return pgm_read_byte(s_charTab+idx);
  return ' ';
}

// TODO some guys do like the confirmation for duplicate
void menuDeleteModel(uint8_t event)
{
  lcd_putsAtt(0,1*FH,PSTR("DELETE MODEL"),0);
  lcd_putsnAtt(1,2*FH, g_model.name,sizeof(g_model.name),BSS);
  lcd_putcAtt(sizeof(g_model.name)*FW+FW,2*FH,'?',0);
  lcd_puts_P(3*FW,5*FH,PSTR("YES     NO"));
  lcd_puts_P(3*FW,6*FH,PSTR("[MENU]  [EXIT]"));

  uint8_t i;
  switch(event){
    case EVT_ENTRY:
      beepWarn();
      break;
    case EVT_KEY_FIRST(KEY_MENU):
      EFile::rm(FILE_MODEL(g_eeGeneral.currModel)); //delete file

      i = g_eeGeneral.currModel;//loop to find next available model
      while (!EFile::exists(FILE_MODEL(i))) {
          i--;
          if(i>MAX_MODELS) i=MAX_MODELS-1;
          if(i==g_eeGeneral.currModel) {
              i=0;
              break;
          }
      }
      g_eeGeneral.currModel = i;

      eeLoadModel(g_eeGeneral.currModel); //load default values
      chainMenu(menuProcModelSelect);
      break;
    case EVT_KEY_FIRST(KEY_EXIT):
      popMenu();
      break;
  }
}

void menuProcModel(uint8_t event)
{
  MENU("SETUP", menuTabModel, e_Model, 15, {0,sizeof(g_model.name)-1,1,0,0,0,0,0,0,6,2,0/*repeated...*/});
  
  int8_t  sub    = mstate2.m_posVert;
  uint8_t subSub = mstate2.m_posHorz;
  uint8_t y = 1*FH;

  lcd_outdezNAtt(7*FW,0,g_eeGeneral.currModel+1,INVERS+LEADING0,2);

  switch(event){
    case EVT_KEY_REPT(KEY_LEFT):
    case EVT_KEY_FIRST(KEY_LEFT):
      if(sub==1 && subSub>0 && s_editMode) mstate2.m_posHorz--;
      break;
    case EVT_KEY_REPT(KEY_RIGHT):
    case EVT_KEY_FIRST(KEY_RIGHT):
      if(sub==1 && subSub<sizeof(g_model.name)-1 && s_editMode) mstate2.m_posHorz++;
      break;
  }

  uint8_t subN = 1;
  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Name"));
    lcd_putsnAtt(10*FW,   y, g_model.name ,sizeof(g_model.name),BSS | (sub==subN ? (s_editMode ? 0 : INVERS) : 0));
    if(sub==subN && s_editMode){
        char v = char2idx(g_model.name[subSub-1]);
        if(p1valdiff || event==EVT_KEY_FIRST(KEY_DOWN) || event==EVT_KEY_FIRST(KEY_UP) 
            || event==EVT_KEY_REPT(KEY_DOWN) || event==EVT_KEY_REPT(KEY_UP))
           CHECK_INCDEC_H_MODELVAR( event,v ,0,NUMCHARS-1);
        v = idx2char(v);
        g_model.name[subSub-1]=v;
        lcd_putcAtt((10+subSub-1)*FW, y, v,INVERS);
    }
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Timer"));
    putsTime(12*FW-1, y, g_model.tmrVal,
        (sub==subN && subSub==0 ? (s_editMode ? BLINK : INVERS):0),
        (sub==subN && subSub==1 ? (s_editMode ? BLINK : INVERS):0) );

    if(sub==subN && (s_editMode || p1valdiff))
      switch (subSub) {
       case 0:
          {
          int8_t min=g_model.tmrVal/60;
          CHECK_INCDEC_H_MODELVAR( event,min ,0,59);
          g_model.tmrVal = g_model.tmrVal%60 + min*60;
         break;
          }
        case 1:
          {
          int8_t sec=g_model.tmrVal%60;
          sec -= checkIncDec_hm( event,sec+2 ,1,62)-2;
          g_model.tmrVal -= sec ;
          if((int16_t)g_model.tmrVal < 0) g_model.tmrVal=0;
          break;
          }
      }
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) { //timer trigger source -> off, abs, stk, stk%, sw/!sw, !m_sw/!m_sw, chx(value > or < than tmrChVal), ch%
    lcd_puts_P(    0,    y, PSTR("Trigger"));
    uint8_t attr = (sub==subN ?  INVERS : 0);
    putsTmrMode(10*FW,y,attr);

    if(sub==subN)
      CHECK_INCDEC_H_MODELVAR( event,g_model.tmrMode ,-(13+2*MAX_DRSWITCH),(13+2*MAX_DRSWITCH));
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Timer "));
    lcd_putsnAtt(  10*FW, y, PSTR("Count DownCount Up  ")+10*g_model.tmrDir,10,(sub==subN ? INVERS:0));
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event,g_model.tmrDir,0,1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("T-Trim"));
    menu_lcd_onoff( 10*FW, y, g_model.thrTrim, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event,g_model.thrTrim,0,1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("T-Expo"));
    menu_lcd_onoff( 10*FW, y, g_model.thrExpo, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event,g_model.thrExpo,0,1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Trim Inc"));
    lcd_putsnAtt(  10*FW, y, PSTR("Exp   ExFineFine  MediumCoarse")+6*g_model.trimInc,6,(sub==subN ? INVERS:0));
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event,g_model.trimInc,0,4);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Trim Sw"));
    putsDrSwitches(9*FW,y,g_model.trimSw,sub==subN ? INVERS:0);
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event,g_model.trimSw,-MAX_DRSWITCH, MAX_DRSWITCH);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Beep Ctr"));
    for(uint8_t i=0;i<7;i++) lcd_putsnAtt((10+i)*FW, y, PSTR("RETA123")+i,1, ((subSub==i) && (sub==subN)) ? BLINK : ((g_model.beepANACenter & (1<<i)) ? INVERS : 0 ) );
    if(sub==subN){
        if((event==EVT_KEY_FIRST(KEY_MENU)) || p1valdiff) {
            killEvents(event);
            s_editMode = false;
            g_model.beepANACenter ^= (1<<subSub);
            STORE_MODELVARS;
        }
    }
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Proto"));
    lcd_putsnAtt(  6*FW, y, PSTR(PROT_STR)+PROT_STR_LEN*g_model.protocol,PROT_STR_LEN,
                  (sub==subN && subSub==0 ? (s_editMode ? BLINK : INVERS):0));
    if(!g_model.protocol) {
      lcd_putsnAtt(  10*FW, y, PSTR("4CH 6CH 8CH 10CH12CH14CH16CH")+4*(g_model.ppmNCH+2),4,(sub==subN && subSub==1  ? (s_editMode ? BLINK : INVERS):0));
      lcd_putsAtt(    17*FW,    y, PSTR("uSec"),0);
      lcd_outdezAtt(  17*FW, y,  (g_model.ppmDelay*50)+300, (sub==subN && subSub==2 ? (s_editMode ? BLINK : INVERS):0));
    }
    if(sub==subN && (s_editMode || p1valdiff))
      switch (subSub){
        case 0:
            CHECK_INCDEC_H_MODELVAR(event,g_model.protocol,0,PROT_MAX);
            break;
        case 1:
            CHECK_INCDEC_H_MODELVAR(event,g_model.ppmNCH,-2,4);
            break;
        case 2:
            CHECK_INCDEC_H_MODELVAR(event,g_model.ppmDelay,-4,10);
            break;
      }
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Shift Sel"));
    lcd_putsnAtt(  10*FW, y, PSTR("POSNEG")+3*g_model.pulsePol,3,(sub==subN ? INVERS:0));
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event,g_model.pulsePol,0,1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("E. Limits"));
		menu_lcd_onoff( 10*FW, y, g_model.extendedLimits, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event,g_model.extendedLimits,0,1);
    if((y+=FH)>7*FH) return;
  }subN++;
  
  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Trainer"));
		menu_lcd_onoff( 10*FW, y, g_model.traineron, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event,g_model.traineron,0,1);
    if((y+=FH)>7*FH) return;
  }subN++;
  
  if(s_pgOfs<subN) {
    lcd_putsAtt(0*FW, y, PSTR("DELETE MODEL   [MENU]"),s_noHi ? 0 : (sub==subN?INVERS:0));
    if(sub==subN && event==EVT_KEY_LONG(KEY_MENU)){
        s_editMode = false;
        s_noHi = NO_HI_LEN;
        killEvents(event);
        pushMenu(menuDeleteModel);
    }
    if((y+=FH)>7*FH) return;
  }subN++;
}

#ifdef HELI
void menuProcHeli(uint8_t event)
{
  MENU("HELI SETUP", menuTabModel, e_Heli, 7, {0 /*repeated*/});

  int8_t  sub    = mstate2.m_posVert;
  uint8_t y = 1*FH;

  uint8_t subN = 1;
  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Swash Type"));
    lcd_putsnAtt(  14*FW, y, PSTR(SWASH_TYPE_STR)+6*g_model.swashR.type,6,(sub==subN ? INVERS:0));
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event,g_model.swashR.type,0,SWASH_TYPE_NUM);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Collective"));
    putsChnRaw(14*FW, y, g_model.swashR.collectiveSource,  sub==subN ? INVERS : 0);
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event, g_model.swashR.collectiveSource, 0, NUM_XCHNRAW);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Swash Ring"));
    lcd_outdezAtt(14*FW, y, g_model.swashR.value,  LEFT|(sub==subN ? INVERS : 0));
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event, g_model.swashR.value, 0, 100);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("ELE Direction"));
    menu_lcd_HYPHINV( 14*FW, y, g_model.swashR.invertELE, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event, g_model.swashR.invertELE, 0, 1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("AIL Direction"));
    menu_lcd_HYPHINV( 14*FW, y, g_model.swashR.invertAIL, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event, g_model.swashR.invertAIL, 0, 1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("COL Direction"));
    menu_lcd_HYPHINV( 14*FW, y, g_model.swashR.invertCOL, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event, g_model.swashR.invertCOL, 0, 1);
    if((y+=FH)>7*FH) return;
  }subN++;
}
#endif

void menuProcModelSelect(uint8_t event)
{
  static MState2 mstate2;
  TITLE("MODELSEL");
  int8_t subOld  = mstate2.m_posVert;
  mstate2.check_submenu_simple(event,MAX_MODELS-1) ;
	
  lcd_puts_P(     9*FW, 0, PSTR("free"));
  lcd_outdezAtt(  17*FW, 0, EeFsGetFree(),0);

  DisplayScreenIndex(e_ModelSelect, DIM(menuTabModel), INVERS);

  int8_t  sub    = mstate2.m_posVert;
  static uint8_t sel_editMode;
  
  switch(event)
  {
    //case  EVT_KEY_FIRST(KEY_MENU):
    case  EVT_KEY_FIRST(KEY_EXIT):
      if(sel_editMode){
        sel_editMode = false;
        beepKey();
        killEvents(event);
        eeLoadModel(g_eeGeneral.currModel = mstate2.m_posVert);
        resetTimer(); // TODO BSS ResetAll ? 
        STORE_GENERALVARS;
        STORE_MODELVARS;
        break;
      }
      //fallthrough
    case  EVT_KEY_FIRST(KEY_LEFT):
    case  EVT_KEY_FIRST(KEY_RIGHT):
      if(g_eeGeneral.currModel != mstate2.m_posVert)
      {
        killEvents(event);
        g_eeGeneral.currModel = mstate2.m_posVert;
        eeLoadModel(g_eeGeneral.currModel);
        resetTimer(); // TODO BSS ResetAll ? 
        STORE_GENERALVARS;
        beepWarn1();
      }
      //case EXIT handled in checkExit
      if(event==EVT_KEY_FIRST(KEY_LEFT))  {killEvents(event);popMenu(true);}
      if(event==EVT_KEY_FIRST(KEY_RIGHT))  chainMenu(menuProcModel);
      break;
    case  EVT_KEY_FIRST(KEY_MENU):
        sel_editMode = true;
        beepKey();
        break;
    case  EVT_KEY_LONG(KEY_MENU):
      if(sel_editMode){
        message(PSTR("Duplicating model"));
        if(eeDuplicateModel(sub)) {
          beepKey();
          sel_editMode = false;
        }
        else beepWarn();
      }
      break;

    case EVT_ENTRY:
      sel_editMode = false;

      mstate2.m_posVert = g_eeGeneral.currModel;
      eeCheck(true); //force writing of current model data before this is changed
      break;
  }
  if(sel_editMode && subOld!=sub){
    EFile::swap(FILE_MODEL(subOld),FILE_MODEL(sub));
  }

  if(sub-s_pgOfs < 1)        s_pgOfs = max(0,sub-1);
  else if(sub-s_pgOfs >4 )  s_pgOfs = min(MAX_MODELS-6,sub-4);
  for(uint8_t i=0; i<6; i++){
    uint8_t y=(i+2)*FH;
    uint8_t k=i+s_pgOfs;
    lcd_outdezNAtt(  3*FW, y, k+1, ((sub==k) ? INVERS : 0) + LEADING0,2);
    static char buf[sizeof(g_model.name)+5];
    if(k==g_eeGeneral.currModel) lcd_putc(1,  y,'*');
    eeLoadModelName(k,buf,sizeof(buf));
    lcd_putsnAtt(  4*FW, y, buf,sizeof(buf),BSS|((sub==k) ? (sel_editMode ? INVERS : 0 ) : 0));
  }

}

void menuProcDiagCalib(uint8_t event)
{
  SIMPLE_MENU("CALIBRATION", menuTabDiag, e_Calib, 4);

  int8_t  sub    = mstate2.m_posVert ;
  static int16_t midVals[7];
  static int16_t loVals[7];
  static int16_t hiVals[7];

  for(uint8_t i=0; i<7; i++) { //get low and high vals for sticks and trims
    int16_t vt = anaIn(i);
    loVals[i] = min(vt,loVals[i]);
    hiVals[i] = max(vt,hiVals[i]);
    //if(i>=4) midVals[i] = (loVals[i] + hiVals[i])/2;
  }

  switch(event)
  {
    case EVT_ENTRY:
      for(uint8_t i=0; i<7; i++) loVals[i] = 15000;
      break;
    case EVT_KEY_BREAK(KEY_DOWN): // !! achtung sub schon umgesetzt
      switch(sub)
      {
        case 2: //get mid
          for(uint8_t i=0; i<7; i++)midVals[i] = anaIn(i);
          beepKey();
          break;
        case 3:
          for(uint8_t i=0; i<7; i++)
            if(abs(loVals[i]-hiVals[i])>50) {
              g_eeGeneral.calibMid[i]  = midVals[i];
              int16_t v = midVals[i] - loVals[i];
              g_eeGeneral.calibSpanNeg[i] = v - v/64;
              v = hiVals[i] - midVals[i];
              g_eeGeneral.calibSpanPos[i] = v - v/64;
            }
          int16_t sum=0;
          for(uint8_t i=0; i<12;i++) sum+=g_eeGeneral.calibMid[i];
          g_eeGeneral.chkSum = sum;
          eeDirty(EE_GENERAL); //eeWriteGeneral();
          beepKey();
          break;
      }
      break;
  }
  for(uint8_t i=1; i<4; i++)
  {
    uint8_t y=i*FH+FH;
    lcd_putsnAtt( 0, y,PSTR("SetMid SetSpanDone   ")+7*(i-1),7,
                    sub==i ? INVERS : 0);
  }
  for(uint8_t i=0; i<7; i++)
  {
    uint8_t y=i*FH;
    lcd_puts_P( 11*FW,   y+1*FH, PSTR("<    >"));
    lcd_outhex4( 8*FW-3, y+1*FH, sub==2 ? loVals[i]  : g_eeGeneral.calibSpanNeg[i]);
    lcd_outhex4(12*FW,   y+1*FH, sub==1 ? anaIn(i) : (sub==2 ? midVals[i] : g_eeGeneral.calibMid[i]));
    lcd_outhex4(17*FW,   y+1*FH, sub==2 ? hiVals[i]  : g_eeGeneral.calibSpanPos[i]);
  }

}

#if defined(PCBV3)
void menuProcFrskyTime(uint8_t event)
{
  MENU("DATE AND TIME", menuTabDiag, e_FrskyTime, 3, {0, 2});

  int8_t  sub    = mstate2.m_posVert - 1; // vertical position (1 = page counter, top/right)
  uint8_t subSub = mstate2.m_posHorz;     // horizontal position
  static struct tm t;
  struct tm *at = &t;

  switch(event)
  {
    case EVT_KEY_LONG(KEY_MENU):
      // get data time from RTC chip (may not implement)
      killEvents(event);
      break;
    case EVT_KEY_FIRST(KEY_MENU):
      if (sub >= 0 && !s_editMode) // set the date and time into RTC chip
      {
        g_ms100 = 0; // start of next second begins now
        g_unixTime = mktime(&t); // update local timestamp and get wday calculated

        RTC rtc;
        rtc.year = t.tm_year + 1900;
        rtc.month = t.tm_mon + 1;
        rtc.mday = t.tm_mday;
        rtc.hour = t.tm_hour;
        rtc.min = t.tm_min;
        rtc.sec = t.tm_sec;
        rtc.wday = t.tm_wday + 1;

        rtc_settime(&rtc);

      }
      break;
  }

  if (!s_editMode) filltm(&g_unixTime, &t);

  lcd_putc(FW*10+2, FH*2, '-'); lcd_putc(FW*13, FH*2, '-');
  lcd_putc(FW*10+1, FH*4, ':'); lcd_putc(FW*13-1, FH*4, ':');

  for(uint8_t i=0; i<2; i++) // 2 rows, date then time
  {
    uint8_t y=(i*2+2)*FH;

    lcd_putsnAtt(0, y, PSTR("DATE:""TIME:")+i*5, 5, 0);

    for(uint8_t j=0; j<3;j++) // 3 settings each for date and time (YMD and HMS)
    {
      uint8_t attr = (sub==i && subSub==j) ? (s_editMode ? BLINK : INVERS) : 0;
      switch(i)
      {
        case 0: // DATE
          switch(j)
          {
            case 0:
              lcd_outdezAtt(FW*10+2, y, at->tm_year+1900, attr);
              if(attr && (s_editMode || p1valdiff)) at->tm_year = checkIncDec( event, at->tm_year, 110, 200, 0);
              break;
            case 1:
              lcd_outdezAtt(FW*13, y, at->tm_mon+1, attr | ((at->tm_mon<9) ? LEADING0 : 0));
              if(attr && (s_editMode || p1valdiff)) at->tm_mon = checkIncDec( event, at->tm_mon, 0, 11, 0);
              break;
            case 2:
              lcd_outdezAtt(FW*16-2, y, at->tm_mday, attr | ((t.tm_mday<10) ? LEADING0 : 0));
              if(attr && (s_editMode || p1valdiff)) at->tm_mday = checkIncDec( event, at->tm_mday, 1, 31, 0);
              break;
          }
          break;

        case 1:
          switch (j)
          {
            case 0:
              lcd_outdezAtt(FW*10+1, y, at->tm_hour, attr | ((at->tm_hour<10) ? LEADING0 : 0));
              if(attr && (s_editMode || p1valdiff)) at->tm_hour = checkIncDec( event, at->tm_hour, 0, 23, 0);
              break;
            case 1:
              lcd_outdezAtt(FW*13-1, y, at->tm_min, attr | ((at->tm_min<10) ? LEADING0 : 0));
              if(attr && (s_editMode || p1valdiff)) at->tm_min = checkIncDec( event, at->tm_min, 0, 59, 0);
              break;
            case 2:
              lcd_outdezAtt(FW*16-2, y, at->tm_sec, attr | ((t.tm_sec<10) ? LEADING0 : 0));
              if(attr && (s_editMode || p1valdiff)) at->tm_sec = checkIncDec( event, at->tm_sec, 0, 59, 0);
              break;
          }
          break;

      }
    }
  }
}
#endif

void menuProcDiagAna(uint8_t event)
{
  SIMPLE_MENU("ANA", menuTabDiag, e_Ana, 2);

  int8_t  sub    = mstate2.m_posVert ;

  for(uint8_t i=0; i<8; i++)
  {
    uint8_t y=i*FH;
    lcd_putsn_P( 4*FW, y,PSTR("A1A2A3A4A5A6A7A8")+2*i,2);
    lcd_outhex4( 8*FW, y,anaIn(i));
    if(i<7)  lcd_outdez(17*FW, y, (int32_t)calibratedStick[i]*100/1024);
    if(i==7) 
    {
      // Create a 2-decimal precision result just for the calibration screen
#ifdef THBATVOLTS
      uint16_t vbat100mV = (uint32_t)((abRunningAvg*35 + ab / 4 * g_eeGeneral.vBatCalib)*10) / 512;
#else
      uint16_t vbat100mV = (uint32_t)(abRunningAvg + 4 * g_eeGeneral.vBatCalib) * 360 / 512;
#endif
      putsVolts(17*FW,y,vbat100mV,(sub==1 ? INVERS : 0)|PREC2);
    }
  }
  if(sub==1) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.vBatCalib, -127, 127);
}

void menuProcDiagKeys(uint8_t event)
{
  SIMPLE_MENU("DIAG", menuTabDiag, e_Keys, 1);

  uint8_t x=7*FW;
  for(uint8_t i=0; i<9; i++)
  {
    uint8_t y=i*FH; //+FH;
    if(i>(SW_ID0-SW_BASE_DIAG)) y-=FH; //overwrite ID0
    bool t=keyState((EnumKeys)(SW_BASE_DIAG+i));
    putsDrSwitches(x,y,i+1,0); //ohne off,on
    lcd_putcAtt(x+FW*4+2,  y,t+'0',t ? INVERS : 0);
  }

  x=0;
  for(uint8_t i=0; i<6; i++)
  {
    uint8_t y=(5-i)*FH+2*FH;
    bool t=keyState((EnumKeys)(KEY_MENU+i));
    lcd_putsn_P(x, y,PSTR(" Menu Exit Down   UpRight Left")+5*i,5);
    lcd_putcAtt(x+FW*5+2,  y,t+'0',t);
  }

  x=14*FW;
  lcd_putsn_P(x, 3*FH,PSTR("Trim- +"),7);
  for(uint8_t i=0; i<4; i++)
  {
    uint8_t y=i*FH+FH*4;
    lcd_img(    x,       y, sticks,i,0);
    bool tm=keyState((EnumKeys)(TRM_BASE+2*i));
    bool tp=keyState((EnumKeys)(TRM_BASE+2*i+1));
    lcd_putcAtt(x+FW*4,  y, tm+'0',tm ? INVERS : 0);
    lcd_putcAtt(x+FW*6,  y, tp+'0',tp ? INVERS : 0);
  }
}

void menuProcDiagVers(uint8_t event)
{
  SIMPLE_MENU("VERSION", menuTabDiag, e_Vers, 1);

  lcd_puts_P(0, 2*FH,stamp4 );
  lcd_puts_P(0, 3*FH,stamp1 );
  lcd_puts_P(0, 4*FH,stamp5 );
  lcd_puts_P(0, 5*FH,stamp2 );
  lcd_puts_P(0, 6*FH,stamp3 );
}

void menuProcTrainer(uint8_t event)
{
  MENU("TRAINER", menuTabDiag, e_Trainer, 7, {0, 3, 3, 3, 3, 0/*, 0*/});

  int8_t  sub    = mstate2.m_posVert;
  uint8_t subSub = mstate2.m_posHorz;
  uint8_t y;
  bool    edit;

  if (SLAVE_MODE) { // i am the slave
    lcd_puts_P(7*FW, 3*FH, PSTR("Slave"));
    return;
  }

  lcd_puts_P(3*FW, 1*FH, PSTR("mode   % src  sw"));

  sub--;
  y = 2*FH;
  for (uint8_t i=0; i<4; i++) {
    volatile TrainerMix *td = &g_eeGeneral.trainer.mix[i];
    putsChnRaw(0, y, i+1, 0);

    edit = (sub==i && subSub==0);
    lcd_putsnAtt(4*FW, y, PSTR("off += :=")+3*td->mode, 3, edit ? (s_editMode ? BLINK : INVERS) : 0);
    if (edit && s_editMode)
      CHECK_INCDEC_H_GENVAR(event, td->mode, 0, 2);

    edit = (sub==i && subSub==1);
    lcd_outdezAtt(11*FW, y, td->studWeight*13/4, edit ? (s_editMode ? BLINK : INVERS) : 0);
    if (edit && s_editMode)
      CHECK_INCDEC_H_GENVAR(event, td->studWeight, -31, 31);

    edit = (sub==i && subSub==2);
    lcd_putsnAtt(12*FW, y, PSTR("ch1ch2ch3ch4")+3*td->srcChn, 3, edit ? (s_editMode ? BLINK : INVERS) : 0);
    if (edit && s_editMode)
      CHECK_INCDEC_H_GENVAR(event, td->srcChn, 0, 3);

    edit = (sub==i && subSub==3);
    putsDrSwitches(15*FW, y, td->swtch, edit ? (s_editMode ? BLINK : INVERS) : 0);
    if (edit && s_editMode)
      CHECK_INCDEC_H_GENVAR(event, td->swtch, -MAX_DRSWITCH, MAX_DRSWITCH);

    y += FH;
  }

  lcd_puts_P(0*FW, y, PSTR("Multiplier"));
  lcd_outdezAtt(13*FW, y, g_eeGeneral.PPM_Multiplier+10, (sub==4 ? INVERS : 0)|PREC1);
  if(sub==4) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.PPM_Multiplier, -10, 40);

  y += FH;

  edit = (sub==5);
  lcd_putsAtt(0*FW, y, PSTR("Cal"), edit ? INVERS : 0);
  uint8_t x = 4*FW+4*FWNUM;
  for (uint8_t i=0; i<4; i++) {
#if defined (DECIMALS_DISPLAYED)
    lcd_outdezAtt(x , y, (g_ppmIns[i]-g_eeGeneral.trainer.calib[i])*2, PREC1);
#else
    lcd_outdezAtt(x , y, (g_ppmIns[i]-g_eeGeneral.trainer.calib[i])/5, 0);
#endif
    x += 4*FWNUM+FW+2;
  }

  if (edit) {
    if (event==EVT_KEY_FIRST(KEY_MENU)){
      memcpy(g_eeGeneral.trainer.calib, g_ppmIns, sizeof(g_eeGeneral.trainer.calib));
      eeDirty(EE_GENERAL);
      beepKey();
    }
  }
}


void menuProcSetup(uint8_t event)
{
#define COUNT_ITEMS 17
#define PARAM_OFS   17*FW

  SIMPLE_MENU("RADIO SETUP", menuTabDiag, e_Setup, COUNT_ITEMS+1);

  int8_t  sub    = mstate2.m_posVert;
  
  if(s_pgOfs==COUNT_ITEMS-7) s_pgOfs= sub<(COUNT_ITEMS-4) ? COUNT_ITEMS-8 : COUNT_ITEMS-6; // TODO BSS needed?

  uint8_t y = 1*FH;

  uint8_t subN = 1;
  if(s_pgOfs<subN) {
    lcd_puts_P(0, y,PSTR("Beeper"));
    lcd_putsnAtt(PARAM_OFS - FW, y, PSTR("Quiet""NoKey""Norm ""Long ""xLong")+5*g_eeGeneral.beeperVal,5,(sub==subN ? INVERS:0));
    if(sub==subN) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.beeperVal, 0, 4);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(0, y,PSTR("Contrast"));
    lcd_outdezAtt(PARAM_OFS,y,g_eeGeneral.contrast,(sub==subN ? INVERS : 0)|LEFT);
    if(sub==subN) {
      CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.contrast, 10, 45);
      lcdSetRefVolt(g_eeGeneral.contrast);
    }
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(0, y,PSTR("Battery warning"));
    putsVolts(PARAM_OFS, y, g_eeGeneral.vBatWarn, (sub==subN ? INVERS : 0)|LEFT);
    if(sub==subN) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.vBatWarn, 40, 120); //4-12V
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(0, y,PSTR("Inactivity alarm"));
    lcd_outdezAtt(PARAM_OFS, y, g_eeGeneral.inactivityTimer, (sub==subN ? INVERS : 0)|LEFT);
    lcd_putc(lcd_lastPos, y, 'm');
    if(sub==subN) g_eeGeneral.inactivityTimer = checkIncDec(event, g_eeGeneral.inactivityTimer, 0, 250, EE_GENERAL); //0..250minutes
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(0, y,PSTR("Filter ADC"));
    lcd_putsnAtt(PARAM_OFS, y, PSTR("SINGOSMPFILT")+4*g_eeGeneral.filterInput,4,(sub==subN ? INVERS:0));
    if(sub==subN) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.filterInput, 0, 2);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(0, y,PSTR("Throttle reverse"));
    menu_lcd_onoff( PARAM_OFS, y, g_eeGeneral.throttleReversed, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.throttleReversed, 0, 1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(0, y,PSTR("Minute beep"));
    menu_lcd_onoff( PARAM_OFS, y, g_eeGeneral.minuteBeep, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.minuteBeep, 0, 1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(0, y,PSTR("Beep countdown"));
    menu_lcd_onoff( PARAM_OFS, y, g_eeGeneral.preBeep, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.preBeep, 0, 1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
      lcd_puts_P(0, y,PSTR("Flash on beep"));
      menu_lcd_onoff( PARAM_OFS, y, g_eeGeneral.flashBeep, sub==subN ) ;
      if(sub==subN) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.flashBeep, 0, 1);
      if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(0, y,PSTR("Light switch"));
    putsDrSwitches(PARAM_OFS-FW,y,g_eeGeneral.lightSw,sub==subN ? INVERS : 0);
    if(sub==subN) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.lightSw, -MAX_DRSWITCH, MAX_DRSWITCH);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(0, y,PSTR("Light off after"));
    if(g_eeGeneral.lightAutoOff) {
      lcd_outdezAtt(PARAM_OFS, y, g_eeGeneral.lightAutoOff*5,LEFT|(sub==subN ? INVERS : 0));
      lcd_putc(lcd_lastPos, y, 's');
    }
    else
      lcd_putsnAtt(PARAM_OFS, y, PSTR("OFF"),3,(sub==subN ? INVERS:0));
    if(sub==subN) CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.lightAutoOff, 0, 600/5);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
      uint8_t b = 1-g_eeGeneral.disableSplashScreen;
      lcd_puts_P(0, y,PSTR("Splash screen"));
      menu_lcd_onoff( PARAM_OFS, y, b, sub==subN ) ;
      if(sub==subN)
      {
          CHECK_INCDEC_H_GENVAR(event, b, 0, 1);
          g_eeGeneral.disableSplashScreen = 1-b;
      }
      if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
      uint8_t b = 1-g_eeGeneral.disableThrottleWarning;
      lcd_puts_P(0, y,PSTR("Throttle Warning"));
      menu_lcd_onoff( PARAM_OFS, y, b, sub==subN ) ;
      if(sub==subN)
      {
          CHECK_INCDEC_H_GENVAR(event, b, 0, 1);
          g_eeGeneral.disableThrottleWarning = 1-b;
      }
      if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
      lcd_puts_P(0, y,PSTR("Switch Warning"));
      lcd_putsnAtt(PARAM_OFS, y, PSTR("Down""OFF ""Up  ")+4*(1+g_eeGeneral.switchWarning),4,(sub==subN ? INVERS:0));
      if(sub==subN)
        CHECK_INCDEC_H_GENVAR(event, g_eeGeneral.switchWarning, -1, 1);
      if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
      uint8_t b = 1-g_eeGeneral.disableMemoryWarning;
      lcd_puts_P(0, y,PSTR("Memory Warning"));
      menu_lcd_onoff( PARAM_OFS, y, b, sub==subN ) ;
      if(sub==subN)
      {
          CHECK_INCDEC_H_GENVAR(event, b, 0, 1);
          g_eeGeneral.disableMemoryWarning = 1-b;
      }
      if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
      uint8_t b = 1-g_eeGeneral.disableAlarmWarning;
      lcd_puts_P(0, y,PSTR("Alarm Warning"));
      menu_lcd_onoff( PARAM_OFS, y, b, sub==subN ) ;
      if(sub==subN)
      {
          CHECK_INCDEC_H_GENVAR(event, b, 0, 1);
          g_eeGeneral.disableAlarmWarning = 1-b;
      }
      if((y+=FH)>7*FH) return;
  }subN++;


  if(s_pgOfs<subN) {
    lcd_puts_P( 1*FW, y, PSTR("Mode"));//sub==3?INVERS:0);
    if(y<7*FH) {for(uint8_t i=0; i<4; i++) lcd_img((6+4*i)*FW, y, sticks,i,0); }
    if((y+=FH)>7*FH) return;

    lcd_putcAtt( 3*FW, y, '1'+g_eeGeneral.stickMode,sub==subN?INVERS:0);
    for(uint8_t i=0; i<4; i++) putsChnRaw( (6+4*i)*FW, y,i+1,0);//sub==3?INVERS:0);

    if(sub==subN) CHECK_INCDEC_H_GENVAR(event,g_eeGeneral.stickMode,0,3);
    if((y+=FH)>7*FH) return;
  }subN++;

}

uint16_t s_timeCumTot;
uint16_t s_timeCumAbs;  //laufzeit in 1/16 sec
uint16_t s_timeCumSw;  //laufzeit in 1/16 sec
uint16_t s_timeCumThr;  //gewichtete laufzeit in 1/16 sec
uint16_t s_timeCum16ThrP; //gewichtete laufzeit in 1/16 sec
uint8_t  s_timerState;
int16_t  s_timerVal;

void timer(uint8_t val)
{
  int8_t tm = g_model.tmrMode;
  static uint16_t s_time;
  static uint16_t s_cnt;
  static uint16_t s_sum;
  static uint8_t sw_toggled;

  if(abs(tm)>=(TMR_VAROFS+MAX_DRSWITCH-1)){ //toggeled switch//abs(g_model.tmrMode)<(10+MAX_DRSWITCH-1)
    static uint8_t lastSwPos;
    if(!(sw_toggled | s_sum | s_cnt | s_time | lastSwPos)) lastSwPos = tm < 0;  // if initializing then init the lastSwPos
    uint8_t swPos = getSwitch(tm>0 ? tm-(TMR_VAROFS+MAX_DRSWITCH-1-1) : tm+(TMR_VAROFS+MAX_DRSWITCH-1-1) ,0);
    if(swPos && !lastSwPos)  sw_toggled = !sw_toggled;  //if switcdh is flipped first time -> change counter state
    lastSwPos = swPos;
  }

  s_cnt++;
  s_sum+=val;
  if((g_tmr10ms-s_time)<100) return; //1 sec
  s_time += 100;
  val     = s_sum/s_cnt;
  s_sum  -= val*s_cnt; //rest
  s_cnt   = 0;

  if(abs(tm)<TMR_VAROFS) sw_toggled = false; // not switch - sw timer off
  else if(abs(tm)<(TMR_VAROFS+MAX_DRSWITCH-1)) sw_toggled = getSwitch((tm>0 ? tm-(TMR_VAROFS-1) : tm+(TMR_VAROFS-1)) ,0); //normal switch

  s_timeCumTot               += 1;
  s_timeCumAbs               += 1;
  if(val) s_timeCumThr       += 1;
  if(sw_toggled) s_timeCumSw += 1;
  s_timeCum16ThrP            += val/2;

  s_timerVal = g_model.tmrVal;
  uint8_t tmrM = abs(g_model.tmrMode);
  if(tmrM==TMRMODE_NONE) s_timerState = TMR_OFF;
  else if(tmrM==TMRMODE_ABS) s_timerVal -= s_timeCumAbs;
  else if(tmrM<TMR_VAROFS) s_timerVal -= (tmrM&1) ? s_timeCum16ThrP/16 : s_timeCumThr;// stick% : stick
  else s_timerVal -= s_timeCumSw; //switch

  switch(s_timerState)
  {
    case TMR_OFF:
      if(g_model.tmrMode != TMRMODE_NONE) s_timerState=TMR_RUNNING;
      break;
    case TMR_RUNNING:
      if(s_timerVal<=0 && g_model.tmrVal) s_timerState=TMR_BEEPING;
      break;
    case TMR_BEEPING:
      if(s_timerVal <= -MAX_ALERT_TIME)   s_timerState=TMR_STOPPED;
      if(g_model.tmrVal == 0)             s_timerState=TMR_RUNNING;
      break;
    case TMR_STOPPED:
      break;
  }

  static int16_t last_tmr;

  if(last_tmr != s_timerVal)  //beep only if seconds advance
  {
      if(s_timerState==TMR_RUNNING)
      {
          if(g_eeGeneral.preBeep && g_model.tmrVal) // beep when 30, 15, 10, 5,4,3,2,1 seconds remaining
          {
              if(s_timerVal==30) {beepAgain=2; beepWarn2();} //beep three times
              if(s_timerVal==20) {beepAgain=1; beepWarn2();} //beep two times
              if(s_timerVal==10)  beepWarn2();
              if(s_timerVal<= 3)  beepWarn2();

              if(g_eeGeneral.flashBeep && (s_timerVal==30 || s_timerVal==20 || s_timerVal==10 || s_timerVal<=3))
                  g_LightOffCounter = FLASH_DURATION;
          }

          if(g_eeGeneral.minuteBeep && (((g_model.tmrDir ? g_model.tmrVal-s_timerVal : s_timerVal)%60)==0)) //short beep every minute
          {
              beepWarn2();
              if(g_eeGeneral.flashBeep) g_LightOffCounter = FLASH_DURATION;
          }
      }
      else if(s_timerState==TMR_BEEPING)
      {
          beepWarn();
          if(g_eeGeneral.flashBeep) g_LightOffCounter = FLASH_DURATION;
      }
  }
  last_tmr = s_timerVal;
  if(g_model.tmrDir) s_timerVal = g_model.tmrVal-s_timerVal; //if counting backwards - display backwards
}


#define MAXTRACE 120
uint8_t s_traceBuf[MAXTRACE];
uint16_t s_traceWr;
uint16_t s_traceCnt;
void trace()   // called in perOut - once envery 0.01sec
{
  //value for time described in g_model.tmrMode
  //OFFABSRUsRU%ELsEL%THsTH%ALsAL%P1P1%P2P2%P3P3%
  uint16_t v = 0;
  if((abs(g_model.tmrMode)>1) && (abs(g_model.tmrMode)<TMR_VAROFS)) {
    v = calibratedStick[CONVERT_MODE(abs(g_model.tmrMode)/2)-1];
    v = (g_model.tmrMode<0 ? RESX-v : v+RESX ) / (RESX/16);
  }
  timer(v);

  uint16_t val = calibratedStick[CONVERT_MODE(3)-1]; //Get throttle channel value
  val = (g_eeGeneral.throttleReversed ? RESX-val : val+RESX) / (RESX/16); //calibrate it
  static uint16_t s_time;
  static uint16_t s_cnt;
  static uint16_t s_sum;
  s_cnt++;
  s_sum+=val;
  if((g_tmr10ms-s_time)<1000) //10 sec
    return;
  s_time= g_tmr10ms;
  val   = s_sum/s_cnt;
  s_sum = 0;
  s_cnt = 0;

  s_traceCnt++;
  s_traceBuf[s_traceWr++] = val;
  if(s_traceWr>=MAXTRACE) s_traceWr=0;
}


uint16_t g_tmr1Latency_max;
uint16_t g_tmr1Latency_min = 0x7ff;
uint16_t g_timeMain;
uint16_t g_time_per10;
void menuProcStatistic2(uint8_t event)
{
  TITLE("STAT2");
  switch(event)
  {
    case EVT_KEY_FIRST(KEY_MENU):
      g_tmr1Latency_min = 0x7ff;
      g_tmr1Latency_max = 0;
      g_timeMain    = 0;
//      g_time_per10 = 0;
      beepKey();
      break;
    case EVT_KEY_FIRST(KEY_DOWN):
      chainMenu(menuProcStatistic);
      break;
    case EVT_KEY_FIRST(KEY_UP):
    case EVT_KEY_FIRST(KEY_EXIT):
      chainMenu(menuProc0);
      break;
  }
  lcd_puts_P( 0*FW,  1*FH, PSTR("tmr1Lat max    us"));
  lcd_outdez(14*FW , 1*FH, g_tmr1Latency_max/2 );
  lcd_puts_P( 0*FW,  2*FH, PSTR("tmr1Lat min    us"));
  lcd_outdez(14*FW , 2*FH, g_tmr1Latency_min/2 );
  lcd_puts_P( 0*FW,  3*FH, PSTR("tmr1 Jitter    us"));
  lcd_outdez(14*FW , 3*FH, (g_tmr1Latency_max - g_tmr1Latency_min) /2 );
  lcd_puts_P( 0*FW,  4*FH, PSTR("tmain max      ms"));
  lcd_outdezAtt(14*FW , 4*FH, (g_timeMain*100)/16 ,PREC2);
  lcd_puts_P( 0*FW,  5*FH, PSTR("t10ms          us"));
  lcd_outdez(14*FW , 5*FH, g_time_per10/2 );
  lcd_puts_P( 3*FW,  7*FH, PSTR("[MENU] to reset"));
}

#ifdef JETI

void menuProcJeti(uint8_t event)
{
  TITLE("JETI");

  switch(event)
  {
    //case EVT_KEY_FIRST(KEY_MENU):
    //  break;
    case EVT_KEY_FIRST(KEY_EXIT):
      JETI_DisableRXD();
      chainMenu(menuProc0);
      break;
  }

  for (uint8_t i = 0; i < 16; i++)
  {
    lcd_putcAtt((i+2)*FW,   3*FH, JetiBuffer[i], BSS);
    lcd_putcAtt((i+2)*FW,   4*FH, JetiBuffer[i+16], BSS);
  }

  if (JetiBufferReady)
  {
    JETI_EnableTXD();
    if (keyState((EnumKeys)(KEY_UP))) jeti_keys &= JETI_KEY_UP;
    if (keyState((EnumKeys)(KEY_DOWN))) jeti_keys &= JETI_KEY_DOWN;
    if (keyState((EnumKeys)(KEY_LEFT))) jeti_keys &= JETI_KEY_LEFT;
    if (keyState((EnumKeys)(KEY_RIGHT))) jeti_keys &= JETI_KEY_RIGHT;

    JetiBufferReady = 0;    // invalidate buffer

    JETI_putw((uint16_t) jeti_keys);
    _delay_ms (1);
    JETI_DisableTXD();

    jeti_keys = JETI_KEY_NOCHANGE;
  }
}
#endif

void menuProcStatistic(uint8_t event)
{
	TITLE("STAT");
	switch(event)
	{
		case EVT_KEY_FIRST(KEY_UP):
			chainMenu(menuProcStatistic2);
			break;
		case EVT_KEY_FIRST(KEY_DOWN):
		case EVT_KEY_FIRST(KEY_EXIT):
			chainMenu(menuProc0);
			break;
	}
	
	lcd_puts_P(  1*FW, FH*1, PSTR("TME"));
	putsTime(    4*FW, FH*1, s_timeCumAbs, 0, 0);
	lcd_puts_P( 17*FW, FH*1, PSTR("TSW"));
	putsTime(   10*FW, FH*1, s_timeCumSw,      0, 0);
	
	lcd_puts_P(  1*FW, FH*2, PSTR("STK"));
	putsTime(    4*FW, FH*2, s_timeCumThr, 0, 0);
	lcd_puts_P( 17*FW, FH*2, PSTR("ST%"));
	putsTime(   10*FW, FH*2, s_timeCum16ThrP/16, 0, 0);
	
	lcd_puts_P( 17*FW, FH*0, PSTR("TOT"));
	putsTime(   10*FW, FH*0, s_timeCumTot, 0, 0);
	
	uint16_t traceRd = s_traceCnt>MAXTRACE ? s_traceWr : 0;
	uint8_t x=5;
	uint8_t y=60;
	lcd_hline(x-3,y,120+3+3);
	lcd_vline(x,y-32,32+3);
	
	for(uint8_t i=0; i<120; i+=6)
	{
		lcd_vline(x+i+6,y-1,3);
	}
	for(uint8_t i=1; i<=120; i++)
	{
		lcd_vline(x+i,y-s_traceBuf[traceRd],s_traceBuf[traceRd]);
		traceRd++;
		if(traceRd>=MAXTRACE) traceRd=0;
		if(traceRd==s_traceWr) break;
	}
	
}

void resetTimer()
{
    s_timerState = TMR_OFF; //is changed to RUNNING dep from mode
    s_timeCumAbs=0;
    s_timeCumThr=0;
    s_timeCumSw=0;
    s_timeCum16ThrP=0;
}

void menuProc0(uint8_t event)
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
      else if(view == e_timer2) {
       resetTimer2();
       // Timer2_running = !Timer2_running;
       beepKey();
      }      
#ifdef FRSKY
      else if (view == e_telemetry) {
        resetTelemetry();
        beepKey();
      }
#endif
      // TODO BSS else resetTimer1()
      break;
    case EVT_KEY_LONG(KEY_EXIT):
      resetTimer();
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

  if(getSwitch(g_model.trimSw,0) && !trimSwLock) setStickCenter();
  trimSwLock = getSwitch(g_model.trimSw,0);

  if (g_eeGeneral.view < 0x10) {
    uint8_t x=FW*2;
    uint8_t att = (g_vbat100mV < g_eeGeneral.vBatWarn ? BLINK : 0) | DBLSIZE;
    for(uint8_t i=0;i<sizeof(g_model.name);i++)
      lcd_putcAtt(x+i*2*FW-i-2, 0*FH, g_model.name[i],DBLSIZE);

    putsVBat(x+4*FW+2, 2*FH, att|NO_UNIT);
    lcd_putc(x+4*FW+2, 3*FH, 'V');

    if(s_timerState != TMR_OFF){
      uint8_t att = DBLSIZE | (s_timerState==TMR_BEEPING ? BLINK : 0);
      putsTime(x+14*FW-2, FH*2, s_timerVal, att,att);
      putsTmrMode(x+7*FW-FW/2,FH*3,0);
    }

    lcd_putsnAtt(x+4*FW+2,     2*FH,PSTR("ExpExFFneMedCrs")+3*g_model.trimInc,3, 0);
    lcd_putsnAtt(x+8*FW-FW/2,2*FH,PSTR("   TTm")+3*g_model.thrTrim,3, 0);

    //trim sliders
    for(uint8_t i=0; i<4; i++)
    {
#define TL 27
      //                        LH LV RV RH
      static uint8_t x[4]    = {128*1/4+2, 4, 128-4, 128*3/4-2};
      static uint8_t vert[4] = {0,1,1,0};
      uint8_t xm,ym;
      xm=x[i];
      int8_t val = max((int8_t)-(TL+1),min((int8_t)(TL+1),(int8_t)(g_model.trim[0][i]/4)));
      if(vert[i]){
        ym=31;
        lcd_vline(xm,   ym-TL, TL*2);

        if(((g_eeGeneral.stickMode&1) != (i&1)) || !(g_model.thrTrim)){
          lcd_vline(xm-1, ym-1,  3);
          lcd_vline(xm+1, ym-1,  3);
        }
        ym -= val;
      }else{
        ym=60;
        lcd_hline(xm-TL,ym,    TL*2);
        lcd_hline(xm-1, ym-1,  3);
        lcd_hline(xm-1, ym+1,  3);
        xm += val;
      }
      DO_SQUARE(xm,ym,7)
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
            val = ((uint16_t)staticTelemetry[i]+g_model.frsky.channels[i].offset)*g_model.frsky.channels[i].ratio / 255;
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

    DO_SQUARE(LBOX_CENTERX,LBOX_CENTERY,BOX_WIDTH);
    DO_SQUARE(RBOX_CENTERX,RBOX_CENTERY,BOX_WIDTH);

    DO_CROSS(LBOX_CENTERX,LBOX_CENTERY,3)
    DO_CROSS(RBOX_CENTERX,RBOX_CENTERY,3)
    DO_SQUARE(LBOX_CENTERX+(calibratedStick[0]*BOX_LIMIT/(2*RESX)), LBOX_CENTERY-(calibratedStick[1]*BOX_LIMIT/(2*RESX)), MARKER_WIDTH)
    DO_SQUARE(RBOX_CENTERX+(calibratedStick[3]*BOX_LIMIT/(2*RESX)), RBOX_CENTERY-(calibratedStick[2]*BOX_LIMIT/(2*RESX)), MARKER_WIDTH)

    // Optimization by Mike Blandford
    {
        uint8_t x, y, len ;			// declare temporary variables
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

uint16_t isqrt32(uint32_t n)
{
    uint16_t c = 0x8000;
    uint16_t g = 0x8000;

    for(;;) {
        if((uint32_t)g*g > n)
            g ^= c;
        c >>= 1;
        if(c == 0)
            return g;
        g |= c;
    }
}

int16_t intpol(int16_t x, uint8_t idx) // -100, -75, -50, -25, 0 ,25 ,50, 75, 100
{
#define D9 (RESX * 2 / 8)
#define D5 (RESX * 2 / 4)
  bool    cv9 = idx >= MAX_CURVE5;
  int8_t *crv = cv9 ? g_model.curves9[idx-MAX_CURVE5] : g_model.curves5[idx];
  int16_t erg;

  x+=RESXu;
  if(x < 0) {
    erg = (int16_t)crv[0] * (RESX/4);
  } else if(x >= (RESX*2)) {
    erg = (int16_t)crv[(cv9 ? 8 : 4)] * (RESX/4);
  } else {
    int16_t a,dx;
    if(cv9){
      a   = (uint16_t)x / D9;
      dx  =((uint16_t)x % D9) * 2;
    } else {
      a   = (uint16_t)x / D5;
      dx  = (uint16_t)x % D5;
    }
    erg  = (int16_t)crv[a]*((D5-dx)/2) + (int16_t)crv[a+1]*(dx/2);
  }
  return erg / 25; // 100*D5/RESX;
}

// static variables used in perOut - moved here so they don't interfere with the stack
// It's also easier to initialize them here.
uint16_t pulses2MHz[120] = {0};
int16_t  anas [NUM_XCHNRAW] = {0};
int32_t  chans[NUM_CHNOUT] = {0};
uint32_t inacCounter = 0;
uint16_t inacSum = 0;
uint8_t  bpanaCenter = 0;
int16_t  sDelay[MAX_MIXERS] = {0};
int32_t  act   [MAX_MIXERS] = {0};
uint8_t  swOn  [MAX_MIXERS] = {0};
uint8_t mixWarning;

void perOut(int16_t *chanOut, uint8_t att)
{
  int16_t  trimA[4];
  uint8_t  anaCenter = 0;


  if(tick10ms) {
    if(s_noHi) s_noHi--;
    if(g_eeGeneral.inactivityTimer && g_vbat100mV>49) {
      inacCounter++;
      uint16_t tsum = 0;
      for(uint8_t i=0;i<4;i++) tsum += anas[i]/128;//div 8 -> reduce sensitivity
      if(tsum!=inacSum){
        inacSum = tsum;
        inacCounter=0;
      }
      if(inacCounter>((uint32_t)g_eeGeneral.inactivityTimer*100*60))
        if((inacCounter&0x3F)==10) beepWarn();
    }
  }

#ifdef HELI
  uint16_t d = 0;
  if(g_model.swashR.value) {
    uint32_t v = (int32_t(calibratedStick[ELE_STICK])*calibratedStick[ELE_STICK] +
        int32_t(calibratedStick[AIL_STICK])*calibratedStick[AIL_STICK]);
    uint32_t q = int32_t(RESX)*g_model.swashR.value/100;
    q *= q;
    if(v>q)
      d = isqrt32(v);
  }
#endif

  for(uint8_t i=0;i<7;i++){        // calc Sticks

    //Normalization  [0..2048] ->   [-1024..1024]

    int16_t v = anaIn(i);
    v -= g_eeGeneral.calibMid[i];
    v  =  v * (int32_t)RESX /  (max((int16_t)100,(v>0 ?
                                     g_eeGeneral.calibSpanPos[i] :
                                     g_eeGeneral.calibSpanNeg[i])));
    if(v <= -RESX) v = -RESX;
    if(v >=  RESX) v =  RESX;
    calibratedStick[i] = v; //for show in expo
    if(!(v/16)) anaCenter |= 1<<(CONVERT_MODE((i+1))-1);


    if(i<4) { //only do this for sticks
      if (!(att&NO_TRAINER) && g_model.traineron) {
        // trainer mode
        TrainerMix* td = &g_eeGeneral.trainer.mix[i];
      	if (td->mode && getSwitch(td->swtch, 1)) {
      	  uint8_t chStud = td->srcChn;
      	  int16_t vStud  = (g_ppmIns[chStud]- g_eeGeneral.trainer.calib[chStud]) /* *2 */ ;
      	  vStud /= 2 ;		// Only 2, because no *2 above
      	  vStud *= td->studWeight ;
      	  vStud /= 31 ;
      	  vStud *= 4 ;
      	  switch (td->mode) {
      	    case 1: v += vStud;   break; // add-mode
      	    case 2: v  = vStud;   break; // subst-mode
      	  }
      	}
      }

#ifdef HELI
      if(d && (i==ELE_STICK || i==AIL_STICK))
        v = int32_t(v)*g_model.swashR.value*RESX/(int32_t(d)*100);
#endif
      
      uint8_t expoDrOn = GET_DR_STATE(i);
      uint8_t stkDir = v>0 ? DR_RIGHT : DR_LEFT;

      if(IS_THROTTLE(i) && g_model.thrExpo){
        v  = 2*expo((v+RESX)/2,g_model.expoData[i].expo[expoDrOn][DR_EXPO][DR_RIGHT]);
        stkDir = DR_RIGHT;
      }
      else
        v  = expo(v,g_model.expoData[i].expo[expoDrOn][DR_EXPO][stkDir]);

      int32_t x = (int32_t)v * (g_model.expoData[i].expo[expoDrOn][DR_WEIGHT][stkDir]+100)/100;
      v = (int16_t)x;
      if (IS_THROTTLE(i) && g_model.thrExpo) v -= RESX;

      //do trim -> throttle trim if applicable
      int32_t vv = 2*RESX;
      if(IS_THROTTLE(i) && g_model.thrTrim) vv = (g_eeGeneral.throttleReversed) ?
                                 ((int32_t)g_model.trim[0][i]-125)*(RESX+v)/(2*RESX) :
                                 ((int32_t)g_model.trim[0][i]+125)*(RESX-v)/(2*RESX);

      //trim
      trimA[i] = (vv==2*RESX) ? g_model.trim[0][i]*2 : (int16_t)vv*2; //    if throttle trim -> trim low end
    }
    anas[i] = v; //set values for mixer
  }

  //===========BEEP CENTER================
  anaCenter &= g_model.beepANACenter;
  if(((bpanaCenter ^ anaCenter) & anaCenter)) beepWarn1();
  bpanaCenter = anaCenter;

  anas[MIX_MAX-1]  = RESX;     // MAX
  anas[MIX_FULL-1] = RESX;     // FULL

#ifdef HELI
  if(g_model.swashR.value)
  {
      uint32_t v = ((int32_t)anas[ELE_STICK]*anas[ELE_STICK] + (int32_t)anas[AIL_STICK]*anas[AIL_STICK]);
      uint32_t q = (int32_t)RESX*g_model.swashR.value/100;
      q *= q;
      if(v>q)
      {
          uint16_t d = isqrt32(v);
          anas[ELE_STICK] = (int32_t)anas[ELE_STICK]*g_model.swashR.value*RESX/((int32_t)d*100);
          anas[AIL_STICK] = (int32_t)anas[AIL_STICK]*g_model.swashR.value*RESX/((int32_t)d*100);
      }
  }

#define REZ_SWASH_X(x)  ((x) - (x)/8 - (x)/128 - (x)/512)   //  1024*sin(60) ~= 886
#define REZ_SWASH_Y(x)  ((x))   //  1024 => 1024

  if(g_model.swashR.type)
  {
      int16_t vp = anas[ELE_STICK]+trimA[ELE_STICK];
      int16_t vr = anas[AIL_STICK]+trimA[AIL_STICK];
      int16_t vc = 0;
      if(g_model.swashR.collectiveSource)
          vc = anas[g_model.swashR.collectiveSource-1];

      if(g_model.swashR.invertELE) vp = -vp;
      if(g_model.swashR.invertAIL) vr = -vr;
      if(g_model.swashR.invertCOL) vc = -vc;

      switch (g_model.swashR.type)
      {
      case (SWASH_TYPE_120):
          vp = REZ_SWASH_Y(vp);
          vr = REZ_SWASH_X(vr);
          anas[MIX_CYC1-1] = vc - vp;
          anas[MIX_CYC2-1] = vc + vp/2 + vr;
          anas[MIX_CYC3-1] = vc + vp/2 - vr;
          break;
      case (SWASH_TYPE_120X):
          vp = REZ_SWASH_X(vp);
          vr = REZ_SWASH_Y(vr);
          anas[MIX_CYC1-1] = vc - vr;
          anas[MIX_CYC2-1] = vc + vr/2 + vp;
          anas[MIX_CYC3-1] = vc + vr/2 - vp;
          break;
      case (SWASH_TYPE_140):
          vp = REZ_SWASH_Y(vp);
          vr = REZ_SWASH_Y(vr);
          anas[MIX_CYC1-1] = vc - vp;
          anas[MIX_CYC2-1] = vc + vp + vr;
          anas[MIX_CYC3-1] = vc + vp - vr;
          break;
      case (SWASH_TYPE_90):
          vp = REZ_SWASH_Y(vp);
          vr = REZ_SWASH_Y(vr);
          anas[MIX_CYC1-1] = vc - vp;
          anas[MIX_CYC2-1] = vc + vr;
          anas[MIX_CYC3-1] = vc - vr;
          break;
      default:
          break;
      }
  }
#endif
  
  if(att&NO_INPUT) { //zero input for setStickCenter()
    for(uint8_t i=0;i<4;i++) {
      if(!IS_THROTTLE(i)) {
        anas[i]  = 0;
        trimA[i] = 0;
      }
    }
    for(uint8_t i=0;i<NUM_PPM;i++) anas[i+PPM_BASE] = 0;
  }
  else {
    for(uint8_t i=0;i<NUM_CAL_PPM;i++)       anas[i+PPM_BASE]   = (g_ppmIns[i] - g_eeGeneral.trainer.calib[i])*2; //add ppm channels
    for(uint8_t i=NUM_CAL_PPM;i<NUM_PPM;i++) anas[i+PPM_BASE]   = g_ppmIns[i]*2; //add ppm channels
  }

  for(uint8_t i=CHOUT_BASE;i<NUM_XCHNRAW;i++) anas[i] = chans[i-CHOUT_BASE]; //other mixes previous outputs

  if(tick10ms) trace(); //trace thr 0..32  (/32)

  memset(chans,0,sizeof(chans));        // All outputs to 0
  
    //========== MIXER LOOP ===============
    mixWarning = 0;
    for(uint8_t i=0;i<MAX_MIXERS;i++){
      MixData &md = g_model.mixData[i];

      if((md.destCh==0) || (md.destCh>NUM_CHNOUT)) break;

      //Notice 0 = NC switch means not used -> always on line
      int16_t v  = 0;
      uint8_t swTog;

      //swOn[i]=false;
      if(!getSwitch(md.swtch,1)){ // switch on?  if no switch selected => on
        swTog = swOn[i];
        swOn[i] = false;
        if(md.srcRaw!=MIX_MAX && md.srcRaw!=MIX_FULL) continue;// if not MAX or FULL - next loop
        if(md.mltpx==MLTPX_REP) continue; // if switch is off and REPLACE then off
        v = (md.srcRaw == MIX_FULL ? -RESX : 0); // switch is off and it is either MAX=0 or FULL=-512
      }
      else {
        swTog = !swOn[i];
        swOn[i] = true;
        v = anas[md.srcRaw-1]; //Switch is on. MAX=FULL=512 or value.
        if(md.mixWarn) mixWarning |= 1<<(md.mixWarn-1); // Mix warning
      }

      //========== INPUT OFFSET ===============
      if(md.sOffset) v += calc100toRESX(md.sOffset);

      //========== DELAY and PAUSE ===============
      if (md.speedUp || md.speedDown || md.delayUp || md.delayDown)  // there are delay values
      {
#define DEL_MULT 256

        //if(init) {
          //act[i]=(int32_t)v*DEL_MULT;
          //swTog = false;
        //}
        int16_t diff = v-act[i]/DEL_MULT;

        if(swTog) {
            //need to know which "v" will give "anas".
            //curves(v)*weight/100 -> anas
            // v * weight / 100 = anas => anas*100/weight = v
          if(md.mltpx==MLTPX_REP)
          {
              act[i] = (int32_t)anas[md.destCh-1+CHOUT_BASE]*DEL_MULT;
              act[i] *=100;
              if(md.weight) act[i] /= md.weight;
          }
          diff = v-act[i]/DEL_MULT;
          if(diff) sDelay[i] = (diff<0 ? md.delayUp :  md.delayDown) * 100;
        }

        if(sDelay[i]){ // perform delay
            if(tick10ms) sDelay[i]--;
            v = act[i]/DEL_MULT;
            diff = 0;
        }

        if(diff && (md.speedUp || md.speedDown)){
          //rate = steps/sec => 32*1024/100*md.speedUp/Down
          //act[i] += diff>0 ? (32768)/((int16_t)100*md.speedUp) : -(32768)/((int16_t)100*md.speedDown);
          //-100..100 => 32768 ->  100*83886/256 = 32768,   For MAX we divide by 2 sincde it's asymmetrical
          if(tick10ms) {
              int32_t rate = (int32_t)DEL_MULT*2048*100;
              if(md.weight) rate /= abs(md.weight);
              act[i] = (diff>0) ? ((md.speedUp>0)   ? act[i]+(rate)/((int16_t)100*md.speedUp)   :  (int32_t)v*DEL_MULT) :
                                  ((md.speedDown>0) ? act[i]-(rate)/((int16_t)100*md.speedDown) :  (int32_t)v*DEL_MULT) ;
          }

          if(((diff>0) && (v<(act[i]/DEL_MULT))) || ((diff<0) && (v>(act[i]/DEL_MULT)))) act[i]=(int32_t)v*DEL_MULT; //deal with overflow
          v = act[i]/DEL_MULT;
        }
      }


      //========== CURVES ===============
      switch(md.curve){
        case 0:
          break;
        case 1:
          if(md.srcRaw == MIX_FULL) //FUL
          {
            if( v<0 ) v=-RESX;   //x|x>0
            else      v=-RESX+2*v;
          }else{
            if( v<0 ) v=0;   //x|x>0
          }
          break;
        case 2:
          if(md.srcRaw == MIX_FULL) //FUL
          {
            if( v>0 ) v=RESX;   //x|x<0
            else      v=RESX+2*v;
          }else{
            if( v>0 ) v=0;   //x|x<0
          }
          break;
        case 3:       // x|abs(x)
          v = abs(v);
          break;
        case 4:       //f|f>0
          v = v>0 ? RESX : 0;
          break;
        case 5:       //f|f<0
          v = v<0 ? -RESX : 0;
          break;
        case 6:       //f|abs(f)
          v = v>0 ? RESX : -RESX;
          break;
        default: //c1..c16
          v = intpol(v, md.curve - 7);
      }

      //========== TRIM ===============
      if((md.carryTrim==0) && (md.srcRaw>0) && (md.srcRaw<=4)) v += trimA[md.srcRaw-1];  //  0 = Trim ON  =  Default

      //========== MULTIPLEX ===============
      int32_t dv = (int32_t)v*md.weight;
      switch(md.mltpx){
        case MLTPX_REP:
          chans[md.destCh-1] = dv;
          break;
        case MLTPX_MUL:
          chans[md.destCh-1] *= dv/100l;
          chans[md.destCh-1] /= RESXl;
          break;
        default:  // MLTPX_ADD
          chans[md.destCh-1] += dv; //Mixer output add up to the line (dv + (dv>0 ? 100/2 : -100/2))/(100);
          break;
        }
    }

  //========== MIXER WARNING ===============
  // 1,2 or 3 "bips" (short beeps)
  //1= 00,08
  //2= 24,32,40
  //3= 56,64,72,80
  // Gruvin:  Sometimes, one or more of these beeps do not fire. That will be because the tmr10ms counter
  //          may not necessarily be exactly (==) the below figures when queried from inside perOut().
  //          But we only ever want a beep to fire once, so we have to use anexact counter match (not a range).
  //          My solution was to make mixWarning global and have the counter checks done inside per10ms();
//  if(mixWarning & 1) if(((g_tmr10ms&0xFF)==  0)) beepWarn1();
//  if(mixWarning & 2) if(((g_tmr10ms&0xFF)== 64) || ((g_tmr10ms&0xFF)== 72)) beepWarn1();
//  if(mixWarning & 4) if(((g_tmr10ms&0xFF)==128) || ((g_tmr10ms&0xFF)==136) || ((g_tmr10ms&0xFF)==144)) beepWarn1();


  //========== LIMITS ===============
  for(uint8_t i=0;i<NUM_CHNOUT;i++){
      // chans[i] holds data from mixer.   chans[i] = v*weight => 1024*100
      // later we multiply by the limit (up to 100) and then we need to normalize
      // at the end chans[i] = chans[i]/100 =>  -1024..1024
      // interpolate value with min/max so we get smooth motion from center to stop
      // this limits based on v original values and min=-1024, max=1024  RESX=1024

      int32_t q = chans[i];// + (int32_t)g_model.limitData[i].offset*100; // offset before limit

      chans[i] /= 100; // chans back to -1024..1024
      ex_chans[i] = chans[i]; //for getswitch

      int16_t ofs = g_model.limitData[i].offset;
      int16_t lim_p = 10*(g_model.limitData[i].max+100);
      int16_t lim_n = 10*(g_model.limitData[i].min-100); //multiply by 10 to get same range as ofs (-1000..1000)
      if(ofs>lim_p) ofs = lim_p;
      if(ofs<lim_n) ofs = lim_n;

      if(q) q = (q>0) ?
                q*((int32_t)lim_p-ofs)/100000 :
               -q*((int32_t)lim_n-ofs)/100000 ; //div by 100000 -> output = -1024..1024

      q += calc1000toRESX(ofs);
      lim_p = calc1000toRESX(lim_p);
      lim_n = calc1000toRESX(lim_n);
      if(q>lim_p) q = lim_p;
      if(q<lim_n) q = lim_n;
      if(g_model.limitData[i].revert) q=-q;// finally do the reverse.

      if(g_model.safetySw[i].swtch)  //if safety sw available for channel check and replace val if needed
          if(getSwitch(g_model.safetySw[i].swtch,0)) q = calc100toRESX(g_model.safetySw[i].val);

      cli();
      chanOut[i] = q; //copy consistent word to int-level
      sei();
  }
}



/******************************************************************************
  the functions below are from int-level
  the functions below are from int-level
  the functions below are from int-level
******************************************************************************/

void setupPulses()
{
  switch(g_model.protocol)
  {
    case PROTO_PPM:
      setupPulsesPPM();
      break;
    case PROTO_SILV_A:
    case PROTO_SILV_B:
    case PROTO_SILV_C:
      setupPulsesSilver();
      break;
    case PROTO_TRACER_CTP1009:
      setupPulsesTracerCtp1009();
      break;
  }
}

//inline int16_t reduceRange(int16_t x)  // for in case we want to have room for subtrims
//{
//    return x-(x/4);  //512+128 =? 640,  640 - 640/4  == 640 * 3/4 => 480 (just below 500msec - it can still reach 500 with offset)
//}

void setupPulsesPPM() // changed 10/05/2010 by dino Issue 128
{
#define PPM_CENTER 1200*2
    int16_t PPM_range = g_model.extendedLimits ? 640*2 : 512*2;   //range of 0.7..1.7msec

    //Total frame length = 22.5msec
    //each pulse is 0.7..1.7ms long with a 0.3ms stop tail
    //The pulse ISR is 2mhz that's why everything is multiplied by 2
    uint8_t j=0;
    uint8_t p=8+g_model.ppmNCH*2; //Channels *2
    uint16_t q=(g_model.ppmDelay*50+300)*2; //Stoplen *2
    uint16_t rest=22500u*2-q; //Minimum Framelen=22.5 ms
    if(p>9) rest=p*(1720u*2 + q) + 4000u*2; //for more than 9 channels, frame must be longer
    for(uint8_t i=0;i<p;i++){ //NUM_CHNOUT
        int16_t v = max(min(g_chans512[i],PPM_range),-PPM_range) + PPM_CENTER;
        rest-=(v+q);
        pulses2MHz[j++] = q;
        pulses2MHz[j++] = v - q + 600; /* as Pat MacKenzie suggests */
    }
    pulses2MHz[j++]=q;
    pulses2MHz[j++]=rest;
    pulses2MHz[j++]=0;
}


uint16_t *pulses2MHzPtr;
#define BITLEN (600u*2)
void _send_hilo(uint16_t hi,uint16_t lo)
{
  *pulses2MHzPtr++=hi; *pulses2MHzPtr++=lo;
}
#define send_hilo_silv( hi, lo) _send_hilo( (hi)*BITLEN,(lo)*BITLEN )

void sendBitSilv(uint8_t val)
{
  send_hilo_silv((val)?2:1,(val)?2:1);
}
void send2BitsSilv(uint8_t val)
{
  sendBitSilv(val&2);sendBitSilv(val&1);
}
// _ oder - je 0.6ms  (gemessen 0.7ms)
//
//____-----_-_-_--_--_   -_--__  -_-_-_-_  -_-_-_-_  --__--__-_______
//         trailer        chan     m1         m2
//
//see /home/thus/txt/silverlit/thus.txt
//m1, m2 most significant bit first |m1-m2| <= 9
//chan: 01=C 10=B
//chk = 0 - chan -m1>>2 -m1 -m2>>2 -m2
//<= 500us Probleme
//>= 650us Probleme
//periode orig: 450ms
void setupPulsesSilver()
{
  int8_t chan=1; //chan 1=C 2=B 0=A?

  switch(g_model.protocol)
  {
    case PROTO_SILV_A: chan=0; break;
    case PROTO_SILV_B: chan=2; break;
    case PROTO_SILV_C: chan=1; break;
  }

  int8_t m1 = (uint16_t)(g_chans512[0]+1024)*2 / 256;
  int8_t m2 = (uint16_t)(g_chans512[1]+1024)*2 / 256;
  if (m1 < 0)    m1=0;
  if (m2 < 0)    m2=0;
  if (m1 > 15)   m1=15;
  if (m2 > 15)   m2=15;
  if (m2 > m1+9) m1=m2-9;
  if (m1 > m2+9) m2=m1-9;
  //uint8_t i=0;
  pulses2MHzPtr=pulses2MHz;
  send_hilo_silv(5,1); //idx 0 erzeugt pegel=0 am Ausgang, wird  als high gesendet
  send2BitsSilv(0);
  send_hilo_silv(2,1);
  send_hilo_silv(2,1);

  send2BitsSilv(chan); //chan 1=C 2=B 0=A?
  uint8_t sum = 0 - chan;

  send2BitsSilv(m1>>2); //m1
  sum-=m1>>2;
  send2BitsSilv(m1);
  sum-=m1;

  send2BitsSilv(m2>>2); //m2
  sum-=m2>>2;
  send2BitsSilv(m2);
  sum-=m2;

  send2BitsSilv(sum); //chk

  sendBitSilv(0);
  pulses2MHzPtr--;
  send_hilo_silv(50,0); //low-impuls (pegel=1) ueberschreiben


}



/*
  TRACE CTP-1009
   - = send 45MHz
   _ = send nix
    start1       0      1           start2
  -------__     --_    -__         -----__
   7ms   2     .8 .4  .4 .8         5   2

 frame:
  start1  24Bits_1  start2  24_Bits2

 24Bits_1:
  7 x Bits  Throttle lsb first
  1 x 0

  6 x Bits  rotate lsb first
  1 x Bit   1=rechts
  1 x 0

  4 x Bits  chk5 = nib2 ^ nib4
  4 x Bits  chk6 = nib1 ^ nib3

 24Bits_2:
  7 x Bits  Vorwaets lsb first 0x3f = mid
  1 x 1

  7 x Bits  0x0e lsb first
  1 x 1

  4 x Bits  chk5 = nib2 ^ nib4
  4 x Bits  chk6 = nib1 ^ nib3

 */

#define BIT_TRA (400u*2)
void sendBitTra(uint8_t val)
{
  if(val) _send_hilo( BIT_TRA*1 , BIT_TRA*2 );
  else    _send_hilo( BIT_TRA*2 , BIT_TRA*1 );
}
void sendByteTra(uint8_t val)
{
  for(uint8_t i=0; i<8; i++, val>>=1) sendBitTra(val&1);
}
void setupPulsesTracerCtp1009()
{
  pulses2MHzPtr=pulses2MHz;
  static bool phase;
  if( (phase=!phase) ){
    uint8_t thr = min(127u,(uint16_t)(g_chans512[0]+1024+8) /  16u);
    uint8_t rot;
    if (g_chans512[1] >= 0)
    {
      rot = min(63u,(uint16_t)( g_chans512[1]+16) / 32u) | 0x40;
    }else{
      rot = min(63u,(uint16_t)(-g_chans512[1]+16) / 32u);
    }
    sendByteTra(thr);
    sendByteTra(rot);
    uint8_t chk=thr^rot;
    sendByteTra( (chk>>4) | (chk<<4) );
    _send_hilo( 5000*2, 2000*2 );
  }else{
    uint8_t fwd = min(127u,(uint16_t)(g_chans512[2]+1024) /  16u) | 0x80;
    sendByteTra(fwd);
    sendByteTra(0x8e);
    uint8_t chk=fwd^0x8e;
    sendByteTra( (chk>>4) | (chk<<4) );
    _send_hilo( 7000*2, 2000*2 );
  }
  *pulses2MHzPtr++=0;
  if((pulses2MHzPtr-pulses2MHz) >= (signed)DIM(pulses2MHz)) alert(PSTR("pulse tab overflow"));
}
