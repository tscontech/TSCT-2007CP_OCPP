#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "itp_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbd_cdcacm.h"
#include "usbhcc/config/config_usbd_config.h"
#include "usbhcc/api/api_usbd.h"

#define LINE_CNT            1
#define ACM_WBUF_SIZE       (128*1024)

struct usbd_cdcacm_ctxt {
    uint8_t *acm_wbuf;
    volatile int   connect;
};

static struct usbd_cdcacm_ctxt g_line[LINE_CNT];

static void usbd_cdcacm_cb(const uint8_t line, const t_usbd_cdcacm_ntf_type ntf_type)
{
    if (line > LINE_CNT) {
        LOG_ERR "usbd_cdcacm_cb() line index error! (%d) \n", line LOG_END
        return;
    }

    switch (ntf_type) {
    case USBD_CDCACM_NTF_CONNECT:
        printf("line %d connect \n", line);
        g_line[line].connect = 1;
        break;

    case USBD_CDCACM_NTF_DISCONNECT:
        printf("line %d disconnect \n", line);
        g_line[line].connect = 0;
        break;

    default:
        break;
    }

    return;
}

int itp_usbd_cd_cdcacm_init(void)
{
    int i, rc;

    for (i = 0; i < LINE_CNT; i++) {
        g_line[i].acm_wbuf = (uint8_t*)itpVmemAlloc(ACM_WBUF_SIZE);
        if (!g_line[i].acm_wbuf) {
            LOG_ERR "alloc ACM write buffer fail! \n" LOG_END
            rc = -1;
            goto end;
        }
    }

    rc = usbd_cdcacm_init();
    if (rc) {
        LOG_ERR "usbd_cdcacm_init() fail! \n" LOG_END
        goto end;
    }
    rc = usbd_cdcacm_start();
    if (rc) {
        LOG_ERR "usbd_cdcacm_start() fail! \n" LOG_END
        goto end;
    }
    rc = usbd_cdcacm_reg_ntf_fn(USBD_CDCACM_NTF_CONNECT, usbd_cdcacm_cb);
    if (rc) {
        LOG_ERR "usbd_cdcacm_reg_ntf_fn() fail! USBD_CDCACM_NTF_CONNECT \n" LOG_END
        goto end;
    }
    rc = usbd_cdcacm_reg_ntf_fn(USBD_CDCACM_NTF_DISCONNECT, usbd_cdcacm_cb);
    if (rc) {
        LOG_ERR "usbd_cdcacm_reg_ntf_fn() fail! USBD_CDCACM_NTF_DISCONNECT \n" LOG_END
        goto end;
    }
    rc = usbd_start((usbd_config_t *)&device_cfg_cdc_acm);
    if (rc) {
        LOG_ERR "usbd_start() fail! \n" LOG_END
        goto end;
    }

end:
    if (rc) {
        for (i = 0; i < LINE_CNT; i++) {
            if (g_line[i].acm_wbuf) {
                itpVmemFree((uint32_t)g_line[i].acm_wbuf);
                g_line[i].acm_wbuf = NULL;
            }
        }

        LOG_ERR "%s() rc = %d \n", __func__, rc LOG_END
    }
    return rc;
}

int itp_usbd_cd_cdcacm_exit(void)
{
    int i, rc;

    rc = usbd_cdcacm_stop();
    if (rc) {
        LOG_ERR "usbd_cdcacm_stop() fail! \n" LOG_END
        goto end;
    }

    rc = usbd_cdcacm_delete();
    if (rc) {
        LOG_ERR "usbd_cdcacm_delete() fail! \n" LOG_END
        goto end;
    }

end:
    for (i = 0; i < LINE_CNT; i++) {
        if (g_line[i].acm_wbuf) {
            itpVmemFree((uint32_t)g_line[i].acm_wbuf);
            g_line[i].acm_wbuf = NULL;
        }
    }
    if (rc)
        LOG_ERR "%s() rc = %d \n", __func__, rc LOG_END
    return rc;
}

static int UsbdAcmRead(int file, char *ptr, int len, void *info)
{
    int rc;
    int line = 0;
    uint32_t actual_len = 0;

    if (!g_line[line].connect)
        return 0;

    rc = usbd_cdcacm_receive(line, (uint8_t * const)ptr, len, (uint32_t * const)&actual_len);
    while (rc == USBD_CDCACM_BUSY) {
        usleep(1000);
        rc = usbd_cdcacm_receive(line, (uint8_t * const)ptr, len, &actual_len);
    }
    if (rc)
        LOG_ERR "usbd_cdcacm_receive() fail! rc %d \n",rc LOG_END

    LOG_DBG "UsbdAcmRead: %d/%d \n", actual_len, len LOG_END

    return actual_len;
}

static int UsbdAcmWrite(int file, char *ptr, int len, void *info)
{
    int rc;
    int line = 0;
   
    if (!g_line[line].connect)
        return 0;

    if (!g_line[line].acm_wbuf)
        return 0;

    if (len > ACM_WBUF_SIZE) {
        LOG_ERR "acm write len(%d) > %d \n", len, ACM_WBUF_SIZE LOG_END
        return 0;
    }

    /* Check last TX finished */
    while (usbd_cdcacm_send_status(line) == USBD_CDCACM_BUSY)
        usleep(1000);

    memcpy((void*)g_line[line].acm_wbuf, (void*)ptr, len);
    rc = usbd_cdcacm_send(line, g_line[line].acm_wbuf, len);
    if (rc)
        LOG_ERR "usbd_cdcacm_send() fail! rc %d \n", rc LOG_END

    LOG_DBG "UsbdAcmWrite: %d \n", len LOG_END

    return rc ? 0 : len;
}

extern ITHUsbModule itp_usbd_base;

static int UsbdAcmIoctl(int file, unsigned long request, void* ptr, void* info)
{
    int line = 0;

    switch (request)
    {
    case ITP_IOCTL_INIT:
        break;

    case ITP_IOCTL_ENABLE:
        break;

    case ITP_IOCTL_DISABLE:
        break;

    case ITP_IOCTL_IS_CONNECTED:
        /* currently only support one line */
        return g_line[line].connect;
        
    default:
        errno = (ITP_DEVICE_USBDACM << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}


const ITPDevice itpDeviceUsbdAcm =
{
    ":usbd acm",
    itpOpenDefault,
    itpCloseDefault,
    UsbdAcmRead,
    UsbdAcmWrite,
    itpLseekDefault,
    UsbdAcmIoctl,
    NULL
};
