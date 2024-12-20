/****************************************************************************
**
**  Name:          btapp_nv.c
**
**  Description:   Contains btapp nvram abstraction source file
**
**
**  Copyright (c) 2019-2020, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
******************************************************************************/

#include "ite/itp.h"
#include "btapp_nv.h"

#define BT_FAT_FILE "B:/btDB"
static int gOperation = -1;
static int gbFatThread = 0;
static pthread_mutex_t gMutexBtAppNv;
static uint8_t* gpBuffer = NULL;
static int    gBufferSize = 0;

static void* btFatOperation(void* arg)
{
    gbFatThread = 1;
    FILE* btFile = NULL;
    while(1)
    {
        //Read operation
        if (gOperation == 0)
        {
            printf("bt storage read operation\n");
            if (gpBuffer && gBufferSize)
            {
                btFile = fopen(BT_FAT_FILE, "rb");
                if (!btFile)
                {
                    printf("open read file: %s is failed\n", BT_FAT_FILE);
                }
                else
                {
                    fread(gpBuffer, gBufferSize, 1, btFile);
                    fclose(btFile);
                }
            }
            else
            {
                printf("btapp_nv.c(%d)invalid bt buffer input...\n", __LINE__);
            }
            gOperation = -1;
        }
        else if (gOperation == 1)
        {
            printf("bt storage write operation\n");
            if (gpBuffer && gBufferSize)
            {
                btFile = fopen(BT_FAT_FILE, "wb");
                if (!btFile)
                {
                    printf("open read file: %s is failed\n", BT_FAT_FILE);
                }
                else
                {
                    fwrite(gpBuffer, gBufferSize, 1, btFile);
                    fclose(btFile);
					ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
                }
            }
            else
            {
                printf("btapp_nv.c(%d)invalid bt buffer input...\n", __LINE__);
            }
            gOperation = -1;
        }
        else
        {
            usleep(100*1000);
        }
    }
}

void config_create_fat_thread(void)
{
    pthread_t task;
    pthread_attr_t attr;
    // init mutex
    pthread_mutex_init(&gMutexBtAppNv, NULL);
    pthread_attr_init(&attr);
    pthread_create(&task, &attr, btFatOperation, NULL);
    usleep(1000);
}

void config_write_dev_db(uint8_t* buffer, int size)
{
    if (!gbFatThread)
    {
        printf("bt FAT operation thread is not created\n");
        return;
    }
    pthread_mutex_lock(&gMutexBtAppNv);
    gpBuffer = buffer;
    gBufferSize = size;
    gOperation = 1;
    usleep(1000);
    while (gOperation != -1)
    {
        usleep(10*1000);
    }
    gpBuffer = NULL;
    gBufferSize = 0;
    pthread_mutex_unlock(&gMutexBtAppNv);
}

void config_read_dev_db(uint8_t* buffer, int size)
{
    if (!gbFatThread)
    {
        printf("bt FAT operation thread is not created\n");
        return;
    }

    pthread_mutex_lock(&gMutexBtAppNv);
    gpBuffer = buffer;
    gBufferSize = size;
    gOperation = 0;
    usleep(1000);
    while (gOperation != -1)
    {
        usleep(10*1000);
    }
    gpBuffer = NULL;
    gBufferSize = 0;
    pthread_mutex_unlock(&gMutexBtAppNv);

}

