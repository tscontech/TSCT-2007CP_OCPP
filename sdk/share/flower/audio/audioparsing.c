#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//================ mp3 =======================


/* indexing = [version][samplerate index]
 * sample rate of frame (Hz)
 */
const static int mp3samplerateTab[3][3] = 
{
    {44100, 48000, 32000},        /* MPEG-1 */
    {22050, 24000, 16000},        /* MPEG-2 */
    {11025, 12000,  8000},        /* MPEG-2.5 */
};

/* indexing = [version][layer]
 * number of samples in one frame (per channel)
 */
const static int samplesPerFrameTab[3][3] = 
{
    {384, 1152, 1152 }, /* MPEG1 */
    {384, 1152,  576 }, /* MPEG2 */
    {384, 1152,  576 }, /* MPEG2.5 */
};

/* indexing = [version][sampleRate][bitRate]
 * for layer3, nSlots = floor(samps/frame * bitRate / sampleRate / 8)
 *   - add one pad slot if necessary
 */
const static int slotTab[3][3][15] = 
{
    {
        /* MPEG-1 */
        { 0, 104, 130, 156, 182, 208, 261, 313, 365, 417, 522, 626, 731, 835,1044 },    /* 44 kHz */
        { 0,  96, 120, 144, 168, 192, 240, 288, 336, 384, 480, 576, 672, 768, 960 },    /* 48 kHz */
        { 0, 144, 180, 216, 252, 288, 360, 432, 504, 576, 720, 864,1008,1152,1440 },    /* 32 kHz */
    },
    {
        /* MPEG-2 */
        { 0,  26,  52,  78, 104, 130, 156, 182, 208, 261, 313, 365, 417, 470, 522 },    /* 22 kHz */
        { 0,  24,  48,  72,  96, 120, 144, 168, 192, 240, 288, 336, 384, 432, 480 },    /* 24 kHz */
        { 0,  36,  72, 108, 144, 180, 216, 252, 288, 360, 432, 504, 576, 648, 720 },    /* 16 kHz */
    },
    {
        /* MPEG-2.5 */
        { 0,  52, 104, 156, 208, 261, 313, 365, 417, 522, 626, 731, 835, 940,1044 },    /* 11 kHz */
        { 0,  48,  96, 144, 192, 240, 288, 336, 384, 480, 576, 672, 768, 864, 960 },    /* 12 kHz */
        { 0,  72, 144, 216, 288, 360, 432, 504, 576, 720, 864,1008,1152,1296,1440 },    /*  8 kHz */
    },
};

/* indexing = [version][layer][bitrate index]
 * bitrate (kbps) of frame
 *   - bitrate index == 0 is "free" mode (bitrate determined on the fly by
 *       counting bits between successive sync words)
 */
const static int bitrateTab[3][3][15] = 
{
    {
        /* MPEG-1 */
        {  0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448}, /* Layer 1 */
        {  0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384}, /* Layer 2 */
        {  0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320}, /* Layer 3 */
    },
    {
        /* MPEG-2 */
        {  0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256}, /* Layer 1 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, /* Layer 2 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, /* Layer 3 */
    },
    {
        /* MPEG-2.5 */
        {  0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256}, /* Layer 1 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, /* Layer 2 */
        {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}, /* Layer 3 */
    },
};

//================ aac =======================

#define AACSYNCWORDH           0xff
#define AACSYNCWORDL           0xf0

#define NUM_AAC_SAMPLE_RATES    12
#define MAX_AAC_FRAME_LENGTH 1500

/* aac sample rates (table 4.5.1) */
const int aacsampRateTab[NUM_AAC_SAMPLE_RATES] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025,  8000
};

//============================================

//============ wav ==========================
typedef signed char             int8_t;
typedef short                   int16_t;
typedef long                    int32_t;
typedef unsigned char           uint8_t;
typedef unsigned short          uint16_t;
typedef unsigned long           uint32_t;
typedef long long               int64_t;
typedef unsigned long long      uint64_t;

typedef struct _riff_t {
	char riff[4] ;	/* "RIFF" (ASCII characters) */
    unsigned long  len ;	/* Length of package (binary, little endian) */
	char wave[4] ;	/* "WAVE" (ASCII characters) */
} riff_t;

typedef struct _format_t {
	char  fmt[4];		/* "fmt_" (ASCII characters) */
	unsigned long   len ;	/* length of FORMAT chunk (always 0x10) */
	unsigned short  type;		/* codec type*/
	unsigned short  channel ;	/* Channel numbers (0x01 = mono, 0x02 = stereo) */
	unsigned long   rate ;	/* Sample rate (binary, in Hz) */
	unsigned long   bps ;	/* Average Bytes Per Second */
 	unsigned short  blockalign ;	/*number of bytes per sample */
	unsigned short  bitpspl ;	/* bits per sample */
} format_t;

