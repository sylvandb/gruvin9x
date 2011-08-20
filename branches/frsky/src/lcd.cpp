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

#include "lcd.h"

uint8_t displayBuf[DISPLAY_W*DISPLAY_H/8];
#define DISPLAY_END (displayBuf+sizeof(displayBuf))
#include "font.lbm"
#define font_5x8_x20_x7f (font+3)

#include "font_dblsize.lbm"
#define font_10x16_x20_x7f (font_dblsize+3)

void lcd_clear()
{
  memset(displayBuf, 0, sizeof(displayBuf));
}

void lcd_img(uint8_t i_x,uint8_t i_y,const prog_uchar * imgdat,uint8_t idx,uint8_t mode)
{
  const prog_uchar  *q = imgdat;
  uint8_t w    = pgm_read_byte(q++);
  uint8_t hb   = (pgm_read_byte(q++)+7)/8;
  uint8_t sze1 = pgm_read_byte(q++);
  q += idx*sze1;
  bool    inv  = (mode & INVERS) ? true : (mode & BLINK ? BLINK_ON_PHASE : false);
  for(uint8_t yb = 0; yb < hb; yb++){
    uint8_t   *p = &displayBuf[ (i_y / 8 + yb) * DISPLAY_W + i_x ];
    for(uint8_t x=0; x < w; x++){
      uint8_t b = pgm_read_byte(q++);
      *p++ = inv ? ~b : b;
    }
  }
}

uint8_t lcd_lastPos;

void lcd_putcAtt(uint8_t x, uint8_t y, const char c, uint8_t mode)
{
  uint8_t *p    = &displayBuf[ y / 8 * DISPLAY_W + x ];

  prog_uchar    *q = &font_5x8_x20_x7f[ + (c-0x20)*5];
  bool         inv = (mode & INVERS) ? true : (mode & BLINK ? BLINK_ON_PHASE : false);
  if(mode & DBLSIZE)
  {
    /* each letter consists of ten top bytes followed by
     * by ten bottom bytes (20 bytes per * char) */
    q = &font_10x16_x20_x7f[(c-0x20)*10 + ((c-0x20)/16)*160];
    for(char i=5; i>=0; i--) {
      /*top byte*/
      uint8_t b1 = i>0 ? pgm_read_byte(q) : 0;
      /*bottom byte*/
      uint8_t b3 = i>0 ? pgm_read_byte(160+q) : 0;
      /*top byte*/
      uint8_t b2 = i>0 ? pgm_read_byte(++q) : 0;
      /*bottom byte*/
      uint8_t b4 = i>0 ? pgm_read_byte(160+q) : 0;

      if(inv) {
        b1=~b1;
        b2=~b2;
        b3=~b3;
        b4=~b4;
      }   

      if(&p[DISPLAY_W+1] < DISPLAY_END){
        p[0]=b1;
        p[1]=b2;
        p[DISPLAY_W] = b3; 
        p[DISPLAY_W+1] = b4; 
        p+=2;
      }   
      q++;
    }   

    lcd_lastPos = x + 2*FW;
  }
  else {
    uint8_t condense=0;

    if (mode & CONDENSED) {
        *p++ = inv ? ~0 : 0;
        condense=1;
    }

    for (char i=5; i!=0; i--) {
        uint8_t b = pgm_read_byte(q++);
        if (condense && i==4) {
            /*condense the letter by skipping column 4 */
            continue;
        }
        if(p<DISPLAY_END) *p++ = inv ? ~b : b;
    }
    if(p<DISPLAY_END) *p++ = inv ? ~0 : 0;

    lcd_lastPos = x + FW;
  }
}

void lcd_putc(uint8_t x,uint8_t y,const char c)
{
  lcd_putcAtt(x,y,c,0);
}

void lcd_putsnAtt(uint8_t x,uint8_t y,const prog_char * s,uint8_t len,uint8_t mode)
{
  while(len!=0) {
    char c;
    switch (mode & (BSS+ZCHAR)) {
      case BSS:
        c = *s;
        break;
      case ZCHAR:
        c = idx2char(*s);
        break;
      default:
        c = pgm_read_byte(s);
        break;
    }
    lcd_putcAtt(x,y,c,mode);
    x+=FW;
    if (mode&DBLSIZE) x+=FW-1;
    s++;
    len--;
  }
}
void lcd_putsn_P(uint8_t x,uint8_t y,const prog_char * s,uint8_t len)
{
  lcd_putsnAtt( x,y,s,len,0);
}