void btapp_init_device_db (void)
{
    config_create_fat_thread();

    memset(&btapp_device_db,0x00,sizeof(btapp_device_db));

    /* set visibility to true by default. If visibility setting
    was stored in nvram previously it will get overwritten when
    all the parameters are read from nvram */
#if ((defined BR_INCLUDED) && (BR_INCLUDED == TRUE))
    btapp_device_db.visibility = FALSE;
    btapp_device_db.pairability = FALSE;
#endif

#if BLE_INCLUDED == TRUE
    btapp_device_db.le_conn_mode = BTA_DM_BLE_NON_CONNECTABLE;
    btapp_device_db.le_disc_mode = BTA_DM_BLE_NON_DISCOVERABLE;
#endif
    /* Set the local device name to default name */
    strcpy(btapp_device_db.local_device_name, btapp_cfg.cfg_dev_name);

    /* Phone needs to store in nvram some information about
    itself and other bluetooth devices with which it regularly communicates
    The information that has to be stored typically are
    local bluetooth name, visbility setting bdaddr, name, link_key,
    trust relationship etc.  */

    /* read device data base stored in nv-ram */
    btapp_nv_init_device_db();

#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
    btapp_nv_init_hs_db();
#endif

#if (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
//    btapp_nv_init_ble_info();
#if (defined BTA_GATT_INCLUDED) && (BTA_GATT_INCLUDED == TRUE)
//    btapp_nv_init_gattc_db();
//    btapp_nv_init_gatts_hndl_range_db();
//    btapp_nv_init_gatts_srv_chg_db();
#endif
#endif

}
/*******************************************************************************
**
** Function         btapp_store_device
**
** Description      stores peer device to NVRAM
**
**
** Returns          void
*******************************************************************************/
BOOLEAN btapp_store_device( tBTAPP_REM_DEVICE * p_rem_device)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (btapp_device_db.device[i].in_use
            && memcmp(&btapp_device_db.device[i].bd_addr, p_rem_device->bd_addr, BD_ADDR_LEN))
            continue;
        memcpy(&btapp_device_db.device[i], p_rem_device, sizeof(btapp_device_db.device[i]));
        btapp_device_db.device[i].in_use = TRUE;

#if BLE_INCLUDED == TRUE
        APPL_TRACE_EVENT1("store ble p_rem_device->device_type = %d", p_rem_device->device_type);
#endif
        break;
    }

    if (i == BTAPP_NUM_REM_DEVICE)
        return FALSE;

    /* update data base in nvram */
    btapp_nv_store_device_db();
    return TRUE;
}

/*******************************************************************************
**
** Function         btapp_get_device_record
**
** Description      gets the device record of a stored device
**
**
** Returns          void
*******************************************************************************/
tBTAPP_REM_DEVICE * btapp_get_device_record(BD_ADDR bd_addr)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (!btapp_device_db.device[i].in_use
            || memcmp(btapp_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN))
            continue;
        return &btapp_device_db.device[i];

    }

    return NULL;
}
/*******************************************************************************
**
** Function         btapp_get_device_record_idx
**
** Description      gets the device record index of a stored device
**
**
** Returns          void
*******************************************************************************/
UINT8 btapp_get_device_record_idx(BD_ADDR bd_addr)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (!btapp_device_db.device[i].in_use
            || memcmp(btapp_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN))
            continue;
        return i;

    }

    return 0xff;
}

/*******************************************************************************
**
** Function         btapp_get_inquiry_record
**
** Description      gets the device record from inquery db
**
**
** Returns          void
*******************************************************************************/
tBTAPP_REM_DEVICE * btapp_get_inquiry_record(BD_ADDR bd_addr)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_INQ_DEVICE; i++)
    {
        if (memcmp(btapp_inq_db.remote_device[i].bd_addr, bd_addr, BD_ADDR_LEN))
            continue;
        return &btapp_inq_db.remote_device[i];
    }

    return NULL;
}


/*******************************************************************************
**
** Function         btapp_alloc_device_record
**
** Description      gets the device record of a stored device
**
**
** Returns          void
*******************************************************************************/
tBTAPP_REM_DEVICE * btapp_alloc_device_record(BD_ADDR bd_addr)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (!btapp_device_db.device[i].in_use)
        {
            /* not in use */
            memset(&btapp_device_db.device[i], 0, sizeof(btapp_device_db.device[i]));
            btapp_device_db.device[i].in_use = TRUE;
            memcpy(btapp_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN);
            return &btapp_device_db.device[i];
        }
    }

    APPL_TRACE_ERROR0("btapp alloc device record failed, all entry has been used!");

    return NULL;
}

/*******************************************************************************
**
** Function         btapp_nv_store_device_db
**
** Description      Stores all parameters into nvram into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_device_db(void)
{
    /* this stores all nvram parameters */
#if (BT_CONFIG_FILE == TRUE)
    UINT32 entry = sizeof(btapp_device_db);
    /* this stores all nvram parameters */
    config_write_dev_db((unsigned char *)&btapp_device_db, entry);
#endif
	config_write_dev_db((unsigned char *)&btapp_device_db, sizeof(btapp_device_db));
}

