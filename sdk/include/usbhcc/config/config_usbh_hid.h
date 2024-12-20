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
#ifndef _CONFIG_USBH_HID_H
#define _CONFIG_USBH_HID_H

#include "../version/ver_usbh_hid.h"
#if VER_USBH_HID_MAJOR != 5 || VER_USBH_HID_MINOR != 4
 #error Incompatible USBH_HID version number!
#endif

/* set to enable mouse support */
#define HID_MOUSE_ENABLE                         0u//1u

/* set to enable keyboard support */
#define HID_KBD_ENABLE                           0u//1u

/* set to enable generic support */
#define HID_GENERIC_ENABLE                       1u

/* set to enable joystick/gamepad support */
#define HID_JOYSTICK_ENABLE                      0u//1u

/* maximum size of report descriptor */
#define HID_REPORT_DESCRIPTOR_MAX_SIZE           512u


/* must be 1 or higher: during parsing, a report descriptor
needs temporal storage for the global properties.
The depth of the storage must be at least the maximum count
of consecutive push instructions within a report descriptor
plus one */
#define HID_PARSER_GLOBAL_PROPERTIES_STACK_DEPTH 1u

/* the maximum number of reports within a report descriptor */
#define HID_PARSER_MAXIMUM_REPORT_COUNT          4u

/* the maximum number of report items within any report that the parser can process */
#define HID_PARSER_MAXIMUM_REPORT_ITEM_COUNT     64u

/* Zero value means that the HID calculates the pool size. */
#define HID_ITEM_POOL_SIZE                       0u

/* Report buffer size */
#define HID_REPORT_DATA_SIZE                     128u

#endif /* ifndef _CONFIG_USBH_HID_H */

