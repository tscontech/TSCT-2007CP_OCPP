#include <sys/ioctl.h>
#include <string.h>
#include "itp_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/api/api_usbh_ehci.h"

#if defined(CFG_USBHCC_DEVICE)
ITHUsbModule itp_usbd_base = ITH_USB0;
uint8_t itp_usbd_idx = 0;
#endif

/* call from usbh_hc_init() */
int usb_get_device_cfg(int *usb_idx, unsigned int *id_pin)
{
    int rc = -1;

#if defined(CFG_USBHCC_DEVICE)
    #if defined(CFG_USBD_USB0)
    (*usb_idx) = itp_usbd_idx = 0;
    #endif
    #if defined(CFG_USBD_USB1)
    itp_usbd_base = ITH_USB1;
    (*usb_idx) = itp_usbd_idx = 1;
    #endif
    (*id_pin) = CFG_GPIO_USB_ID_PIN;
    rc = 0;
#endif

    return rc;
}

static void usbh_enum_fail_cb(t_usbh_port_hdl port_hdl, uint16_t vid, uint16_t pid)
{
    LOG_ERR "%s() vid: 0x%04X  pid: 0x%04X \n", vid, pid LOG_END
}

int itp_usbh_init(void)
{
    int rc;

    /* initialize USB host stack. */
    rc = usbh_init();
    if (rc) {
        LOG_ERR "usbh_init() fail! \n" LOG_END
        goto end;
    }
    /* initialize USB host controller. */
    rc = usbh_hc_init(0, usbh_ehci_hc, 0);
    if (rc) {
        LOG_ERR "usbh_hc_init(0,0) fail! \n" LOG_END
            goto end;
    }

#if (CFG_CHIP_FAMILY == 970)
    rc = usbh_hc_init(1, usbh_ehci_hc, 1);
    if (rc) {
        LOG_ERR "usbh_hc_init(1,1) fail! \n" LOG_END
            goto end;
    }
#endif
    rc = usbh_register_enum_failed_cb(usbh_enum_fail_cb);
    if (rc) {
        LOG_ERR "usbh_register_enum_failed_cb() fail! \n" LOG_END
            goto end;
    }

end:
    if (rc)
        LOG_ERR "%s() rc = %d \n", __func__, rc LOG_END
    return rc;
}

int itp_usbh_stop(void)
{
    int rc;

    rc = usbh_stop();
    if (rc) {
        LOG_ERR "usbh_stop() fail! \n" LOG_END
        goto end;
    }

    rc = usbh_hc_stop(0);
    if (rc) {
        LOG_ERR "usbh_hc_stop(0) fail! \n" LOG_END
        goto end;
    }

#if (CFG_CHIP_FAMILY == 970)
    rc = usbh_hc_stop(1);
    if (rc) {
        LOG_ERR "usbh_hc_stop(1) fail! \n" LOG_END
        goto end;
    }
#endif

end:
    if (rc)
        LOG_ERR "%s() rc = %d \n", __func__, rc LOG_END
    return rc;
}

int itp_usbh_exit(void)
{
    int rc;

    rc = usbh_delete();
    if (rc) {
        LOG_ERR "usbh_delete() fail! \n" LOG_END
        goto end;
    }

    rc = usbh_hc_delete(0);
    if (rc) {
        LOG_ERR "usbh_hc_delete(0) fail! \n" LOG_END
        goto end;
    }

#if (CFG_CHIP_FAMILY == 970)
    rc = usbh_hc_delete(1);
    if (rc) {
        LOG_ERR "usbh_hc_delete(1) fail! \n" LOG_END
        goto end;
    }
#endif

end:
    if (rc)
        LOG_ERR "%s() rc = %d \n", __func__, rc LOG_END
    return rc;
}
