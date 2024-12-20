/*****************************************************************************
**
**  Name:             btapp_dg.h
**
**  Description:     This file contains btapp internal interface
**                   definition
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#ifndef BTAPP_DG_H
#define BTAPP_DG_H

#define BTAPP_DG_ID_SPP_0          0
#define BTAPP_DG_ID_SPP_1          1
#define BTAPP_DG_ID_SPP_2          2
#define BTAPP_DG_ID_SPP_3          3

#define BTAPP_DG_NUM_SERVICES      4

#define BTAPP_DG_DATA_TEST_TOUT    2
#define BTAPP_DG_SENT_Q_MAX_CNTS   10

typedef struct
{
    BUFFER_Q        loopback_data_q;
    BUFFER_Q        sent_data_q;
    UINT16          port_handle;
    UINT16          api_handle;
    UINT16          tx_sent;
    UINT16          rx_sent;
    UINT16          mtu;
    BOOLEAN         is_open;
    BOOLEAN         is_used;
    UINT8           port_id;
    tBTA_SERVICE_ID service_id;
    UINT32          time_taken;
    UINT32          data_send;
    UINT32          prev_data_send;
    UINT32          data_recvd;
    UINT32          prev_data_recvd;
    TIMER_LIST_ENT  data_test_tle;
    UINT8           data_pattern;
} tBTAPP_DG_APP_CB;

/* typedef for BTAPP DG control block */
typedef struct
{
    tBTAPP_DG_APP_CB app_cb[BTAPP_DG_NUM_SERVICES];
    BOOLEAN         flowed_off;
    UINT16          flowed_port_handle;
} tBTAPP_DG_CB;

extern tBTAPP_DG_CB btapp_dg_cb;

extern void btapp_dg_init(void);
extern void btapp_dg_register_entry(UINT8 index);
extern void btapp_dg_deregister_entry(UINT8 index);
extern void btapp_dg_register_entry_nums(UINT8 nums);
extern void btapp_dg_deregister_all(void);
extern void btapp_dg_close_connection(UINT8 conn_index);
extern void btapp_dg_connect(void);
extern void btapp_dg_set_device_authorized (tBTAPP_REM_DEVICE * p_device_rec);
extern void btapp_dg_spp_send_data(UINT8 app_id, UINT8* pbuf, UINT16 len);
extern void btapp_dg_spp_rcv_data(UINT8 app_id, UINT8* pbuf, UINT16 len);
extern void btapp_dg_spp_test_send(void);
extern void btapp_dg_spp_send_raw(UINT8 test_cnt, UINT16 send_interval, UINT8* buf, UINT16 len);
extern void btapp_dg_spp_send_tht(UINT8 app_id, UINT16 test_cnt);

#endif
