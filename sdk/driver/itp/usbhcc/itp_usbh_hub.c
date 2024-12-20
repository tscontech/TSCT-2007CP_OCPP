#include <sys/ioctl.h>
#include <string.h>
#include "itp_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usb_host.h"
#include "usbhcc/api/api_usbh_hub.h"


static int usbh_hub_cb(t_usbh_unit_id uid, t_usbh_ntf ntf)
{
    printf("usbh_hub_cb: uid %d, %s \n", uid,
        ((ntf == USBH_NTF_CONNECT) ? "connected" : "disconnected"));

    printf("hub present %d \n", usbh_hub_present(uid));

    //printf("port handle: %p \n", usbh_hub_get_port_hdl(uid));

    return 0;
}

int itp_usbh_hub_init(void)
{
    int rc;

    rc = usbh_hub_init();
    if (rc) {
        LOG_ERR "usbh_hub_init() fail! \n" LOG_END
        goto end;
    }
    rc = usbh_hub_register_ntf(0, (USBH_NTF_CONNECT), usbh_hub_cb);
    if (rc) {
        LOG_ERR "usbh_hub_register_ntf() fail! \n" LOG_END
        goto end;
    }

end:
    return rc;
}

int itp_usbh_hub_stop(void)
{
    int rc;

    rc = usbh_hub_stop();
    if (rc) {
        LOG_ERR "usbh_hub_stop() fail! \n" LOG_END
        goto end;
    }

end:
    return rc;
}

int itp_usbh_hub_exit(void)
{
    int rc;

    rc = usbh_hub_delete();
    if (rc) {
        LOG_ERR "usbh_hub_delete() fail! \n" LOG_END
        goto end;
    }

end:
    return rc;
}