void lcd_putsAtt(uint8_t x,uint8_t y,const prog_char * s,uint8_t mode)
{
  while(1) {
    char c = (mode & BSS) ? *s++ : pgm_read_byte(s++);
    if(!c) break;
    lcd_putcAtt(x,y,c,mode);
    x+=FW;
    if(mode&DBLSIZE) x+=FW;
  }
}

void lcd_puts_P(uint8_t x,uint8_t y,const prog_char * s)
{
  lcd_putsAtt( x, y, s, 0);
}

void lcd_outhex4(uint8_t x,uint8_t y,uint16_t val)
{
  x+=FWNUM*4;
  for(int i=0; i<4; i++)
  {
    x-=FWNUM;
    char c = val & 0xf;
    c = c>9 ? c+'A'-10 : c+'0';
    lcd_putcAtt(x,y,c,c>='A'?CONDENSED:0);
    val>>=4;
  }
}
void lcd_outdez(uint8_t x,uint8_t y, int16_t val)
{
  lcd_outdezAtt(x,y,val,0);
}

void lcd_outdezAtt(uint8_t x,uint8_t y, int16_t val, uint8_t mode)
{
  lcd_outdezNAtt( x, y, val, mode, 5);
}

/*
G: That's it. I've had enough of the growing annoying  outdez function. 
   Rewriting it from scratch

USAGE:
  lcd_outdezNAtt(x-coord, y-coord, (un)signed-value{0..65535|0..+/-32768}, 
                  mode_flags, length{0..63}[+[LEADING0|TRAILING0])

  Available mode_flas: PREC{1..3}, UNSIGN (for programmer selected signed numbers
                  to allow for unsigned values up to the ful 65535 16-bit limt))

  LEADING0 means pad 0 to the left of sig. digits up to 'len' total characters
  TRAILING0 means pad 0 to the right of sig. digits up to 'len' total characters
            (for use with manual decimal parts on right of period (.))
*/
#define PREC(n) (n>>4 & 3)
void lcd_outdezNAtt(uint8_t x, uint8_t y, int16_t val, uint8_t mode, uint8_t len)
{
  char digits[5]; // sig. digits buffered in reverse order
  uint8_t lastDigit;
  uint8_t prec = PREC(mode);
  uint8_t maxLen = len & 0x3f;

  bool neg = false;
  if (~mode & UNSIGN && val < 0) { neg=true; val=-val; }

  // Buffer characters and determine the significant digit count
  for (lastDigit = 0; lastDigit < maxLen; lastDigit++)
  {
    digits[lastDigit] = ((uint16_t)val % 10) + '0';
    val = (uint16_t)val / 10;
    if (!val) break;
  }

  lcd_lastPos = x;
  if (~mode & LEFT) // determine correct x-coord starting point for decimal aligned
  {
    // Starting point for regular unsigned, non-decimal number
    if (mode & DBLSIZE) 
      lcd_lastPos -= 2*FW*(lastDigit+1) + ((len & (LEADING0|TRAILING0)) ? 2*(FW-2)*(maxLen - lastDigit - 1) : 0);
    else
      lcd_lastPos -= (FW-1)*(lastDigit+1) + ((len & (LEADING0|TRAILING0)) ? (FW-1)*(maxLen - lastDigit - 1) : 0);

    if (prec>0) lcd_lastPos += (mode & DBLSIZE) ? (2*prec*(FW-2)) + 4 : (prec*(FW-1)) + 2;

    if (neg) lcd_lastPos -= (mode & DBLSIZE) ? 2*FW : FW;
  }

  if (neg) lcd_putcAtt(lcd_lastPos++, y, '-', mode); // apply sign when required

  if (prec && !lastDigit) 
    for(uint8_t i = 0; i < prec; i++)
      digits[++lastDigit]='0';

  if (len & LEADING0)
    for (int8_t i=lastDigit+1; i < maxLen; i++)
      lcd_putcAtt(lcd_lastPos-1, y, '0', mode);

  for (int8_t i=lastDigit; i >= 0; i--)
  {
    lcd_putcAtt(lcd_lastPos-1, y, digits[i], mode);

    // Draw decimal point
    // Use direct screen writes to save flash, function calls, stack, cpu load
    bool inv = (mode & INVERS) ? true : (mode & BLINK ? BLINK_ON_PHASE : false);
    uint8_t *p = &displayBuf[ y / 8 * DISPLAY_W + lcd_lastPos ];
    uint8_t bits;
    if (prec && i==prec) // .. then draw a d'point
    { 
      if (mode & DBLSIZE)
      {
        bits = 0; if (inv) bits ^= 0xff;
        p[0] = bits; p[1] = bits;
        bits = 0x60; if (inv) bits ^= 0xff;
        p[DISPLAY_W] = bits;
        p[DISPLAY_W+1] = bits;
        lcd_lastPos+=3;
      }
      else
      {
        bits = 0x40; if (inv) bits ^= 0xff;
        p[0] = bits;
        lcd_lastPos += 2;
      }
    }
  }
  if (len & TRAILING0)
    for (int8_t i=lastDigit+1; i < maxLen; i++)
      lcd_putcAtt(lcd_lastPos-1, y, '0', mode);
}