/*******************************************************************************
**
** Function         btapp_nv_init_device_db
**
** Description      Inits device data base with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_device_db(void)
{
	config_read_dev_db((unsigned char *)&btapp_device_db, sizeof(btapp_device_db));
#if (BT_CONFIG_FILE == TRUE)
    int i = 0;
    int j = 0;
    UINT32 entry = sizeof(btapp_device_db);
    config_read_dev_db((unsigned char *)&btapp_device_db, entry);
    APPL_TRACE_EVENT0("--- stored linkkey ---\r\n");
    for(i = 0; i < BTAPP_NUM_REM_DEVICE;i++)
    {
        APPL_TRACE_EVENT1("\r\n%s\r\n", btapp_device_db.device[i].name);
        for(j = 0; j < LINK_KEY_LEN; j++)
        {
            APPL_TRACE_EVENT1("%02x", btapp_device_db.device[i].link_key[j]);
        }
    }
    APPL_TRACE_EVENT0("\r\n");
 #endif
    //if(!bt_config)
    //{
    //  bt_config = fopen(btsnoop_path, "rb");
    //}

    //ret = fread(&btapp_device_db, 1, sizeof(btapp_device_db), bt_config);

}

#if( defined BTA_HS_INCLUDED ) && (BTA_HS_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_nv_init_hs_db
**
** Description      Inits HS settings with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_hs_db(void)
{

}
/*******************************************************************************
**
** Function         btapp_nv_store_hs_db
**
** Description      Stores all parameters into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_hs_db(void)
{

}
#endif

#if (defined BLE_INCLUDED) && (BLE_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_nv_init_ble_info
**
** Description      Inits ble info information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_ble_info(void)
{

}
/*******************************************************************************
**
** Function         btapp_nv_store_ble_local_keys
**
** Description      Stores ble local id keys into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_ble_local_keys(void)
{

}

#if( defined BTA_GATT_INCLUDED ) && (BTA_GATT_INCLUDED == TRUE)
/*******************************************************************************
**
** Function         btapp_nv_init_gattc_db
**
** Description      Inits GATT client database with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_gattc_db(void)
{

}
/*******************************************************************************
**
** Function         btapp_nv_store_gattc_db
**
** Description      Stores all parameters into nvram into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_gattc_db(void)
{

}
/*******************************************************************************
**
** Function         btapp_nv_init_gatts_hndl_range_db
**
** Description      Inits GATTS handle map with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_gatts_hndl_range_db(void)
{

}

/*******************************************************************************
**
** Function         btapp_nv_store_gatts_hndl_range_db
**
** Description      Stores all parameters into nvram into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_gatts_hndl_range_db(void)
{

}

/*******************************************************************************
**
** Function         btapp_nv_init_gatts_srv_chg_db
**
** Description      Initialize GATTS service change db with information from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_init_gatts_srv_chg_db(void)
{

}
/*******************************************************************************
**
** Function         btapp_nv_store_gatts_srv_chg_db
**
** Description      Stores all parameters into nvram into nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_nv_store_gatts_srv_chg_db(void)
{

}

#endif
#endif

/*******************************************************************************
**
** Function         btapp_delete_device
**
** Description      deletes the device from nvram
**
**
** Returns          void
*******************************************************************************/
void btapp_delete_device(BD_ADDR bd_addr)
{
    UINT8 i;

    for (i=0; i<BTAPP_NUM_REM_DEVICE; i++)
    {
        if (memcmp(btapp_device_db.device[i].bd_addr, bd_addr, BD_ADDR_LEN))
            continue;
        //Simply sort
        for ( ;i < (BTAPP_NUM_REM_DEVICE-1); i++)
        {
            if (!btapp_device_db.device[i+1].in_use)
            {
                memset(&btapp_device_db.device[i], 0, sizeof(btapp_device_db.device[i+1]));
                break;
            }
            memcpy(&btapp_device_db.device[i], &btapp_device_db.device[i+1], sizeof(btapp_device_db.device[i+1]));

        }

        if (i==(BTAPP_NUM_REM_DEVICE-1))
            memset(&btapp_device_db.device[i], 0, sizeof(btapp_device_db.device[i+1]));
        break;

    }

    /* update data base in nvram */
    btapp_nv_store_device_db();

}

