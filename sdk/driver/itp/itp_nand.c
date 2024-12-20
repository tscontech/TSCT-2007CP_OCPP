/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * PAL NAND functions.
 *
 * @author Jim Tan
 * @version 1.0
 *
 *
 *
 */
#include <errno.h>
#include <malloc.h>
#include <string.h>

#include "itp_cfg.h"
#include "ite/ite_nand.h"
#include "nand/mmp_nand.h"

#if defined(CFG_NAND_RESERVED_SIZE) && (CFG_NAND_RESERVED_SIZE!=0)
    #define ENABLE_CHECK_RESERVED_AREA
    
    #ifndef CFG_UPGRADE_IMAGE_POS
    #define CFG_UPGRADE_IMAGE_POS 0
    #endif
    
    #if defined(CFG_NET_ETHERNET_MAC_ADDR_POS)
        #if (CFG_NET_ETHERNET_MAC_ADDR_POS>=CFG_UPGRADE_IMAGE_POS)
        #error "ERROR: CFG_NET_ETHERNET_MAC_ADDR_POS can't larger than CFG_UPGRADE_IMAGE_POS"
        #endif
    #endif
    
    #if (CFG_UPGRADE_IMAGE_POS>=CFG_NAND_RESERVED_SIZE)
    #error "ERROR: CFG_UPGRADE_IMAGE_POS can't larger than CFG_NAND_RESERVED_SIZE"
    #endif
#endif
/***************************************************************************

****************************************************************************/
#ifdef	CFG_SPI_NAND
#ifdef CFG_SPI_NAND_USE_AXISPI
	#include "ssp/mmp_axispi.h"
#else
#include "ssp/mmp_spi.h"
#endif

#ifdef	CFG_SPI_NAND_USE_SPI0
#define NAND_SPI_PORT	SPI_0
#endif

#ifdef	CFG_SPI_NAND_USE_SPI1
#define NAND_SPI_PORT	SPI_1
#endif

#ifdef CFG_SPI_NAND_USE_AXISPI
    #define NAND_SPI_PORT	SPI_0
    #define NAND_SPI_CLOCK  SPI_CLK_40M
#else

#ifndef NAND_SPI_PORT
    #error "ERROR: itp_nand.c MUST define the SPI NAND bus(SPI0 or SPI1)"
#endif

    #ifdef	CFG_SPI0_40MHZ_ENABLE
        #define NAND_SPI_CLOCK	SPI_CLK_40M
    #else
        #define NAND_SPI_CLOCK	SPI_CLK_20M
    #endif
#endif

#endif
	    	
extern int remap_image_posistion;
/********************
 * global variable *
 ********************/
static ITE_NF_INFO	itpNfInfo;
static uint8_t GoodBlkCodeMark[4]={'F','I', 'N','E'};

static int gFsMaxSec = 0;
static int gFsSecSize = 0;
/********************
 * private function *
 ********************/
static int FlushBootRom()
{
    char *BlkBuf=NULL;
    int  res=true;
	uint32_t  i,j;
	uint32_t  BakBlkNum;
    
    if(!itpNfInfo.BootRomSize)
    {
    	errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | 1;	//nand initial fail
    	LOG_ERR "nand block initial error: \n" LOG_END
    	goto end;
    }
    
    //allocate block buffer
    BlkBuf = malloc( itpNfInfo.PageInBlk * (itpNfInfo.PageSize + itpNfInfo.SprSize) );
    if(!BlkBuf)
    {
    	errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | 1;	//nand initial fail
    	LOG_ERR "nand block initial error: \n" LOG_END
    	goto end;
    }
    
    //calculate block number of boot ROM
	BakBlkNum = 0;
    
    for(i=0; i<BakBlkNum; i++)
    {
    	char *tmpbuf=BlkBuf;
    	
    	//read a block data from bak area(include spare data)
    	for(j=0; j<itpNfInfo.PageInBlk;j++)
    	{
    		if( iteNfRead(i, j, tmpbuf)!=true )				
    		{
    			errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | 1;
    			LOG_ERR "itp nand read block error: \n" LOG_END
    			res = -1;
    			goto end;
    		}
    		tmpbuf+=(itpNfInfo.PageSize + itpNfInfo.SprSize);
    	}
    	
    	//write to boot area a block size
    	tmpbuf=BlkBuf;
    	for(j=0;j<itpNfInfo.PageInBlk;j++)
    	{
    		if(iteNfWrite(i, j, tmpbuf, tmpbuf+itpNfInfo.SprSize)!=true )
    		{
    			errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | 1;	//nand initial fail
    			LOG_ERR "itp nand write block error: \n" LOG_END
    			res = -1;
    			goto end;
    		}
    		tmpbuf+=(itpNfInfo.PageSize + itpNfInfo.SprSize);
    	}    	
    }
     
end:
	return 	res;
}

