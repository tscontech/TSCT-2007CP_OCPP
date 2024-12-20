#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "itp_cfg.h"
#include "ite/itp.h"
#include "usbhcc/api/api_usbd_mst.h"
#include "usbhcc/api/api_scsi_tgt.h"
#include "usbhcc/config/config_usbd_config.h"
#include "usbhcc/api/api_usbd.h"

#define ITP_MST_MAX_LUNS       4

/** same with itp_fat.c */
typedef struct
{
    F_DRIVER* driver;
    F_DRIVERINIT initfunc;
    unsigned long param;
} Driver;

typedef struct
{
    unsigned long reserved;           // reserved size in byte
    unsigned long cacheSize;          // norCache size
    bool          deviceMode;         // device mode
    unsigned long partitionOffset;    // partition offset
} NORDrvParam;

static const uint32_t diskTable[] = { CFG_USBD_CD_MST_DISKS, -1 };
static uint32_t g_nluns = 0;
static int g_connected = 0;

//<<<<<<<<<
/** from config_scsi_tgt.h */
uint8_t N_LUNS_MST = 0;
/** from config_scsi_tgt.c */
scsim_table_entry_t  scsi_media_table_mst[ITP_MST_MAX_LUNS] = { 0 };
//>>>>>>>>>

int itp_usbd_cd_mst_init(void)
{
    int i, rc;
    ITPPartition disk;
    char *str;
    Driver *diskDriverTlb;
    NORDrvParam *drvParam;

    /** unmount */
#if defined(CFG_FS_FAT)
    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_DISABLE, NULL);
    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_UNMOUNT, (void*)diskTable);
#endif

#if defined(CFG_NOR_ENABLE)

    #if defined(CFG_USB_DEVICE_DRIVE)
        str = CFG_USB_DEVICE_DRIVE;
    #else
        str = "A";
    #endif

    disk.disk = ITP_DISK_NOR;
    rc = ioctl(ITP_DEVICE_FAT, ITP_IOCTL_GET_PARTITION, &disk);

    if (rc == 0)
    {
        diskDriverTlb = (Driver *)ioctl(ITP_DEVICE_FAT, ITP_IOCTL_GET_TABLE, (void *)ITP_DISK_NOR);
        if (!diskDriverTlb)
        {
            LOG_ERR "[usbd][mst][NOR] get disk%d's driver table fail! \n", ITP_DISK_NOR LOG_END
            rc = -1;
            goto end;
        }

        drvParam = (NORDrvParam *)diskDriverTlb->param;
        drvParam->deviceMode = true;

        if (strcmp(str, "A") == 0)
            drvParam->partitionOffset = 0x10;
        else if (strcmp(str, "B") == 0)
            drvParam->partitionOffset = 0x10 + disk.size[0]/512;
        else if (strcmp(str, "C") == 0)
            drvParam->partitionOffset = 0x10 + (disk.size[0]+ disk.size[1])/512;
        else if (strcmp(str, "D") == 0)
            drvParam->partitionOffset = 0x10 +(disk.size[0]+ disk.size[1]+ disk.size[2])/512;
        else
        {
            drvParam->deviceMode = false;
            printf("invalid disk driver\n");
            drvParam->partitionOffset = 0x10;
            rc = -1;
            goto end;
        }
        //printf("norPartitionOffset=0x%x\n", drvParam->partitionOffset);
    }
    else
    {
        LOG_ERR "NOR ITP_IOCTL_GET_PARTITION fail! \n" LOG_END
        goto end;
    }

    ioctl(ITP_DEVICE_FAT, ITP_IOCTL_UNMOUNT, (void *)ITP_DISK_NOR);
