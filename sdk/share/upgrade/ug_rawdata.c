﻿#include <sys/ioctl.h>
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include "ite/ug.h"
#include "ug_cfg.h"

#ifdef	CFG_NAND_RESERVED_SIZE
static int gNandReservedSize = CFG_NAND_RESERVED_SIZE;
#else
static int gNandReservedSize = 0;
#endif

#pragma pack(1)
typedef struct
{
    uint32_t position;
    uint32_t rawdata_size;
} rawdata_t;

int ugUpgradeRawData(ITCStream *f, ITPDisk disk, bool restore)
{
    int ret = 0;
    int fd = -1;
    int pos2;
    rawdata_t rdata;
    char* buf = NULL;
    char* buf2 = NULL;
    uint32_t filesize, bufsize, readsize, size2, remainsize, alignsize, blocksize, pos, gapsize, streampos;

    // read raw data header
    readsize = itcStreamRead(f, &rdata, sizeof(rawdata_t));
    if (readsize != sizeof(rawdata_t))
    {
        ret = __LINE__;
        LOG_ERR "Cannot read file: %ld != %ld\n", readsize, sizeof(rawdata_t) LOG_END
        goto end;
    }

    rdata.position      = itpLetoh32(rdata.position);
    rdata.rawdata_size  = itpLetoh32(rdata.rawdata_size);

    streampos = 0;

    if (rdata.rawdata_size == 0)
    {
        if (restore)
        {
            LOG_INFO "[%d%%] On restore mode, skip position 0x%X.\n", ugGetProrgessPercentage(), rdata.position LOG_END
            goto end;
        }
        else
        {
            streampos = itcStreamTell(f);
            itcStreamSeek(f, 0, SEEK_SET);
            rdata.rawdata_size = f->size;

            LOG_DBG "streampos=0x%X\n", streampos LOG_END
        }
    }

    LOG_INFO "[%d%%] Write data to position 0x%X, size %ld bytes\n", ugGetProrgessPercentage(), rdata.position, rdata.rawdata_size LOG_END

    // upgrade
    if (disk == ITP_DISK_NAND)
    {
    	if(rdata.position == gNandReservedSize)
    	{
    		//add force to MOUNT NAND
    		ret = ioctl(UG_DEVICE, ITP_IOCTL_MOUNT, (void*)disk);   
    	    
    		if(!rdata.position)	printf("WARNING: raw data position is 0!!\n");
    		fd = open(":nand", O_RDWR, 1);
    		printf("open NAND drive with FAT+FTL OK!\n");
    	}
    	else
    	{
    		fd = open(":nand", O_RDWR, 0);
    		printf("open NAND device OK!\n");
    	}    	
        
        LOG_DBG "nand fd=0x%X\n", fd LOG_END
        
        #ifdef CFG_NET_ETHERNET_MAC_ADDR_NAND
        if (fd != -1)
        {
            if(ioctl(fd, ITP_IOCTL_NAND_CHECK_MAC_AREA, NULL))
            {
                LOG_ERR "nand MAC address has NO space\n" LOG_END
                printf( "nand MAC address has NO space\n" );
                fd = -1;
            }
        }
        #endif
    }
    else if (disk == ITP_DISK_NOR)
    {
        fd = open(":nor", O_RDWR, 0);
        LOG_DBG "nor fd=0x%X\n", fd LOG_END
    }
    else if (disk == ITP_DISK_SD0)
    {
        fd = open(":sd0", O_RDWR, 0);
        LOG_DBG "sd0 fd=0x%X\n", fd LOG_END
    }
    else if (disk == ITP_DISK_SD1)
    {
        fd = open(":sd1", O_RDWR, 0);
        LOG_DBG "sd1 fd=0x%X\n", fd LOG_END
    }
    else
    {
        LOG_DBG "Unknown disk: %d\n", disk LOG_END
        ret = -1;
        goto end;
    }

    if (fd == -1)
    {
        LOG_ERR "Open device error: %d\n", disk LOG_END
        ret = -1;
        goto end;
    }

    if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        ret = __LINE__;
        LOG_ERR "Get block size error: %d\n", errno LOG_END
        goto end;
    }

    if (ioctl(fd, ITP_IOCTL_GET_GAP_SIZE, &gapsize))
    {
        ret = __LINE__;
        LOG_ERR "get gap size error: %d\n", errno LOG_END
        goto end;
    }

    LOG_DBG "blocksize=%d gapsize=%d\n", blocksize, gapsize LOG_END

    filesize = rdata.rawdata_size;
    alignsize = ITH_ALIGN_UP(filesize, blocksize + gapsize);
    bufsize = CFG_UG_BUF_SIZE < alignsize ? CFG_UG_BUF_SIZE : alignsize;
    buf = malloc(bufsize);
    if (!buf)
    {
        ret = __LINE__;
        LOG_ERR "Out of memory: %ld\n", bufsize LOG_END
        goto end;
    }

   	pos = rdata.position / (blocksize + gapsize);
    alignsize = rdata.position % (blocksize + gapsize);
    	
    if (alignsize > 0)
    {
        LOG_WARN "Position 0x%X and block+gap size 0x%X are not aligned; align to 0x%X\n", rdata.position, (blocksize + gapsize), (pos * (blocksize + gapsize)) LOG_END
    }

    pos2 = lseek(fd, pos, SEEK_SET);
    if (pos2 != pos)
    {
        ret = __LINE__;
        LOG_ERR "Cannot lseek: %d != %d\n", pos2, pos LOG_END
        goto end;
    }

    LOG_INFO "[%d%%] Writing", ugGetProrgessPercentage() LOG_END

    remainsize = filesize;

    if (streampos > 0)
    {
        alignsize = cpu_to_le32(rdata.rawdata_size);
        memcpy(buf, &alignsize, 4);

        if ( !pos2 && (disk == ITP_DISK_NAND) )
        {
        	readsize = (remainsize < blocksize+gapsize) ? remainsize : blocksize+gapsize;
        	pos2++;
        }
        else
        {
        	if(disk == ITP_DISK_NAND)	readsize = (remainsize < blocksize) ? remainsize : blocksize;
            else	readsize = (remainsize < bufsize) ? remainsize : bufsize;
        }
        readsize -= 4;

        size2 = itcStreamRead(f, &buf[4], readsize);
        if (size2 != readsize)
        {
            ret = __LINE__;
            LOG_ERR "Cannot read file: %ld != %ld\n", size2, readsize LOG_END
            goto end;
        }

        alignsize = ITH_ALIGN_UP(readsize + 4, blocksize) / blocksize;
        size2 = write(fd, buf, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            LOG_ERR "Cannot write file: %ld != %ld\n", size2, alignsize LOG_END
            goto end;
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
    }

    do
    {
#if defined(CFG_CHIP_PKG_IT9910)    
        usleep(1000);
#endif        
        if ( !pos2 && (disk == ITP_DISK_NAND) )
        {
       		readsize = (remainsize < blocksize+gapsize) ? remainsize : blocksize+gapsize;
        	pos2++;
        }
        else
        {
        	if(disk == ITP_DISK_NAND)	readsize = (remainsize < blocksize) ? remainsize : blocksize;
            else	readsize = (remainsize < bufsize) ? remainsize : bufsize;
        }
        size2 = itcStreamRead(f, buf, readsize);
        if (size2 != readsize)
        {
            ret = __LINE__;
            LOG_ERR "Cannot read file: %ld != %ld\n", size2, readsize LOG_END
            goto end;
        }
#if defined(CFG_CHIP_PKG_IT9910)          
        usleep(1000);
#endif        
        alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;
        size2 = write(fd, buf, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            LOG_ERR "Cannot write file: %ld != %ld\n", size2, alignsize LOG_END
            goto end;
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
    }
    while (remainsize > 0);
    putchar('\n');

    // verify
    pos2 = lseek(fd, pos, SEEK_SET);
    if (pos2 != pos)
    {
        ret = __LINE__;
        LOG_ERR "Cannot lseek: %d != %d\n", pos2, pos LOG_END
        goto end;
    }

    buf2 = malloc(bufsize);
    if (!buf2)
    {
        ret = __LINE__;
        LOG_ERR "Out of memory: %ld\n", bufsize LOG_END
        goto end;
    }

    LOG_INFO "[%d%%] Verifying", ugGetProrgessPercentage() LOG_END

    remainsize = filesize;

    if (streampos > 0)
    {
        if (itcStreamSeek(f, 0, SEEK_SET))
        {
            ret = __LINE__;
            LOG_ERR "Cannot seek file: %d\n", errno LOG_END
            goto end;
        }

        if ( !pos2 && (disk == ITP_DISK_NAND) )
        {
        	readsize = (remainsize < blocksize+gapsize) ? remainsize : blocksize+gapsize;
        	pos2++;
        }
        else
        {
        	if(disk == ITP_DISK_NAND)	readsize = (remainsize < blocksize) ? remainsize : blocksize;
            else	readsize = (remainsize < bufsize) ? remainsize : bufsize;
        }
        readsize -= 4;

        alignsize = cpu_to_le32(rdata.rawdata_size);
        memcpy(buf, &alignsize, 4);

        size2 = itcStreamRead(f, &buf[4], readsize);
        if (size2 != readsize)
        {
            ret = __LINE__;
            LOG_ERR "Cannot read file: %ld != %ld\n", size2, readsize LOG_END
            goto end;
        }

        alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;

        size2 = read(fd, buf2, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            LOG_ERR "Cannot read: %ld != %ld\n", size2, alignsize LOG_END
            goto end;
        }

        if (memcmp(buf, buf2, readsize + 4))
        {
        #ifndef _WIN32
            ret = __LINE__;
            LOG_ERR "\nVerify failed.\n" LOG_END
            #if defined(CFG_NAND_ENABLE)
            LOG_ERR "\nMaybe the issue that the upgrade use buffer is too small to upgrade.\n" LOG_END
            #endif            
            goto end;
        #endif // _WIN32
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
    }
    else
    {
        if (itcStreamSeek(f, -(long)filesize, SEEK_CUR))
        {
            ret = __LINE__;
            LOG_ERR "Cannot seek file: %d\n", errno LOG_END
            goto end;
        }
    }

    do
    {
        if ( !pos2 && (disk == ITP_DISK_NAND) )
        {
        	readsize = (remainsize < blocksize+gapsize) ? remainsize : blocksize+gapsize;
        	pos2++;
        }
        else
        {
        	if(disk == ITP_DISK_NAND)	readsize = (remainsize < blocksize) ? remainsize : blocksize;
            else	readsize = (remainsize < bufsize) ? remainsize : bufsize;
        }
        size2 = itcStreamRead(f, buf, readsize);
        if (size2 != readsize)
        {
            ret = __LINE__;
            LOG_ERR "Cannot read file: %ld != %ld\n", size2, readsize LOG_END
            goto end;
        }

        alignsize = ITH_ALIGN_UP(readsize, blocksize) / blocksize;

        size2 = read(fd, buf2, alignsize);
        if (size2 != alignsize)
        {
            ret = __LINE__;
            LOG_ERR "Cannot read: %ld != %ld\n", size2, alignsize LOG_END
            goto end;
        }

        if (memcmp(buf, buf2, readsize))
        {
        #ifndef _WIN32
            ret = __LINE__;
            LOG_ERR "\nVerify failed.\n" LOG_END
            #if defined(CFG_NAND_ENABLE)
            LOG_ERR "\nMaybe the issue that the upgrade use buffer is too small to upgrade.\n" LOG_END
            #endif            
            goto end;
        #endif // _WIN32
        }

        remainsize -= readsize;

        putchar('.');
        fflush(stdout);
    }
    while (remainsize > 0);
    putchar('\n');

    if (streampos != 0)
    {
        itcStreamSeek(f, streampos, SEEK_SET);

        ret = ugUpgradeBackupFinish();
        if (ret)
            goto end;
    }

end:
    if (fd != -1)
        close(fd);

    if (buf2)
        free(buf2);

    if (buf)
        free(buf);

    return ret;
}
