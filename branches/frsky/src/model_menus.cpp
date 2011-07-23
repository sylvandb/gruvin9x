#include "menus.h"
#include "templates.h"

enum EnumTabModel {
  e_ModelSelect,
  e_Model,
#ifdef HELI
  e_Heli,
#endif
  e_ExpoAll,
  e_MixAll,
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

void menuProcModelSelect(uint8_t event);
void menuProcModel(uint8_t event);
#ifdef HELI
void menuProcHeli(uint8_t event);
#endif
void menuProcExpoAll(uint8_t event);
void menuProcMixAll(uint8_t event);
void menuProcLimits(uint8_t event);
void menuProcCurve(uint8_t event);
void menuProcCustomSwitches(uint8_t event);
void menuProcSafetySwitches(uint8_t event);
#ifdef FRSKY
void menuProcTelemetry(uint8_t event);
#endif
#ifdef TEMPLATES
void menuProcTemplates(uint8_t event);
#endif

MenuFuncP_PROGMEM APM menuTabModel[] = {
  menuProcModelSelect,
  menuProcModel,
#ifdef HELI
  menuProcHeli,
#endif
  menuProcExpoAll,
  menuProcMixAll,
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

const prog_char * s_warning = 0;
const prog_char * s_warning_bss;
uint8_t           s_warning_bss_len;
uint8_t           s_confirmation = 0;

void displayWarning(uint8_t event)
{
  if (s_warning) {
    lcd_empty_rect(10, 16, 108, 40);
    lcd_rect(10, 16, 108, 40);
    lcd_puts_P(16, 3*FH, s_warning);
    // could be a place for a s_warning_bss
    lcd_puts_P(16, 5*FH, PSTR("[EXIT]"));

    switch(event) {
      case EVT_KEY_FIRST(KEY_EXIT):
        killEvents(event);
        s_warning = 0;
        break;
    }
  }
}

void displayConfirmation(uint8_t event)
{
  if (s_warning) {
    s_confirmation = false;
    lcd_empty_rect(10, 16, 108, 40);
    lcd_rect(10, 16, 108, 40);
    lcd_puts_P(16, 3*FH, s_warning);
    if (s_warning_bss)
      lcd_putsnAtt(16, 4*FH, s_warning_bss, s_warning_bss_len, BSS);
    lcd_puts_P(16, 5*FH, PSTR("[MENU]    [EXIT]"));

    switch(event) {
      case EVT_KEY_FIRST(KEY_MENU):
        s_confirmation = true;
        // no break
      case EVT_KEY_FIRST(KEY_EXIT):
        killEvents(event);
        s_warning = 0;
        break;
    }
  }
}

void menuProcModel(uint8_t _event)
{
  uint8_t event = (s_warning ? 0 : _event);

  if (s_confirmation) {
    EFile::rm(FILE_MODEL(g_eeGeneral.currModel)); // delete file

    uint8_t i = g_eeGeneral.currModel; // loop to find next available model
    while (!EFile::exists(FILE_MODEL(i))) {
      i--;
      if (i>MAX_MODELS) i=MAX_MODELS-1;
      if (i==g_eeGeneral.currModel) {
        i=0;
        break;
      }
    }
    g_eeGeneral.currModel = i;
    STORE_GENERALVARS;
    eeLoadModel(i); // load default values
    s_confirmation = 0;
    chainMenu(menuProcModelSelect);
  }

  MENU("SETUP", menuTabModel, e_Model, 18, {0,sizeof(g_model.name)-1,1,0,0,0,0,0,0,4,4,4,0,0,6,2,0/*,0*/});

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
        char v = char2idx(g_model.name[subSub]);
        if(p1valdiff || event==EVT_KEY_FIRST(KEY_DOWN) || event==EVT_KEY_FIRST(KEY_UP)
            || event==EVT_KEY_REPT(KEY_DOWN) || event==EVT_KEY_REPT(KEY_UP))
           CHECK_INCDEC_H_MODELVAR( event,v ,0,NUMCHARS-1);
        v = idx2char(v);
        g_model.name[subSub]=v;
        lcd_putcAtt((10+subSub)*FW, y, v,INVERS);
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
      CHECK_INCDEC_H_MODELVAR( event,g_model.tmrMode ,-(13+2*MAX_SWITCH),(13+2*MAX_SWITCH));
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
    putsSwitches(10*FW,y,g_model.trimSw,sub==subN ? INVERS:0);
    if(sub==subN) CHECK_INCDEC_H_MODELVAR(event,g_model.trimSw,-MAX_SWITCH, MAX_SWITCH);
    if((y+=FH)>7*FH) return;
  }subN++;

  for (uint8_t i=0; i<MAX_PHASES-1; i++) {
    if(s_pgOfs<subN) {
      lcd_puts_P(    0,    y, PSTR("F.Phase"));
      lcd_outdezAtt(lcd_lastPos+FW, y, 1+i, 0);
      putsSwitches(10*FW,y,g_model.phaseData[i].swtch,(sub==subN && subSub==0) ? (s_editMode ? BLINK : INVERS):0);
      if(sub==subN && subSub==0 && (s_editMode || p1valdiff)) CHECK_INCDEC_H_MODELVAR(event,g_model.phaseData[i].swtch,-MAX_SWITCH, MAX_SWITCH);
      for (uint8_t t=0; t<NUM_STICKS; t++) {
        int16_t v = g_model.trim[i+1][t];
        if (v < -125) {
          uint8_t c = 128 + v;
          if (c >= i+1) c++;
          lcd_putcAtt((14+t)*FW, y, '0'+c, (sub==subN && subSub==t+1) ? (s_editMode ? BLINK : INVERS) : 0);
        }
        else {
          v = -129;
          putsChnLetter((14+t)*FW, y, t+1, (sub==subN && subSub==t+1) ? (s_editMode ? BLINK : INVERS) : 0);
        }
        if (sub==subN && subSub==t+1 && (s_editMode || p1valdiff)) {
          v = checkIncDec(event, v, -129, -126, 0);
          if (checkIncDec_Ret) {
            if (v == -125) v = 0;
            g_model.trim[i+1][t] = (int8_t)v;
            STORE_MODELVARS;
          }
        }
      }
      if((y+=FH)>7*FH) return;
    }subN++;
  }

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
    lcd_putsAtt(0*FW, y, PSTR("DELETE MODEL   [MENU]"),s_noHi ? 0 : (sub==subN?INVERS:0));
    if(sub==subN && event==EVT_KEY_LONG(KEY_MENU)) {
      s_editMode = false;
      s_noHi = NO_HI_LEN;
      killEvents(event);
      s_warning = PSTR("DELETE MODEL");
      s_warning_bss = g_model.name;
      s_warning_bss_len = sizeof(g_model.name);
    }
  }