void lcd_mask(uint8_t *p, uint8_t mask, uint8_t att)
{
  assert(p < DISPLAY_END);

  if (att & LCD_BLACK)
    *p |= mask;
  else if (att & LCD_WHITE)
    *p &= ~mask;
  else
    *p ^= mask;
}

void lcd_plot(uint8_t x,uint8_t y, uint8_t att)
{
  uint8_t *p   = &displayBuf[ y / 8 * DISPLAY_W + x ];
  if (p<DISPLAY_END)
    lcd_mask(p, BITMASK(y%8), att);
}

void lcd_hlineStip(int8_t x, uint8_t y, int8_t  w, uint8_t pat, uint8_t att)
{
  if (y >= DISPLAY_H) return;
  if (w<0) { x+=w; w=-w; }
  if (x<0) { w+=x; x=0; }
  if (x+w > DISPLAY_W) { w = DISPLAY_W - x; }

  uint8_t *p  = &displayBuf[ y / 8 * DISPLAY_W + x ];
  uint8_t msk = BITMASK(y%8);
  while(w) {
    if(pat&1) {
      lcd_mask(p, msk, att);
      pat = (pat >> 1) | 0x80;
    }
    else {
      pat = pat >> 1;
    }
    w--;
    p++;
  }
}

void lcd_hline(uint8_t x,uint8_t y, int8_t w, uint8_t att)
{
  lcd_hlineStip(x, y, w, 0xff, att);
}

void lcd_vlineStip(uint8_t x, int8_t y, int8_t h, uint8_t pat)
{
  if (y<0) { h+=y; y=0; }
  if (y+h > DISPLAY_H) { h = DISPLAY_H - y; }

  uint8_t *p  = &displayBuf[ y / 8 * DISPLAY_W + x ];
  y = y % 8;
  if (y) {
    assert(p < DISPLAY_END);
    *p ^= ~(BITMASK(y)-1) & pat;
    p += DISPLAY_W;
    h -= 8-y;
  }
  while (h>0) {
    assert(p < DISPLAY_END);
    *p ^= pat;
    p += DISPLAY_W;
    h -= 8;
  }
  h = (h+8) % 8;
  if (h) {
    p -= DISPLAY_W;
    assert(p < DISPLAY_END);
    *p ^= ~(BITMASK(h)-1) & pat;
  }
}

void lcd_vline(uint8_t x, int8_t y, int8_t h)
{
  lcd_vlineStip(x, y, h, 0xff);
}

void lcd_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t pat)
{
  lcd_vlineStip(x, y, h, pat);
  lcd_hlineStip(x, y+h-1, w, pat);
  lcd_vlineStip(x+w-1, y, h, pat);
  lcd_hlineStip(x, y, w, pat);
}

void lcd_filled_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t att)
{
  for (uint8_t i=y+h-1; i>=y; i--)
    lcd_hline(x, i, w, att);
}

void putsTime(uint8_t x,uint8_t y,int16_t tme,uint8_t att,uint8_t att2)
{
  if (tme<0) {
    lcd_putcAtt(x - ((att&DBLSIZE) ? FWNUM*6-3 : FWNUM*3), y, '-', att);
    tme = -tme;
  }

  lcd_putcAtt(x, y, ':', att&att2);
  lcd_outdezNAtt((att&DBLSIZE) ? x + 2 : x, y, tme/60, att, 2|LEADING0);
  x += (att&DBLSIZE) ? FWNUM*6-2 : FW*3-1;
  lcd_outdezNAtt(x, y, tme%60, att2, 2|LEADING0);
}

