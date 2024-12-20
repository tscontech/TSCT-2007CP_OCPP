#include <sys/ioctl.h>
#include <string.h>
#include "itp_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/api/api_usbh_cdcecm.h"
#include "usbhcc/config/config_usbh_cdcecm.h"

struct usbh_cdcecm_ctxt {
    uint8_t state;
    uint8_t reserved[3];
};

static struct usbh_cdcecm_ctxt ecm_ctxt[USBH_CDCECM_MAX_UNITS] = { 0 };
static int socket_inited = 0;

extern int usbh_cdcecm_tx_cb(t_usbh_unit_id uid, t_usbh_ntf ntf);
extern int usbh_cdcecm_rx_cb(t_usbh_unit_id uid, t_usbh_ntf ntf);

static int cdcecm_ntf_fn(t_usbh_unit_id uid, t_usbh_ntf ntf)
{
    ithPrintf("[ECM]: uid %d, %s \n", uid,
        ((ntf == USBH_NTF_CONNECT) ? "connected" : "disconnected"));

    /* TODO: multi-uint */
    if (ntf == USBH_NTF_CONNECT) {
        #if defined(CFG_USBH_CD_CDCECM)
        if (!socket_inited) {
            itpRegisterDevice(ITP_DEVICE_SOCKET, &itpDeviceSocket);
            ioctl(ITP_DEVICE_SOCKET, ITP_IOCTL_INIT, NULL);
            socket_inited = 1;
        }
        else { // for power sleep
            ioctl(ITP_DEVICE_ETHERNET, ITP_IOCTL_RESUME, NULL);
        }
        #endif

        #if defined(CFG_USBH_CD_CDCECM_EX)
        if (!socket_inited) {
            ioctl(ITP_DEVICE_ECM_EX, ITP_IOCTL_INIT, NULL);
            socket_inited = 1;
        }
        else { // for power sleep
            ioctl(ITP_DEVICE_ECM_EX, ITP_IOCTL_RESUME, NULL);
        }
        #endif
    }
    else {
        ;
    }

    ecm_ctxt[uid].state = ntf;

    return 0;
}

int itp_usbh_cdcecm_init(void)
{
    int rc, i;

    rc = usbh_cdcecm_init();
    if (rc) {
        LOG_ERR "usbh_cdcecm_init() fail! \n" LOG_END
        goto end;
    }
    for (i = 0; i < USBH_CDCECM_MAX_UNITS; i++) {
        rc = usbh_cdcecm_register_ntf(i, USBH_NTF_CONNECT, cdcecm_ntf_fn);
        if (rc) {
            LOG_ERR "usbh_cdcecm_register_ntf(%d) fail! USBH_NTF_CONNECT \n", i LOG_END
            goto end;
        }
        rc = usbh_cdcecm_register_ntf(i, USBH_NTF_CDCECM_TX, usbh_cdcecm_tx_cb);
        if (rc) {
            LOG_ERR "usbh_cdcecm_register_ntf(%d) fail! USBH_NTF_CDCECM_TX \n", i LOG_END
            goto end;
        }
        rc = usbh_cdcecm_register_ntf(i, USBH_NTF_CDCECM_RX, usbh_cdcecm_rx_cb);
        if (rc) {
            LOG_ERR "usbh_cdcecm_register_ntf(%d) fail! USBH_NTF_CDCECM_RX \n", i LOG_END
            goto end;
        }
    }

end:
    return rc;
}

int itp_usbh_cdcecm_stop(void)
{
    int rc;

    rc = usbh_cdcecm_stop();
    if (rc) {
        LOG_ERR "usbh_cdcecm_stop() fail! \n" LOG_END
        goto end;
    }

end:
    return rc;
}

int itp_usbh_cdcecm_exit(void)
{
    int rc;

    rc = usbh_cdcecm_delete();
    if (rc) {
        LOG_ERR "usbh_cdcecm_delete() fail! \n" LOG_END
        goto end;
    }

end:
    return rc;
}