  displayConfirmation(_event);
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

static uint8_t s_currIdx;
static uint8_t s_curveChan;
void menuProcCurveOne(uint8_t event); // TODO plus haut
void deleteExpoMix(uint8_t expo, uint8_t idx);
void editExpoVals(uint8_t event, uint8_t which, bool edit, uint8_t y, uint8_t idt)
{
  uint8_t invBlk = edit ? INVERS : 0;
  // if(edit && stopBlink) invBlk = INVERS;

  ExpoData *ed = expoaddress(idt);

  switch(which)
  {
    case 0:
      lcd_outdezAtt(9*FW, y, ed->expo, invBlk);
      if(edit) CHECK_INCDEC_H_MODELVAR(event, ed->expo,-100, 100);
      break;
    case 1:
      lcd_outdezAtt(9*FW, y, ed->weight, invBlk);
      if(edit) CHECK_INCDEC_H_MODELVAR(event, ed->weight, 0, 100);
      break;
    case 2:
      putsFlightPhases(6*FW, y, ed->flightPhase, invBlk);
      if(edit) CHECK_INCDEC_H_MODELVAR(event, ed->flightPhase, -MAX_PHASES+1, MAX_PHASES-1);
      break;
    case 3:
      putsSwitches(6*FW, y, ed->swtch, invBlk);
      if(edit) CHECK_INCDEC_H_MODELVAR(event, ed->swtch, -MAX_DRSWITCH, MAX_DRSWITCH);
      break;
    case 4:
      lcd_putsnAtt(6*FW, y, PSTR(CURV_STR)+3*(ed->curve+(ed->curve >= CURVE_BASE+4 ? 4 : 0)), 3, invBlk);
      if(invBlk) CHECK_INCDEC_H_MODELVAR(event, ed->curve, 0, 15);
      if(invBlk && ed->curve>=CURVE_BASE && event==EVT_KEY_FIRST(KEY_MENU)) {
        s_curveChan = ed->curve - (ed->curve >= CURVE_BASE+4 ? CURVE_BASE-4 : CURVE_BASE);
        pushMenu(menuProcCurveOne);
      }
      break;
    case 5:
      lcd_putsnAtt(6*FW, y, PSTR("---PosNeg")+9-3*ed->mode, 3, invBlk);
      if(edit) ed->mode = 4 - checkIncDec_hm(event, 4-ed->mode, 1, 3);
      break;
    case 6:
      lcd_putsAtt(0*FW, y, PSTR("Delete"), invBlk);
      if(edit && event==EVT_KEY_FIRST(KEY_MENU)) {
        deleteExpoMix(1, idt);
        killEvents(event);
        popMenu();
      }
      break;
  }
}

// TODO in gruvin9x.cpp
extern void applyExpos(int16_t *anas);
void menuProcExpoOne(uint8_t event)
{
  SUBMENU("EXPO/DR", 7, {0});

  ExpoData *ed = expoaddress(s_currIdx);

  putsChnRaw(lcd_lastPos+FW/2,0,ed->chn+1,0);

  int8_t  sub    = mstate2.m_posVert;

  uint8_t  y = FH;

  for (uint8_t i=0; i<7; i++) {
    // TODO optim ?
    lcd_putsnAtt(0, y, PSTR("Expo  WeightPhase Swtch Curve Mode        ")+6*i, 6, 0);
    editExpoVals(event, i, sub==i, y, s_currIdx);
    y+=FH;
  }

#define WCHART 32
#define X0     (128-WCHART-2)
#define Y0     32

  lcd_vlineStip(X0, 0, DISPLAY_H, 0xee);
  lcd_hlineStip(X0-WCHART, Y0, WCHART*2, 0xee);

  int16_t anas[NUM_STICKS] = {0};
  for(int8_t xv=-WCHART; xv<WCHART; xv++) {
    anas[ed->chn] = xv*(RESX/WCHART);
    applyExpos(anas);
    uint16_t yv = (RESX + anas[ed->chn]) / 2;
    yv = (DISPLAY_H-1) - yv * (DISPLAY_H-1) / RESX;
    lcd_plot(X0+xv, yv, LCD_BLACK);
  }

  // TODO optimization
  int16_t x512  = calibratedStick[ed->chn];
  anas[ed->chn] = x512;
  applyExpos(anas);
  int16_t y512  = anas[ed->chn];

  //dy/dx
  // TODO really needed?
  // int16_t dy  = x512>0 ? y512-expo(x512-20,kView) : expo(x512+20,kView)-y512;
  // lcd_outdezNAtt(14*FW, 2*FH,   dy*(100/20), LEADING0|PREC2,3);
  // lcd_outdezNAtt(14*FW, 2*FH,   dy*(100/20), 0,3);

  lcd_outdezAtt(20*FW, 6*FH, x512*100/RESX, 0);
  lcd_outdezAtt(14*FW, 1*FH, (y512*100+(y512>0 ? RESX/2 : -RESX/2))/RESX, 0);

  x512 = X0+x512/(RESXu/WCHART);
  y512 = (DISPLAY_H-1) - (uint16_t)((y512+RESX)/2) * (DISPLAY_H-1) / RESX;

  lcd_vline(x512, y512-3,3*2+1);
  lcd_hline(x512-3, y512,3*2+1);
}

uint8_t getExpoMixCount(uint8_t expo)
{
  uint8_t count = 0;
  uint8_t ch ;

  for(int8_t i=(expo ? MAX_EXPOS-1 : MAX_MIXERS-1); i>=0; i--) {
    ch = (expo ? expoaddress(i)->mode : mixaddress(i)->destCh);
    if (ch != 0) {
      count++;
    }
  }
  return count;
}

bool reachExpoMixCountLimit(uint8_t expo)
{
  // check mixers count limit
  if (getExpoMixCount(expo) >= (expo ? MAX_EXPOS : MAX_MIXERS)) {
    s_warning = (expo ? PSTR("No free expo!") : PSTR("No free mixer!"));
    return true;
  }
  return false;
}

void deleteExpoMix(uint8_t expo, uint8_t idx)
{
  if (expo) {
    memmove(expoaddress(idx), expoaddress(idx+1), (MAX_EXPOS-(idx+1))*sizeof(ExpoData));
    memset(expoaddress(MAX_EXPOS-1), 0, sizeof(ExpoData));
  }
  else {
    memmove(mixaddress(idx), mixaddress(idx+1), (MAX_MIXERS-(idx+1))*sizeof(MixData));
    memset(mixaddress(MAX_MIXERS-1), 0, sizeof(MixData));
  }
  STORE_MODELVARS;
}

static int8_t s_currCh;
void insertExpoMix(uint8_t expo, uint8_t idx)
{
  if (expo) {
    ExpoData *expo = expoaddress(idx);
    memmove(expo+1, expo, (MAX_EXPOS-(idx+1))*sizeof(ExpoData));
    memset(expo,0,sizeof(ExpoData));
    expo->mode = 3; // pos&neg
    expo->chn = s_currCh - 1;
    expo->weight = 100;
  }
  else {
    MixData *mix = mixaddress(idx);
    memmove(mix+1, mix, (MAX_MIXERS-(idx+1))*sizeof(MixData));
    memset(mix,0,sizeof(MixData));
    mix->destCh = s_currCh;
    mix->srcRaw = s_currCh;
    mix->weight = 100;
  }
  STORE_MODELVARS;
}

void copyExpoMix(uint8_t expo, uint8_t idx)
{
  if (expo) {
    ExpoData *expo = expoaddress(idx);
    memmove(expo+1, expo, (MAX_EXPOS-(idx+1))*sizeof(ExpoData));
  }
  else {
    MixData *mix = mixaddress(idx);
    memmove(mix+1, mix, (MAX_MIXERS-(idx+1))*sizeof(MixData));
  }
  STORE_MODELVARS;
}

void memswap(void *a, void *b, uint8_t size)
{
  uint8_t *x = (uint8_t*)a;
  uint8_t *y = (uint8_t*)b;
  uint8_t temp ;

  while (size--) {
    temp = *x;
    *x++ = *y;
    *y++ = temp;
  }
}

bool swapExpo(uint8_t &idx, uint8_t up)
{
  ExpoData *x = expoaddress(idx);
  int8_t tgt_idx = (up ? idx-1 : idx+1);

  if (tgt_idx < 0) {
    if (x->chn == 0)
      return false;
    x->chn--;
    return true;
  }

  if (tgt_idx == MAX_EXPOS) {
    if (x->chn == NUM_STICKS-1)
      return false;
    x->chn++;
    return true;
  }

  ExpoData *y = expoaddress(tgt_idx);
  if(x->chn != y->chn || !y->mode) {
    if (up) {
      if (x->chn>0) x->chn--;
      else return false;
    }
    else {
      if (x->chn<NUM_STICKS-1) x->chn++;
      else return false;
    }
    return true;
  }

  memswap(x, y, sizeof(ExpoData));
  idx = tgt_idx;
  return true;
}

bool swapMix(uint8_t &idx, uint8_t up)
{
  MixData *x = mixaddress(idx);
  int8_t tgt_idx = (up ? idx-1 : idx+1);

  if (tgt_idx < 0) {
    if (x->destCh == 1)
      return false;
    x->destCh--;
    return true;
  }

  if (tgt_idx == MAX_MIXERS) {
    if (x->destCh == NUM_CHNOUT)
      return false;
    x->destCh++;
    return true;
  }

  MixData *y = mixaddress(tgt_idx);
  if(x->destCh != y->destCh) {
    if (up) {
      if (x->destCh>1) x->destCh--;
      else return false;
    }
    else {
      if (x->destCh<NUM_CHNOUT) x->destCh++;
      else return false;
    }
    return true;
  }

  memswap(x, y, sizeof(MixData));
  idx = tgt_idx;
  return true;
}

bool swapExpoMix(uint8_t expo, uint8_t &idx, uint8_t up)
{
  bool result = (expo ? swapExpo(idx, up) : swapMix(idx, up));
  if (result)
    STORE_MODELVARS;
  return result;
}

#define XD X0-2
// static uint8_t s_curveChan;
void menuProcCurveOne(uint8_t event)
{
  bool    cv9 = s_curveChan >= MAX_CURVE5;

  SUBMENU("CURVE", 2+(cv9 ? 9 : 5), { 9,0/*repeated...*/});
  lcd_outdezAtt(6*FW, 0, s_curveChan+1, INVERS);

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

void menuProcMixOne(uint8_t event)
{
  SIMPLE_SUBMENU_NOTITLE(14);
  TITLEP(s_currCh ? PSTR("INSERT MIX ") : PSTR("EDIT MIX "));
  MixData *md2 = mixaddress(s_currIdx) ;
  putsChn(lcd_lastPos+1*FW,0,md2->destCh,0);
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
        putsSwitches(10*FW,  y,md2->swtch,attr);
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->swtch, -MAX_SWITCH, MAX_SWITCH);
        break;
      case 6:
        lcd_puts_P(  2*FW,y,PSTR("F.Phase"));
        putsFlightPhases(10*FW,  y,md2->flightPhase, attr);
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->flightPhase, -MAX_PHASES+1, MAX_PHASES-1);
        break;
      case 7:
        lcd_puts_P(  2*FW,y,PSTR("Warning"));
        if(md2->mixWarn)
          lcd_outdezAtt(FW*10,y,md2->mixWarn,attr|LEFT);
        else
          lcd_putsAtt(  FW*10,y,PSTR("OFF"),attr);
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->mixWarn, 0,3);
        break;
      case 8:
        lcd_puts_P(  2*FW,y,PSTR("Multpx"));
        lcd_putsnAtt(10*FW, y,PSTR("Add     MultiplyReplace ")+8*md2->mltpx,8,attr);
        if(attr) CHECK_INCDEC_H_MODELVAR( event, md2->mltpx, 0, 2);
        break;
      case 9:
        lcd_puts_P(  2*FW,y,PSTR("Delay Down"));
        lcd_outdezAtt(FW*16,y,md2->delayDown,attr);
        if(attr)  CHECK_INCDEC_H_MODELVAR( event, md2->delayDown, 0,15);
        break;
      case 10:
        lcd_puts_P(  2*FW,y,PSTR("Delay Up"));
        lcd_outdezAtt(FW*16,y,md2->delayUp,attr);
        if(attr)  CHECK_INCDEC_H_MODELVAR( event, md2->delayUp, 0,15);
        break;
      case 11:
        lcd_puts_P(  2*FW,y,PSTR("Slow  Down"));
        lcd_outdezAtt(FW*16,y,md2->speedDown,attr);
        if(attr)  CHECK_INCDEC_H_MODELVAR( event, md2->speedDown, 0,15);
        break;
      case 12:
        lcd_puts_P(  2*FW,y,PSTR("Slow  Up"));
        lcd_outdezAtt(FW*16,y,md2->speedUp,attr);
        if(attr)  CHECK_INCDEC_H_MODELVAR( event, md2->speedUp, 0,15);
        break;
      case 13:   lcd_putsAtt(  2*FW,y,PSTR("DELETE MIX [MENU]"),attr);
        if(attr && event==EVT_KEY_LONG(KEY_MENU)){
          killEvents(event);
          deleteExpoMix(0, s_currIdx);
          beepWarn1();
          popMenu();
        }
        break;
    }
  }
}