void putsVolts(uint8_t x, uint8_t y, uint16_t volts, uint8_t att, bool noUnit)
{
  lcd_outdezAtt(x, y, volts, att|PREC1);
  if(!noUnit) lcd_putcAtt(lcd_lastPos, y, 'v', att);
}

void putsVBat(uint8_t x,uint8_t y,uint8_t att, bool noUnit)
{
  putsVolts(x, y, g_vbat100mV, att, noUnit);
}

void putsChnRaw(uint8_t x, uint8_t y, uint8_t idx, uint8_t att)
{
  if (idx==0)
    lcd_putsnAtt(x,y,PSTR("----"),4,att);
  else if(idx<=4)
    lcd_putsnAtt(x,y,modi12x3+g_eeGeneral.stickMode*16+4*(idx-1),4,att);
  else if(idx<=NUM_XCHNRAW)
    lcd_putsnAtt(x,y,PSTR("P1  P2  P3  MAX FULLCYC1CYC2CYC3PPM1PPM2PPM3PPM4PPM5PPM6PPM7PPM8CH1 CH2 CH3 CH4 CH5 CH6 CH7 CH8 CH9 CH10CH11CH12CH13CH14CH15CH16"TELEMETRY_CHANNELS)+4*(idx-5),4,att);
}

void putsChn(uint8_t x, uint8_t y, uint8_t idx, uint8_t att)
{
  if (idx > 0 && idx <= NUM_CHNOUT)
    putsChnRaw(x, y, idx+20, att);
}

void putsChnLetter(uint8_t x, uint8_t y, uint8_t idx, uint8_t attr)
{
  lcd_putsnAtt(x, y, PSTR("RETA")+CHANNEL_ORDER(idx)-1, 1, attr);
}

void putsSwitches(uint8_t x,uint8_t y,int8_t idx,uint8_t att)
{
  switch(idx){
    case  0:          lcd_putsAtt(x,y,PSTR("---"),att);return;
    case  MAX_SWITCH: lcd_putsAtt(x,y,PSTR("ON "),att);return;
    case -MAX_SWITCH: lcd_putsAtt(x,y,PSTR("OFF"),att);return;
  }
  if (idx<0) lcd_putcAtt(x-FW, y, '!', att);
  lcd_putsnAtt(x,y,get_switches_string()+3*(abs(idx)-1),3,att);
}

void putsFlightPhase(uint8_t x, uint8_t y, int8_t idx, uint8_t att)
{
  if (idx==0) { lcd_putsAtt(x,y,PSTR("---"),att); return; }
  if (idx < 0) { lcd_putcAtt(x-FW, y, '!', att); idx = -idx; }
  lcd_putsAtt(x, y, PSTR("FP"), att);
  lcd_putcAtt(x+2*FW, y, '0'+idx-1, att);
}

void putsTmrMode(uint8_t x, uint8_t y, uint8_t attr)
{
  int8_t tm = g_model.tmrMode;
  if(abs(tm)<TMR_VAROFS) {
    lcd_putsnAtt(  x, y, PSTR("OFFABSRUsRU%ELsEL%THsTH%ALsAL%P1 P1%P2 P2%P3 P3%")+3*abs(tm),3,attr);
    if(tm<(-TMRMODE_ABS)) lcd_putcAtt(x-1*FW,  y,'!',attr);
    return;
  }

  if(abs(tm)<(TMR_VAROFS+MAX_SWITCH-1)) { //normal on-off
    putsSwitches( x,y,tm>0 ? tm-(TMR_VAROFS-1) : tm+(TMR_VAROFS-1),attr);
    return;
  }

  putsSwitches( x,y,tm>0 ? tm-(TMR_VAROFS+MAX_SWITCH-1-1) : tm+(TMR_VAROFS+MAX_SWITCH-1-1),attr);//momentary on-off
  lcd_putcAtt(x+3*FW,  y,'m',attr);
}