/********************
 * public function *
 ********************/
static int NandOpen(const char* name, int flags, int mode, void* info)
{
    int blkSize = 0;
    int i, tmpDev = 0;
    uint32_t opCnt = 0;

    opCnt = itpNfInfo.openCnt;
    
    LOG_DBG "NandOpen: blk=%x, page=%x\n",itpNfInfo.PageInBlk,itpNfInfo.PageSize LOG_END 
    
    //check ITP NF device
    if(opCnt >= ITE_NF_OPEN_MAX)
    {
        LOG_ERR "ITP NAND open fail, over open!!\n" LOG_END
        return -1;
    }    
    
    for (i = 0; i < ITE_NF_OPEN_MAX; i++)
    {
        if (!(itpNfInfo.usedDev & (0x1 << i)))
        {
            //ithEnterCritical();            
            tmpDev = i;
            itpNfInfo.usedDev |= (0x1 << i);
            itpNfInfo.openCnt++;
            //ithExitCritical();
            break;
        }
    }
    
    itpNfInfo.blkGap[tmpDev] = 4;    
    if(mode == ITP_NAND_FTL_MODE)
    {
    	if(!gFsMaxSec || !gFsSecSize)   mmpNandGetCapacity((unsigned long*)&gFsMaxSec, (unsigned long*)&gFsSecSize);
        itpNfInfo.blkGap[tmpDev] = 0;
    }
    else
    {        
        if(mode == ITP_NAND_BLK_MODE)   itpNfInfo.blkGap[tmpDev] = 0;
        else    itpNfInfo.blkGap[tmpDev] = 4;
    }
    itpNfInfo.currMode[tmpDev] = mode;
    
    itpNfInfo.CurBlk[tmpDev] = 0;
    blkSize = itpNfInfo.PageInBlk*itpNfInfo.PageSize;
    if(!blkSize)
    {
        LOG_ERR "NAND Block size = 0!!\n" LOG_END
        return -1;
    }

#ifdef ENABLE_CHECK_RESERVED_AREA
    //to get reserved area BL/MAC/img position
    #ifdef CFG_NET_ETHERNET_MAC_ADDR_NAND
    if( CFG_NET_ETHERNET_MAC_ADDR_POS % blkSize )
    {
        LOG_ERR "CFG_NET_ETHERNET_MAC_ADDR_POS is not at the block boundary, (%x,%x)\n",CFG_NET_ETHERNET_MAC_ADDR_POS, blkSize LOG_END
        return -1;
    }

    itpNfInfo.blEndBlk = CFG_NET_ETHERNET_MAC_ADDR_POS/blkSize;
    #else
    itpNfInfo.blEndBlk = CFG_UPGRADE_IMAGE_POS/blkSize;
    #endif
    if( CFG_UPGRADE_IMAGE_POS % blkSize )
    {
        LOG_ERR "CFG_UPGRADE_IMAGE_POS is not at the block boundary, (%x,%x)\n",CFG_UPGRADE_IMAGE_POS, blkSize LOG_END
        return -1;
    }
    
    if( CFG_NAND_RESERVED_SIZE % blkSize )
    {
        LOG_ERR "CFG_NAND_RESERVED_SIZE is not at the block boundary, (%x,%x)\n",CFG_NAND_RESERVED_SIZE, blkSize LOG_END
        return -1;
    } 
    
    itpNfInfo.bootImgStartBlk = CFG_UPGRADE_IMAGE_POS/blkSize;
    itpNfInfo.bootImgEndBlk = CFG_NAND_RESERVED_SIZE/blkSize;
    LOG_DBG "reserved area, A1=0x%x, A2=0x%x, A3=0x%x(blks)\n",itpNfInfo.blEndBlk,itpNfInfo.bootImgStartBlk,itpNfInfo.bootImgEndBlk LOG_END
    
    #ifdef CFG_NET_ETHERNET_MAC_ADDR_NAND
    if(!itpNfInfo.blEndBlk)
        LOG_INFO "MacPos=0x%x, ImgPos=0x%x, bSize=0x%x\n",CFG_NET_ETHERNET_MAC_ADDR_POS,CFG_UPGRADE_IMAGE_POS,blkSize LOG_END
    #else
    if(!itpNfInfo.blEndBlk)
        LOG_INFO "MacPos=0x%x, ImgPos=0x%x, bSize=0x%x\n",0,CFG_UPGRADE_IMAGE_POS,blkSize LOG_END
    #endif
    
    return tmpDev;
#else
    //return fail if NAND has NO reserved size(Because it does NOT make sense)
    LOG_WARN "It's forbidden for NAND itp driver without reserved size\n" LOG_END
    return -1;
#endif

}

