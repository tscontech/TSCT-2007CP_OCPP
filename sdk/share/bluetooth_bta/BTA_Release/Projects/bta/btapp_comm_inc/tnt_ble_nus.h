/*
 * $ Copyright Cypress Semiconductor $
 */

/** @file
 *
 * This file provides definitions and function prototypes for Hello Sensor
 * device
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "bte_glue.h"

typedef struct
{
    BD_ADDR       remote_bd_addr;                         /**< Device address */
    uint8_t                         ble_addr_type;                          /**< LE Address type */
    uint8_t     ble_evt_type;                           /**< Scan result event type */
    int8_t                          rssi;                                   /**< Set to #BTM_INQ_RES_IGNORE_RSSI, if not valid */
    uint8_t                         flag;
} wiced_bt_ble_scan_results_t;
typedef void (wiced_bt_ble_scan_result_cback_t) (wiced_bt_ble_scan_results_t *p_scan_result, uint8_t *p_adv_data);

extern uint8_t *wiced_bt_ble_check_advertising_data( uint8_t *p_adv, uint8_t type, uint8_t *p_length);

extern void tnt_ble_pair_scan_done(void);
extern void tnt_dualmode_application_start(void);
extern void tnt_ble_nus_init( void );
extern void tnt_ble_on_advertisement_stopped(void);
extern bool tnt_ble_is_connected(void);
extern void tnt_ble_disconnect(void);
extern bool tnt_ble_adv_restart(void);
extern int tnt_ble_send_data(char *buf, unsigned int len);
extern bool tnt_ble_tx_is_ready(void);
extern bool tnt_ble_scan_start(wiced_bt_ble_scan_result_cback_t *p_scan_result_cback);
extern bool tnt_ble_scan_stop(void);
extern void tnt_ble_pair_scan_start(void);
extern void tnt_ble_disable(void);
extern void tnt_bt_enter_low_power_mode(bool enter);
extern void tnt_bt_application_init(bool init);

