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

#ifdef TRANSLATIONS
#include "eeprom_v4.h"
#include "eeprom_v3.h"
#endif

#include "gruvin9x.h"
#include "templates.h"

EFile theFile;  //used for any file operation
EFile theFile2; //sometimes we need two files

#define FILE_TYP_GENERAL 1
#define FILE_TYP_MODEL   2

#define partCopy(sizeDst,sizeSrc)                         \
      pSrc -= (sizeSrc);                                  \
      pDst -= (sizeDst);                                  \
      memmove(pDst, pSrc, (sizeSrc));                     \
      memset (pDst+(sizeSrc), 0,  (sizeDst)-(sizeSrc));
#define fullCopy(size) partCopy(size,size)
void generalDefault()
{
  memset(&g_eeGeneral,0,sizeof(g_eeGeneral));
  g_eeGeneral.myVers   =  EEPROM_VER;
  g_eeGeneral.currModel=  0;
  g_eeGeneral.contrast = 25;
  g_eeGeneral.vBatWarn = 90;
#ifdef DEFAULTMODE1
  g_eeGeneral.stickMode=  0; // default to mode 1
#else
  g_eeGeneral.stickMode=  2; // default to mode 2
#endif
  for (int i = 0; i < 7; ++i) {
    g_eeGeneral.calibMid[i]     = 0x200;
    g_eeGeneral.calibSpanNeg[i] = 0x180;
    g_eeGeneral.calibSpanPos[i] = 0x180;
  }
  int16_t sum=0;
  for(int i=0; i<12;i++) sum+=g_eeGeneral.calibMid[i];
  g_eeGeneral.chkSum = sum;
}

#ifdef TRANSLATIONS
uint8_t Translate(EEGeneral *p)
{
  if (p->myVers == EEPROM_VER_r584 || p->myVers == EEPROM_ER9X_VER) {
    alert(p->myVers == EEPROM_VER_r584 ? PSTR("EEprom Data v3") : PSTR("EEprom Data Er9x v4"), true);
    message(PSTR("EEPROM Converting"));
    memset(&p->frskyRssiAlarms, 0 , sizeof(p->frskyRssiAlarms));
    if (p->myVers == EEPROM_ER9X_VER) {
      if (p->lightSw == MAX_DRSWITCH-6)
        p->lightSw += 6;
      if (p->lightSw == -MAX_DRSWITCH+6)
        p->lightSw -= 6;
    }
    EEPROM_V3::EEGeneral *old = (EEPROM_V3::EEGeneral *)p;
    p->disableMemoryWarning = old->disableMemoryWarning;
    p->switchWarning = old->disableSwitchWarning ? 0 : -1;
    for (uint8_t id=0; id<MAX_MODELS; id++) {
      theFile.openRd(FILE_MODEL(id));
      uint16_t sz = theFile.readRlc((uint8_t*)&g_model, sizeof(EEPROM_V4::ModelData));
      if(sz > 0) {
        EEPROM_V4::ModelData *v4 = (EEPROM_V4::ModelData *)&g_model;
        EEPROM_V3::ModelData *v3 = (EEPROM_V3::ModelData *)&g_model;
        SwashRingData swashR;
        swashR.invertELE = v4->swashInvertELE;
        swashR.invertAIL = v4->swashInvertAIL;
        swashR.invertCOL = v4->swashInvertCOL;
        swashR.type = v4->swashType;
        swashR.collectiveSource = v4->swashCollectiveSource;
        swashR.value = v4->swashRingValue;
        g_model.tmrMode = v3->tmrMode;
        g_model.tmrDir = v3->tmrDir;
        g_model.tmrVal = v3->tmrVal;
        g_model.protocol = v3->protocol;
        g_model.ppmNCH = v3->ppmNCH;
        g_model.thrTrim = v3->thrTrim;
        g_model.thrExpo = v3->thrExpo;
        g_model.trimInc = v3->trimInc;

        g_model.pulsePol = v3->pulsePol;
        if (p->myVers == EEPROM_ER9X_VER) {
          g_model.extendedLimits = v4->extendedLimits;
          g_model.traineron = v4->traineron;
        }
        else {
          g_model.extendedLimits = 0;
          g_model.traineron = 0;
        }
        g_model.ppmDelay = v3->ppmDelay;
        g_model.trimSw = v3->trimSw;
        g_model.beepANACenter = v3->beepANACenter;
        g_model.tmr2Mode = 0;
        g_model.tmr2Dir = 0;
        g_model.tmr2Val = 0;
        for (uint8_t i=0; i<MAX_MIXERS; i++)
          memmove(&g_model.mixData[i], &v3->mixData[i], sizeof(MixData)); // MixerData size changed!
        memmove(&g_model.limitData[0], &v3->limitData[0], sizeof(LimitData)*NUM_CHNOUT);
        memmove(&g_model.expoData[0], &v3->expoData[0], sizeof(ExpoData)*4);
        memset(&g_model.flightPhaseSw, 0, MAX_PHASES-1);
        memmove(&g_model.trim[0][0], &v3->trim[0], 4);
        memset(&g_model.trim[1][0], 0, 4*(MAX_PHASES-1));
        memmove(&g_model.curves5[0][0], &v3->curves5[0][0], 5*MAX_CURVE5);
        memmove(&g_model.curves9[0][0], &v3->curves9[0][0], 9*MAX_CURVE9);
        if (p->myVers == EEPROM_VER_r584) {
          memmove(&g_model.customSw[0], &v3->customSw[0], sizeof(CustomSwData)*6);
          memset(&g_model.customSw[6], 0, sizeof(CustomSwData)*6);
        }
        else {
          memmove(&g_model.customSw[0], &v4->customSw[0], sizeof(CustomSwData)*12);
        }
        if (p->myVers == EEPROM_VER_r584) {
          memset(&g_model.safetySw[0], 0, sizeof(SafetySwData)*NUM_CHNOUT + sizeof(SwashRingData) + sizeof(FrSkyData));
        }
        else {
          memmove(&g_model.safetySw[0], &v4->safetySw[0], sizeof(SafetySwData)*NUM_CHNOUT);
          memcpy(&g_model.swashR, &swashR, sizeof(SwashRingData));
          for (uint8_t i=0; i<2; i++) {
            g_model.frsky.channels[i].ratio = v4->frsky.channels[i].ratio;
            g_model.frsky.channels[i].type = v4->frsky.channels[i].type;
            g_model.frsky.channels[i].offset = 0;
            g_model.frsky.channels[i].alarms_value[0] = v4->frsky.channels[i].alarms_value[0];
            g_model.frsky.channels[i].alarms_value[1] = v4->frsky.channels[i].alarms_value[1];
            g_model.frsky.channels[i].alarms_level = v4->frsky.channels[i].alarms_level;
            g_model.frsky.channels[i].alarms_greater = v4->frsky.channels[i].alarms_greater;
            g_model.frsky.channels[i].barMin = 0;
            g_model.frsky.channels[i].barMax = 0;
          }
        }
        theFile.writeRlc(FILE_MODEL(id), FILE_TYP_MODEL, (uint8_t*)&g_model, sizeof(g_model), 200);
      }
    }
    p->myVers = EEPROM_VER;
    theFile.writeRlc(FILE_GENERAL, FILE_TYP_GENERAL, (uint8_t*)p, sizeof(EEGeneral), 200);
    return 1;
  }

  return 0;
}
#endif