static int NandClose(int file, void* info)
{
    LOG_DBG "NandClose::%x, %x\n",itpNfInfo.PageInBlk,itpNfInfo.PageSize LOG_END   
    
    /* clear these parameters of LBA mode  */
    if(itpNfInfo.openCnt)
    {
        itpNfInfo.CurBlk[file] = 0;
        itpNfInfo.blkGap[file] = 0;  
        itpNfInfo.currMode[file] = 0;
        itpNfInfo.usedDev &= ~(0x1 << file);    
        itpNfInfo.openCnt--;        
    }
    
    //errno = ENOENT;
    //errno = (ITP_DEVICE_FAT << ITP_DEVICE_ERRNO_BIT) | ret;
    return 0;
}

static int NandRead(int file, char *ptr, int len, void* info)
{
    int  DoneLen = 0;
	uint32_t  blk = 0;
	uint32_t  pg = 0;
	uint32_t  doLen = 0;
    char *databuf;
	char *tmpbuf;
	char *srcbuf=ptr;
    int blkSize;
    uint32_t areaBoundary;

	//printf("itpNfRd::buf=%x, b=%x, l=%x!!\n",ptr,itpNfInfo.CurBlk,len);
	
    blkSize = itpNfInfo.PageInBlk*itpNfInfo.PageSize;
    
    if(itpNfInfo.currMode[file] == ITP_NAND_FTL_MODE)
    {
    	unsigned long sector,sCnt;
    	
    	sector = itpNfInfo.CurBlk[file] * (blkSize / gFsSecSize);
    	sCnt = len*(blkSize/gFsSecSize);
    	//printf("itpmmpRd:%x,%x,%x\n",sector,sCnt,ptr);
    	
    	if( mmpNandReadSector(sector, sCnt, ptr) == 0 )
    	{
    		DoneLen = len;
    		itpNfInfo.CurBlk[file] += DoneLen;
    	}
    	goto end;
    }    

    //check baundary of bootloader/Mac address/boot image
    //check current block index "tpNfInfo.CurBlk" for suring BL/Mac/image area
    //if "tpNfInfo.CurBlk" or "tpNfInfo.CurBlk+len" is over the boundary, then return fail
    
    if(blkSize)
    {
#ifdef ENABLE_CHECK_RESERVED_AREA    
        if(itpNfInfo.CurBlk[file] < itpNfInfo.blEndBlk)  
            areaBoundary = itpNfInfo.blEndBlk;
        else if(itpNfInfo.CurBlk[file] < itpNfInfo.bootImgStartBlk)
            areaBoundary = itpNfInfo.bootImgStartBlk;
        else 
            areaBoundary = itpNfInfo.bootImgEndBlk;
#else
        areaBoundary = itpNfInfo.CurBlk[file] + len;
#endif           
        if( (itpNfInfo.CurBlk[file] + (uint32_t)len) > (uint32_t)areaBoundary )
        {
            LOG_ERR "itpNfRead has NO enough space for reading, curB=%x, len=%x, boundary=%x\n",itpNfInfo.CurBlk[file] ,len,areaBoundary LOG_END
            printf( "itpNfRead has NO enough space for reading, curB=%x, len=%x, boundary=%x\n",itpNfInfo.CurBlk[file] ,len,areaBoundary );
            
#ifdef ENABLE_CHECK_RESERVED_AREA  
           if(areaBoundary==itpNfInfo.blEndBlk)
               printf("OUT of bootloader area!!\n");
           else if(areaBoundary==itpNfInfo.bootImgStartBlk)
               printf("OUT of MAC-address area!!\n");
           else if(areaBoundary==itpNfInfo.bootImgEndBlk)
               printf("OUT of boot-image area!!\n");
           else
               printf("UNKNOW condition::%x,%x\n",areaBoundary,itpNfInfo.bootImgEndBlk);
#endif   
            goto end;
        }    
    }
    else
    {
        LOG_ERR "itpNfRead Error, block size = 0!!\n" LOG_END
        goto end; 
    }

    if( !ptr || !len )
    {
    	errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;	//ptr or len error
    	LOG_ERR "ptr or len error: \n" LOG_END
    	goto end;
    }
    
    if(!itpNfInfo.NumOfBlk)
    {
    	errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;	//nand initial fail
    	LOG_ERR "nand block initial error: \n" LOG_END
    	goto end;
    }

    databuf = (char *)malloc(itpNfInfo.PageInBlk*itpNfInfo.PageSize);
    if(databuf==NULL)
    {
    	errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;	//out of memory:
        LOG_ERR "out of memory:\n" LOG_END
    	goto end;  	
    }
	
   	for(blk=itpNfInfo.CurBlk[file]; blk<areaBoundary; blk++)
   	{
#ifdef ENABLE_CHECK_RESERVED_AREA
        if( blk >= (uint32_t)areaBoundary )
        {
            LOG_ERR "itpNfRead impossible condition: out of boundary:blk=%d, Boundary=%d\n", blk, areaBoundary LOG_END
            printf( "itpNfRead impossible condition: out of boundary:blk=%d, Boundary=%d\n", blk, areaBoundary );
            //printf("ERROR:: NO enough space for nand writing data, %x, %x, %x, %x\n",blk,itpNfInfo.blEndBlk,itpNfInfo.bootImgStartBlk,itpNfInfo.bootImgEndBlk);
           
            itpNfInfo.CurBlk[file] = blk;   //set current block index
            goto end;
        }  
#endif        
        //check if "blk" a bad block
        if( iteNfIsBadBlockForBoot(blk) != true )
        {
            //true means "GOOD BLOCK"; !true means "BAD BLOCK"
            continue;
        }

		tmpbuf = databuf;
   		for( pg=0; pg<itpNfInfo.PageInBlk; pg++)
   		{ 
   			if( iteNfReadPart(blk, pg, tmpbuf, LL_RP_DATA)!=true )
   			{
   				errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;	//read fail
   				return -1;
   			}
   			tmpbuf += itpNfInfo.PageSize;
   		}

        if(blk)
        {
            memcpy((void*)srcbuf, (void*)(databuf + itpNfInfo.blkGap[file]), itpNfInfo.PageInBlk * itpNfInfo.PageSize - itpNfInfo.blkGap[file]);
            srcbuf += (itpNfInfo.PageInBlk * itpNfInfo.PageSize) - itpNfInfo.blkGap[file];
        }
        else
        {
            memcpy((void*)srcbuf, (void*)databuf, itpNfInfo.PageInBlk*itpNfInfo.PageSize);  		
            srcbuf+=(itpNfInfo.PageInBlk*itpNfInfo.PageSize);
        }

        DoneLen++;
        if(DoneLen>len)
        {
            LOG_ERR "itpNfRead impossible condition: DoneLen:%d, len:%d\n", DoneLen, len LOG_END
            itpNfInfo.CurBlk[file] = blk+1; 
            goto end;
        }
        
        if(DoneLen==len)    break;
   	}

   	//printf("itpNfRd:cBlk=%x, b=%x\n",itpNfInfo.CurBlk[file],blk);
   	itpNfInfo.CurBlk[file] = blk+1; 
    
end:
	if(databuf)
	{
		free(databuf);
		databuf = NULL;
	}	
    return DoneLen;
}

