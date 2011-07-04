/*
   Insert obligatories here

   */

#ifndef menus_h
#define menus_h

#include <inttypes.h>
#include "gruvin9x.h"

#define IS_THROTTLE(x)  (((2-(g_eeGeneral.stickMode&1)) == x) && (x<4))

#define NO_HI_LEN 25

#define WCHART 32
#define X0     (128-WCHART-2)
#define Y0     32
#define WCHARTl 32l
#define X0l     (128l-WCHARTl-2)
#define Y0l     32l
#define RESX    1024
#define RESXu   1024u
#define RESXul  1024ul
#define RESXl   1024l
#define RESKul  100ul
#define RESX_PLUS_TRIM (RESX+128)

typedef void (*MenuFuncP)(uint8_t event);

void DisplayScreenIndex(uint8_t index, uint8_t count, uint8_t attr);

extern uint8_t s_pgOfs;
extern uint8_t s_noHi;

extern int16_t expo(int16_t x, int16_t k);

void menu_lcd_onoff(uint8_t x, uint8_t y, uint8_t value, uint8_t mode);
void menu_lcd_HYPHINV(uint8_t x, uint8_t y, uint8_t value, uint8_t mode);

extern MenuFuncP g_menuStack[5];
extern uint8_t g_menuStackPtr;

/// goto given Menu, but substitute current menu in menuStack
void    chainMenu(MenuFuncP newMenu);
/// goto given Menu, store current menu in menuStack
void    pushMenu(MenuFuncP newMenu);
///deliver address of last menu which was popped from
MenuFuncP lastPopMenu();
/// return to last menu in menustack
/// if uppermost is set true, thenmenu return to uppermost menu in menustack
void    popMenu(bool uppermost=false);

void menuMainView(uint8_t event);
void menuProcSetup(uint8_t event);
void menuProcModelSelect(uint8_t event);
void menuProcStatistic(uint8_t event);
void menuProcStatistic2(uint8_t event);

// Menus related stuff ...
struct MState2
{
  uint8_t m_posVert;
  uint8_t m_posHorz;
  void init(){m_posVert=m_posHorz=0;};
  void check(uint8_t event, uint8_t curr, MenuFuncP *menuTab, uint8_t menuTabSize, prog_uint8_t *subTab, uint8_t subTabMax, uint8_t maxrow);
  void check_simple(uint8_t event, uint8_t curr, MenuFuncP *menuTab, uint8_t menuTabSize, uint8_t maxrow);
  void check_submenu_simple(uint8_t event, uint8_t maxrow);
};

typedef PROGMEM void (*MenuFuncP_PROGMEM)(uint8_t event);

#define TITLEP(pstr) lcd_putsAtt(0,0,pstr,INVERS)
#define TITLE(str)   TITLEP(PSTR(str))

#define MENU(title, tab, menu, lines_count, lines...) \
TITLE(title); \
static MState2 mstate2; \
static prog_uint8_t APM mstate_tab[] = lines; \
mstate2.check(event,menu,tab,DIM(tab),mstate_tab,DIM(mstate_tab)-1,lines_count-1)

#define SIMPLE_MENU(title, tab, menu, lines_count) \
TITLE(title); \
static MState2 mstate2; \
mstate2.check_simple(event,menu,tab,DIM(tab),lines_count-1)

#define SUBMENU(title, lines_count, lines...) \
TITLE(title); \
static MState2 mstate2; \
static prog_uint8_t APM mstate_tab[] = lines; \
mstate2.check(event,0,NULL,0,mstate_tab,DIM(mstate_tab)-1,lines_count-1)

#define SIMPLE_SUBMENU_NOTITLE(lines_count) \
static MState2 mstate2; \
mstate2.check_submenu_simple(event,lines_count-1)

#define SIMPLE_SUBMENU(title, lines_count) \
TITLE(title); \
SIMPLE_SUBMENU_NOTITLE(lines_count-1)

#endif
