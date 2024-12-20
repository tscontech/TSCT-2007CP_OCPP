#include <string.h>
#include "ite/itp.h"
#include "usbhcc/api/api_scsi_tgt.h"


/** from config_scsi_tgt.h */
uint8_t N_LUNS_MST = 0;

/** from config_scsi_tgt.c */
scsim_table_entry_t  scsi_media_table_mst[1] = { 0 };