static uint8_t s_maxLines = 8;
static int8_t s_copyTgtOfs;
static uint8_t s_copySrcIdx;
static uint8_t s_copySrcCh;
static uint8_t s_copySrcRow;
#define COPY_MODE 1
#define MOVE_MODE 2
static uint8_t s_copyMode = 0;

#define FIRST 0x10
void displayMixerLine(uint8_t row, uint8_t mix, uint8_t ch, uint8_t idx, uint8_t cur, uint8_t event)
{
  uint8_t y = (row-s_pgOfs)*FH;
  MixData *md = mixaddress(mix);
  if (idx > 0)
    lcd_putsnAtt(FW, y, PSTR("+=*=:=")+md->mltpx*2, 2, 0);

  putsChnRaw(4*FW, y, md->srcRaw, 0);

  uint8_t attr = ((s_copyMode || cur != row) ? 0 : INVERS);
  lcd_outdezAtt(11*FW, y, md->weight, attr);
  if (attr != 0)
    CHECK_INCDEC_H_MODELVAR(event, md->weight, -125, 125);

  if (md->curve) lcd_putsnAtt(12*FW, y, PSTR(CURV_STR)+md->curve*3, 3, 0);
  if (md->swtch) putsSwitches(16*FW+FW/2, y, md->swtch, 0);

  char cs = ' ';
  if (md->speedDown || md->speedUp)
    cs = 'S';
  if ((md->delayUp || md->delayDown))
    cs = (cs =='S' ? '*' : 'D');
  lcd_putcAtt(20*FW+1, y, cs, 0);

  if (s_copyMode) {
    if ((s_copyMode==COPY_MODE || s_copyTgtOfs == 0) && s_copySrcCh == ch && mix == (s_copySrcIdx + (s_copyTgtOfs<0))) {
      /* draw a border around the raw on selection mode (copy/move) */
      lcd_rect(22, y-1, DISPLAY_W-1-21, 9, s_copyMode == COPY_MODE ? 0xff : 0x55);
    }

    if (row == cur) {
      /* invert the raw when it's the current one */
      lcd_filled_rect(23, y, DISPLAY_W-1-23, 7);
    }
  }
}