static int NandWrite(int file, char *ptr, int len, void* info)
{
    int  DoneLen = 0;
    char *DataBuf;
	char *tmpDataBuf;
	char *tmpSrcBuf = ptr;
    char *SprBuf= NULL;
    uint32_t blk,pg;
    int blkSize;
    uint32_t areaBoundary = 0;
    
    //printf("itpNfWt::buf=%x, b=%x, l=%x!!\n", ptr, itpNfInfo.CurBlk[file], len);
    if( !ptr || !len )
    {
    	errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;	//ptr or len error
    	LOG_ERR "ptr or len error: \n" LOG_END
    	goto end;
    }
    
    if(!itpNfInfo.NumOfBlk)
    {
    	errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;	//nand initial fail
    	LOG_ERR "nand block initial error: \n" LOG_END
    	goto end;
    }

    blkSize = itpNfInfo.PageInBlk*itpNfInfo.PageSize;
    
    if(blkSize) areaBoundary = CFG_NAND_RESERVED_SIZE/blkSize;  //(unit:block)
    else
    {        
        printf("NfErr::block size = 0!!\n");
        LOG_ERR "block size = 0:\n" LOG_END
	    goto end;  	
    }
        
    if(itpNfInfo.currMode[file] == ITP_NAND_FTL_MODE)
    {
    	unsigned long sector,sCnt;
    	
    	sector = itpNfInfo.CurBlk[file] * (blkSize / gFsSecSize);
    	sCnt = len*(blkSize/gFsSecSize);
    	//printf("itpmmpWt:%x,%x,%x\n",sector,sCnt,ptr);
    	if( mmpNandWriteSector(sector, sCnt, ptr) == 0 )
    	{
    		DoneLen = len;
    		itpNfInfo.CurBlk[file] += len;
    	}
    	goto end;
    }
        
#ifdef ENABLE_CHECK_RESERVED_AREA
        //check baundary of bootloader/Mac address/boot image
    //check current block index "tpNfInfo.CurBlk" for suring area of BL/Mac/image
    //if "tpNfInfo.CurBlk" is over the boundary, then return fail
    if(itpNfInfo.CurBlk[file] < itpNfInfo.blEndBlk)  
        areaBoundary = itpNfInfo.blEndBlk;
    else if(itpNfInfo.CurBlk[file] < itpNfInfo.bootImgStartBlk)
        areaBoundary = itpNfInfo.bootImgStartBlk;
    else 
        areaBoundary = itpNfInfo.bootImgEndBlk;
#endif        
        
    if( (itpNfInfo.CurBlk[file] + (uint32_t)len) > areaBoundary )
    {
        LOG_ERR "write over the boundary, curB=%x, len=%x, boundary=%x\n",itpNfInfo.CurBlk[file],len,areaBoundary LOG_END
        printf( "write over the boundary, curB=%x, len=%x, boundary=%x\n",itpNfInfo.CurBlk[file],len,areaBoundary );
        goto end;
    }

    DataBuf = (char *)malloc(itpNfInfo.PageInBlk*itpNfInfo.PageSize);
    if(DataBuf==NULL)
    {
    	errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;	//out of memory:
        LOG_ERR "out of memory:\n" LOG_END
    	goto end;  	
    }

    if(!itpNfInfo.SprSize)
    {
    	errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;	//nand initial fail
        LOG_ERR "nand spare initial error: \n" LOG_END
    	goto end;
    }
    
    SprBuf = (char *)malloc(itpNfInfo.SprSize);
    if(SprBuf==NULL)
    {
    	errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;	//out of memory:
        LOG_ERR "out of memory:\n" LOG_END
    	goto end;  	
    }

    memset((uint8_t*)SprBuf, 0xFF, itpNfInfo.SprSize);

   	for(blk=itpNfInfo.CurBlk[file]; blk<areaBoundary; blk++)
   	{
   	    int allNfPgWtHasPass;
   	    
        if( blk >= areaBoundary )
        {
           LOG_ERR "itpNfWrite out of boundary: blk:%d, Boundary:%d\n", blk, areaBoundary LOG_END
           printf("ERROR:: NO enough space for nand writing data, %x, %x, %x, %x\n",blk,itpNfInfo.blEndBlk,itpNfInfo.bootImgStartBlk,itpNfInfo.bootImgEndBlk);
#ifdef ENABLE_CHECK_RESERVED_AREA           
           if(areaBoundary==itpNfInfo.blEndBlk)
               printf("OUT of bootloader area!!\n");
           else if(areaBoundary==itpNfInfo.bootImgStartBlk)
               printf("OUT of MAC-address area!!\n");
           else if(areaBoundary==itpNfInfo.bootImgEndBlk)
               printf("OUT of boot-image area!!\n");
           else
               printf("UNKNOW condition::%x,%x\n",areaBoundary,itpNfInfo.bootImgEndBlk);
#endif           
           itpNfInfo.CurBlk[file] = blk;  //set current block index
           goto end;
        }        
        
        //check if "blk" a bad block
        if( iteNfIsBadBlockForBoot(blk) != true )
        {
            //true means "GOOD BLOCK"; !true means "BAD BLOCK"
            continue;
        }        
        
   		//erase current block
   		if( iteNfErase(blk)!=true )
   		{
   			errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;	//erase fail
   			LOG_ERR "erase block fail: %d\n", blk LOG_END

#if 1
   			printf("NfErsErr:: need to mark bad block(blk=0x%x)\n",blk);
   			//mark as bad block
   			{
   			    char db[4096] = {0};    //max page size = 4096
   			    char sb[512] = {0};     //max spare size = 512
   			    iteNfWrite(blk, 0, db, sb);
   			}   			
   			continue;
#else
   			//DoneLen = 0;
   			itpNfInfo.CurBlk[file] = blk;
   			goto end;
#endif
   		}

        if(blk)
        {
            memcpy((void*)DataBuf, (void*)GoodBlkCodeMark, itpNfInfo.blkGap[file]);
            memcpy((void*)(DataBuf + itpNfInfo.blkGap[file]), (void*)tmpSrcBuf, itpNfInfo.PageInBlk * itpNfInfo.PageSize - itpNfInfo.blkGap[file]);
        }
        else
        {
            memcpy((void*)DataBuf, (void*)tmpSrcBuf, itpNfInfo.PageInBlk*itpNfInfo.PageSize);
        }
        tmpDataBuf = DataBuf;
   		
        allNfPgWtHasPass = 1;
    	for( pg=0; pg<itpNfInfo.PageInBlk; pg++)
    	{
    		//SetSprBuff(blk, pg, SprBuf);
    		if( iteNfWrite(blk, pg, tmpDataBuf, SprBuf)!=true )
    		{
   				errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | __LINE__;	//erase fail
   				LOG_ERR "write fail:[%d,%d]\n", blk, pg LOG_END
   				
                //mark as bad block
                {
                    char db[4096] = {0};    //max page size = 4096
                    char sb[512] = {0};     //max spare size = 512
                    iteNfWrite(blk, 0, db, sb);
                }
                tmpDataBuf -= itpNfInfo.PageSize * pg;
                LOG_ERR "buffer address return to 0x%x:(ori=0x%x)\n", tmpDataBuf, (uint32_t)tmpDataBuf - (itpNfInfo.PageSize*pg) LOG_END
                allNfPgWtHasPass = 0;            
                break;
   				//DoneLen = 0;
   				//itpNfInfo.CurBlk[file] = blk;
   				//goto end;
    		}
			tmpDataBuf+=itpNfInfo.PageSize;
    	}
    	if(allNfPgWtHasPass)   DoneLen++;
    	else                   continue;
    	
        if(DoneLen>len)
        {
            LOG_ERR "itpNfWrite impossible condition: DoneLen:%d, len:%d\n", DoneLen, len LOG_END
            itpNfInfo.CurBlk[file] = blk+1;
            goto end;
        }
        
        if(DoneLen==len)
        {
            break;
        }

        if(blk)	tmpSrcBuf += itpNfInfo.PageInBlk*itpNfInfo.PageSize - itpNfInfo.blkGap[file];
        else	tmpSrcBuf += itpNfInfo.PageInBlk*itpNfInfo.PageSize;		
    }
    //itpNfInfo.CurBlk[file] += DoneLen;    
    printf("itpNfWt:cBlk=%x, b=%x, new-cb=%x\n",itpNfInfo.CurBlk[file],blk,blk+1);
    itpNfInfo.CurBlk[file] = blk+1; 

end:
	if(SprBuf)
	{
		free(SprBuf);
		SprBuf = NULL;
	}		
	if(DataBuf)
	{
		free(DataBuf);
		DataBuf = NULL;
	}	
    return DoneLen;
}

