#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "itp_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbd.h"
#include "usbhcc/api/api_usb_host.h"

#if defined(CFG_USBD_CD_CDCACM)
extern int itp_usbd_cd_cdcacm_init(void);
extern int itp_usbd_cd_cdcacm_exit(void);
#endif

#if defined(CFG_USBD_CD_MST)
extern int itp_usbd_cd_mst_init(void);
extern int itp_usbd_cd_mst_exit(void);
#endif

#if defined(CFG_USBD_CD_HID)
extern int itp_usbd_hid_init(void);
extern int itp_usbd_hid_exit(void);
#endif

/*
** Tell USB device driver if the board is currently running USB powered
** or not.
**
*/
int usbd_is_self_powered(void)
{
    /* allways self powered */
    return 1;
}

void usbd_conn_state(usbd_conn_state_t new_state)
{
    switch (new_state)
    {
    case usbdcst_invalid:
        printf("usbd state: usbdcst_invalid \n");
        /*  */
        break;

    case usbdcst_offline:
        printf("usbd state: cable not connected, stack stopped \n");
        /* cable not connected, stack stopped */
        break;

    case usbdcst_ready:
        printf("usbd state: cabe not connected, stack started \n");
        /* cabe not connected, stack started */
        break;

    case usbdcst_stopped:
        printf("usbd state: cable connected, stack stopped; bus powered device may draw 500uA from USB \n");
        /* cable connected, stack stopped; bus powered device may draw 500uA from USB */
        break;

    case usbdcst_not_cfg:
        printf("usbd state: online, not configured; bus powered device may draw 10mA from USB \n");
        /* online, not configured; bus powered device may draw 10mA from USB */
        break;

    case usbdcst_cfg:
        printf("usbd state: configured \n");
        /* configured; bus powered device may the maximum amount of current
        specifyed in the configuration descriptor */
        break;

    case usbdcst_suspend:
        printf("usbd state: suspended; bus powered device may draw 500uA from USB \n");
        /* suspended; bus powered device may draw 500uA from USB */
        break;

    case usbdcst_suspend_hic:
        printf("usbd state: suspended; bus powered device may draw 2.5 mA from USB \n");
        /* suspended; bus powered device may draw 2.5 mA from USB */
        break;
    } /* switch */

} /* usbd_conn_state */


static int usb_device_init(void)
{
    int rc;

    rc = usbd_init();
    if (rc) {
        LOG_ERR "usbd_init() fail! \n" LOG_END
        goto end;
    }

#if defined(CFG_USBD_CD_CDCACM)
    rc = itp_usbd_cd_cdcacm_init();
    if (rc) {
        LOG_ERR "usb_device_cd_cdcacm_init() fail! \n" LOG_END
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_MST)
    rc = itp_usbd_cd_mst_init();
    if (rc) {
        LOG_ERR "itp_usbd_cd_mst_init() fail! \n" LOG_END
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_HID)
    rc = itp_usbd_hid_init();
    if (rc) {
        LOG_ERR "usb_device_cd_hid_init() fail! \n" LOG_END
        goto end;
    }
#endif

end:
    if (rc)
        LOG_ERR "%s() rc = %d \n", __func__, rc LOG_END
    return rc;
}

static int usb_device_exit(void)
{
    int rc;

    rc = usbd_stop();
    if (rc) {
        LOG_ERR "usbd_stop() fail! \n" LOG_END
        goto end;
    }

    rc = usbd_delete();
    if (rc) {
        LOG_ERR "usbd_delete() fail! \n" LOG_END
        goto end;
    }

#if defined(CFG_USBD_CD_CDCACM)
    rc = itp_usbd_cd_cdcacm_exit();
    if (rc) {
        LOG_ERR "usb_device_cd_cdcacm_exit() fail! \n" LOG_END
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_MST)
    rc = itp_usbd_cd_mst_exit();
    if (rc) {
        LOG_ERR "itp_usbd_cd_mst_exit() fail! \n" LOG_END
        goto end;
    }
#endif

#if defined(CFG_USBD_CD_HID)
    rc = itp_usbd_hid_exit();
    if (rc) {
        LOG_ERR "usb_device_cd_hid_exit() fail! \n" LOG_END
        goto end;
    }
#endif

end:
    if (rc)
        LOG_ERR "%s() rc = %d \n", __func__, rc LOG_END
        return rc;
}

static pthread_t detect_task;
static sem_t detect_sem;
static int exit;
volatile bool usbd_connect = false;

extern ITHUsbModule itp_usbd_base;
extern uint8_t itp_usbd_idx;


static void* UsbDeviceDetectHandler(void* arg)
{
    int rc;

    while (1) {
        if (ithUsbIsDeviceMode(itp_usbd_base) == true) {
            if (usbd_connect == false) {
                rc = usbh_hc_stop(itp_usbd_idx);
                if (rc) {
                    LOG_ERR "usbh_hc_stop(%d) fail! \n", itp_usbd_idx LOG_END
                    goto end;
                }

                printf("enter device mode! \n");
                rc = usb_device_init();
                if (rc)
                    LOG_ERR "usb_device_init() fail!\n" LOG_END

                usbd_vbus(1, 0);
                usbd_connect = true;
            }
        }
        else {
            if (usbd_connect == true) {
                printf("leave device mode! \n");
                usbd_connect = false;

                usbd_vbus(0, 0);
                usb_device_exit();

                rc = usbh_hc_start(itp_usbd_idx);
                if (rc)
                    LOG_ERR "usbh_hc_start(%d) fail! \n", itp_usbd_idx LOG_END
            }
        }

        itpSemWaitTimeout(&detect_sem, 30);
        if (exit)
            goto end;
    }

end:
    return NULL;
}

static int UsbdInit(void)
{
    int rc;
    pthread_attr_t attr;

    exit = 0;
    sem_init(&detect_sem, 0, 0);

    pthread_attr_init(&attr);
    rc = pthread_create(&detect_task, &attr, UsbDeviceDetectHandler, NULL);
    if (rc)
        LOG_ERR " create usb device detect thread fail! 0x%08X \n", rc LOG_END

end:
    return rc;
}

static int UsbdExit(void)
{
    int rc;

    rc = usb_device_exit();
    if (rc)
        LOG_ERR "usb_device_exit() fail! \n" LOG_END

    exit = 1;
    sem_post(&detect_sem);
    pthread_join(detect_task, NULL);
    sem_destroy(&detect_sem);

    return 0;
}

static int UsbdIoctl(int file, unsigned long request, void* ptr, void* info)
{
    int res;

    switch (request)
    {
    case ITP_IOCTL_INIT:
        res = UsbdInit();
        if (res)
        {
            errno = (ITP_DEVICE_USBD << ITP_DEVICE_ERRNO_BIT) | res;
            return -1;
        }
        break;

    case ITP_IOCTL_EXIT:
        res = UsbdExit();
        break;

    default:
        errno = (ITP_DEVICE_USBD << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}

const ITPDevice itpDeviceUsbd =
{
    ":usbd",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    UsbdIoctl,
    NULL
};