void displayExpoLine(uint8_t row, uint8_t expo, uint8_t ch, uint8_t idx, uint8_t cur, uint8_t event)
{
  uint8_t y = (row-s_pgOfs)*FH;
  ExpoData *ed = expoaddress(expo);

  lcd_outdezAtt(7*FW, y, ed->expo, 0);

  uint8_t attr = ((s_copyMode || cur != row) ? 0 : INVERS);
  lcd_outdezAtt(10*FW, y, ed->weight, attr);
  if (attr != 0)
    CHECK_INCDEC_H_MODELVAR(event, ed->weight, 0, 100);

  putsSwitches(11*FW+FW/2, y, ed->swtch, 0);
  if (ed->curve) lcd_putsnAtt(16*FW, y, PSTR(CURV_STR)+ed->curve*3, 3, 0);
  if (ed->mode!=3) lcd_putc(20*FW, y, ed->mode == 2 ? 127 : 126);//'|' : (stkVal[i] ? '<' : '>'),0);*/

  if (s_copyMode) {
    if ((s_copyMode==COPY_MODE || s_copyTgtOfs == 0) && s_copySrcCh == ch && expo == (s_copySrcIdx + (s_copyTgtOfs<0))) {
      /* draw a border around the raw on selection mode (copy/move) */
      lcd_rect(22, y-1, DISPLAY_W-1-21, 9, s_copyMode == COPY_MODE ? 0xff : 0x55);
    }

    if (row == cur) {
      /* invert the raw when it's the current one */
      lcd_filled_rect(23, y, DISPLAY_W-1-23, 7);
    }
  }
}

