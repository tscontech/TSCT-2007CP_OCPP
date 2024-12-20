#include <stdio.h>
#include "flower/flower.h"
#include "flower/fliter_priv_def.h"
#include "ite/itv.h"
#include "ite/ith.h"


//=============================================================================
//                              Constant Definition
//=============================================================================
//=============================================================================
//                              Private Function Declaration
//=============================================================================

static void displaycam_init(IteFilter *f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
	
}

static void displaycam_uninit(IteFilter *f)
{
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);
}

static void displaycam_process(IteFilter *f)
{
	IteQueueblk 			  rec_blk0 = {0};
	IteSensorStream*          sensorinfo    = NULL;
	ITV_DBUF_PROPERTY         dispProp = {0};
	char *dbuf = NULL;
	unsigned char   index = 0;
    DEBUG_PRINT("[%s] Filter(%d)\n", __FUNCTION__, f->filterDes.id);

    while(f->run) {
        //printf("[%s] Filter(%d). thread run\n", __FUNCTION__, f->filterDes.id);
        if(ite_queue_get(f->input[0].Qhandle, &rec_blk0) == 0) {

			sensorinfo = (IteSensorStream*) rec_blk0.private1;
		
		    dbuf    = itv_get_dbuf_anchor();
			
            if (dbuf != NULL);
            {
                dispProp.src_w    = sensorinfo->Width;
                dispProp.src_h    = sensorinfo->Height;
                dispProp.ya       = sensorinfo->DataAddrY;
                dispProp.ua       = sensorinfo->DataAddrU;
                dispProp.va       = sensorinfo->DataAddrV;
                dispProp.pitch_y  = sensorinfo->PitchY;
                dispProp.pitch_uv = sensorinfo->PitchUV;
                dispProp.format   = MMP_ISP_IN_NV12;
				itv_update_dbuf_anchor(&dispProp);
            }
			free(sensorinfo);
	            
        }				
        usleep(1000);
    }
	

    DEBUG_PRINT("[%s] Filter(%d) end\n", __FUNCTION__, f->filterDes.id);
    
    //return NULL;
}

static IteMethodDes FilterDisplayCamera_methods[] = {

    {0, NULL}
};

IteFilterDes FilterDisplayCamera = {
    ITE_FILTER_DISPLAY_CAM_ID,
    displaycam_init,
    displaycam_uninit,
    displaycam_process,
    FilterDisplayCamera_methods
};



