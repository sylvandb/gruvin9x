#include "menus.h"

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

void menuProcModelSelect(uint8_t event);
void menuProcModel(uint8_t event);
#ifdef HELI
void menuProcHeli(uint8_t event);
#endif
void menuProcExpoAll(uint8_t event);
void menuProcMix(uint8_t event);
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

static int8_t s_currMixIdx;
static int8_t s_currDestCh;
static bool   s_currMixInsMode;

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

static uint8_t s_curveChan;

#define XD X0-2

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
  SIMPLE_SUBMENU_NOTITLE(13);
  // TODO BSS TITLE/TITLEP update lcd_lastPosition instead or returning something
  uint8_t x = TITLEP(s_currMixInsMode ? PSTR("INSERT MIX ") : PSTR("EDIT MIX "));
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

  uint8_t attr = s_noHi ? 0 : ((sub==NUM_TEMPLATES+1) ? INVERS : 0);
  lcd_putsAtt(  1*FW,y,PSTR("CLEAR MIXES [MENU]"),attr);
  y+=FH;
}
#endif

