/***************************************************************************
 *
 *            Copyright (c) 2010-2017 by HCC Embedded
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
#ifndef _CONFIG_USBH_HID_GENERIC_H
#define _CONFIG_USBH_HID_GENERIC_H

#include "../version/ver_usbh_hid.h"
#if VER_USBH_HID_MAJOR != 5 || VER_USBH_HID_MINOR != 4
 #error Incompatible USBH_HID version number!
#endif

#define MAX_GENERIC_UNITS               4// 1
#define MAX_GENERIC_CB                  1

#define RAW_REPORTS                     1// 0    /* Set if raw reports will be read/written */
#define MAX_GEN_REPORTS                 2    /* Maximum no. reports. */
#define MAX_GENERIC_UNIT_REPORT_ITEMS   8    /* Number of report items a generic unit may have */

#define USBH_HID_GENERIC_BUFFER_SUPPORT 0// 1

#endif /* ifndef _CONFIG_USBH_HID_GENERIC_H */

