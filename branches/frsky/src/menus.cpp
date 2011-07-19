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
#include "menus.h"

int16_t calibratedStick[NUM_STICKS+NUM_POTS];
int16_t ex_chans[NUM_CHNOUT];          // Outputs + intermidiates
uint8_t s_pgOfs;
uint8_t s_editMode;
uint8_t s_noHi;

int16_t g_chans512[NUM_CHNOUT];

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

MenuFuncP g_menuStack[5];
uint8_t g_menuStackPtr = 0;

MenuFuncP lastPopMenu()
{
  return  g_menuStack[g_menuStackPtr+1];
}

void popMenu(bool uppermost)
{
  if(g_menuStackPtr>0){
    g_menuStackPtr = uppermost ? 0 : g_menuStackPtr-1;
    beepKey();
    (*g_menuStack[g_menuStackPtr])(EVT_ENTRY_UP);
  }else{
    alert(PSTR("menuStack underflow"));
  }
}

void chainMenu(MenuFuncP newMenu)
{
  g_menuStack[g_menuStackPtr] = newMenu;
  (*newMenu)(EVT_ENTRY);
  beepKey();
}
void pushMenu(MenuFuncP newMenu)
{

  g_menuStackPtr++;
  if(g_menuStackPtr >= DIM(g_menuStack))
  {
    g_menuStackPtr--;
    alert(PSTR("menuStack overflow"));
    return;
  }
  beepKey();
  g_menuStack[g_menuStackPtr] = newMenu;
  (*newMenu)(EVT_ENTRY);
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

void timer(uint8_t val)
{
  int8_t tm = g_model.tmrMode;
  static uint16_t s_time;
  static uint16_t s_cnt;
  static uint16_t s_sum;
  static uint8_t sw_toggled;

  if(abs(tm)>=(TMR_VAROFS+MAX_SWITCH-1)){ //toggeled switch//abs(g_model.tmrMode)<(10+MAX_SWITCH-1)
    static uint8_t lastSwPos;
    if(!(sw_toggled | s_sum | s_cnt | s_time | lastSwPos)) lastSwPos = tm < 0;  // if initializing then init the lastSwPos
    uint8_t swPos = getSwitch(tm>0 ? tm-(TMR_VAROFS+MAX_SWITCH-1-1) : tm+(TMR_VAROFS+MAX_SWITCH-1-1) ,0);
    if(swPos && !lastSwPos)  sw_toggled = !sw_toggled;  //if switcdh is flipped first time -> change counter state
    lastSwPos = swPos;
  }

  s_cnt++;
  s_sum+=val;
  if((get_tmr10ms()-s_time)<100) return; //1 sec
  s_time += 100;
  val     = s_sum/s_cnt;
  s_sum  -= val*s_cnt; //rest
  s_cnt   = 0;

  if(abs(tm)<TMR_VAROFS) sw_toggled = false; // not switch - sw timer off
  else if(abs(tm)<(TMR_VAROFS+MAX_SWITCH-1)) sw_toggled = getSwitch((tm>0 ? tm-(TMR_VAROFS-1) : tm+(TMR_VAROFS-1)) ,0); //normal switch

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
  if((get_tmr10ms()-s_time)<1000) //10 sec
    return;
  s_time= get_tmr10ms();
  val   = s_sum/s_cnt;
  s_sum = 0;
  s_cnt = 0;

  s_traceCnt++;
  s_traceBuf[s_traceWr++] = val;
  if(s_traceWr>=MAXTRACE) s_traceWr=0;
}

void resetTimer()
{
    s_timerState = TMR_OFF; //is changed to RUNNING dep from mode
    s_timeCumAbs=0;
    s_timeCumThr=0;
    s_timeCumSw=0;
    s_timeCum16ThrP=0;
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

// TODO should be in gruvin9x.cpp
int16_t applyCurve(int16_t x, uint8_t idx, uint8_t srcRaw)
{
  switch(idx) {
  case 0:
    return x;
  case 1:
    if (srcRaw == MIX_FULL) { //FULL
      if (x<0 ) x=-RESX;   //x|x>0
      else x=-RESX+2*x;
    }
    else {
      if (x<0) x=0;   //x|x>0
    }
    return x;
  case 2:
    if (srcRaw == MIX_FULL) { //FULL
      if (x>0) x=RESX;   //x|x<0
      else x=RESX+2*x;
    }
    else {
      if (x>0) x=0;   //x|x<0
    }
    return x;
  case 3:       // x|abs(x)
    return abs(x);
  case 4:       //f|f>0
    return x>0 ? RESX : 0;
  case 5:       //f|f<0
    return x<0 ? -RESX : 0;
  case 6:       //f|abs(f)
    return x>0 ? RESX : -RESX;
  }
  return intpol(x, idx-7);
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

// TODO should be in gruvin9x.cpp
void applyExpos(int16_t *anas)
{
  static int16_t anas2[4]; // values before expo, to ensure same expo base when multiple expo lines are used
  memcpy(anas2, anas, sizeof(anas2));

  uint8_t flightPhase = getFlightPhase(false);

  for (uint8_t i=0; i<DIM(g_model.expoData); i++) {
    ExpoData &ed = g_model.expoData[i];
    if (ed.mode==0) break; // end of list
    // TODO duplicate
    if (ed.flightPhase != 0) {
      if (ed.flightPhase > 0) {
        if (flightPhase != ed.flightPhase)
          continue;
      }
      else {
        if (flightPhase == -ed.flightPhase)
          continue;
      }
    }
    if (getSwitch(ed.swtch, 1)) {
      int16_t v = anas2[ed.chn];
      if((v<0 && ed.mode&1) || (v>=0 && ed.mode&2)) {
        int16_t k = ed.expo;
        if (IS_THROTTLE(i) && g_model.thrExpo)
          v = 2*expo((v+RESX)/2, k);
        else
          v = expo(v, k);
        if (ed.curve) v = applyCurve(v, ed.curve, 0);
        v = ((int32_t)v * ed.weight) / 100;
        if (IS_THROTTLE(i) && g_model.thrExpo) v -= RESX;
        anas[ed.chn] = v;
      }
    }
  }
}

void perOut(int16_t *chanOut, uint8_t att)
{
  int16_t  trimA[4];
  uint8_t  anaCenter = 0;

  if(tick10ms) {
    if(s_noHi) s_noHi--;
      if (g_eeGeneral.inactivityTimer && g_vbat100mV>49) {
      inacCounter++;
      uint16_t tsum = 0;
      for(uint8_t i=0;i<4;i++) tsum += anaIn(i)/64;  // reduce sensitivity
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

  uint8_t flightPhaseTrim = getFlightPhase(true);
  uint8_t flightPhase = getFlightPhase(false);

  for(uint8_t i=0;i<7;i++){        // Sticks & Pots

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


    if (i<4) { //only do this for sticks
      if (!(att&NO_TRAINER) && g_model.traineron) {
        // trainer mode
        TrainerMix* td = &g_eeGeneral.trainer.mix[i];
        if (td->mode && getSwitch(td->swtch, 1)) {
          uint8_t chStud = td->srcChn;
          int16_t vStud  = (g_ppmIns[chStud]- g_eeGeneral.trainer.calib[chStud]) /* *2 */ ;
          vStud /= 2 ;      // Only 2, because no *2 above
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
      
    }
    anas[i] = v; //set values for mixer
  }

  /* EXPOs */
  applyExpos(anas);

  /* TRIMs */
  for(uint8_t i=0; i<4; i++) {
      // do trim -> throttle trim if applicable
      int16_t v = anas[i];
      int32_t vv = 2*RESX;
      if(IS_THROTTLE(i) && g_model.thrTrim) vv = (g_eeGeneral.throttleReversed) ?
                                 ((int32_t)g_model.trim[flightPhaseTrim][i]-125)*(RESX+v)/(2*RESX) :
                                 ((int32_t)g_model.trim[flightPhaseTrim][i]+125)*(RESX-v)/(2*RESX);

      //trim
      trimA[i] = (vv==2*RESX) ? g_model.trim[flightPhaseTrim][i]*2 : (int16_t)vv*2; //    if throttle trim -> trim low end
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
      MixData *md = mixaddress( i ) ;

      if((md->destCh==0) || (md->destCh>NUM_CHNOUT)) break;

      if (md->flightPhase != 0) {
        if (md->flightPhase > 0) {
          if (flightPhase != md->flightPhase)
            continue;
        }
        else {
          if (flightPhase == -md->flightPhase)
            continue;
        }
      }

      //Notice 0 = NC switch means not used -> always on line
      int16_t v  = 0;
      uint8_t swTog;

      //swOn[i]=false;
      if(!getSwitch(md->swtch,1)){ // switch on?  if no switch selected => on
        swTog = swOn[i];
        swOn[i] = false;
        if(md->srcRaw!=MIX_MAX && md->srcRaw!=MIX_FULL) continue;// if not MAX or FULL - next loop
        if(md->mltpx==MLTPX_REP) continue; // if switch is off and REPLACE then off
        v = (md->srcRaw == MIX_FULL ? -RESX : 0); // switch is off and it is either MAX=0 or FULL=-512
      }
      else {
        swTog = !swOn[i];
        swOn[i] = true;
        uint8_t k = md->srcRaw-1;
        v = anas[k]; //Switch is on. MAX=FULL=512 or value.
        if(k>=CHOUT_BASE && (k<i)) v = chans[k]; // if we've already calculated the value - take it instead // anas[i+CHOUT_BASE] = chans[i]
        if(md->mixWarn) mixWarning |= 1<<(md->mixWarn-1); // Mix warning
      }

      //========== INPUT OFFSET ===============
      if(md->sOffset) v += calc100toRESX(md->sOffset);

      //========== DELAY and PAUSE ===============
      if (md->speedUp || md->speedDown || md->delayUp || md->delayDown)  // there are delay values
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
          if(md->mltpx==MLTPX_REP)
          {
              act[i] = (int32_t)anas[md->destCh-1+CHOUT_BASE]*DEL_MULT;
              act[i] *=100;
              if(md->weight) act[i] /= md->weight;
          }
          diff = v-act[i]/DEL_MULT;
          if(diff) sDelay[i] = (diff<0 ? md->delayUp :  md->delayDown) * 100;
        }

        if(sDelay[i]){ // perform delay
            if(tick10ms) sDelay[i]--;
            v = act[i]/DEL_MULT;
            diff = 0;
        }

        if(diff && (md->speedUp || md->speedDown)){
          //rate = steps/sec => 32*1024/100*md->speedUp/Down
          //act[i] += diff>0 ? (32768)/((int16_t)100*md->speedUp) : -(32768)/((int16_t)100*md->speedDown);
          //-100..100 => 32768 ->  100*83886/256 = 32768,   For MAX we divide by 2 sincde it's asymmetrical
          if(tick10ms) {
              int32_t rate = (int32_t)DEL_MULT*2048*100;
              if(md->weight) rate /= abs(md->weight);
              act[i] = (diff>0) ? ((md->speedUp>0)   ? act[i]+(rate)/((int16_t)100*md->speedUp)   :  (int32_t)v*DEL_MULT) :
                                  ((md->speedDown>0) ? act[i]-(rate)/((int16_t)100*md->speedDown) :  (int32_t)v*DEL_MULT) ;
          }

          if(((diff>0) && (v<(act[i]/DEL_MULT))) || ((diff<0) && (v>(act[i]/DEL_MULT)))) act[i]=(int32_t)v*DEL_MULT; //deal with overflow
          v = act[i]/DEL_MULT;
        }
      }


      //========== CURVES ===============
      if (md->curve)
        v = applyCurve(v, md->curve, md->srcRaw);

      //========== TRIM ===============
      if((md->carryTrim==0) && (md->srcRaw>0) && (md->srcRaw<=4)) v += trimA[md->srcRaw-1];  //  0 = Trim ON  =  Default

      //========== MULTIPLEX ===============
      int32_t dv = (int32_t)v*md->weight;
      switch(md->mltpx){
        case MLTPX_REP:
          chans[md->destCh-1] = dv;
          break;
        case MLTPX_MUL:
          chans[md->destCh-1] *= dv/100l;
          chans[md->destCh-1] /= RESXl;
          break;
        default:  // MLTPX_ADD
          chans[md->destCh-1] += dv; //Mixer output add up to the line (dv + (dv>0 ? 100/2 : -100/2))/(100);
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
  /* {
    uint16_t tmr10ms ;
    tmr10ms = get_tmr10ms() ;
    if(mixWarning & 1) if(((tmr10ms&0xFF)==  0)) beepWarn1();
    if(mixWarning & 2) if(((tmr10ms&0xFF)== 64) || ((tmr10ms&0xFF)== 72)) beepWarn1();
    if(mixWarning & 4) if(((tmr10ms&0xFF)==128) || ((tmr10ms&0xFF)==136) || ((tmr10ms&0xFF)==144)) beepWarn1();
  } */

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
        int16_t v = max(min(g_chans512[i],(int16_t)PPM_range),(int16_t)-PPM_range) + (int16_t)PPM_CENTER;
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