static int NandLseek(int file, int ptr, int dir, void* info)
{
	int DefBlk = CFG_NAND_RESERVED_SIZE / (itpNfInfo.PageInBlk*itpNfInfo.PageSize);
	
    switch(dir)
    {
    default:
    case SEEK_SET:
    	if(itpNfInfo.currMode[file] == ITP_NAND_FTL_MODE)
    	{
    		//int DefBlk = CFG_NAND_RESERVED_SIZE / (itpNfInfo.PageInBlk*itpNfInfo.PageSize);
    		if(DefBlk>ptr)
    		{
    			printf("ERROR: current seek position is abnormal, ori pos=%x, rev pos=%x\n",ptr,DefBlk);
    		}
    		else
    		{
    			itpNfInfo.CurBlk[file] = ptr - DefBlk;
    		}
    	}
    	else
    	{
    		itpNfInfo.CurBlk[file] = ptr; //raw data mode(without FTL)
    	}	
        #ifdef ENABLE_CHECK_RESERVED_AREA
        {
           if(itpNfInfo.CurBlk[file]<itpNfInfo.blEndBlk)
               LOG_INFO "itpNfSeek to bootloader area!!(%x,%x)\n",itpNfInfo.CurBlk[file],itpNfInfo.blEndBlk LOG_END
           else if(itpNfInfo.CurBlk[file]<itpNfInfo.bootImgStartBlk)
               LOG_INFO "itpNfSeek to MAC address area!!(%x,%x)\n",itpNfInfo.CurBlk[file],itpNfInfo.bootImgStartBlk LOG_END
           else if(itpNfInfo.CurBlk[file]<itpNfInfo.bootImgEndBlk)
               LOG_INFO "itpNfSeek to IMAGE area!!(%x,%x)\n",itpNfInfo.CurBlk[file],itpNfInfo.bootImgEndBlk LOG_END
           else
               LOG_INFO "itpNfSeek to UNKNOW area::%x, %x\n",itpNfInfo.CurBlk[file],itpNfInfo.blEndBlk,itpNfInfo.bootImgStartBlk,itpNfInfo.bootImgEndBlk LOG_END
        }
        #endif
        break;
    case SEEK_CUR:
        itpNfInfo.CurBlk[file] += ptr;
        break;
    case SEEK_END:
        break;
    }
    if(itpNfInfo.currMode[file] == ITP_NAND_FTL_MODE)
    	return itpNfInfo.CurBlk[file] + DefBlk;
    else
    	return itpNfInfo.CurBlk[file];
}