void menuProcExpoMix(uint8_t expo, uint8_t __event)
{
  uint8_t _event = (s_warning ? 0 : __event);
  uint8_t event = _event;
  uint8_t key = (event & 0x1f);

  if (s_copyMode) {
    if (key == KEY_EXIT)
      event -= KEY_EXIT;
  }

  SIMPLE_MENU_NOTITLE(menuTabModel, expo ? e_ExpoAll : e_MixAll, s_maxLines);
  TITLEP(expo ? PSTR("EXPO/DR") : PSTR("MIXER"));

  uint8_t sub = mstate2.m_posVert;

  switch(_event)
  {
    case EVT_ENTRY:
    case EVT_ENTRY_UP:
      s_copyMode = 0;
      s_copyTgtOfs = 0;
      break;
    case EVT_KEY_LONG(KEY_EXIT):
      if (s_copyMode && s_copyTgtOfs == 0) {
        deleteExpoMix(expo, s_currIdx);
      }
      killEvents(_event);
      // no break
    case EVT_KEY_BREAK(KEY_EXIT):
      if (s_copyTgtOfs) {
        // cancel the current copy / move operation
        if (s_copyMode == COPY_MODE) {
          deleteExpoMix(expo, s_currIdx);
        }
        else {
          do {
            swapExpoMix(expo, s_currIdx, s_copyTgtOfs > 0);
            if (s_copyTgtOfs < 0)
              s_copyTgtOfs++;
            else
              s_copyTgtOfs--;
          } while (s_copyTgtOfs != 0);
        }
        sub = mstate2.m_posVert = s_copySrcRow;
      }
      s_copyMode = 0;
      s_copyTgtOfs = 0;
      break;
    case EVT_KEY_BREAK(KEY_MENU):
      if (!s_currCh || (s_copyMode && !s_copyTgtOfs)) {
        s_copyMode = (s_copyMode == COPY_MODE ? MOVE_MODE : COPY_MODE);
        s_copySrcIdx = s_currIdx;
        s_copySrcCh = expo ? expoaddress(s_currIdx)->chn+1 : mixaddress(s_currIdx)->destCh;
        s_copySrcRow = sub;
        break;
      }
      // no break
    case EVT_KEY_LONG(KEY_MENU):
      if (s_copyTgtOfs) {
        s_copyMode = 0;
        s_copyTgtOfs = 0;
      }
      else {
        if (s_copyMode) s_currCh = 0;
        if (s_currCh) {
          if (reachExpoMixCountLimit(expo)) break;
          insertExpoMix(expo, s_currIdx);
        }
        pushMenu(expo ? menuProcExpoOne : menuProcMixOne);
        s_copyMode = 0;
        return;
      }
      killEvents(_event);
      break;
    case EVT_KEY_LONG(KEY_LEFT):
    case EVT_KEY_LONG(KEY_RIGHT):
      if (s_copyMode && !s_copyTgtOfs) {
        if (reachExpoMixCountLimit(expo)) break;
        s_currCh = (expo ? expoaddress(s_currIdx)->chn+1 : mixaddress(s_currIdx)->destCh);
        if (_event == EVT_KEY_LONG(KEY_RIGHT)) s_currIdx++;
        insertExpoMix(expo, s_currIdx);
        pushMenu(expo ? menuProcExpoOne : menuProcMixOne);
        s_copyMode = 0;
        return;
      }
      break;
    case EVT_KEY_REPT(KEY_UP):
    case EVT_KEY_FIRST(KEY_UP):
    case EVT_KEY_REPT(KEY_DOWN):
    case EVT_KEY_FIRST(KEY_DOWN):
      if (s_copyMode) {
        uint8_t next_ofs = (key == KEY_UP ? s_copyTgtOfs - 1 : s_copyTgtOfs + 1);

        if (s_copyTgtOfs==0 && s_copyMode==COPY_MODE) {
          // insert a mix on the same channel (just above / just below)
          if (reachExpoMixCountLimit(expo)) break;
          copyExpoMix(expo, s_currIdx);
          if (key==KEY_DOWN) s_currIdx++;
          else if (sub-s_pgOfs >= 6) s_pgOfs++;
        }
        else if (next_ofs==0 && s_copyMode==COPY_MODE) {
          // delete the mix
          deleteExpoMix(expo, s_currIdx);
          if (key==KEY_UP) s_currIdx--;
        }
        else {
          // only swap the mix with its neighbor
          if (!swapExpoMix(expo, s_currIdx, key==KEY_UP)) break;
        }

        s_copyTgtOfs = next_ofs;
      }
      break;
  }

  lcd_outdezAtt(lcd_lastPos+2*FW+FW/2, 0, getExpoMixCount(expo), 0);
  lcd_puts_P(lcd_lastPos, 0, expo ? PSTR("/14") : PSTR("/32"));

  s_currCh = 0;
  uint8_t cur = 1;
  uint8_t i = 0;

  for (uint8_t ch=1; ch<=(expo ? NUM_STICKS : NUM_CHNOUT); ch++) {
    MixData *md=NULL; ExpoData *ed=NULL;
    if (expo ? (i<MAX_EXPOS && (ed=expoaddress(i))->chn+1 == ch && ed->mode) : (i<MAX_MIXERS && (md=mixaddress(i))->destCh == ch)) {
      if (s_pgOfs < cur && cur-s_pgOfs < 8) {
        if (expo)
          putsChnRaw(0, (cur-s_pgOfs)*FH, ch, 0);
        else
          putsChn(0, (cur-s_pgOfs)*FH, ch, 0); // show CHx
      }
      uint8_t mixCnt = 0;
      do {
        if (s_copyMode) {
          if (s_copyMode == MOVE_MODE && s_pgOfs < cur && cur-s_pgOfs < 8 && s_copySrcCh == ch && s_copyTgtOfs != 0 && i == (s_copySrcIdx + (s_copyTgtOfs<0))) {
            uint8_t y = (cur-s_pgOfs)*FH;
            lcd_rect(22, y-1, DISPLAY_W-1-21, 9, 0x55);
            cur++;
          }
          if (s_currIdx == i) {
            sub = mstate2.m_posVert = cur;
            s_currCh = ch;
          }
        }
        else if (sub == cur) {
          s_currIdx = i;
        }
        if (s_pgOfs < cur && cur-s_pgOfs < 8) {
          if (expo)
            displayExpoLine(cur, i, ch, mixCnt, sub, _event);
          else
            displayMixerLine(cur, i, ch, mixCnt, sub, _event);
        }
        cur++; mixCnt++; i++; md++; ed++;
      } while (expo ? (i<MAX_EXPOS && ed->chn+1 == ch && ed->mode) : (i<MAX_MIXERS && md->destCh == ch));
      if (s_copyMode == MOVE_MODE && s_pgOfs < cur && cur-s_pgOfs < 8 && s_copySrcCh == ch && i == (s_copySrcIdx + (s_copyTgtOfs<0))) {
        uint8_t y = (cur-s_pgOfs)*FH;
        lcd_rect(22, y-1, DISPLAY_W-1-21, 9, 0x55);
        cur++;
      }
    }
    else {
      uint8_t attr = 0;
      if (sub == cur) {
        s_currIdx = i;
        s_currCh = ch;
        if (!s_copyMode) {
          attr = INVERS;
        }
      }
      if (s_pgOfs < cur && cur-s_pgOfs < 8) {
        if (expo)
          putsChnRaw(0, (cur-s_pgOfs)*FH, ch, attr);
        else
          putsChn(0, (cur-s_pgOfs)*FH, ch, attr); // show CHx
        if (s_copyMode == MOVE_MODE && s_copySrcCh == ch) {
          uint8_t y = (cur-s_pgOfs)*FH;
          lcd_rect(22, y-1, DISPLAY_W-1-21, 9, 0x55);
        }
      }
      cur++;
    }
  }
  s_maxLines = cur;
  if (sub >= s_maxLines-1) mstate2.m_posVert = s_maxLines-1;
  displayWarning(__event);
}

