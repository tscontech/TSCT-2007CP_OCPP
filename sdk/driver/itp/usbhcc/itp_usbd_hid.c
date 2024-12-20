#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "itp_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbd_hid.h"
#include "usbhcc/api/api_usbd_hid_generic.h"
#include "usbhcc/config/config_usbd_config.h"
#include "usbhcc/api/api_usbd.h"
#include "usbhcc/config/hid/ite_usbd_hid_config.h"

static bool gConnected = false;
static uint8_t *gHid_wbuf = NULL;

bool itp_usbd_hid_is_connected()
{
    return gConnected;
}

void itp_usbd_hid_reg_read_ntf(uint8_t report_id, t_usbd_hid_ntf_fn ntf_fn)
{
    int rc;
    rc = usbd_ghid_register_ntf(0, report_id, USBD_GHID_OUT_CB, ntf_fn, 0);
    if (rc) {
        LOG_ERR "itp_usbd_hid_reg_read_ntf() fail! \n" LOG_END
    }
}

int itp_usbd_hid_read_data(uint8_t report_id, uint8_t *pbuf)
{
    int rc;
    uint16_t read_len = 0;
    if(!gConnected) return 0;

    rc = usbd_ghid_read_data(0, report_id, (uint8_t *)pbuf, (uint16_t *)&read_len);
    if (rc)
        LOG_ERR "usbd_ghid_read_data() fail! rc %d \n",rc LOG_END

    // LOG_DBG "UsbdHidRead: %d/%d \n", read_len, len LOG_END

    return read_len;
}

int itp_usbd_hid_write_data(uint8_t report_id, uint8_t *pbuf, int len)
{
    int rc;
    if(!gConnected || !gHid_wbuf) return 0;
    
    if (len > USBD_HID_MAX_REPORT_LENGTH) {
        LOG_ERR "hid write len(%d) > %d \n", len, USBD_HID_MAX_REPORT_LENGTH LOG_END
        return 0;
    }
    
    memcpy((void*)gHid_wbuf, (void*)pbuf, len);
    rc = usbd_ghid_write_data(0, report_id, gHid_wbuf);

    if (rc != USBD_HID_SUCCESS && rc != USBD_HID_ERR_NOT_READY)
        LOG_ERR "usbd_ghid_write_data() fail! rc %d \n", rc LOG_END

    // LOG_DBG "UsbdHidWrite: %d \n", len LOG_END

    return rc ? 0 : len;
}

int itp_usbd_hid_init(void)
{
    int rc;

    rc = usbd_hid_init();
    if (rc) {
        LOG_ERR "usbd_hid_init() fail! \n" LOG_END
        goto end;
    }
    rc = usbd_ghid_init();
    if (rc) {
        LOG_ERR "usbd_ghid_init() fail! \n" LOG_END
        goto end;
    }
    rc = usbd_ghid_start();
    if (rc) {
        LOG_ERR "usbd_ghid_start() fail! \n" LOG_END
        goto end;
    }
    rc = usbd_hid_start();
    if (rc) {
        LOG_ERR "usbd_hid_start() fail! \n" LOG_END
        goto end;
    }
    rc = usbd_start((usbd_config_t *)&device_cfg_hid_gen);
    if (rc) {
        LOG_ERR "usbd_start() fail! \n" LOG_END
        goto end;
    }
    
    gHid_wbuf = malloc(USBD_HID_MAX_REPORT_LENGTH);
    if (!gHid_wbuf) {
        LOG_ERR "alloc HID write buffer fail! \n" LOG_END
        rc = -1;
        goto end;
    }
    gConnected = true;
    
end:
    if (rc)
        LOG_ERR "%s() rc = %d \n", __func__, rc LOG_END
    return rc;
}

int itp_usbd_hid_exit(void)
{
    int rc;
    
    gConnected = false;
    if (gHid_wbuf) {
        free(gHid_wbuf);
        gHid_wbuf = NULL;
    }
    
    rc = usbd_ghid_stop();
    if (rc) {
        LOG_ERR "usbd_ghid_stop() fail! \n" LOG_END
        goto end;
    }
    rc = usbd_ghid_delete();
    if (rc) {
        LOG_ERR "usbd_ghid_delete() fail! \n" LOG_END
        goto end;
    }
    rc = usbd_hid_stop();
    if (rc) {
        LOG_ERR "usbd_hid_stop() fail! \n" LOG_END
        goto end;
    }
    rc = usbd_hid_delete();
    if (rc) {
        LOG_ERR "usbd_hid_delete() fail! \n" LOG_END
        goto end;
    }

end:
    if (rc)
        LOG_ERR "%s() rc = %d \n", __func__, rc LOG_END
    return rc;
}
