/***************************************************************************
 *
 *            Copyright (c) 2010-2020 by HCC Embedded
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
#ifndef _API_USBH_EHCI_H
#define _API_USBH_EHCI_H

#include "../version/ver_usbh_ehci.h"
#if VER_USBH_EHCI_MAJOR != 3 || VER_USBH_EHCI_MINOR != 20
 #error Incompatible USBH_EHCI version number!
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void * const  usbh_ehci_hc;

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBH_EHCI_H */

