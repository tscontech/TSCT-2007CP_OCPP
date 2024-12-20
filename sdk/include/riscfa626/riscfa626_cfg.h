/*
* @brief riscfa626 LIB Driver
*
* @note
* Copyright(C) CHIPSBRAIN CO., Ltd., 1999 ~ 2016
 * All rights reserved.
*
* File Name 	: riscfa626_cfg.h
*
* Version	: V1.17
* Date 		: 2015.08.07
* Description : riscfa626 LIB Header
*/
#ifndef __riscfa626_CONFIG_LIB_H_
#define __riscfa626_CONFIG_LIB_H_

//#define SUPPORT_BASIC_LIB
//#define SUPPORT_BASIC_PLUS_LIB
//#define SUPPORT_STANDARD_LIB
 #define SUPPORT_STANDARD_PLUS_LIB
// #define SUPPORT_PREMIERE_LIB
//#define SUPPORT_PREMIERE_PLUS_LIB

#ifdef SUPPORT_PREMIERE_PLUS_LIB
	#define SUPPORT_SW_AES
	#define SUPPORT_EEPROM
	#define SUPPORT_DOUBLE_PROTECTION
	#define SUPPORT_ENC_FIXED_MODE
	#define SUPPORT_VERSION
	//#define SUPPORT_EEPROM_PASSWORD
#endif

#ifdef SUPPORT_PREMIERE_LIB
	#define SUPPORT_SW_AES
	#define SUPPORT_EEPROM
	#define SUPPORT_DOUBLE_PROTECTION
	#define SUPPORT_ENC_FIXED_MODE
	#define SUPPORT_VERSION
	//#define SUPPORT_EEPROM_PASSWORD
#endif

#ifdef SUPPORT_STANDARD_PLUS_LIB
	#define SUPPORT_SW_AES
	//#define SUPPORT_HW_AES
	#define SUPPORT_EEPROM
	#define SUPPORT_ENC_FIXED_MODE
	#define SUPPORT_VERSION
	#define SUPPORT_EEPROM_PASSWORD
#endif

#ifdef SUPPORT_STANDARD_LIB
	#define SUPPORT_SW_AES
	//#define SUPPORT_HW_AES
	//#define SUPPORT_EEPROM
	#define SUPPORT_ENC_FIXED_MODE
	#define SUPPORT_VERSION
	//#define SUPPORT_EEPROM_PASSWORD
#endif

#ifdef SUPPORT_BASIC_PLUS_LIB
	//#define SUPPORT_SW_AES
	#define SUPPORT_HW_AES
	#define SUPPORT_EEPROM
	//#define SUPPORT_ENC_FIXED_MODE
	//#define SUPPORT_VERSION
	#define SUPPORT_EEPROM_PASSWORD
#endif

#ifdef SUPPORT_BASIC_LIB
	//#define SUPPORT_SW_AES
	#define SUPPORT_HW_AES
	//#define SUPPORT_EEPROM
	//#define SUPPORT_ENC_FIXED_MODE
	//#define SUPPORT_VERSION
	//#define SUPPORT_EEPROM_PASSWORD
#endif

#ifdef SUPPORT_HW_AES
	#define _HW_AES_COUNT (10)
#endif


#define SUPPORT_BYPASSMODE
//#define	SUPPORT_INKJET_COUNT
// #define SUPPORT_POWEROFF_NODELAY
//#define SUPPORT_POWER_TIM_DELAY

//*****************************************************************
// COMMON DEFINE
//*****************************************************************
#define SUPPORT_STANDARD_C_LIB

#if defined(SUPPORT_POWEROFF_NODELAY) || defined(SUPPORT_POWER_TIM_DELAY)
#define SUPPORT_POWEROFF
#endif

#if defined(SUPPORT_POWEROFF) || defined(SUPPORT_SU_MODE)
#define SUPPORT_STREAMCIPHER_CTRL
#endif

#ifdef SUPPORT_STANDARD_C_LIB

#if defined(SUPPORT_PREMIERE_PLUS_LIB) || defined(SUPPORT_PREMIERE_LIB)
	#define SUPPORT_CLIB_MEMCMPV
#endif
#if defined(SUPPORT_STANDARD_LIB) || defined(SUPPORT_STANDARD_PLUS_LIB)
	//#define SUPPORT_CLIB_MEMCMPV
#endif
#if defined(SUPPORT_BASIC_PLUS_LIB) || defined(SUPPORT_BASIC_LIB)
	//#define SUPPORT_CLIB_MEMCMPV
#endif

#ifdef SUPPORT_VERSION
	#define SUPPORT_CLIB_STRLEN
#endif
	#define SUPPORT_CLIB_MEMCPY
	#define SUPPORT_CLIB_MEMCMP
	#define SUPPORT_CLIB_MEMSET
#endif
#endif
