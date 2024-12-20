/***************************************************************************
 *
 *            Copyright (c) 2011-2020 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci ut 76
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#ifndef CONFIG_OAL_OS_H
#define CONFIG_OAL_OS_H

/* OAL user definition file for FreeRTOS */
#include "../version/ver_oal_os.h"
#if VER_OAL_FREERTOS_MAJOR != 2 || VER_OAL_FREERTOS_MINOR != 6
 #error Incompatible OAL_FREERTOS version number!
#endif
#include "../version/ver_oal.h"
#if VER_OAL_MAJOR != 2 || VER_OAL_MINOR < 2
 #error Incompatible OAL version number!
#endif

/* priorities */
#define OAL_HIGHEST_PRIORITY 5
#define OAL_HIGH_PRIORITY    4
#define OAL_NORMAL_PRIORITY  3
#define OAL_LOW_PRIORITY     2
#define OAL_LOWEST_PRIORITY  1

/* Event flag to use for user tasks invoking internal functions. */
/* E.g.: One task calls an internal function that needs to wait for an event */
/* NOTE: The value of this flag should be > 0x80 because lower bits */
/* might be used by internal tasks */
#define OAL_EVENT_FLAG 0x100u

/* no. of max. interrupt sources */
#if (CFG_CHIP_FAMILY == 970) // Irene Lin
#define OAL_ISR_COUNT  3
#else
#define OAL_ISR_COUNT  2
#endif


/* Controls if architecture uses FPU. */
/* If it is  set to 1 every task has added a portTASK_USES_FLOATING_POINT() */
/* at start to prevent errors when compiler decides to use FPU registers */
/* in optimization process. */
#define OAL_USE_FPU 0

#include <FreeRTOS.h>
#if INCLUDE_vTaskSuspend == 0
 #error OAL - INCLUDE_vTaskSuspend must be set!
#endif
#if INCLUDE_xTaskGetCurrentTaskHandle == 0
 #error OAL - INCLUDE_xTaskGetCurrentTaskHandle must be set!
#endif
#if INCLUDE_vTaskDelay == 0
 #error OAL - INCLUDE_vTaskDelay must be set!
#endif
#if INCLUDE_vTaskDelete == 0
 #error OAL - INCLUDE_vTaskDelete must be set!
#endif
#if configUSE_MUTEXES == 0
 #error OAL - configUSE_MUTEXES must be set!
#endif

#endif /* ifndef CONFIG_OAL_OS_H */