#endif

    /** start mst stack */
    rc = mstd_init();
    if (rc) {
        LOG_ERR "mstd_init() fail! \n" LOG_END
        goto end;
    }
    rc = mstd_start();
    if (rc) {
        LOG_ERR "mstd_start() fail! \n" LOG_END
        goto end;
    }
    for (i = 0; i < g_nluns; i++) {
        rc = scsis_enable_lun(SCSI_MST_IDX, i, 0);
        if (rc) {
            LOG_ERR "scsis_enable_lun(%d) fail!  \n", i LOG_END
            goto end;
        }
    }
    rc = usbd_start((usbd_config_t *)&device_cfg_mst);
    if (rc) {
        LOG_ERR "usbd_start() fail! \n" LOG_END
        goto end;
    }

    g_connected = 1;

end:
    if (rc)
        LOG_ERR "%s() rc = %d \n", __func__, rc LOG_END

    return rc;
}

int itp_usbd_cd_mst_exit(void)
{
    int i, rc;
    Driver *diskDriverTlb;
    NORDrvParam *drvParam;
    
    g_connected = 0;

    for (i = 0; i < g_nluns; i++) {
        rc = scsis_disable_lun(SCSI_MST_IDX, i);
        if (rc) {
            LOG_ERR "scsis_disable_lun(%d) fail!  \n", i LOG_END
        }
    }

    rc = mstd_stop();
    if (rc) {
        LOG_ERR "mstd_stop() fail! \n" LOG_END
    }

    mstd_delete();

#if defined(CFG_NOR_ENABLE)
    diskDriverTlb = (Driver *)ioctl(ITP_DEVICE_FAT, ITP_IOCTL_GET_TABLE, (void *)ITP_DISK_NOR);
    if (!diskDriverTlb)
    {
        LOG_ERR "[usbd][mst][NOR] get disk%d's driver table fail! \n", ITP_DISK_NOR LOG_END
        rc = -1;
    }
    else
    {
        drvParam = (NORDrvParam *)diskDriverTlb->param;
        drvParam->deviceMode = false;
    }
#endif

#if defined(CFG_FS_FAT)
    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_MOUNT, (void*)diskTable);
    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_ENABLE, NULL);
#endif // #if defined(CFG_FS_FAT)

    if (rc)
        LOG_ERR "%s() rc = %d \n", __func__, rc LOG_END

    return rc;
}

static void UsbdMstInit(void)
{
    int i;
    Driver *diskDriverTlb;

    /** cal. max luns */
    g_nluns = ITH_COUNT_OF(diskTable) -1;  // not include -1
    if (g_nluns > ITP_MST_MAX_LUNS) {
        LOG_WARN "[usbd][mst] max luns(%d) > %d ! \n", g_nluns, ITP_MST_MAX_LUNS LOG_END
        g_nluns = ITP_MST_MAX_LUNS;
    }
    /** for usb device mst stack use */
    N_LUNS_MST = g_nluns; 
    printf("[usbd][mst] max lun %d \n", N_LUNS_MST);

    /** get media table */
    for (i = 0; i < g_nluns; i++) {
        diskDriverTlb = (Driver *)ioctl(ITP_DEVICE_FAT, ITP_IOCTL_GET_TABLE, (void *)diskTable[i]);
        if (diskDriverTlb) {
            scsi_media_table_mst[i].driver_init = diskDriverTlb->initfunc;
            scsi_media_table_mst[i].param = diskDriverTlb->param;
            scsi_media_table_mst[i].iso = 0;
        }
        else
            LOG_ERR "[usbd][mst] get disk%d's driver table fail! \n", diskTable[i] LOG_END
    }

    return;
}

static int UsbdMstIoctl(int file, unsigned long request, void* ptr, void* info)
{
    switch (request)
    {
    case ITP_IOCTL_INIT:
        UsbdMstInit();
        break;

    case ITP_IOCTL_IS_CONNECTED:
        return g_connected;

    default:
        errno = (ITP_DEVICE_USBDFSG << ITP_DEVICE_ERRNO_BIT) | 1;
        return -1;
    }
    return 0;
}


const ITPDevice itpDeviceUsbdFsg =
{
    ":usbd fsg",
    itpOpenDefault,
    itpCloseDefault,
    itpReadDefault,
    itpWriteDefault,
    itpLseekDefault,
    UsbdMstIoctl,
    NULL
};