/***************************************************************************
 *
 *            Copyright (c) 2010-2018 by HCC Embedded
 *
 * This software is copyrighted by and is the sole property of
 * HCC.  All rights, title, ownership, or other interests
 * in the software remain the property of HCC.  This
 * software may only be used in accordance with the corresponding
 * license agreement.  Any unauthorized use, duplication, transmission,
 * distribution, or disclosure of this software is expressly forbidden.
 *
 * This Copyright notice may not be removed or modified without prior
 * written consent of HCC.
 *
 * HCC reserves the right to modify this software without notice.
 *
 * HCC Embedded
 * Budapest 1133
 * Vaci Ut 76
 * Hungary
 *
 * Tel:  +36 (1) 450 1302
 * Fax:  +36 (1) 450 1303
 * http: www.hcc-embedded.com
 * email: info@hcc-embedded.com
 *
 ***************************************************************************/
#include "usbhcc/api/api_mdriver.h"
#include "usbhcc/api/api_mdriver_mst.h"
#include "usbhcc/api/api_scsi.h"
#include "usbhcc/config/config_scsi.h"
#include "usbhcc/psp/include/psp_string.h"

#include "usbhcc/version/ver_mdriver_mst.h"
#if VER_MDRIVER_MST_MAJOR != 1 || VER_MDRIVER_MST_MINOR != 3
 #error Incompatible MDRIVER_MST version number!
#endif
#include "usbhcc/version/ver_usbh_mst.h"
#if VER_USBH_MST_MAJOR != 3
 #error Incompatible SCSI version number!
#endif
#include "usbhcc/version/ver_psp_string.h"
#if VER_PSP_STRING_MAJOR != 1
 #error Incompatible PSP_STRING version number!
#endif

static F_DRIVER  mst_driver[SCSI_MAX_UNITS * SCSI_MAX_LUN];

#define LOW_LEVEL_PERFORMANCE
#if defined(LOW_LEVEL_PERFORMANCE)
F_DRIVER *g_mst_driver = NULL;
#endif

/****************************************************************************
 *
 * mst_readmultiplesector
 *
 * read multiple sectors from MST
 *
 * INPUTS
 *
 * driver - driver structure
 * data - data pointer where to store data
 * sector - where to read data from
 * cnt - number of sectors to read
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/
static int mst_readmultiplesector ( F_DRIVER * driver, void * data, unsigned long sector, int cnt )
{
  int  rc;

  if ( scsi_read( (unsigned char)driver->user_data, sector, (unsigned long)cnt, data ) )
  {
    rc = MST_ERROR;
  }
  else
  {
    rc = MST_NO_ERROR;
  }

  return rc;
}


/****************************************************************************
 * Read one sector
 ***************************************************************************/
static int mst_readsector ( F_DRIVER * driver, void * data, unsigned long sector )
{
  int  rc;

  if ( scsi_read( (unsigned char)driver->user_data, sector, 1, data ) )
  {
    rc = MST_ERROR;
  }
  else
  {
    rc = MST_NO_ERROR;
  }

  return rc;
}


/****************************************************************************
 *
 * mst_writemultiplesector
 *
 * write multiple sectors into MST
 *
 * INPUTS
 *
 * driver - driver structure
 * data - data pointer
 * sector - where to write data
 * cnt - number of sectors to write
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/
static int mst_writemultiplesector ( F_DRIVER * driver, void * data, unsigned long sector, int cnt )
{
  int  rc;

  if ( scsi_write( (unsigned char)driver->user_data, sector, (unsigned long)cnt, data ) )
  {
    rc = MST_ERROR;
  }
  else
  {
    rc = MST_NO_ERROR;
  }

  return rc;
}


/****************************************************************************
 * Write one sector
 ***************************************************************************/
