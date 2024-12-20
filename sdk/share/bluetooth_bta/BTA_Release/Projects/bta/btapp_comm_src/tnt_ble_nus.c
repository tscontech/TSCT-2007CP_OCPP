
#include <string.h>

#include "bta_platform.h"
#include "bte_glue.h"

//#include "tnt_stick_config.h"
//#include <hal_base.h>
//#include "tnt_app_main.h"
//#include "tnt_stick_config.h"
//#include "rkdebug.h"

#include "tnt_ble_nus.h"
#include "gki.h"


#define BT_TASK_SIZE  configMINIMAL_STACK_SIZE

/******************************************************************************
 *                           Function Prototypes
 ******************************************************************************/
uint8_t *wiced_bt_ble_check_advertising_data( uint8_t *p_adv, uint8_t type, uint8_t *p_length)
{
    return NULL;
}

void tnt_ble_pair_scan_done(void)
{

}

void tnt_dualmode_application_start(void)
{

}

void tnt_ble_nus_init( void )
{

}

void tnt_ble_on_advertisement_stopped(void)
{

}

bool tnt_ble_is_connected(void)
{
    return FALSE;
}

void tnt_ble_disconnect(void)
{

}

bool tnt_ble_adv_restart(void)
{
    return FALSE;
}

int tnt_ble_send_data(char *buf, unsigned int len)
{
    return 0;
}

bool tnt_ble_tx_is_ready(void)
{
    return FALSE;
}

bool tnt_ble_scan_start(wiced_bt_ble_scan_result_cback_t *p_scan_result_cback)
{
    return FALSE;
}

bool tnt_ble_scan_stop(void)
{
    return FALSE;
}

void tnt_ble_pair_scan_start(void)
{

}

void tnt_ble_disable(void)
{

}

void tnt_bt_enter_low_power_mode(bool enter)
{
    
}

void tnt_bt_application_init(bool init)
{	
    taskinfo_t gki_task_config[GKI_MAX_TASKS]={
       [AUDIO_PLAY_TASK]    = { 4, BT_TASK_SIZE},
       [USERIAL_TASK]       = { 1, BT_TASK_SIZE},
       [HCISU_TASK]         = { 2, BT_TASK_SIZE},
       [UCODEC_TASK]        = { 3, BT_TASK_SIZE},
       [BTU_TASK]           = { 2, BT_TASK_SIZE},
       [BTAPPL_TASK]        = { 1, BT_TASK_SIZE},
       [TICKS_TASK]         = { 1, BT_TASK_SIZE},
       [BTAPP_CONSOLE_TASK] = { 1, BT_TASK_SIZE}, //bte logmsg thread
    };

    GKI_set_task_prio_stack(gki_task_config); //must before bt init


    printf("bt init start\r\n");

    btapp_init(0, NULL);



}