bool eeLoadGeneral()
{
  theFile.openRd(FILE_GENERAL);
  uint8_t sz = theFile.readRlc((uint8_t*)&g_eeGeneral, sizeof(EEGeneral));
  if((sz == sizeof(EEGeneral) && g_eeGeneral.myVers==EEPROM_VER)
#ifdef TRANSLATIONS
  || Translate(&g_eeGeneral)
#endif
  ) {
    uint16_t sum=0;
    for(int i=0; i<12;i++) sum+=g_eeGeneral.calibMid[i];
    return g_eeGeneral.chkSum == sum;
  }
  return false;
}

void modelDefault(uint8_t id)
{
  memset(&g_model, 0, sizeof(g_model));
  strcpy_P(g_model.name,PSTR("MODEL     "));
  g_model.name[5]='0'+(id+1)/10;
  g_model.name[6]='0'+(id+1)%10;

  //for(uint8_t i= 0; i<4; i++){
  //  g_model.mixData[i].destCh = i+1;
  //  g_model.mixData[i].srcRaw = i+1;
  //  g_model.mixData[i].weight = 100;
  //}
  applyTemplate(0); //default 4 channel template
}
void eeLoadModelName(uint8_t id,char*buf,uint8_t len)
{
  if(id<MAX_MODELS)
  {
    //eeprom_read_block(buf,(void*)modelEeOfs(id),sizeof(g_model.name));
    theFile.openRd(FILE_MODEL(id));
    memset(buf,' ',len);
    if(theFile.readRlc((uint8_t*)buf,sizeof(g_model.name)) == sizeof(g_model.name) )
    {
      uint16_t sz=theFile.size();
      buf+=len;
      while(sz){ --buf; *buf='0'+sz%10; sz/=10;}
    }
  }
}

