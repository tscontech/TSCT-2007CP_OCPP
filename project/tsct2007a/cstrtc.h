/**
*       @file
*               cstrtc.h
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2018.01.12 <br>
*               author: bmlee <br>
*               description: <br>
*/

#ifndef __CSTRTC_H__
#define __CSTRTC_H__

#define COSTEL_RTC_ISL1208

#ifdef COSTEL_RTC_ISL1208

#define IIC_RTC_ID          (0xDE>>1)

#define ISL1208_REG_SC  0x00
#define ISL1208_REG_MN  0x01
#define ISL1208_REG_HR  0x02
#define ISL1208_REG_HR_MIL     (1<<7)   /* 24h/12h mode */
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
#define ISL1208_REG_MNA 0x0ditpRtcInit
#define ISL1208_REG_HRA 0x0e
#define ISL1208_REG_DTA 0x0f
#define ISL1208_REG_MOA 0x10
#define ISL1208_REG_DWA 0x11
#define ISL1208_ALARM_SECTION_LEN 6

/* user section */
#define ISL1208_REG_USR1 0x12
#define ISL1208_REG_USR2 0x13
#define ISL1208_USR_SECTION_LEN 2
#endif

void itpRtcSetTime(long sec, long usec);

#endif