/* The DATA chunk */

typedef struct _data_t {
	char data[4] ;	/* "data" (ASCII characters) */
	int  len ;	/* length of data */
} data_t;

typedef struct _wave_header_t
{
	riff_t riff_chunk;
	format_t format_chunk;
	data_t data_chunk;
} wave_header_t;
//================================================


#define PARSING_BUFFER_SIZE 80*1024
#define DATA_READ_SIZE 64 * 1024

// parsing buffer read index
static int parsingRIdx=0;
// parsing buffer write index
static int parsingWIdx=0;
static char parsingBuffer[PARSING_BUFFER_SIZE];
static int gnSamplePerFrame = 0;
static int gnSampleRate = 0;
static int gnSampleRateIndex = 0;
static int gnInitSampleRate =0;
static int gnFrames = 0;
static int sampleRateHistory[4][2];
static int samplePerFrameHistory[3];


static int parsing_audio_init()
{

    gnSampleRate = 0;
    gnSamplePerFrame = 0;

    return 0;
}

static int parsing_data_init()
{
    int nTemp ;
    parsingRIdx = 0;
    parsingWIdx = 0;
    memset(parsingBuffer,0,PARSING_BUFFER_SIZE);
    memset(sampleRateHistory,0,sizeof(sampleRateHistory));
    memset(samplePerFrameHistory,0,sizeof(samplePerFrameHistory));
    gnFrames = 0;
    gnSampleRateIndex = 0;
    parsing_audio_init();
    return 0;
}

static int parsing_audio_check(int samplerate)
{
    int i=0,j=0;    
    
    if (384 == gnSamplePerFrame){
        samplePerFrameHistory[0]++;
    } else if (1152 == gnSamplePerFrame){
        samplePerFrameHistory[1]++;
    } else if (576 == gnSamplePerFrame){
        samplePerFrameHistory[2]++;
    }

    if (samplerate==gnSampleRate){
        if (sampleRateHistory[0][0]==0){
            sampleRateHistory[0][0] =gnSampleRate;
            sampleRateHistory[0][1] =1;
            //printf("init sample rate %d \n",sampleRateHistory[0][0]);
            gnSampleRateIndex=1;
            gnInitSampleRate = gnSampleRate;
            return 0;
        }
            
        for (j=0 ;j<4;j++){            
             if (sampleRateHistory[j][0] == gnSampleRate){
                sampleRateHistory[j][1]++;
                break;
            } else if (sampleRateHistory[j][0] == 0){
                sampleRateHistory[gnSampleRateIndex][0] = gnSampleRate; 
                sampleRateHistory[gnSampleRateIndex][1] = 1; 
                //printf("add index %d sample rate %d \n",j,sampleRateHistory[gnSampleRateIndex][0]);
                if (gnSampleRateIndex<4)
                    gnSampleRateIndex++;
                break;
            } 
        }
        return 0;
    } else {
        //printf("parsing_audio_check sample rate diff %d,%d \n",samplerate,gnSampleRate);
        return 1;
    }
}



static int parsing_mp3(char* parsing_buf,int size,int* pParsingSize)
{
    int i=0;
    int layer;
    int brIdx;
    int srIdx;
    int id;
    int ver;
    int paddingBit;
    int bitRate;
    int sampling_rate;
    int samplesperframe;
    int frameLength=0;
    int frameCounts = 0;

    for(i=0; i+2<size; i++) {
        if (!( (char)parsing_buf[i] == (char)0xff && ((parsing_buf[i+1] & 0xe0) == 0xe0) )) {    
            continue;
        }        
        
        layer = 4 - ((parsing_buf[i+1] >> 1) & 0x3);
        brIdx = (parsing_buf[i+2] >> 4) & 0xf;
        srIdx = (parsing_buf[i+2] >> 2) & 0x3;

        if (srIdx == 3 || layer == 4 || brIdx == 15){
            continue; // illegal frame
        }

        /* keep frame information */
        
        id  = (parsing_buf[i+1] >> 3) & 0x3;
        ver = (id == 0 ? 2 : (id & 0x1 ? 0 : 1));
        paddingBit = (parsing_buf[i+2]  >> 1) & 0x01;                
        bitRate;
        //int crc = (buffer[i+1] & 0x1);

        sampling_rate = mp3samplerateTab[ver][srIdx];
        samplesperframe = samplesPerFrameTab[ver][layer-1];
        if (layer ==3) {
            frameLength = slotTab[ver][srIdx][brIdx]+ paddingBit;
        } else if (layer ==2) {
            bitRate = bitrateTab[ver][layer-1][brIdx]*1000;
            if (sampling_rate)
                frameLength = (144 * bitRate / sampling_rate) + paddingBit;
        } else if (layer == 1) {
            bitRate = bitrateTab[ver][layer-1][brIdx]*1000;                    
            frameLength = ((12 * bitRate / sampling_rate) + paddingBit)*4;
        }

        if (gnSampleRate == 0) {
            gnSampleRate = sampling_rate;
            gnSamplePerFrame = samplesperframe;
        } else {
            if (parsing_audio_check(sampling_rate)){
                parsing_audio_init();
                continue;
            }
        }
        if (frameLength == 0){
           continue;
        }

        *pParsingSize = i;
        i += frameLength-1 ;
        
        (frameCounts)++;
        
    }
    if (*pParsingSize==0){
        *pParsingSize = size;
    } 
    return frameCounts;
}


