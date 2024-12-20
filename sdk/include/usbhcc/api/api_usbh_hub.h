/***************************************************************************
 *
 *            Copyright (c) 2010-2013 by HCC Embedded
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
#ifndef _API_USBH_HUB_H
#define _API_USBH_HUB_H

#include <stdint.h>
#include "api_usb_host.h"

#include "../version/ver_usbh_hub.h"
#if VER_USBH_HUB_MAJOR != 2 || VER_USBH_HUB_MINOR != 8
 #error Incompatible USBH_HUB version number!
#endif
#include "../version/ver_usb_host.h"
#if VER_USB_HOST_MAJOR != 3
 #error Incompatible USB_HOST version number!
#endif


#ifdef __cplusplus
extern "C" {
#endif

int usbh_hub_register_ntf ( t_usbh_unit_id id, t_usbh_ntf ntf, t_usbh_ntf_fn ntf_fn );
int usbh_hub_present ( t_usbh_unit_id uid );
t_usbh_port_hdl usbh_hub_get_port_hdl ( t_usbh_unit_id uid );

int usbh_hub_init ( void );
int usbh_hub_start ( void );
int usbh_hub_stop ( void );
int usbh_hub_delete ( void );

#ifdef __cplusplus
}
#endif

#endif /* ifndef _API_USBH_HUB_H */

