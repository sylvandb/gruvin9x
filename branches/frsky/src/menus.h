/*
   Insert obligatories here

   */

#ifndef menus_h
#define menus_h

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

extern bool warble;
extern int16_t p1valdiff;

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

/* 
   "lines..." below is a list of numbers representing the column count (minus 1) for each row
  
    Ex. Assuming all fields "11, 22, 33, etc" in this display are editibale elements ...

    TEST MENU         4/4
      11   22   33
      44   55   66
      77   88  99  00 
      
      ... one would use, MENU(... , 4, {0, 2, 2, 3}); ... 4 rows, last row with 4 columns.

      (The page counter (4/4) occupies line 0. So first 'lines' entry is always 0.)
*/
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
