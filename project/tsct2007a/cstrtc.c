/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL RTC software functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "ite/audio.h"
#include "audio_mgr.h"
#include "ctrlboard.h"
#include "SDL/SDL.h"

#define COSTEL_RTC_ISL1208

#define LOCAL_TIMER_DUTY	(1000)
#define LOCAL_NET_TIMER_DUTY	(500)

#ifdef COSTEL_RTC_ISL1208
#define IIC_RTC_ID          (0xDE>>1)

#define ISL1208_REG_SC  0x00
#define ISL1208_REG_MN  0x01
#define ISL1208_REG_HR  0x02
#define ISL1208_REG_HR_MIL     0x80   /* 24h/12h mode */
#define ISL1208_REG_HR_PM      (1<<5)   /* PM/AM bit in 12h mode */
#define ISL1208_REG_DT  0x03
#define ISL1208_REG_MO  0x04
#define ISL1208_REG_YR  0x05
#define ISL1208_REG_DW  0x06
#define ISL1208_RTC_SECTION_LEN 7

 /* control/status section */
#define ISL1208_REG_SR  0x07
#define ISL1208_REG_SR_ARST    (1<<7)   /* auto reset */
#define ISL1208_REG_SR_XTOSCB  (1<<6)   /* crystal oscillator */
#define ISL1208_REG_SR_WRTC    (1<<4)   /* write rtc */
#define ISL1208_REG_SR_ALM     (1<<2)   /* alarm */
#define ISL1208_REG_SR_BAT     (1<<1)   /* battery */
#define ISL1208_REG_SR_RTCF    (1<<0)   /* rtc fail */
#define ISL1208_REG_INT 0x08
#define ISL1208_REG_09  0x09    /* reserved */
#define ISL1208_REG_ATR 0x0a
#define ISL1208_REG_DTR 0x0b

/* alarm section */
#define ISL1208_REG_SCA 0x0c
#define ISL1208_REG_MNA 0x0d
#define ISL1208_REG_HRA 0x0e
#define ISL1208_REG_DTA 0x0f
#define ISL1208_REG_MOA 0x10
#define ISL1208_REG_DWA 0x11
#define ISL1208_ALARM_SECTION_LEN 6

/* user section */
#define ISL1208_REG_USR1 0x12
#define ISL1208_REG_USR2 0x13
#define ISL1208_USR_SECTION_LEN 2

static int gRtcFd=-1;
static SDL_TimerID sTimerLocal;

static uint8_t _BCD2DEC(uint8_t byte)
{
	uint8_t B1 = (byte>>4);
	uint8_t B2 = (byte&0xF);
	
	if( (B1>9) || (B2>9) )
	{
		printf("[RTC] _BCD2DEC something wrong, BCD=%x, B1=%x, B2=%x\n",byte,B1,B2);
		B1 = B1 %9;
		B2 = B2 %9;
		return 0x00;
	}
	
	return (B1*10) + B2;
}

static uint8_t _DEC2BCD(uint8_t dec)
{
	uint8_t B1 = dec/10;
	uint8_t B2 = dec%10;
		
	if( dec>=100 )
	{
		printf("[RTC] _DEC2BCD something wrong, dec=%x, B1=%x, B2=%x\n",dec,B1,B2);
		return 0x00;
	}
	
	return (B1<<4) | B2;
}

static bool readRtcReg1(uint8_t cmd, uint8_t *data)
{
	ITPI2cInfo 	evt;
	int ret;

	evt.slaveAddress   = IIC_RTC_ID;
	evt.cmdBuffer      = &cmd;
	evt.cmdBufferSize  = 1;
	evt.dataBuffer     = data;
	evt.dataBufferSize = 1;	
	
	ret = read(gRtcFd, &evt, 1);
	if (ret < 0)
	{
		printf("[RTC] iic read fail, %d\n", ret);
		return false;		
	}
	
	return true;
}

static bool writeRtcReg1(uint8_t reg, uint8_t data)
{
	ITPI2cInfo 	evt;
	uint8_t regdata[2];
	int ret;

	regdata[0] = reg;
	regdata[1] = data;
	
	evt.slaveAddress   = IIC_RTC_ID;
	evt.cmdBuffer      = regdata;
	evt.cmdBufferSize  = 2;
	evt.dataBuffer     = 0;
	evt.dataBufferSize = 0;

	ret = write(gRtcFd, &evt, 1);
	if (ret < 0)
	{
		printf("[RTC] iic write fail, %d\n", ret);
		return false;
	}
	
	return true;
}

