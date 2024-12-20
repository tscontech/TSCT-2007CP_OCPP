/***************************************************************************
 *
 *            Copyright (c) 2008-2019 by HCC Embedded
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
#ifndef _CONFIG_USBD_H_
#define _CONFIG_USBD_H_

#include "../psp/include/psp_types.h"
#include "config_oal_os.h"

#include "../version/ver_usbd.h"
#if VER_USBD_MAJOR != 3 || VER_USBD_MINOR != 22
 #error "Invalid USBD version."
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Maximum number of class driver to be supported. Some class drivers
   need multiple entries. */
#define MAX_NO_OF_CDRVS          8


/* Number of endpoints to be supported. Set it to match the capabilities
   of the low-level driver. */
#define MAX_NO_OF_EP             9


/* Define this macro with value 1 if your device shall be able to execute
    remote wakeup signaling on the USB bus. */
#define USBD_REMOTE_WAKEUP       0


/* Define this macro with value 1 if your device shall be able to perform
    Test Mode Support on the USB bus.  */
#define USBD_TEST_MODE_SUPPORT   0


/* Maximum number of interfaces in the configuration. Interface ID:s shall
   start from 0 and must increase without holes. */
#define MAX_NO_OF_INTERFACES     6

/* Enable or disable SOF timer functionality. */
#define USBD_SOFTMR_SUPPORT      0

/* Enable or disable isochronous endpoint support. */
#ifdef CFG_USBD_CD_AUDIO
 #define USBD_ISOCHRONOUS_SUPPORT 1
#else
 #define USBD_ISOCHRONOUS_SUPPORT 0
#endif

/* Enable or disable handling of MS OS descriptors and string */
#define USBD_MS_OS_DSC_SUPPORT   0
#define USBD_MS_OS_VENDOR_CODE   0xBCu     /* bMS_VendorCode, must match with vendor code returned in OS string descriptor */


/* Control task stack size. */
#define USBD_CONTROL_STACK_SIZE  4096 //1024  // Irene Lin

/* ep0_task stack size. */
#define USBD_EP0_STACK_SIZE      4096 //1024 // Irene Lin


/* Add endpoints at start. Only one configuration is possible,
   SET_CONFIGURATION is answered by USB controller automatically (SH7727) */
#define ADD_EPS_AT_USBD_START    0

#ifdef __cplusplus
}
#endif

#endif /* ifndef _CONFIG_USBD_H_ */

