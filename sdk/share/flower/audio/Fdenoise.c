#include <stdio.h>
#include "flower/flower.h"
#include "include/audioqueue.h"

//=============================================================================
//                              Constant Definition
//=============================================================================
//#ifdef CFG_BUILD_ASR
//#define RUN_IN_ARMLITE
//#endif

#ifdef RUN_IN_ARMLITE
#include "ite/audio.h"
#else
#include "type_def.h"
#include "aecm_core.h"
#include "basic_op.h"
#include "hd_aec.h"
#include "rfft_256.h"
#include "dft_filter_bank.h"
#endif

//#define DENOISE_DUMP_ITE

#ifdef DENOISE_DUMP_ITE
#include "ite/itp.h"
static char *Get_Storage_path(void)
{
    ITPDriveStatus* driveStatusTable;
    ITPDriveStatus* driveStatus = NULL;
    int i;

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    for (i = 0; i < ITP_MAX_DRIVE; i++)
    {
        driveStatus = &driveStatusTable[i];
		 if (driveStatus->disk >= ITP_DISK_MSC00 && driveStatus->disk <= ITP_DISK_MSC17)
        {
            if (driveStatus->avail )
            {
                printf("USB #%d inserted: %s\n", driveStatus->disk - ITP_DISK_MSC00, driveStatus->name[0]);
                return driveStatus->name[0];
            }
        }
    }
    return NULL;
} 
#endif

//=============================================================================
//                              Private Function Declaration
//=============================================================================

typedef struct Denoisestate{
    IteFilter *msF;
    rbuf_ite *Buf;
    int framesize;
	int samplerate;
	
    #ifdef DENOISE_DUMP_ITE
    mblkq in_copy_q;
    mblkq out_copy_q;
    #endif

}Denoisestate;

static inline int16_t saturate(int val) {
	return (val>32767) ? 32767 : ( (val<-32767) ? -32767 : val);
}

static void denoise_init(IteFilter *f){
    Denoisestate *s=(Denoisestate *)ite_new(Denoisestate,1);
	f->data=s;
	s->samplerate = CFG_AUDIO_SAMPLING_RATE;
    s->framesize = 128;
    s->Buf =ite_rbuf_init(s->framesize*30);//maximum :48K 1 channel 40ms buffer data
	
#ifdef RUN_IN_ARMLITE
    iteAudioOpenEngine(ITE_SBC_CODEC);
#endif

   #ifdef RUN_IN_ARMLITE
   printf("denoise in ARM-LITE\n");
   iteAecCommand(NR_CMD_INIT, 0, 0, 0, 0, 0);	
   #else 
   //NR_Create(&anr_config[0], 1);
   /*default UniForm_Filt_Bank_Create(&drc_config[0],25,7,19161,2757)*/
   UniForm_Filt_Bank_Create(&drc_config[0],25,7,19161,2757);
   #endif
   s->msF = f;

}



static void denoise_uninit(IteFilter *f)
{
    Denoisestate *s=(Denoisestate*)f->data;
	ite_rbuf_flush(s->Buf);

    UniForm_Filt_Bank_Destroy(&drc_config[0]); 
	ite_rbuf_free(s->Buf);
    free(s);
}

/*
input[0]:speaker data : play to local data
input[1]:Line out     : send to far end data
*/
static void denoise_process(IteFilter *f){
    Denoisestate *s=(Denoisestate*)f->data;
	IteQueueblk blk ={0};
	int ret=0;
    int nbytes=s->framesize*2;
	unsigned char buffer[256];

	#ifdef DENOISE_DUMP_ITE
    mblkqinit(&s->in_copy_q);
    mblkqinit(&s->out_copy_q);
    #endif
	
	while(f->run){
        
        IteAudioQueueController(f,0,30,5);
		
		if(ite_queue_get(f->input[0].Qhandle, &blk) == 0){
			mblk_ite *om=blk.datap;
            ret=ite_rbuf_put(s->Buf,om->b_rptr,om->size);
            if(om) freemsg_ite(om);

			while(ite_rbuf_get(buffer,s->Buf,nbytes)){
				mblk_ite *out;
				out = allocb_ite(nbytes);
				
				#ifdef DENOISE_DUMP_ITE
				mblk_ite *in;
				in = allocb_ite(nbytes);
				#endif
			
				#ifdef RUN_IN_ARMLITE
           	 	iteAecCommand(NR_CMD_PROCESS,(unsigned int) buffer,0,(unsigned int) out->b_wptr,nbytes,0);    
            	#else
            	//Overlap_Add((Word16*)in->b_rptr,(Word16*)out->b_wptr,&anr_config[0]);
				Uniform_Filt_Bank(buffer, out->b_wptr, &drc_config[0], s->framesize);
            	#endif

				out->b_wptr += nbytes;

				#ifdef DENOISE_DUMP_ITE
				memcpy(in->b_wptr, buffer, nbytes);
				in->b_wptr += nbytes;
				
	            putmblkq(&s->in_copy_q, dupmblk(in));
	            putmblkq(&s->out_copy_q, dupmblk(out));
				
				if(in) freemsg_ite(in);
	            #endif
				
				blk.datap = out;
                ite_queue_put(f->output[0].Qhandle, &blk);
			}
		}
	}
    
    ite_mblk_queue_flush(f->input[0].Qhandle);
    ite_mblk_queue_flush(f->output[0].Qhandle);


	#ifdef DENOISE_DUMP_ITE
{
    FILE *infile;
    FILE *outfile;
    static int index = 0;
    mblk_ite *in,*out;
    char fname[128];
    char USBPATH = 'A';
    if(USBPATH == NULL){
        printf("USB not insert \n");
    }else{
        printf("save audio data in USB %c:/ \n",USBPATH);
        sprintf(fname,"%c:/input%03d.raw",USBPATH,index);
        infile=fopen(fname,"w");
        sprintf(fname,"%c:/out%03d.raw",USBPATH,index);
        outfile=fopen(fname,"w");
    }

    while (1)
    {
        in=out=NULL;
        in=getmblkq(&s->in_copy_q);
        out=getmblkq(&s->out_copy_q);
        if (in && out)
        {
            fwrite(in->b_rptr,nbytes,1,infile);
            freemsg_ite(in);            
            fwrite(out->b_rptr,nbytes,1,outfile);
            freemsg_ite(out);
        }
        else
        {
            flushmblkq(&s->in_copy_q);
            flushmblkq(&s->out_copy_q);
            fclose(infile);
            fclose(outfile);
            break;
        }
    }    
	index++;
}
    #endif
	return NULL;
}



static IteMethodDes denoise_methods[] = {
    {0, NULL}
};


IteFilterDes FilterDenoise = {
    ITE_FILTER_DENOISE_ID,
    denoise_init,
    denoise_uninit,
    denoise_process,
    denoise_methods
};