void menuProcExpoAll(uint8_t event)
{
  return menuProcExpoMix(1, event);
}

void menuProcMixAll(uint8_t event)
{
  return menuProcExpoMix(0, event);
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
          LimitData *ld = limitaddress(sub);
          switch (subSub) {
          case 0:
              ld->offset = (ld->revert) ? -v : v;
              STORE_MODELVARS;
              break;
          }
      }
      break;
  }

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
            ld->min -= 100;
            if(g_model.extendedLimits)
              CHECK_INCDEC_H_MODELVAR( event, ld->min, -125,125);
            else
              CHECK_INCDEC_H_MODELVAR( event, ld->min, -100,100);
            ld->min += 100;
          }
          break;
        case 2:
          lcd_outdezAtt( 17*FW, y, (int8_t)(ld->max+100),    attr);
          if(attr && (s_editMode || p1valdiff)) {
            ld->max += 100;
            if(g_model.extendedLimits)
              CHECK_INCDEC_H_MODELVAR( event, ld->max, -125,125);
            else
              CHECK_INCDEC_H_MODELVAR( event, ld->max, -100,100);
            ld->max -= 100;
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

void menuProcCustomSwitches(uint8_t event)
{
  MENU("CUSTOM SWITCHES", menuTabModel, e_CustomSwitches, NUM_CSW+1, {0, 2/*repeated...*/});

  uint8_t y = 0;
  uint8_t k = 0;
  int8_t  sub    = mstate2.m_posVert - 1;
  uint8_t subSub = mstate2.m_posHorz;

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
        if (cs.v1 > NUM_XCHNRAW-NUM_TELEMETRY)
          lcd_outdezAtt( 20*FW, y, 125+cs.v2  ,subSub==2 ? attr : 0);
        else
#endif
        lcd_outdezAtt( 20*FW, y, cs.v2  ,subSub==2 ? attr : 0);
    }
    else if(cstate == CS_VBOOL)
    {
        putsSwitches(13*FW, y, cs.v1  ,subSub==1 ? attr : 0);
        putsSwitches(17*FW, y, cs.v2  ,subSub==2 ? attr : 0);
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
              CHECK_INCDEC_H_MODELVAR( event, cs.v1, -MAX_SWITCH,MAX_SWITCH);
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
              CHECK_INCDEC_H_MODELVAR( event, cs.v2, -MAX_SWITCH,MAX_SWITCH);
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

void menuProcSafetySwitches(uint8_t event)
{
  MENU("SAFETY SWITCHES", menuTabModel, e_SafetySwitches, NUM_CHNOUT+1, {0, 2/*repeated*/});

  uint8_t y = 0;
  uint8_t k = 0;
  int8_t  sub    = mstate2.m_posVert - 1;
  uint8_t subSub = mstate2.m_posHorz;

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
          putsSwitches(6*FW, y, sd->swtch  , attr);
          if(attr && (s_editMode || p1valdiff)) {
              CHECK_INCDEC_H_MODELVAR( event, sd->swtch, -MAX_SWITCH, MAX_SWITCH);
          }
          break;
      case 1:
          lcd_outdezAtt(16*FW, y, sd->val,   attr);
          if(attr && (s_editMode || p1valdiff)) {
              CHECK_INCDEC_H_MODELVAR( event, sd->val, -125, 125);
          }
          break;
      }
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
  SIMPLE_MENU("TEMPLATES", menuTabModel, e_Templates, NUM_TEMPLATES+2);

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
  lcd_putsAtt(  1*FW,y,PSTR("CLEAR MIXES [MENU]"),attr);
  y+=FH;
}
#endif