long _extRtcGetTime1(void)
{	
	long rtcSec = 0;

#ifndef CST_NO_RTC
	uint8_t data;
	struct tm l_tInfo;
	
	memset(&l_tInfo, 0, sizeof(struct tm));
	
	if (!readRtcReg1(ISL1208_REG_YR, &data)) 		return 0;	
	l_tInfo.tm_year = _BCD2DEC(data) + 100;
	
	if (!readRtcReg1(ISL1208_REG_MO, &data)) 		return 0;	
	l_tInfo.tm_mon = _BCD2DEC(data);
	
	if (!readRtcReg1(ISL1208_REG_DT, &data))		return 0;	
	l_tInfo.tm_mday = _BCD2DEC(data);

	if (!readRtcReg1(ISL1208_REG_DW, &data))		return 0;
	l_tInfo.tm_wday = data - 1;

	if (!readRtcReg1(ISL1208_REG_HR, &data))		return 0;	
	data &= ~ISL1208_REG_HR_MIL; // 20190114... 24 hour display ...
//	data &= ~ISL1208_REG_HR_PM;  // 20190114... 24 hour display ...
	l_tInfo.tm_hour = _BCD2DEC(data);

	if (!readRtcReg1(ISL1208_REG_MN, &data))		return 0;	
	l_tInfo.tm_min = _BCD2DEC(data);
	
	if (!readRtcReg1(ISL1208_REG_SC, &data))		return 0;	
	l_tInfo.tm_sec = _BCD2DEC(data);

	rtcSec = mktime((struct tm*)&l_tInfo);

	printf("[[[%04d-%02d-%02d:%02d.%02d.%02d]]] \n", l_tInfo.tm_year+1900, l_tInfo.tm_mon+1, l_tInfo.tm_mday, l_tInfo.tm_hour, l_tInfo.tm_min, l_tInfo.tm_sec);
#endif		
    return rtcSec;
}

#endif

void itpRtcInit1(void)
{
#ifndef CST_NO_RTC
    #ifdef COSTEL_RTC_ISL1208
	uint8_t data = 0x00;

    gRtcFd = open(":i2c0", O_RDWR);    
    if (gRtcFd==-1)	
		printf("[RTC] error: open fd fail(fd=%d)!n", gRtcFd);

	if (!readRtcReg1(ISL1208_REG_SR, &data))
		printf("[RTC] Can't read RTC SR register\n");

	if (data & ISL1208_REG_SR_RTCF)
	{
		printf("[RTC] Initialize RTC value with 1514764800 (2018-01-01 00:00:00)\n");
	    itpRtcSetTime(CFG_RTC_DEFAULT_TIMESTAMP, 0);
	}

	writeRtcReg1(ISL1208_REG_SR, 0x00);
	writeRtcReg1(ISL1208_REG_INT, 0x10);
 	writeRtcReg1(ISL1208_REG_ATR, 0x00);
 	writeRtcReg1(ISL1208_REG_DTR, 0x00);
#endif

	_extRtcGetTime1(); // when booting... get real time from RTC for sync display time ...20190114
#endif
}

void itpRtcSetTime1(long sec, long usec)
{
   ithEnterCritical();

#ifdef COSTEL_RTC_ISL1208
	struct timeval setTV;
	time_t rtcSec = 0;
	struct timeval systime;

	setTV.tv_sec = sec;

	struct	tm *t = gmtime(&setTV);

#ifndef CST_NO_RTC
	if (!writeRtcReg1(ISL1208_REG_SR, ISL1208_REG_SR_WRTC)) return;
	
	if (t->tm_year > 99)
	{
		if (!writeRtcReg1(ISL1208_REG_YR, _DEC2BCD(t->tm_year % 100))) return;
	}
	else
	{
		if (!writeRtcReg1(ISL1208_REG_YR, _DEC2BCD(t->tm_year))) return;
	}
	if (!writeRtcReg1(ISL1208_REG_MO, _DEC2BCD(t->tm_mon))) return;
	if (!writeRtcReg1(ISL1208_REG_DT, _DEC2BCD(t->tm_mday))) return;
	if (!writeRtcReg1(ISL1208_REG_DW,  t->tm_wday%7 + 1)) return;

//	if (!writeRtcReg1(ISL1208_REG_HR, (_DEC2BCD(t->tm_hour) ))) return;   // max 12 hour display at 1 day
	if (!writeRtcReg1(ISL1208_REG_HR, (_DEC2BCD(t->tm_hour) | ISL1208_REG_HR_MIL ))) return;  // max 24 hour diplay at 1day
	if (!writeRtcReg1(ISL1208_REG_MN, _DEC2BCD(t->tm_min))) return;
	if (!writeRtcReg1(ISL1208_REG_SC, _DEC2BCD(t->tm_sec))) return;

	if (!writeRtcReg1(ISL1208_REG_SR, 0))	return;
#endif

	rtcSec = mktime(t);

	// system time setting.    
	systime.tv_sec = rtcSec;
	systime.tv_usec = 0;

	if (settimeofday(&systime, NULL) < 0 )
		 printf("Failed in call to set the system time\n"); 
#endif

	 printf(" >> %04d-%02d-%02d:%02d.%02d.%02d \n", t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

	 ithExitCritical();
}
