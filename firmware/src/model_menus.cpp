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
#include "templates.h"

#define WCHART 32
#define X0     (128-WCHART-2)
#define Y0     32

enum EnumTabModel {
  e_ModelSelect,
  e_Model,
#ifdef HELI
  e_Heli,
#endif
  e_PhasesAll,
  e_ExposAll,
  e_MixAll,
  e_Limits,
  e_CurvesAll,
  e_CustomSwitches,
  e_FunctionSwitches,
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
void menuProcPhasesAll(uint8_t event);
void menuProcExposAll(uint8_t event);
void menuProcMixAll(uint8_t event);
void menuProcLimits(uint8_t event);
void menuProcCurvesAll(uint8_t event);
void menuProcCustomSwitches(uint8_t event);
void menuProcFunctionSwitches(uint8_t event);
void menuProcSafetySwitches(uint8_t event);
#ifdef FRSKY
void menuProcTelemetry(uint8_t event);
#endif
#ifdef TEMPLATES
void menuProcTemplates(uint8_t event);
#endif
void menuProcExpoOne(uint8_t event);

const MenuFuncP_PROGMEM menuTabModel[] PROGMEM = {
  menuProcModelSelect,
  menuProcModel,
#ifdef HELI
  menuProcHeli,
#endif
  menuProcPhasesAll,
  menuProcExposAll,
  menuProcMixAll,
  menuProcLimits,
  menuProcCurvesAll,
  menuProcCustomSwitches,
  menuProcFunctionSwitches,
  menuProcSafetySwitches,
#ifdef FRSKY
  menuProcTelemetry,
#endif
#ifdef TEMPLATES
  menuProcTemplates
#endif
};

const pm_char * s_warning = 0;
const pm_char * s_warning_info;
uint8_t           s_warning_info_len;
// uint8_t s_warning_info_att not needed now
uint8_t           s_confirmation = 0;

void displayBox()
{
  lcd_filled_rect(10, 16, 108, 40, WHITE);
  lcd_rect(10, 16, 108, 40);
  lcd_puts_P(16, 3*FH, s_warning);
  // could be a place for a s_warning_info
}

void displayPopup(const pm_char * pstr)
{
  s_warning = pstr;
  displayBox();
  s_warning = 0;
  refreshDisplay();
}

void displayWarning(uint8_t event)
{
  if (s_warning) {
    displayBox();
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
  s_confirmation = false;
  displayBox();
  if (s_warning_info)
    lcd_putsnAtt(16, 4*FH, s_warning_info, s_warning_info_len, ZCHAR);
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

#define COPY_MODE 1
#define MOVE_MODE 2
static uint8_t s_copyMode = 0;
static int8_t s_copySrcRow;
static int8_t s_copyTgtOfs;

void menuProcModelSelect(uint8_t event)
{
  char name[sizeof(g_model.name)];

  TITLE("MODELSEL");

  // flush eeprom write
  eeFlush();

  if (s_confirmation) {
    EFile::rm(FILE_MODEL(m_posVert)); // delete file
    s_confirmation = 0;
    s_copyMode = 0;
  }

  uint8_t _event = (s_warning ? 0 : event);
  uint8_t _event_ = (IS_RE1_EVT(_event) ? 0 : _event);

  if (s_copyMode || !EFile::exists(FILE_MODEL(g_eeGeneral.currModel))) {
    if ((_event & 0x1f) == KEY_EXIT)
      _event_ -= KEY_EXIT;
  }

  int8_t oldSub = m_posVert;
  if (!check_submenu_simple(_event_, MAX_MODELS-1)) return;
  int8_t sub = m_posVert;

  lcd_puts_P(     9*FW, 0, PSTR("free"));
  lcd_outdezAtt(  17*FW, 0, EeFsGetFree(),0);

  DisplayScreenIndex(e_ModelSelect, DIM(menuTabModel), (sub == g_eeGeneral.currModel) ? INVERS : 0);

#ifdef NAVIGATION_RE1
  if (scrollRE > 0 && s_editMode < 0) {
    chainMenu(menuProcModel);
    return;
  }
#endif

  switch(_event)
  {
      case EVT_ENTRY:
        m_posVert = sub = g_eeGeneral.currModel;
        s_copyMode = 0; // TODO only this one?
        s_copyTgtOfs = 0;
        s_copySrcRow = -1;
        s_editMode = -1;
        break;
      case EVT_KEY_LONG(KEY_EXIT):
        if (s_copyMode && s_copyTgtOfs == 0 && g_eeGeneral.currModel != sub && EFile::exists(FILE_MODEL(sub))) {
          s_warning = PSTR("DELETE MODEL");
          killEvents(_event);
          break;
        }
        // no break
      case EVT_KEY_BREAK(KEY_EXIT):
        if (s_copyMode) {
          sub = m_posVert = (s_copyMode == MOVE_MODE || s_copySrcRow<0) ? (16+sub+s_copyTgtOfs) % 16 : s_copySrcRow; // TODO reset s_copySrcRow?
          s_copyMode = 0; // TODO only this one?
          s_copySrcRow = -1;
          s_copyTgtOfs = 0;
          killEvents(_event);
        }
        break;
#ifdef NAVIGATION_RE1
      case EVT_KEY_BREAK(BTN_RE1):
        s_editMode = (s_editMode == 0 && sub == g_eeGeneral.currModel) ? -1 : 0;
        break;
      case EVT_KEY_LONG(BTN_RE1):
#endif
      case EVT_KEY_LONG(KEY_MENU):
      case EVT_KEY_BREAK(KEY_MENU):
        if (s_copyMode && (s_copyTgtOfs || s_copySrcRow>=0)) {
          displayPopup(s_copyMode==COPY_MODE ? PSTR("Copying model...") : PSTR("Moving model..."));
          eeCheck(true); // force writing of current model data before this is changed

          uint8_t cur = (16 + sub + s_copyTgtOfs) % 16;

          if (s_copyMode == COPY_MODE) {
            if (!theFile.copy(FILE_MODEL(cur), FILE_MODEL(s_copySrcRow)))
              cur = sub;
          }

          s_copySrcRow = g_eeGeneral.currModel; // to update the currModel value
          while (sub != cur) {
            uint8_t src = cur;
            cur = (s_copyTgtOfs > 0 ? cur+15 : cur+1) % 16;
            EFile::swap(FILE_MODEL(src), FILE_MODEL(cur));
            if (src == s_copySrcRow)
              s_copySrcRow = cur;
            else if (cur == s_copySrcRow)
              s_copySrcRow = src;
          }

          if (s_copySrcRow != g_eeGeneral.currModel) {
            g_eeGeneral.currModel = s_copySrcRow;
            STORE_GENERALVARS;
          }

          s_copyMode = 0; // TODO only this one?
          s_copySrcRow = -1;
          s_copyTgtOfs = 0;
          return;
        }
        else if (_event == EVT_KEY_LONG(KEY_MENU) || IS_RE1_EVT_TYPE(_event, EVT_KEY_LONG)) {
#ifdef NAVIGATION_RE1
          s_editMode = -1;
#endif
          displayPopup(PSTR("Loading model..."));
          eeCheck(true); // force writing of current model data before this is changed
          if (g_eeGeneral.currModel != sub) {
            g_eeGeneral.currModel = sub;
            STORE_GENERALVARS;
            eeLoadModel(sub);
          }
          s_copyMode = 0;
          killEvents(event);
          return;
        }
        else if (EFile::exists(FILE_MODEL(sub))) {
          s_copyMode = (s_copyMode == COPY_MODE ? MOVE_MODE : COPY_MODE);
        }
        break;
      case EVT_KEY_FIRST(KEY_LEFT):
      case EVT_KEY_FIRST(KEY_RIGHT):
        if (sub == g_eeGeneral.currModel) {
          chainMenu(_event == EVT_KEY_FIRST(KEY_RIGHT) ? menuProcModel : menuTabModel[DIM(menuTabModel)-1]);
          return;
        }
        beepWarn();
        break;
      case EVT_KEY_FIRST(KEY_UP):
      case EVT_KEY_FIRST(KEY_DOWN):
        if (s_copyMode) {
          int8_t next_ofs = (_event == EVT_KEY_FIRST(KEY_UP) ? s_copyTgtOfs+1 : s_copyTgtOfs-1);
          if (next_ofs == 16 || next_ofs == -16)
            next_ofs = 0;

          if (s_copySrcRow < 0 && s_copyMode==COPY_MODE) {
            s_copySrcRow = oldSub;
            // find a hole (in the first empty slot above / below)
            m_posVert = eeFindEmptyModel(s_copySrcRow, _event==EVT_KEY_FIRST(KEY_DOWN));
            if (m_posVert == (uint8_t)-1) {
              // no free room for duplicating the model
              beepWarn();
              m_posVert = oldSub;
              s_copyMode = 0; // TODO only this one?
              s_copyTgtOfs = 0;
              s_copySrcRow = -1;
            }
            next_ofs = 0;
            sub = m_posVert;
          }
          s_copyTgtOfs = next_ofs;
        }
        break;
  }

  if (sub-s_pgOfs < 1) s_pgOfs = max(0, sub-1);
  else if (sub-s_pgOfs > 5)  s_pgOfs = min(MAX_MODELS-7, sub-4);

  // printf("copy_mode=%d s_copySrcRow=%d s_copyTgtOfs=%d sub=%d\n", s_copyMode, s_copySrcRow, s_copyTgtOfs, sub); fflush(stdout);

  for (uint8_t i=0; i<7; i++) {
    uint8_t y=(i+1)*FH;
    uint8_t k=i+s_pgOfs;
    lcd_outdezNAtt(3*FW+2, y, k+1, LEADING0+((!s_copyMode && sub==k) ? INVERS : 0), 2);

    if (s_copyMode == MOVE_MODE || (s_copyMode == COPY_MODE && s_copySrcRow >= 0)) {
      if (k == sub) {
        if (s_copyMode == COPY_MODE) {
          k = s_copySrcRow;
          lcd_putc(20*FW+2, y, '+');
        }
        else {
          k = sub + s_copyTgtOfs;
        }
      }
      else if (s_copyTgtOfs < 0 && ((k < sub && k >= sub+s_copyTgtOfs) || (k-16 < sub && k-16 >= sub+s_copyTgtOfs)))
        k += 1;
      else if (s_copyTgtOfs > 0 && ((k > sub && k <= sub+s_copyTgtOfs) || (k+16 > sub && k+16 <= sub+s_copyTgtOfs)))
        k += 15;
    }

    k %= 16;

    if (EFile::exists(FILE_MODEL(k))) {
      uint16_t size = eeLoadModelName(k, name);
      putsModelName(4*FW, y, name, k, 0);
      lcd_outdezAtt(20*FW, y, size, 0);
      if (k==g_eeGeneral.currModel && (s_copySrcRow<0 || i+s_pgOfs!=sub)) lcd_putc(1, y, '*');
    }

    if (s_copyMode && sub==i+s_pgOfs) {
      lcd_filled_rect(9, y, DISPLAY_W-1-9, 7);
      lcd_rect(8, y-1, DISPLAY_W-1-7, 9, s_copyMode == COPY_MODE ? 0xff : 0x55);
    }
  }

  if (s_warning) {
    eeLoadModelName(sub, name);
    s_warning_info = name;
    s_warning_info_len = sizeof(g_model.name);
    displayConfirmation(event);
  }
}

void EditName(uint8_t x, uint8_t y, char *name, uint8_t size, uint8_t event, bool active, uint8_t & cur)
{
  lcd_putsnAtt(x, y, name, size, ZCHAR | (active ? ((s_editMode>0) ? 0 : INVERS) : 0));

  if (active) {
    if (s_editMode>0) {
      uint8_t next = cur;
      char c = name[next];
      char v = c;
      if (p1valdiff || event==EVT_KEY_FIRST(KEY_DOWN) || event==EVT_KEY_FIRST(KEY_UP)
          || event==EVT_KEY_REPT(KEY_DOWN) || event==EVT_KEY_REPT(KEY_UP)) {
         v = checkIncDec(event, abs(v), 0, ZCHAR_MAX, 0);
         if (c < 0) v = -v;
         STORE_MODELVARS;
      }

      switch(event) {
        case EVT_KEY_BREAK(KEY_LEFT):
          if (next>0) next--;
          break;
        case EVT_KEY_BREAK(KEY_RIGHT):
          if (next<size-1) next++;
          break;
#ifdef NAVIGATION_RE1
        case EVT_KEY_LONG(BTN_RE1):
          if (v==0) {
            s_editMode = 0;
            killEvents(BTN_RE1);
            break;
          }
#endif
        case EVT_KEY_LONG(KEY_LEFT):
        case EVT_KEY_LONG(KEY_RIGHT):
        if (v>=-26 && v<=26) {
          v = -v; // toggle case
          STORE_MODELVARS; // TODO optim if (c!=v) at the end
          if (event==EVT_KEY_LONG(KEY_LEFT))
            killEvents(KEY_LEFT);
        }
      }

      name[cur] = v;
      lcd_putcAtt(x+cur*FW, y, idx2char(v), INVERS);
      cur = next;
    }
    else {
      cur = 0;
    }
  }
}

#undef PARAM_OFS
#define PARAM_OFS (9*FW+2)
void menuProcModel(uint8_t event)
{
  lcd_outdezNAtt(7*FW,0,g_eeGeneral.currModel+1,INVERS+LEADING0,2);
  MENU("SETUP", menuTabModel, e_Model, (g_model.protocol ? 9 : 10), {0,ZCHAR|(sizeof(g_model.name)-1),3,0,0,0,0,6,2,1});

  uint8_t  sub    = m_posVert;
  uint8_t y = 1*FH;

  uint8_t subN = 1;
  if(s_pgOfs<subN) {
    lcd_puts_P(0*FW, y, PSTR("Name"));
    EditName(PARAM_OFS, y, g_model.name, sizeof(g_model.name), event, sub==subN, m_posHorz);
    if((y+=FH)>7*FH) return;
  }subN++;

  TimerData *timer = &g_model.timer1;
  for (uint8_t i=0; i<1/*TODO*/; i++, timer=&g_model.timer2) {
    if (s_pgOfs<subN) {
      lcd_putsnAtt(0*FW, y, PSTR("Timer Timer2")+6*i, 6, 0);
      putsTmrMode(PARAM_OFS, y, timer->mode, sub==subN && m_posHorz==0 ? ((s_editMode>0) ? BLINK : INVERS) : 0);
      putsTime(14*FW-3, y, timer->val,
          (sub==subN && m_posHorz==1 ? ((s_editMode>0) ? BLINK : INVERS):0),
          (sub==subN && m_posHorz==2 ? ((s_editMode>0) ? BLINK : INVERS):0) );
      lcd_putsnAtt(19*FW, y, PSTR("\x7e\x7f")+1-timer->dir,1,sub==subN && m_posHorz==3 ? ((s_editMode>0) ? BLINK : INVERS) : 0);
      if (sub==subN && (s_editMode>0 || p1valdiff)) {
        switch (m_posHorz) {
         case 0:
           CHECK_INCDEC_MODELVAR(event, timer->mode, -(13+2*MAX_SWITCH),(13+2*MAX_SWITCH));
           break;
         case 1:
         {
           int8_t min = timer->val/60;
           CHECK_INCDEC_MODELVAR(event, min, 0, 59);
           timer->val = timer->val%60 + min*60;
           break;
         }
         case 2:
         {
           int8_t sec = timer->val%60;
           sec -= checkIncDecModel(event, sec+2, 1, 62)-2;
           timer->val -= sec ;
           if ((int16_t)timer->val < 0) timer->val=0;
           break;
         }
         case 3:
           CHECK_INCDEC_MODELVAR(event, timer->dir, 0, 1);
           break;
        }
      }
      if ((y+=FH)>7*FH) return;
    } subN++;
  }

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("E.Limits"));
    menu_lcd_onoff( PARAM_OFS, y, g_model.extendedLimits, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_MODELVAR(event,g_model.extendedLimits,0,1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("E.Trims"));
    menu_lcd_onoff( PARAM_OFS, y, g_model.extendedTrims, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_MODELVAR(event,g_model.extendedTrims,0,1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Trim Inc"));
    lcd_putsnAtt(  PARAM_OFS, y, PSTR("Exp   ExFineFine  MediumCoarse")+6*g_model.trimInc,6,(sub==subN ? INVERS:0));
    if(sub==subN) CHECK_INCDEC_MODELVAR(event,g_model.trimInc,0,4);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("T-Trim"));
    menu_lcd_onoff(PARAM_OFS, y, g_model.thrTrim, sub==subN && m_posHorz==0) ;
    if (sub==subN) CHECK_INCDEC_MODELVAR(event,g_model.thrTrim,0,1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Beep Ctr"));
    for(uint8_t i=0;i<7;i++) lcd_putsnAtt(PARAM_OFS+i*FW, y, PSTR("RETA123")+i,1, ((m_posHorz==i) && (sub==subN)) ? BLINK : ((g_model.beepANACenter & (1<<i)) ? INVERS : 0 ) );
    if(sub==subN){
      if((event==EVT_KEY_FIRST(KEY_MENU)) || p1valdiff) {
        killEvents(event);
        s_editMode = 0;
        g_model.beepANACenter ^= (1<<m_posHorz);
        STORE_MODELVARS;
      }
    }
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(0, y, PSTR("Proto"));
    lcd_putsnAtt(PARAM_OFS, y, PSTR(PROT_STR)+PROT_STR_LEN*g_model.protocol,PROT_STR_LEN,
                  (sub==subN && m_posHorz==0 ? (s_editMode>0 ? BLINK : INVERS):0));
    if(!g_model.protocol) { // if PPM proto
      lcd_putsnAtt(PARAM_OFS+4*FW, y, PSTR("4CH 6CH 8CH 10CH12CH14CH16CH")+4*(g_model.ppmNCH+2), 4, ((sub==subN && m_posHorz==1) ? ((s_editMode>0) ? BLINK : INVERS) : 0));
      lcd_putsAtt(PARAM_OFS+11*FW, y, PSTR("u"),0);
      lcd_outdezAtt(PARAM_OFS+11*FW, y, (g_model.ppmDelay*50)+300, ((sub==subN && m_posHorz==2) ? ((s_editMode>0) ? BLINK : INVERS) : 0));
#ifdef DSM2
    } else if (g_model.protocol == 2) { // if DSM2 proto
      if (DSM_RANGE_CHECK_BUTTON) // gruvin9x.h 
        lcd_putsAtt(PARAM_OFS+4*FW, y, PSTR(" Rng Chk"), BLINK); // indicate range check mode active
#endif
    }
    else if (sub==subN) {
      m_posHorz = 0;
    }
    if (sub==subN && (s_editMode>0 || p1valdiff || g_model.protocol) ) {
      switch (m_posHorz) {
        case 0:
            CHECK_INCDEC_MODELVAR(event,g_model.protocol,0,PROT_MAX);
            if (checkIncDec_Ret)
              resetProto();
            break;
        case 1:
            CHECK_INCDEC_MODELVAR(event,g_model.ppmNCH,-2,4);
            break;
        case 2:
            CHECK_INCDEC_MODELVAR(event,g_model.ppmDelay,-4,10);
            break;
      }
    }
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    if (!g_model.protocol) {
      lcd_puts_P(0, y, PSTR("PPM frame"));
      lcd_puts_P(PARAM_OFS+3*FW, y, PSTR("ms"));
      lcd_outdezAtt(PARAM_OFS, y, (int16_t)g_model.ppmFrameLength*5 + 225, ((sub==subN && m_posHorz==0) ? (s_editMode>0 ? BLINK : INVERS) : 0) | PREC1|LEFT);
      lcd_putsnAtt(PARAM_OFS+6*FW, y, PSTR("POSNEG")+3*g_model.pulsePol, 3, (sub==subN && m_posHorz==1) ? INVERS : 0);
      if(sub==subN && (s_editMode>0 || p1valdiff)) {
        switch (m_posHorz) {
          case 0:
            CHECK_INCDEC_MODELVAR(event, g_model.ppmFrameLength, -20, 20);
            break;
          case 1:
            CHECK_INCDEC_MODELVAR(event,g_model.pulsePol,0,1);
            break;
        }
      }
    }
    else {

    }
  }
}

static uint8_t s_currIdx;

void menuProcPhaseOne(uint8_t event)
{
  PhaseData *phase = phaseaddress(s_currIdx);
  putsFlightPhase(13*FW, 0, s_currIdx+1, 0);

  SUBMENU("FLIGHT PHASE", (s_currIdx==0 ? 3 : 5), {ZCHAR|(sizeof(phase->name)-1), 0, 3, 0/*, 0*/});

  int8_t sub = m_posVert;

  for (uint8_t i=0, k=0, y=2*FH; i<5; i++, k++, y+=FH) {
    if (s_currIdx == 0 && i==1) i = 3;
    uint8_t attr = sub==k ? INVERS : 0;
    switch(i) {
      case 0:
        lcd_puts_P(0, y, PSTR("Name"));
        EditName(10*FW, y, phase->name, sizeof(phase->name), event, attr, m_posHorz);
        break;
      case 1:
        lcd_puts_P(0, y, PSTR("Switch"));
        putsSwitches(10*FW,  y, phase->swtch, attr);
        if(attr) CHECK_INCDEC_MODELVAR(event, phase->swtch, -MAX_DRSWITCH, MAX_DRSWITCH);
        break;
      case 2:
        lcd_puts_P(0, y, PSTR("Trims"));
        for (uint8_t t=0; t<NUM_STICKS; t++) {
          int16_t v = getTrimValue(s_currIdx, t);
          if (v > TRIM_EXTENDED_MAX) {
            uint8_t p = v - TRIM_EXTENDED_MAX - 1;
            if (p >= s_currIdx) p++;
            lcd_putcAtt((10+t)*FW, y, '0'+p, (attr && m_posHorz==t) ? ((s_editMode>0) ? BLINK : INVERS) : 0);
          }
          else {
            v = TRIM_EXTENDED_MAX;
            putsChnLetter((10+t)*FW, y, t+1, (attr && m_posHorz==t) ? ((s_editMode>0) ? BLINK : INVERS) : 0);
          }
          if (attr && m_posHorz==t && ((s_editMode>0) || p1valdiff)) {
            v = checkIncDec(event, v, TRIM_EXTENDED_MAX, TRIM_EXTENDED_MAX+MAX_PHASES-1, EE_MODEL);
            if (checkIncDec_Ret) {
              if (v == TRIM_EXTENDED_MAX) v = 0;
              setTrimValue(s_currIdx, t, v);
            }
          }
        }
        break;
      case 3:
        lcd_puts_P(0, y, PSTR("Fade In"));
        lcd_outdezAtt(10*FW, y, phase->fadeIn, attr|LEFT);
        if(attr) CHECK_INCDEC_MODELVAR(event, phase->fadeIn, 0, 15);
        break;
      case 4:
        lcd_puts_P(0, y, PSTR("Fade Out"));
        lcd_outdezAtt(10*FW, y, phase->fadeOut, attr|LEFT);
        if(attr) CHECK_INCDEC_MODELVAR(event, phase->fadeOut, 0, 15);
        break;
    }
  }
}

void menuProcPhasesAll(uint8_t event)
{
  SIMPLE_MENU("FLIGHT PHASES", menuTabModel, e_PhasesAll, 1+MAX_PHASES+1);

  int8_t sub = m_posVert - 1;

  switch (event) {
    case EVT_KEY_FIRST(KEY_MENU):
#ifdef NAVIGATION_RE1
    case EVT_KEY_BREAK(BTN_RE1):
#endif
      if (sub == MAX_PHASES) {
        s_editMode = 0;
        trimsCheckTimer = 200; // 2 seconds
      }
      // no break
    case EVT_KEY_FIRST(KEY_RIGHT):
      if (sub >= 0 && sub < MAX_PHASES) {
        s_currIdx = sub;
        pushMenu(menuProcPhaseOne);
      }
      break;
  }

  uint8_t att;
  for (uint8_t i=0; i<MAX_PHASES; i++) {
    uint8_t y=(i+1)*FH;
    att = i==sub ? INVERS : 0;
    PhaseData *p = phaseaddress(i);
    putsFlightPhase(0, y, i+1, att);
    lcd_putsnAtt(4*FW, y, p->name, 6, ZCHAR);
    if (i == 0) {
      lcd_puts_P(11*FW+FW/2, y, PSTR("(default)"));
    }
    else {
      putsSwitches(11*FW+FW/2, y, p->swtch, 0);
      for (uint8_t t=0; t<NUM_STICKS; t++) {
        // TODO duplicated code
        int16_t v = getTrimValue(i, t);
        if (v > TRIM_EXTENDED_MAX) {
          uint8_t c = v - TRIM_EXTENDED_MAX - 1;
          if (c >= i) c++;
          lcd_putc((16+t)*FW-FW/2, y, '0'+c);
        }
        else {
          putsChnLetter((16+t)*FW-FW/2, y, t+1, 0);
        }
      }
    }
    if (p->fadeIn) lcd_putc(20*FW+2, y, 'I');
    if (p->fadeOut) lcd_putc(20*FW+2, y, 'O');
    if (p->fadeIn && p->fadeOut) lcd_putc(20*FW+2, y, '*');
  }

  att = (sub==MAX_PHASES && !trimsCheckTimer) ? INVERS : 0;
  lcd_putsAtt(0, 7*FH, PSTR("Check     Trims"), att);
  putsFlightPhase(6*FW, 7*FH, getFlightPhase()+1, att);
}

#ifdef HELI
void menuProcHeli(uint8_t event)
{
  SIMPLE_MENU("HELI SETUP", menuTabModel, e_Heli, 7);

  int8_t  sub    = m_posVert;
  uint8_t y = 1*FH;

  uint8_t subN = 1;
  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Swash Type"));
    lcd_putsnAtt(  14*FW, y, PSTR(SWASH_TYPE_STR)+6*g_model.swashR.type,6,(sub==subN ? INVERS:0));
    if(sub==subN) CHECK_INCDEC_MODELVAR(event,g_model.swashR.type,0,SWASH_TYPE_NUM);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Collective"));
    putsChnRaw(14*FW, y, g_model.swashR.collectiveSource,  sub==subN ? INVERS : 0);
    if(sub==subN) CHECK_INCDEC_MODELVAR(event, g_model.swashR.collectiveSource, 0, NUM_XCHNRAW);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("Swash Ring"));
    lcd_outdezAtt(14*FW, y, g_model.swashR.value,  LEFT|(sub==subN ? INVERS : 0));
    if(sub==subN) CHECK_INCDEC_MODELVAR(event, g_model.swashR.value, 0, 100);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("ELE Direction"));
    menu_lcd_HYPHINV( 14*FW, y, g_model.swashR.invertELE, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_MODELVAR(event, g_model.swashR.invertELE, 0, 1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("AIL Direction"));
    menu_lcd_HYPHINV( 14*FW, y, g_model.swashR.invertAIL, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_MODELVAR(event, g_model.swashR.invertAIL, 0, 1);
    if((y+=FH)>7*FH) return;
  }subN++;

  if(s_pgOfs<subN) {
    lcd_puts_P(    0,    y, PSTR("COL Direction"));
    menu_lcd_HYPHINV( 14*FW, y, g_model.swashR.invertCOL, sub==subN ) ;
    if(sub==subN) CHECK_INCDEC_MODELVAR(event, g_model.swashR.invertCOL, 0, 1);
    if((y+=FH)>7*FH) return;
  }subN++;
}
#endif

static uint8_t s_curveChan;

typedef int16_t (*FnFuncP) (int16_t x);

int16_t expoFn(int16_t x)
{
  ExpoData *ed = expoaddress(s_currIdx);
  int16_t anas[NUM_STICKS] = {0};
  anas[ed->chn] = x;
  applyExpos(anas);
  return anas[ed->chn];
}

int16_t curveFn(int16_t x)
{
  return intpol(x, s_curveChan);
}

void DrawCurve(FnFuncP fn)
{
  lcd_vlineStip(X0, 0, DISPLAY_H, 0xee);
  lcd_hlineStip(X0-WCHART, Y0, WCHART*2, 0xee);

  for (int8_t xv=-WCHART+1; xv<WCHART; xv++) {
    uint16_t yv = (RESX + fn(xv * (RESX/WCHART))) / 2;
    yv = (DISPLAY_H-1) - yv * (DISPLAY_H-1) / RESX;
    lcd_plot(X0+xv, yv, BLACK);
  }
}

void menuProcCurveOne(uint8_t event)
{
  uint8_t points;
  int8_t *crv;
  static int8_t dfltCrv;

  TITLE("CURVE");
  lcd_outdezAtt(5*FW+1, 0, s_curveChan+1, INVERS|LEFT);
  theFile.DisplayProgressBar(20*FW+1);

  if (s_curveChan >= MAX_CURVE5) {
    points = 9;
    crv = g_model.curves9[s_curveChan-MAX_CURVE5];
  }
  else {
    points = 5;
    crv = g_model.curves5[s_curveChan];
  }

  switch(event) {
    case EVT_ENTRY:
      dfltCrv = 0;
      break;
    case EVT_KEY_FIRST(KEY_MENU):
      if (s_editMode<=0) {
        switch (m_posHorz) {
          case 0:
            s_editMode = 1;
            break;
          case 1:
            if (++dfltCrv > 4)
              dfltCrv = -4;
            for (uint8_t i=0; i<points; i++)
              crv[i] = (i-(points/2)) * dfltCrv * 50 / (points-1);
            break;
        }
      }
      break;
    case EVT_KEY_FIRST(KEY_EXIT):
      killEvents(event);
      if (s_editMode>0) {
        m_posHorz = 0;
        s_editMode = 0;
      }
      else {
        popMenu();
      }
      break;
    case EVT_KEY_REPT(KEY_LEFT):
    case EVT_KEY_FIRST(KEY_LEFT):
      if (m_posHorz>0) m_posHorz--;
      break;
    case EVT_KEY_REPT(KEY_RIGHT):
    case EVT_KEY_FIRST(KEY_RIGHT):
      if (m_posHorz<((s_editMode>0) ? points-1 : 1)) m_posHorz++;
      break;
  }

  for (uint8_t i = 0; i < points; i++) {
    uint8_t x, y;
    if (i>4) {
      x = 8*FW; y = (i-4) * FH;
    }
    else {
      x = 4*FW; y = (i+1) * FH;
    }
    uint8_t attr = (s_editMode>0 && m_posHorz==i) ? INVERS : 0;
    lcd_outdezAtt(x, y, crv[i], attr);
  }

  lcd_puts_P(0*FW, 7*FH, PSTR("MODE"));
  lcd_putsnAtt(5*FW-2, 7*FH, PSTR("EDIT ""PRSET")+5*(s_editMode<=0)*m_posHorz, 5, (s_editMode>0) ? 0 : INVERS);

  if (s_editMode > 0) {
    for (uint8_t i=0; i<points; i++) {
      uint8_t xx = X0-1-WCHART+i*WCHART/(points/2);
      uint8_t yy = (DISPLAY_H-1) - (100 + crv[i]) * (DISPLAY_H-1) / 200;

      if (m_posHorz==i) {
        lcd_filled_rect(xx-1, yy-2, 5, 5); // do selection square
        if (p1valdiff || event==EVT_KEY_FIRST(KEY_DOWN) || event==EVT_KEY_FIRST(KEY_UP) || event==EVT_KEY_REPT(KEY_DOWN) || event==EVT_KEY_REPT(KEY_UP))
          CHECK_INCDEC_MODELVAR( event, crv[i], -100,100);  // edit on up/down
      }
      else {
        lcd_filled_rect(xx, yy-1, 3, 3); // do markup square
      }
    }
  }

  DrawCurve(curveFn);
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

inline void editExpoVals(uint8_t event, uint8_t which, bool edit, uint8_t y, uint8_t idt)
{
  uint8_t invBlk = edit ? INVERS : 0;
  // if(edit && stopBlink) invBlk = INVERS;

  ExpoData *ed = expoaddress(idt);

  switch(which)
  {
    case 0:
      lcd_outdezAtt(9*FW+5, y, ed->weight, invBlk);
      if(edit) CHECK_INCDEC_MODELVAR(event, ed->weight, 0, 100);
      break;
    case 1:
      lcd_outdezAtt(9*FW+5, y, ed->expo, invBlk);
      if(edit) CHECK_INCDEC_MODELVAR(event, ed->expo, -100, 100);
      break;
    case 2:
      {
        int8_t phase = ed->negPhase ? -ed->phase : +ed->phase;
        putsFlightPhase(6*FW+5, y, phase, invBlk);
        if(edit) { phase = checkIncDecModel(event, phase, -MAX_PHASES, MAX_PHASES); ed->negPhase = (phase < 0); ed->phase = abs(phase); }
      }
      break;
    case 3:
      putsSwitches(6*FW+5, y, ed->swtch, invBlk);
      if(edit) CHECK_INCDEC_MODELVAR(event, ed->swtch, -MAX_DRSWITCH, MAX_DRSWITCH);
      break;
    case 4:
      lcd_putsnAtt(6*FW+5, y, PSTR("---x>0x<0")+9-3*ed->mode, 3, invBlk);
      if(edit) ed->mode = 4 - checkIncDecModel(event, 4-ed->mode, 1, 3);
      break;
    case 5:
      putsCurve(6*FW+5, y, ed->curve+(ed->curve >= CURVE_BASE+4 ? 4 : 0), invBlk);
      if(invBlk) CHECK_INCDEC_MODELVAR(event, ed->curve, 0, 15);
      if(invBlk && ed->curve>=CURVE_BASE && event==EVT_KEY_FIRST(KEY_MENU)) {
        s_curveChan = ed->curve - (ed->curve >= CURVE_BASE+4 ? CURVE_BASE-4 : CURVE_BASE);
        pushMenu(menuProcCurveOne);
      }
      break;
  }
}

void menuProcExpoOne(uint8_t event)
{
  ExpoData *ed = expoaddress(s_currIdx);
  putsChnRaw(7*FW+FW/2,0,ed->chn+1,0);

  SIMPLE_SUBMENU("DR/EXPO", 6);

  int8_t  sub = m_posVert;

  uint8_t  y = FH;

  for (uint8_t i=0; i<7; i++) {
    lcd_putsnAtt(0, y, PSTR("Weight""Expo  ""Phase ""Swtch ""When  ""Curve ""      ")+6*i, 6, 0);
    editExpoVals(event, i, sub==i, y, s_currIdx);
    y+=FH;
  }

  DrawCurve(expoFn);

  int16_t x512  = calibratedStick[ed->chn];
  int16_t anas[NUM_STICKS] = {0};
  anas[ed->chn] = x512;
  applyExpos(anas);
  int16_t y512  = anas[ed->chn];

  lcd_outdezAtt(20*FW, 6*FH, x512*25/256, 0);
  lcd_outdezAtt(14*FW, 1*FH, y512*25/256, 0);

  x512 = X0+x512/(RESXu/WCHART);
  y512 = (DISPLAY_H-1) - (uint16_t)((y512+RESX)/2) * (DISPLAY_H-1) / RESX;

  lcd_vline(x512, y512-3,3*2+1);
  lcd_hline(x512-3, y512,3*2+1);
}

void menuProcMixOne(uint8_t event)
{
  TITLEP(s_currCh ? PSTR("INSERT MIX ") : PSTR("EDIT MIX "));
  MixData *md2 = mixaddress(s_currIdx) ;
  putsChn(lcd_lastPos+1*FW,0,md2->destCh,0);
  SIMPLE_SUBMENU_NOTITLE(13);

  int8_t  sub    = m_posVert;

  for(uint8_t k=0; k<7; k++)
  {
    uint8_t y = (k+1) * FH;
    uint8_t i = k + s_pgOfs;
    uint8_t attr = sub==i ? INVERS : 0;
    switch(i) {
      case 0:
        lcd_puts_P(  2*FW,y,PSTR("Source"));
        putsChnRaw(   FW*10,y,md2->srcRaw,attr);
        if(attr) CHECK_INCDEC_MODELVAR( event, md2->srcRaw, 1,NUM_XCHNRAW);
        break;
      case 1:
        lcd_puts_P(  2*FW,y,PSTR("Weight"));
        lcd_outdezAtt(FW*10,y,md2->weight,attr|LEFT);
        if(attr) CHECK_INCDEC_MODELVAR( event, md2->weight, -125,125);
        break;
      case 2:
        lcd_puts_P(  2*FW,y,PSTR("Offset"));
        lcd_outdezAtt(FW*10,y,md2->sOffset,attr|LEFT);
        if(attr) CHECK_INCDEC_MODELVAR( event, md2->sOffset, -125,125);
        break;
      case 3:
        lcd_puts_P(  2*FW,y,PSTR("Trim"));
        lcd_putsnAtt(FW*10,y, PSTR("ON OFF")+3*md2->carryTrim,3,attr);  //default is 0=ON
        if(attr) CHECK_INCDEC_MODELVAR( event, md2->carryTrim, 0,1);
        break;
      case 4:
        lcd_putsAtt(  2*FW,y,PSTR("Curves"),0);
        putsCurve(FW*10, y, md2->curve, attr);
        if(attr) CHECK_INCDEC_MODELVAR( event, md2->curve, 0,MAX_CURVE5+MAX_CURVE9+7-1);
        if(attr && md2->curve>=CURVE_BASE && event==EVT_KEY_FIRST(KEY_MENU)){
          s_curveChan = md2->curve-CURVE_BASE;
          pushMenu(menuProcCurveOne);
        }
        break;
      case 5:
        lcd_puts_P(  2*FW,y,PSTR("Switch"));
        putsSwitches(10*FW,  y,md2->swtch,attr);
        if(attr) CHECK_INCDEC_MODELVAR( event, md2->swtch, -MAX_SWITCH, MAX_SWITCH);
        break;
      case 6:
        lcd_puts_P(  2*FW,y,PSTR("F.Phase"));
        putsFlightPhase(10*FW, y, md2->phase, attr);
        if(attr) CHECK_INCDEC_MODELVAR( event, md2->phase, -MAX_PHASES, MAX_PHASES);
        break;
      case 7:
        lcd_puts_P(  2*FW,y,PSTR("Warning"));
        if(md2->mixWarn)
          lcd_outdezAtt(FW*10,y,md2->mixWarn,attr|LEFT);
        else
          lcd_putsAtt(  FW*10,y,PSTR("OFF"),attr);
        if(attr) CHECK_INCDEC_MODELVAR( event, md2->mixWarn, 0,3);
        break;
      case 8:
        lcd_puts_P(  2*FW,y,PSTR("Multpx"));
        lcd_putsnAtt(10*FW, y,PSTR("Add     MultiplyReplace ")+8*md2->mltpx,8,attr);
        if(attr) CHECK_INCDEC_MODELVAR( event, md2->mltpx, 0, 2);
        break;
      case 9:
        lcd_puts_P(  2*FW,y,PSTR("Delay Down"));
        lcd_outdezAtt(FW*16,y,md2->delayDown,attr);
        if(attr)  CHECK_INCDEC_MODELVAR( event, md2->delayDown, 0,15);
        break;
      case 10:
        lcd_puts_P(  2*FW,y,PSTR("Delay Up"));
        lcd_outdezAtt(FW*16,y,md2->delayUp,attr);
        if(attr)  CHECK_INCDEC_MODELVAR( event, md2->delayUp, 0,15);
        break;
      case 11:
        lcd_puts_P(  2*FW,y,PSTR("Slow  Down"));
        lcd_outdezAtt(FW*16,y,md2->speedDown,attr);
        if(attr)  CHECK_INCDEC_MODELVAR( event, md2->speedDown, 0,15);
        break;
      case 12:
        lcd_puts_P(  2*FW,y,PSTR("Slow  Up"));
        lcd_outdezAtt(FW*16,y,md2->speedUp,attr);
        if(attr)  CHECK_INCDEC_MODELVAR( event, md2->speedUp, 0,15);
        break;
    }
  }
}

static uint8_t s_maxLines = 8;
static uint8_t s_copySrcIdx;
static uint8_t s_copySrcCh;

inline void displayMixerLine(uint8_t row, uint8_t mix, uint8_t ch, uint8_t idx, uint8_t cur, uint8_t event)
{
  uint8_t y = (row-s_pgOfs)*FH;
  MixData *md = mixaddress(mix);
  if (idx > 0)
    lcd_putsnAtt(FW, y, PSTR("+=*=:=")+md->mltpx*2, 2, 0);

  putsChnRaw(4*FW+2, y, md->srcRaw, 0);

  uint8_t attr = ((s_copyMode || cur != row) ? 0 : INVERS);
  lcd_outdezAtt(11*FW+7, y, md->weight, attr);
  if (attr != 0)
    CHECK_INCDEC_MODELVAR(event, md->weight, -125, 125);

  if (md->curve) putsCurve(12*FW+7, y, md->curve);
  if (md->swtch) putsSwitches(16*FW+6, y, md->swtch);

  char cs = ' ';
  if (md->speedDown || md->speedUp)
    cs = 'S';
  if ((md->delayUp || md->delayDown))
    cs = (cs =='S' ? '*' : 'D');
  lcd_putcAtt(20*FW+3, y, cs, 0);

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

inline void displayExpoLine(uint8_t row, uint8_t expo, uint8_t ch, uint8_t idx, uint8_t cur, uint8_t event)
{
  uint8_t y = (row-s_pgOfs)*FH;
  ExpoData *ed = expoaddress(expo);

  uint8_t attr = ((s_copyMode || cur != row) ? 0 : INVERS);
  lcd_outdezAtt(6*FW-2, y, ed->weight, attr);
  if (attr != 0)
    CHECK_INCDEC_MODELVAR(event, ed->weight, 0, 100);
  lcd_outdezAtt(9*FW+1, y, ed->expo, 0);
  putsFlightPhase(10*FW, y, ed->negPhase ? -ed->phase : +ed->phase);
  putsSwitches(13*FW+4, y, ed->swtch, 0); // normal switches
  if (ed->mode!=3) lcd_putc(17*FW, y, ed->mode == 2 ? 126 : 127);//'|' : (stkVal[i] ? '<' : '>'),0);*/
  if (ed->curve) putsCurve(18*FW+2, y, ed->curve+(ed->curve >= CURVE_BASE+4 ? 4 : 0));

  if (s_copyMode) {
    if ((s_copyMode==COPY_MODE || s_copyTgtOfs == 0) && s_copySrcCh == ch && expo == (s_copySrcIdx + (s_copyTgtOfs<0))) {
      /* draw a border around the raw on selection mode (copy/move) */
      lcd_rect(18, y-1, DISPLAY_W-18, 9, s_copyMode == COPY_MODE ? 0xff : 0x55);
    }
    if (row == cur) {
      /* invert the raw when it's the current one */
      lcd_filled_rect(19, y, DISPLAY_W-20, 7);
    }
  }
}

void menuProcExpoMix(uint8_t expo, uint8_t _event_)
{
  uint8_t _event = (s_warning ? 0 : _event_);
  uint8_t event = _event;
  uint8_t key = (event & 0x1f);

  if (s_copyMode) {
    if (key == KEY_EXIT)
      event -= KEY_EXIT;
  }

  TITLEP(expo ? PSTR("DR/EXPO") : PSTR("MIXER"));
  lcd_outdezAtt(lcd_lastPos+2*FW+FW/2, 0, getExpoMixCount(expo));
  lcd_puts_P(lcd_lastPos, 0, expo ? PSTR("/14") : PSTR("/32"));
  SIMPLE_MENU_NOTITLE(menuTabModel, expo ? e_ExposAll : e_MixAll, s_maxLines);

  uint8_t sub = m_posVert;

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
        sub = m_posVert = s_copySrcRow;
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
        killEvents(_event);
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
            sub = m_posVert = cur;
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
  if (sub >= s_maxLines-1) m_posVert = s_maxLines-1;
  displayWarning(_event_);
}

void menuProcExposAll(uint8_t event)
{
  return menuProcExpoMix(1, event);
}

void menuProcMixAll(uint8_t event)
{
  return menuProcExpoMix(0, event);
}

void menuProcLimits(uint8_t event)
{
  MENU("LIMITS", menuTabModel, e_Limits, NUM_CHNOUT+2, {0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3/*, 0*/});

  static bool swVal[NUM_CHNOUT];

  int8_t sub = m_posVert - 1;

  for (uint8_t i=0; i<7; i++) {
    uint8_t y = (i+1)*FH;
    uint8_t k = i+s_pgOfs;

    if (k==NUM_CHNOUT) {
      //last line available - add the "copy trim menu" line
      uint8_t attr = (sub==NUM_CHNOUT) ? INVERS : 0;
      lcd_putsAtt(3*FW, y, PSTR("COPY TRIM [MENU]"), s_noHi ? 0 : attr);
      if (attr && event==EVT_KEY_LONG(KEY_MENU)) {
        s_noHi = NO_HI_LEN;
        killEvents(event);
        moveTrimsToOffsets(); // if highlighted and menu pressed - copy trims
      }
      return;
    }

    LimitData *ld = limitaddress(k) ;
    int16_t v = (ld->revert) ? -ld->offset : ld->offset;
    if((g_chans512[k] - v) >  50) swVal[k] = (true==ld->revert);// Switch to raw inputs?  - remove trim!
    if((g_chans512[k] - v) < -50) swVal[k] = (false==ld->revert);
    putsChn(0,y,k+1,0);
    lcd_putcAtt(12*FW+FW/2, y, (swVal[k] ? 127 : 126),0); //'<' : '>'
    for (uint8_t j=0; j<4; j++) {
      uint8_t attr = ((sub==k && m_posHorz==j) ? ((s_editMode>0) ? BLINK : INVERS) : 0);
      uint8_t active = (attr && (s_editMode>0 || p1valdiff)) ;
      switch(j)
      {
        case 0:
          lcd_outdezAtt(  8*FW, y,  ld->offset, attr|PREC1);
          if (active) {
            ld->offset = checkIncDec(event, ld->offset, -1000, 1000, EE_MODEL);
          }
          else if (attr && event==EVT_KEY_LONG(KEY_MENU)) {
            int16_t zero = g_chans512[k];
            ld->offset = (ld->revert) ? -zero : zero;
            s_editMode = 0;
            STORE_MODELVARS;
          }
          break;
        case 1:
          lcd_outdezAtt(  12*FW, y, (int8_t)(ld->min-100),   attr);
          if (active) {
            ld->min -= 100;
            if(g_model.extendedLimits)
              CHECK_INCDEC_MODELVAR( event, ld->min, -125,125);
            else
              CHECK_INCDEC_MODELVAR( event, ld->min, -100,100);
            ld->min += 100;
          }
          break;
        case 2:
          lcd_outdezAtt( 17*FW, y, (int8_t)(ld->max+100),    attr);
          if (active) {
            ld->max += 100;
            if(g_model.extendedLimits)
              CHECK_INCDEC_MODELVAR( event, ld->max, -125,125);
            else
              CHECK_INCDEC_MODELVAR( event, ld->max, -100,100);
            ld->max -= 100;
          }
          break;
        case 3:
          lcd_putsnAtt(   18*FW, y, PSTR("---INV")+ld->revert*3,3,attr);
          if (active) {
            CHECK_INCDEC_MODELVAR(event, ld->revert, 0, 1);
          }
          break;
      }
    }
  }
}

void menuProcCurvesAll(uint8_t event)
{
  SIMPLE_MENU("CURVES", menuTabModel, e_CurvesAll, 1+MAX_CURVE5+MAX_CURVE9);

  int8_t  sub    = m_posVert - 1;

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
    putsStrIdx(0, y, PSTR("CV"), k+1, attr);
    int8_t *crv = cv9 ? g_model.curves9[k-MAX_CURVE5] : g_model.curves5[k];
    for (uint8_t j = 0; j < (5); j++) {
      lcd_outdezAtt( j*(3*FW+3) + 7*FW + 2, y, crv[j], 0);
    }
    y += FH;yd++;
    if(cv9){
      for (uint8_t j = 0; j < 4; j++) {
        lcd_outdezAtt( j*(3*FW+3) + 7*FW + 2, y, crv[j+5], 0);
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
  int8_t  sub    = m_posVert - 1;

  for(uint8_t i=0; i<7; i++){
    y=(i+1)*FH;
    k=i+s_pgOfs;
    if(k==NUM_CSW) break;
    uint8_t attr = (sub==k ? ((s_editMode>0) ? BLINK : INVERS)  : 0);
    CustomSwData &cs = g_model.customSw[k];

    //write SW names here
    lcd_putsnAtt( 0*FW , y, PSTR("SW"),2,0);
    lcd_putc(  2*FW, y, k + (k>8 ? 'A'-9: '1'));
    lcd_putsnAtt( 4*FW - 1, y, PSTR(CSWITCH_STR)+CSW_LEN_FUNC*cs.func,CSW_LEN_FUNC,m_posHorz==0 ? attr : 0);

    uint8_t cstate = CS_STATE(cs.func);

    if(cstate == CS_VOFS)
    {
        putsChnRaw(    12*FW, y, cs.v1  ,m_posHorz==1 ? attr : 0);
#if defined(FRSKY)
        if (cs.v1 > NUM_XCHNRAW-NUM_TELEMETRY)
          lcd_outdezAtt( 20*FW, y, 125+cs.v2  ,m_posHorz==2 ? attr : 0);
        else
#endif
        lcd_outdezAtt( 20*FW, y, cs.v2  ,m_posHorz==2 ? attr : 0);
    }
    else if(cstate == CS_VBOOL)
    {
        putsSwitches(13*FW, y, cs.v1  ,m_posHorz==1 ? attr : 0);
        putsSwitches(17*FW, y, cs.v2  ,m_posHorz==2 ? attr : 0);
    }
    else // cstate == CS_COMP
    {
        putsChnRaw(    12*FW, y, cs.v1  ,m_posHorz==1 ? attr : 0);
        putsChnRaw(    17*FW, y, cs.v2  ,m_posHorz==2 ? attr : 0);
    }

    if((s_editMode>0 || p1valdiff) && attr)
      switch (m_posHorz) {
        case 0:
          CHECK_INCDEC_MODELVAR( event, cs.func, 0,CS_MAXF);
          if(cstate != CS_STATE(cs.func))
          {
              cs.v1  = 0;
              cs.v2 = 0;
          }
          break;
        case 1:
          switch (cstate) {
          case (CS_VOFS):
              CHECK_INCDEC_MODELVAR( event, cs.v1, 0,NUM_XCHNRAW);
              break;
          case (CS_VBOOL):
              CHECK_INCDEC_MODELVAR( event, cs.v1, -MAX_SWITCH,MAX_SWITCH);
              break;
          case (CS_VCOMP):
              CHECK_INCDEC_MODELVAR( event, cs.v1, 0,NUM_XCHNRAW);
              break;
          default:
              break;
          }
          break;
        case 2:
          switch (cstate) {
          case (CS_VOFS):
              CHECK_INCDEC_MODELVAR( event, cs.v2, -125,125);
              break;
          case (CS_VBOOL):
              CHECK_INCDEC_MODELVAR( event, cs.v2, -MAX_SWITCH,MAX_SWITCH);
              break;
          case (CS_VCOMP):
              CHECK_INCDEC_MODELVAR( event, cs.v2, 0,NUM_XCHNRAW);
              break;
          default:
              break;
          }
      }
  }
}

void menuProcFunctionSwitches(uint8_t event)
{
  MENU("FUNC SWITCHES", menuTabModel, e_FunctionSwitches, NUM_FSW+1, {0, 1/*repeated*/});

  uint8_t y = 0;
  uint8_t k = 0;
  int8_t  sub    = m_posVert - 1;

  for(uint8_t i=0; i<7; i++) {
    y=(i+1)*FH;
    k=i+s_pgOfs;
    if(k==NUM_CHNOUT) break;
    FuncSwData *sd = &g_model.funcSw[k];
    for (uint8_t j=0; j<2; j++) {
      uint8_t attr = ((sub==k && m_posHorz==j) ? ((s_editMode>0) ? BLINK : INVERS) : 0);
      uint8_t active = (attr && (s_editMode>0 || p1valdiff));
      switch (j) {
        case 0:
          putsSwitches(1*FW, y, sd->swtch, attr);
          if (active) {
            CHECK_INCDEC_MODELVAR( event, sd->swtch, -MAX_SWITCH, MAX_SWITCH);
          }
          break;
        case 1:
          if (sd->swtch) {
            lcd_putsnAtt(5*FW, y, PSTR(FSWITCH_STR)+FSW_LEN_FUNC*sd->func, FSW_LEN_FUNC, attr);
            if (active) {
              CHECK_INCDEC_MODELVAR( event, sd->func, 0, FUNC_MAX-1);
            }
          }
          else if (attr) {
            m_posHorz = 0;
          }
          break;
      }
    }
  }
}

void menuProcSafetySwitches(uint8_t event)
{
  MENU("SAFETY SWITCHES", menuTabModel, e_SafetySwitches, NUM_CHNOUT+1, {0, 1/*repeated*/});
 
  uint8_t y = 0;
  uint8_t k = 0;

  for(uint8_t i=0; i<7; i++){
    y=(i+1)*FH;
    k=i+s_pgOfs;
    if(k==NUM_CHNOUT) break;
    SafetySwData *sd = &g_model.safetySw[k];
    putsChn(0,y,k+1,0);
    for(uint8_t j=0; j<=2;j++){
      uint8_t attr = ((m_posVert-1==k && m_posHorz==j) ? ((s_editMode>0) ? BLINK : INVERS) : 0);
      uint8_t active = (attr && (s_editMode>0 || p1valdiff));
      switch(j)
      {
        case 0:
          putsSwitches(6*FW, y, sd->swtch  , attr);
          if (active) {
            CHECK_INCDEC_MODELVAR( event, sd->swtch, -MAX_SWITCH, MAX_SWITCH);
          }
          break;
        case 1:
          lcd_outdezAtt(16*FW, y, sd->val,   attr);
          if (active) {
            CHECK_INCDEC_MODELVAR( event, sd->val, -125, 125);
          }
          break;
      }
    }
  }
}

#ifdef FRSKY
void menuProcTelemetry(uint8_t event)
{
#if defined FRSKY_HUB || defined WS_HOW_HIGH
  MENU("FR-SKY TELEMETRY", menuTabModel, e_Telemetry, 14, {0, 0, -1, 0, 0, 1, 2, 2, -1, 0, 0, 1, 2, 2});
#else
  MENU("TELEMETRY", menuTabModel, e_Telemetry, 13, {0, -1, 0, 0, 1, 2, 2, -1, 0, 0, 1, 2, 2});
#endif

  int8_t  sub    = m_posVert;
  uint8_t blink;
  uint8_t y;
  uint8_t ch = 0;
  uint16_t ratio;

  switch(event) {

    case EVT_KEY_FIRST(KEY_MENU): // press [MENU] to advance to next whole volt
      if (sub == 3)
        g_model.frsky.channels[0].ratio = ((g_model.frsky.channels[0].ratio + 100) % 4096)/100*100;
      else if (sub == 9)
        g_model.frsky.channels[1].ratio = ((g_model.frsky.channels[1].ratio + 100) % 4096)/100*100;
      if (sub == 3 || sub == 9) eeDirty(EE_MODEL);
      break;

    case EVT_KEY_LONG(KEY_MENU): // press [MENU_LONG] to select standard 2:1 or 4:1 resistor divder (max volts)

      if (sub == 4) ch = 0;
      else if (sub == 10) ch = 1;
      else break;

      ratio = g_model.frsky.channels[ch].ratio;  
      if (ratio < 660) ratio = 660;
      else if (ratio < 1320) ratio = 1320;
      else ratio = 0;
      g_model.frsky.channels[ch].ratio = ratio;
      eeDirty(EE_MODEL);
      
      break;

    case EVT_KEY_BREAK(KEY_DOWN):
    case EVT_KEY_BREAK(KEY_UP):
    case EVT_KEY_BREAK(KEY_LEFT):
    case EVT_KEY_BREAK(KEY_RIGHT):
      if(s_editMode>0 && (sub != 4 && sub != 10)) // gbar lines are only other edit mode fields
        FRSKY_setModelAlarms(); // update Fr-Sky module when edit mode exited
  }

  blink = (s_editMode>0) ? BLINK : INVERS ;
  uint8_t subN = 1;
  uint8_t t;

#if defined(FRSKY_HUB) || defined(WS_HOW_HIGH)
  if(s_pgOfs<subN) {
    y = (subN-s_pgOfs)*FH;
    lcd_puts_P(0, y, PSTR("Data Port"));
#if defined(WS_HOW_HIGH)
#define USR_PROTO_STR  "None fsHubWSHH ASCII"
#define USR_PROTO_MAX  4
#else
#define USR_PROTO_STR "None fsHubASCII"
#define USR_PROTO_MAX 3
#endif
    lcd_putsnAtt(10*FW, y, PSTR(USR_PROTO_STR)+5*g_model.frsky.usrProto, 5, sub==subN ? INVERS:0);
    if (sub==subN)
      CHECK_INCDEC_MODELVAR(event, g_model.frsky.usrProto, 0, USR_PROTO_MAX);
  }
  subN++;
#endif

  for (int i=0; i<2; i++) {
    if(s_pgOfs<subN) {
      y=(subN-s_pgOfs)*FH;
      lcd_puts_P(0, y, PSTR("A  channel"));
      lcd_outdezAtt(2*FW, y, 1+i, 0);
    }
    subN++;

    if(s_pgOfs<subN) {
      y=(subN-s_pgOfs)*FH;
      lcd_putsAtt(4, y, PSTR("Type"), 0); // TODO puts_P
      lcd_putsnAtt(7*FW, y, PSTR("Volts""raw  ")+5*g_model.frsky.channels[i].type, 5, (sub==subN ? INVERS:0));
      if (sub==subN)  CHECK_INCDEC_MODELVAR(event, g_model.frsky.channels[i].type, 0, 1);
    }
    subN++;

    // subN == 3
    if(s_pgOfs<subN) {
      y=(subN-s_pgOfs)*FH;
      lcd_putsAtt(4, y, PSTR("MaxV"), 0);
      lcd_outdezNAtt(7*FW+1, y, g_model.frsky.channels[i].ratio, (sub==subN ? INVERS:0)|PREC2|LEFT);
      lcd_putc(lcd_lastPos, y, 'v');

      if (g_model.frsky.channels[i].type == 0) // display 'cal(ibrate) and arrow only if type == volts
      {
        frskyPutAValue(16*FW, y, i, frskyTelemetry[i].value, LEFT|PREC2);
        lcd_puts_P(12*FW-3, y, PSTR("cal"));
        lcd_hline(14*FW+3, y+3, 7);
        lcd_plot(14*FW+8, y+2);
        lcd_plot(14*FW+8, y+4);
      }

      if (sub==subN)
          g_model.frsky.channels[i].ratio = checkIncDec(event, g_model.frsky.channels[i].ratio, 0, 4095/* 40.95V */, EE_MODEL);
    }
    subN++;

    // subN == 4
    if(s_pgOfs<subN) {
      y=(subN-s_pgOfs)*FH;
      lcd_puts_P(4, y, PSTR("G.Bar"));

      // These voltages need to extend all the way out a possbile maxVolts 40.95 (4096)
      frskyPutAValue(7*FW+1, y, i, g_model.frsky.channels[i].barMin, (sub==subN && m_posHorz==0 ? blink:0)|LEFT|PREC2);

      lcd_puts_P(lcd_lastPos+FW, y, PSTR("to"));
      
      frskyPutAValue(lcd_lastPos+FW, y, i, g_model.frsky.channels[i].barMax, (sub==subN && m_posHorz==1 ? blink:0)|LEFT|PREC2);

      if(sub==subN && m_posHorz==0 && (s_editMode>0 || p1valdiff)) g_model.frsky.channels[i].barMin =
        checkIncDec(event, g_model.frsky.channels[i].barMin, 0, g_model.frsky.channels[i].barMax, EE_MODEL);
      if(sub==subN && m_posHorz==1 && (s_editMode>0 || p1valdiff)) g_model.frsky.channels[i].barMax =
        checkIncDec(event, g_model.frsky.channels[i].barMax, g_model.frsky.channels[i].barMin, 255, EE_MODEL);
    }
    subN++;

    for (int j=0; j<2; j++) {
      if(s_pgOfs<subN) {
        y=(subN-s_pgOfs)*FH;
        lcd_putsAtt(4, y, PSTR("Alarm"), 0); lcd_putc(lcd_lastPos, y, '1'+j);
        lcd_putsnAtt(8*FW, y, PSTR("---YelOrgRed")+3*ALARM_LEVEL(i, j),3,(sub==subN && m_posHorz==0 ? blink:0));
        lcd_putsnAtt(13*FW, y, PSTR("<>")+ALARM_GREATER(i, j),1,(sub==subN && m_posHorz==1 ? blink:0));
      
        frskyPutAValue(16*FW, y, i, g_model.frsky.channels[i].alarms_value[j], (sub==subN && m_posHorz==2 ? blink:0)|LEFT|PREC2);

        if(sub==subN && (s_editMode>0 || p1valdiff)) {
          switch (m_posHorz) {
            case 0:
              t = ALARM_LEVEL(i, j);
              g_model.frsky.channels[i].alarms_level = (g_model.frsky.channels[i].alarms_level & ~(3<<(2*j)))
                + (checkIncDec(event, t, 0, 3, EE_MODEL) << (2*j));
              break;
            case 1:
              t = ALARM_GREATER(i, j);
              g_model.frsky.channels[i].alarms_greater = (g_model.frsky.channels[i].alarms_greater & ~(1<<j))
                + (checkIncDec(event, t, 0, 1, EE_MODEL) << j);
              if(checkIncDec_Ret)
                FRSKY_setModelAlarms();
              break;
            case 2:
              g_model.frsky.channels[i].alarms_value[j] = 
                checkIncDec(event, g_model.frsky.channels[i].alarms_value[j], 0, 255, EE_MODEL);
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
  SIMPLE_MENU("TEMPLATES", menuTabModel, e_Templates, 1+NUM_TEMPLATES+1);

  uint8_t y = 0;
  uint8_t k = 0;
  int8_t  sub    = m_posVert - 1;

  switch(event)
  {
    case EVT_KEY_LONG(KEY_MENU):
      killEvents(event);
      //apply mixes or delete
      s_noHi = NO_HI_LEN;
      if (sub>=0 && sub<(int8_t)NUM_TEMPLATES)
        applyTemplate(sub);
      if (sub==NUM_TEMPLATES)
        clearMixes();
      beepWarn1();
      break;
  }

  y=1*FH;
  for(uint8_t i=0; i<7; i++){
    k=i+s_pgOfs;
    if(k==NUM_TEMPLATES) break;

    //write mix names here
    lcd_outdezNAtt(3*FW, y, k+1, (sub==k ? INVERS : 0)|LEADING0, 2);
    lcd_putsnAtt(4*FW, y, PSTR(TEMPLATES_STR)+TEMPLATES_LEN*k, TEMPLATES_LEN, (s_noHi ? 0 : (sub==k ? INVERS  : 0)));
    y+=FH;
  }
  if(y>7*FH) return;

  uint8_t attr = s_noHi ? 0 : ((sub==NUM_TEMPLATES) ? INVERS : 0);
  lcd_putsAtt(  1*FW,y,PSTR("CLEAR MIXES [MENU]"),attr);
  y+=FH;
}
#endif