static int parsing_aac(char* parsing_buf,int size,int* pParsingSize)
{
    int i=0;
    int sampling_rate;
    int samplesperframe;
    int frameLength=0;
    int frameCounts=0;
    int nTemp=0;

    for(i=0; i+2<size; i++) {

       
        if (!(((parsing_buf[i+0] & AACSYNCWORDH)== AACSYNCWORDH)&&((parsing_buf[i+1] & AACSYNCWORDL) == AACSYNCWORDL)))
        {
        	
            continue;
        }

        if( ((parsing_buf[i+1] & 0x06) >> 1) != 0x0 )
            continue;

        // Sample rate index
        if( ((parsing_buf[i+2] & 0x3c) >> 2) >= NUM_AAC_SAMPLE_RATES ) {
            continue;
        }

        frameLength = (((parsing_buf[i+3] << 16) + (parsing_buf[i+4] <<8) + (parsing_buf[i+5])) >> 5) & 0x1fff;
		
        if(frameLength > MAX_AAC_FRAME_LENGTH)
            continue;      
    
        nTemp = (parsing_buf[i+2] & 0x3c) >> 2;
        sampling_rate = aacsampRateTab[nTemp];
		printf("sampling_rate %d\n", sampling_rate);
        if (frameLength == 0){
           continue;
        }

        if (gnSampleRate == 0) {
            gnSampleRate = sampling_rate;
            gnSamplePerFrame = 1024;
        } else {
            if (parsing_audio_check(sampling_rate)){
                parsing_audio_init();
                continue;
            }
        }

        *pParsingSize = i;
        i += frameLength-1 ;
        
        (frameCounts)++;
    
    }
    if (*pParsingSize==0){
        *pParsingSize = size;
    }
    return frameCounts;
}

static int parsing_wav(FILE* fp)
{
	double wavtime;
    char header1[sizeof(riff_t)];
    char header2[sizeof(format_t)];
    char header3[sizeof(data_t)];
    
    riff_t *riff_chunk=(riff_t*)header1;
    format_t *format_chunk=(format_t*)header2;
    data_t *data_chunk=(data_t*)header3;
    
    int count;
        
    fread(header1,sizeof(header1),1,fp);

    if (0!=strncmp(riff_chunk->riff, "RIFF", 4) || 0!=strncmp(riff_chunk->wave, "WAVE", 4)){
        printf("RIFF WAVE head error\n");
        goto not_a_wav;
    }else{
          fseek(fp, sizeof(riff_t), SEEK_SET);
    }
    
    fread(header2, sizeof(header2),1,fp) ;
    if(0!=strncmp(format_chunk->fmt,"fmt ",4)){
        printf("fmt head error\n");
        goto not_a_wav;
    }
    
    if (format_chunk->len-0x10>0)
    {
        fseek(fp,(format_chunk->len-0x10),SEEK_CUR);
    }
    fread(header3, sizeof(header3),1,fp) ;

    count=0;
    while (strncmp(data_chunk->data, "data", 4)!=0 && count<30)
    {
        printf("skipping chunk=%s len=%i\n", data_chunk->data, data_chunk->len);
        fseek(fp,data_chunk->len,SEEK_CUR);
        count++;
        fread(header3, sizeof(header3),1,fp) ;
    }    
    
    wavtime = (double)data_chunk->len/(double)format_chunk->bps;
    return wavtime;
    
    not_a_wav:
        printf("not a wav\n");
        return 0;
}