static int NandIoctl(int file, unsigned long request, void* ptr, void* info)
{
	int res = -1;
	
    switch (request)
    {
	    case ITP_IOCTL_INIT:
	    	#ifdef CFG_UPGRADE_IMAGE_POS
	    	{
	    	    int pos = (int)CFG_UPGRADE_IMAGE_POS;
	    	    remap_image_posistion = pos;
	    	    printf("itpNand.1: IOCTL init img_pos = %x\n",remap_image_posistion, CFG_UPGRADE_IMAGE_POS);  
	    	}
            #endif

	    	itpNfInfo.BootRomSize = (uint32_t)CFG_NAND_RESERVED_SIZE;
	    	#ifdef	CFG_NOR_SHARE_SPI_NAND
	    	itpNfInfo.enSpiCsCtrl = 1;
	    	#else
	    	itpNfInfo.enSpiCsCtrl = 0;
	    	#endif
	    	
	    	#ifdef	CFG_SPI_NAND_ADDR_HAS_DUMMY_BYTE
	    	LOG_INFO "set AHDB=1\n" LOG_END
	    	itpNfInfo.addrHasDummyByte = 1;
	    	#else
	    	LOG_INFO "set AHDB=0\n" LOG_END
	    	itpNfInfo.addrHasDummyByte = 0;
	    	#endif
	    	
	    	printf("ITP_IOCTL_INIT[01](%x,%x)(%x,%x)\n",&itpNfInfo,itpNfInfo.NumOfBlk,itpNfInfo.BootRomSize,itpNfInfo.enSpiCsCtrl);
	        if( iteNfInitialize(&itpNfInfo)==true )
	        {
	        	#ifdef  CFG_UPGRADE_FILE_FOR_NAND_PROGRAMMER
	        	itpNfInfo.rmpFlg = 0;     	
	        	if(1)//!itpNfInfo.Init)
	        	{
	        	    unsigned char *pBuf;
	        	    pBuf = (unsigned char *)malloc(itpNfInfo.PageSize + 512);
    	        	if(pBuf!=NULL)
    	        	{
    	        	    //read 0th page of 0th block for checking remap info	
    	        	    iteNfRead(0, 0, pBuf);
    	        	    //ithPrintVram8(pBuf,128);
    	        	    if( (pBuf[20]==0x55) && (pBuf[21]==0xAA) )
    	        	    {
    	        	        itpNfInfo.rmpFlg = 1;
    	        	    }
    	        	    printf("itpNand: rmpFlag = %x, (%x, %x)\n", itpNfInfo.rmpFlg, pBuf[20], pBuf[21]);
    	        	    free(pBuf);
    	        	}	
    	        }
	        	#endif	            
	        	
	        	{
	        	    int i=0;
	        	    
	        	    /* initial itpNfInfo */
	        	    itpNfInfo.openCnt = 0;
	        	    itpNfInfo.usedDev = 0;	
	        	    for(i=0; i<ITE_NF_OPEN_MAX; i++)    itpNfInfo.CurBlk[i] = 0;
	        	    for(i=0; i<ITE_NF_OPEN_MAX; i++)    itpNfInfo.blkGap[i] = 0;
	        	    for(i=0; i<ITE_NF_OPEN_MAX; i++)    itpNfInfo.currMode[i] = 0;  
	        	}

	        	printf("ITP_IOCTL_INIT[02](%x,%x)(%x), init=%x\n",&itpNfInfo, itpNfInfo.NumOfBlk, itpNfInfo.BootRomSize,itpNfInfo.Init);
	        	res = 0;
	        }
	        #ifdef	CFG_NOR_SHARE_SPI_NAND
	        mmpSpiSetControlMode(SPI_CONTROL_NOR);
	        mmpSpiResetControl();
	        #endif
	        break;
	
	    case ITP_IOCTL_GET_BLOCK_SIZE:
	        if(itpNfInfo.NumOfBlk)
	        {
        		*(unsigned long*)ptr = (itpNfInfo.PageInBlk)*(itpNfInfo.PageSize) - itpNfInfo.blkGap[file];
	        	res = 0;
	        }
	        break;
	        
	    case ITP_IOCTL_FLUSH:
	        {
	            uint32_t mode=(uint32_t)ptr;
	            
	            if(!itpNfInfo.Init)
	            {
	                printf("NAND Flush fail: NAND not initial, yet\n");
	                res = 0;
	                break;
	            }
	            
	            if(mode == ITP_NAND_FTL_MODE)
	            {
	                uint8_t *pBuf = malloc(4096+512);
	                
	                if(pBuf != NULL)
	                {
	                    if( mmpNandReadSector(0, 1, pBuf) == 0 )    res = 0;
	                    else    LOG_ERR "flush NAND FAIL, read nand error!!\n" LOG_END  
	                        
	                    free(pBuf);	                                          
	                }
	                else
	                {
	                    LOG_ERR "flush NAND FAIL, out of memory!!\n" LOG_END
	                }
	            }
	            else
	            {
	                if(FlushBootRom()==true)    res = 0;
	            }
	        }
	        break;

	    case ITP_IOCTL_SCAN:
	    	{
	    		uint32_t *blk=(uint32_t *)ptr;
	        	if(iteNfIsBadBlockForBoot(*blk)==true )
	        	{
	        		//*(unsigned long*)ptr = itpNfInfo.PageInBlk * itpNfInfo.PageSize;
	        		res = 0;
	        	}
	        }
	        break;

        case ITP_IOCTL_GET_GAP_SIZE:
            {
               	*(unsigned long*)ptr = itpNfInfo.blkGap[file];
                res = 0;
            }
            break;

        case ITP_IOCTL_NAND_CHECK_MAC_AREA:
#ifdef CFG_NET_ETHERNET_MAC_ADDR_NAND
            //check MAC address area(need 1 good block)
            {
                int isNoGoodBlock = 1;
                int i;
                
                for(i=itpNfInfo.blEndBlk; i<itpNfInfo.bootImgStartBlk; i++)
                {
                    //check bad block
                    if( iteNfIsBadBlockForBoot(i) == true )
                    {
                        //got one good block
                        isNoGoodBlock = 0;
                        break;
                    }
                }
                if(isNoGoodBlock)
                {
                    LOG_ERR "This NAND has too many bad blocks in MAC-address area\n" LOG_END
                    ithDelay(10000);
                }
                else
                    res = 0;
            }
#endif
            break;

        case ITP_IOCTL_NAND_CHECK_REMAP:
            if(itpNfInfo.rmpFlg)    res = 0;
            
	    	#ifdef CFG_UPGRADE_IMAGE_POS
            remap_image_posistion = (int)CFG_UPGRADE_IMAGE_POS;
            #endif
            
            printf("itpNand: IOCTL remap = %x, img_pos = %x\n",itpNfInfo.rmpFlg, remap_image_posistion); 
            break;

        case ITP_IOCTL_SET_WRITE_PROTECT:
            /**************************************************************************
            note: wp[0]: 0 is WP disable, 1 is WP enable
                  wp[1]: the start sector of WP(sector size is 512Bytes)
                  wp[2]: the end sector of WP(sector size is 512Bytes)
                  ex: if start address of WP is 0, end address of WP is 0xD00000
                      then, set wp[0]=1, wp[1]=0, wp[2]=(0xD00000/0x200)=0x6800
            ***************************************************************************/
            /*
            {
                unsigned long *wp = (unsigned long *)ptr;
                if(mmpNandWriteProtect(wp))
                {
                    printf("itpNand: IOCTL_SET_WP, FAIL: wp0=%d, wp1=%d, wp2=%d\n",wp[0],wp[1],wp[2]);
                }
                else
                {
                    printf("itpNand: IOCTL_SET_WP, wp0=%d, wp1=%d, wp2=%d\n",wp[0],wp[1],wp[2]);
                    res = 0;
                }
            }
            */     
            break;

	    default:
	        errno = (ITP_DEVICE_NAND << ITP_DEVICE_ERRNO_BIT) | 1;
	        break;
    }
    return res;
}

const ITPDevice itpDeviceNand =
{
    ":nand",
    NandOpen,
    NandClose,
    NandRead,
    NandWrite,
    NandLseek,
    NandIoctl,
    NULL
};