static int mst_writesector ( F_DRIVER * driver, void * data, unsigned long sector )
{
  int  rc;

  if ( scsi_write( (unsigned char)driver->user_data, sector, 1, data ) )
  {
    rc = MST_ERROR;
  }
  else
  {
    rc = MST_NO_ERROR;
  }

  return rc;
}


/****************************************************************************
 *
 * mst_getphy
 *
 * determinate MST physicals
 *
 * INPUTS
 *
 * driver - driver structure
 * phy - this structure has to be filled with physical information
 *
 * RETURNS
 *
 * error code or zero if successful
 *
 ***************************************************************************/
static int mst_getphy ( F_DRIVER * driver, F_PHY * phy )
{
  t_lun_info  info;
  int         rc = MST_ERROR;

  if ( scsi_get_lun_info( (unsigned char)driver->user_data, &info ) == SCSI_SUCCESS )
  {
    phy->number_of_sectors = info.no_of_sectors;
    phy->bytes_per_sector = (unsigned short)info.sector_size;
    rc = MST_NO_ERROR;
  }

  return rc;
}


/****************************************************************************
 *
 * mst_release
 *
 * Releases a drive
 *
 * INPUTS
 *
 * driver_param - driver parameter
 *
 ***************************************************************************/
static void mst_release ( F_DRIVER * driver )
{
  (void)scsi_release( (uint8_t)driver->user_data );
}


/****************************************************************************
 *
 * mst_ioctl
 *
 * Runs an IOCTL call
 *
 * INPUTS
 *
 * driver_param - driver parameter
 *
 ***************************************************************************/
static int mst_ioctl ( F_DRIVER * driver, unsigned long msg, void * iparam, void * oparam )
{
  int  rc;

  HCC_UNUSED_ARG( iparam );
  HCC_UNUSED_ARG( oparam );

  rc = MST_ERROR;

  if ( msg == F_IOCTL_MSG_FLUSH )
  {
    rc = scsi_flush( (uint8_t)driver->user_data );
    if ( rc == SCSI_SUCCESS )
    {
      rc = MST_NO_ERROR;
    }
  }

  return rc;
}



/****************************************************************************
 *
 * mst_getstatus
 *
 * get status of card, missing or/and removed,changed,writeprotect
 *
 * INPUTS
 *
 * driver - driver structure
 *
 * RETURNS
 *
 * F_ST_xxx code for high level
 *
 ***************************************************************************/
static long mst_getstatus ( F_DRIVER * driver )
{
  int   st;
  long  rc = 0;

  st = scsi_get_unit_state( (unsigned char)driver->user_data );
  if ( st & SCSI_ST_CONNECTED )
  {
    if ( st & SCSI_ST_CHANGED )
    {
      rc |= F_ST_CHANGED;
    }

    if ( st & SCSI_ST_WRPROTECT )
    {
      rc |= F_ST_WRPROTECT;
    }
  }
  else
  {
    rc |= F_ST_MISSING;
  }

  return rc;
} /* mst_getstatus */



/****************************************************************************
 *
 * f_mstglueinit
 *
 * this init function has to be passed for highlevel to initiate the
 * driver functions
 *
 * INPUTS
 *
 * driver_param - driver parameter
 *
 * RETURNS
 *
 * driver structure pointer
 *
 ***************************************************************************/
F_DRIVER * mst_initfunc ( unsigned long driver_param )
{
  F_DRIVER * driver = mst_driver + driver_param;

  (void)psp_memset( driver, 0, sizeof( F_DRIVER ) );
  driver->readsector = mst_readsector;
  driver->writesector = mst_writesector;
  driver->readmultiplesector = mst_readmultiplesector;
  driver->writemultiplesector = mst_writemultiplesector;
  driver->getstatus = mst_getstatus;
  driver->getphy = mst_getphy;
  driver->release = mst_release;
  driver->ioctl = mst_ioctl;
  driver->user_data = driver_param;
#if defined(LOW_LEVEL_PERFORMANCE)
  g_mst_driver = driver;
#endif
  return driver;
}