static int parsing_data(char* pBuf,int nBufSize,int nType)
{
    int nParsingSize=0;
    int nFrame =0;
    // prepare parsing buffer
    if (parsingWIdx>sizeof(parsingBuffer)){
        parsingWIdx = 0;
        return 0;
    } 
    if (parsingWIdx+nBufSize > sizeof(parsingBuffer)){
        parsingWIdx = (sizeof(parsingBuffer) - nBufSize);
    
    }
    memcpy(&parsingBuffer[parsingWIdx],pBuf,nBufSize);
    parsingWIdx+=nBufSize;

    if (parsingWIdx>sizeof(parsingBuffer)){
        parsingWIdx = 0;
    }

    //
    switch(nType)
    {
        // parsing mp3
        case 1:
            nFrame = parsing_mp3(parsingBuffer,parsingWIdx,&nParsingSize);
            break;

        // parsing aac
        case 2:
            //nFrame = parsing_aac(parsingBuffer,parsingWIdx,&nParsingSize);
            break;

        default:
            break;
    }
    
    // copy unparsing data to parsing buffer
    if (parsingWIdx-nParsingSize>0){
    memcpy(parsingBuffer,&parsingBuffer[parsingRIdx],parsingWIdx-nParsingSize);
    } else {
        return nFrame;
    }
    // reset read,write pointer
    parsingRIdx = 0;
    parsingWIdx = parsingWIdx-nParsingSize;
    // reset part of parsing buffer
    if (nBufSize-parsingWIdx>0){
    memset(&parsingBuffer[parsingWIdx],0,nBufSize-parsingWIdx);
    } else {
        return nFrame;
    }
    gnFrames += nFrame;
    return nFrame;
}

static int audio_get_type(char* filename)
{
    char*                    ext;

    ext = strrchr(filename, '.');
    if (!ext)
    {
        printf("Invalid file name: %s\n", filename);
        return -1;
    }
    ext++;

    if (stricmp(ext, "mp3") == 0)
    {
        return 1;
    }
    
    else if (stricmp(ext, "aac") == 0 || stricmp(ext, "m4a") == 0)
    {
        return 2;
    }    
    else if (stricmp(ext, "wav") == 0 )
    {
        return 3;
    }
    else
    {
        printf("Unsupport file format: %s\n", ext);
        return -1;
    }
    return -1;
}


void parsingMp3IsID3v2(FILE *fd,int *tag){
    char pbuf[1024];
    int rbytes=fread(pbuf,1,1024,fd);
    *tag=0;
    if (memcmp("ID3", pbuf, 3) == 0)
    {
        *tag    = (pbuf[6] << 21) | (pbuf[7] << 14) | (pbuf[8] << 7) | (pbuf[9] << 0);
        *tag    += 10;
        fseek(fd,*tag,SEEK_SET);
        printf("idV3tag (%d)\n",*tag);
    }
}


int audioGetTotalTime(char* filename)
{
    int     nSize   = 0;
    int     done    = 0;
    int     nTemp   = 0;
    char    readBuffer[DATA_READ_SIZE];
    
    FILE    *finput = NULL;
    nSize = DATA_READ_SIZE;
	int totalTime = 0;
	int type = -1;

    if ((finput = fopen(filename, "rb")) == NULL)
    {
        printf("Can not open file \n");
        return -1;
    }

	type = audio_get_type(filename);
	if(type!=1 && type!=3)
		return 0;
	
	parsing_data_init();

	switch(type){
		case 1://mp3
		case 2://aac, not support now
		{
			do
		    {
		        // read input audio
		        nSize = fread(readBuffer, 1, DATA_READ_SIZE, finput);
		        if (nSize != DATA_READ_SIZE)
		        {
		            printf("eof \n");
		            done = 1;
		        }

		        nTemp += parsing_data(readBuffer, nSize, type);
		        usleep(1000);
		    } while (!done);
		    
		    if(done){
		        if(gnSampleRate != 0){
					totalTime = (gnFrames*gnSamplePerFrame)/gnSampleRate;//second
					printf("gnFrames %d, gnSamplePerFrame %d, gnSampleRate %d\n",gnFrames, gnSamplePerFrame, gnSampleRate);
		        }
				else
					printf("parsing error!!\n");
		    }
			
		    fclose(finput);
			break;
		}
		case 3://wav
		{
			totalTime = parsing_wav(finput);//seconds
			fclose(finput);
			break;
		}
		
	}

	return totalTime;
}