#ifdef FRSKY
void putsTelemetry(uint8_t x, uint8_t y, uint8_t val, uint8_t unit, uint8_t att, bool noUnit)
{
  if (unit == 0/*v*/) {
    putsVolts(x, y, val, att, noUnit);
  }
  else {
    lcd_outdezAtt(x, y, val, att);
  }
}
#endif

void lcdSendCtl(uint8_t val)
{
  PORTC_LCD_CTRL &= ~(1<<OUT_C_LCD_CS1);
#ifdef LCD_MULTIPLEX
  DDRA = 0xFF; // set LCD_DAT pins to output
#endif
  PORTC_LCD_CTRL &= ~(1<<OUT_C_LCD_A0);
  PORTC_LCD_CTRL &= ~(1<<OUT_C_LCD_RnW);
  PORTA_LCD_DAT = val;
  PORTC_LCD_CTRL |=  (1<<OUT_C_LCD_E);
  PORTC_LCD_CTRL &= ~(1<<OUT_C_LCD_E);
  PORTC_LCD_CTRL |=  (1<<OUT_C_LCD_A0);
#ifdef LCD_MULTIPLEX
  DDRA = 0x00; // set LCD_DAT pins to input
#endif
  PORTC_LCD_CTRL |=  (1<<OUT_C_LCD_CS1);
}


#define delay_1us() _delay_us(1)
void delay_1_5us(int ms)
{
  for(int i=0; i<ms; i++) delay_1us();
}


void lcd_init()
{
  // /home/thus/txt/datasheets/lcd/KS0713.pdf
  // ~/txt/flieger/ST7565RV17.pdf  from http://www.glyn.de/content.asp?wdid=132&sid=

  PORTC_LCD_CTRL &= ~(1<<OUT_C_LCD_RES);  //LCD_RES
  delay_1us();
  delay_1us();//    f520  call  0xf4ce  delay_1us() ; 0x0xf4ce
  PORTC_LCD_CTRL |= (1<<OUT_C_LCD_RES); //  f524  sbi 0x15, 2 IOADR-PORTC_LCD_CTRL; 21           1
  delay_1_5us(1500);
  lcdSendCtl(0xe2); //Initialize the internal functions
  lcdSendCtl(0xae); //DON = 0: display OFF
  lcdSendCtl(0xa1); //ADC = 1: reverse direction(SEG132->SEG1)
  lcdSendCtl(0xA6); //REV = 0: non-reverse display
  lcdSendCtl(0xA4); //EON = 0: normal display. non-entire
  lcdSendCtl(0xA2); // Select LCD bias=0
  lcdSendCtl(0xC0); //SHL = 0: normal direction (COM1->COM64)
  lcdSendCtl(0x2F); //Control power circuit operation VC=VR=VF=1
  lcdSendCtl(0x25); //Select int resistance ratio R2 R1 R0 =5
  lcdSendCtl(0x81); //Set reference voltage Mode
  lcdSendCtl(0x22); // 24 SV5 SV4 SV3 SV2 SV1 SV0 = 0x18
  lcdSendCtl(0xAF); //DON = 1: display ON
  g_eeGeneral.contrast = 0x22;
}

void lcdSetRefVolt(uint8_t val)
{
  lcdSendCtl(0x81);
  lcdSendCtl(val);
}

void refreshDiplay()
{
  uint8_t *p=displayBuf;
  for(uint8_t y=0; y < 8; y++) {
    lcdSendCtl(0x04);
    lcdSendCtl(0x10); //column addr 0
    lcdSendCtl( y | 0xB0); //page addr y
    PORTC_LCD_CTRL &= ~(1<<OUT_C_LCD_CS1);
#ifdef LCD_MULTIPLEX
    DDRA = 0xFF; // set LCD_DAT pins to output
#endif
    PORTC_LCD_CTRL |=  (1<<OUT_C_LCD_A0);
    PORTC_LCD_CTRL &= ~(1<<OUT_C_LCD_RnW);
    for(uint8_t x=128; x>0; --x) {
      PORTA_LCD_DAT = *p++;
      PORTC_LCD_CTRL |= (1<<OUT_C_LCD_E);
      PORTC_LCD_CTRL &= ~(1<<OUT_C_LCD_E);
    }
    PORTC_LCD_CTRL |=  (1<<OUT_C_LCD_A0);
    PORTC_LCD_CTRL |=  (1<<OUT_C_LCD_CS1);
  }
}