uint16_t eeFileSize(uint8_t id)
{
    theFile.openRd(FILE_MODEL(id));
    return theFile.size();
}

bool eeModelExists(uint8_t id)
{
    return EFile::exists(FILE_MODEL(id));
}

void eeLoadModel(uint8_t id)
{
  if(id<MAX_MODELS)
  {
    theFile.openRd(FILE_MODEL(id));
    uint16_t sz = theFile.readRlc((uint8_t*)&g_model, sizeof(g_model));

    if(sz != sizeof(ModelData)) {
    	//alert("Error Loading Model");
    	modelDefault(id);
    }

    resetTimer2();

#ifdef FRSKY
    FRSKY_setModelAlarms();
#endif
  }
}

bool eeDuplicateModel(uint8_t id)
{
  uint8_t i;
  for( i=id+1; i<MAX_MODELS; i++)
  {
    if(! EFile::exists(FILE_MODEL(i))) break;
  }
  if(i==MAX_MODELS) return false; //no free space in directory left

  theFile.openRd(FILE_MODEL(id));
  theFile2.create(FILE_MODEL(i),FILE_TYP_MODEL,600);
  uint8_t buf[15];
  uint8_t l;
  while((l=theFile.read(buf,15)))
  {
    theFile2.write(buf,l);
//    if(theFile.errno()==ERR_TMO)
//    {
//        //wait for 10ms and try again
//        uint16_t tgtime = get_tmr10ms() + 100;
//        while (get_tmr10ms()!=tgtime);
//        theFile2.write(buf,l);
//    }
    wdt_reset();
  }
  theFile2.closeTrunc();
  //XXX: todo error handling
  return true;
}
void eeReadAll()
{
  if(!EeFsOpen()  ||
     EeFsck() < 0 ||
     !eeLoadGeneral()
  )
  {
    alert(PSTR("Bad EEprom Data"), true);
    message(PSTR("EEPROM Formatting"));
    EeFsFormat();
    //alert(PSTR("format ok"));
    generalDefault();
    // alert(PSTR("default ok"));

    uint16_t sz = theFile.writeRlc(FILE_GENERAL,FILE_TYP_GENERAL,(uint8_t*)&g_eeGeneral,sizeof(EEGeneral),200);
    if(sz!=sizeof(EEGeneral)) alert(PSTR("genwrite error"));

    modelDefault(0);
    //alert(PSTR("modef ok"));
    theFile.writeRlc(FILE_MODEL(0),FILE_TYP_MODEL,(uint8_t*)&g_model,sizeof(g_model),200);
    //alert(PSTR("modwrite ok"));
  }

  eeLoadModel(g_eeGeneral.currModel);
}


static uint8_t  s_eeDirtyMsk;
static uint16_t s_eeDirtyTime10ms;
void eeDirty(uint8_t msk)
{
  if(!msk) return;
  s_eeDirtyMsk      |= msk;
  s_eeDirtyTime10ms  = get_tmr10ms();
}
#define WRITE_DELAY_10MS 100
void eeCheck(bool immediately)
{
  uint8_t msk  = s_eeDirtyMsk;
  if(!msk) return;
  if( !immediately && ((get_tmr10ms() - s_eeDirtyTime10ms) < WRITE_DELAY_10MS)) return;
  s_eeDirtyMsk = 0;
  if(msk & EE_GENERAL){
    if(theFile.writeRlc(FILE_TMP, FILE_TYP_GENERAL, (uint8_t*)&g_eeGeneral,
                        sizeof(EEGeneral),20) == sizeof(EEGeneral))
    {
      EFile::swap(FILE_GENERAL,FILE_TMP);
    }else{
      if(theFile.errno()==ERR_TMO){
        s_eeDirtyMsk |= EE_GENERAL; //try again
        s_eeDirtyTime10ms = get_tmr10ms() - WRITE_DELAY_10MS;
      }else{
        alert(PSTR("EEPROM overflow"));
      }
    }
    //first finish GENERAL, then MODEL !!avoid Toggle effect
  }
  else if(msk & EE_MODEL){
    if(theFile.writeRlc(FILE_TMP, FILE_TYP_MODEL, (uint8_t*)&g_model,
                        sizeof(g_model),20) == sizeof(g_model))
    {
      EFile::swap(FILE_MODEL(g_eeGeneral.currModel),FILE_TMP);
    }else{
      if(theFile.errno()==ERR_TMO){
        s_eeDirtyMsk |= EE_MODEL; //try again
        s_eeDirtyTime10ms = get_tmr10ms() - WRITE_DELAY_10MS;
      }else{
        alert(PSTR("EEPROM overflow"));
      }
    }
  }
  //beepWarn1();
}
