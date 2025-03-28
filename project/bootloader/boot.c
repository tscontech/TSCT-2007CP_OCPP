﻿#include <sys/ioctl.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "ite/itp.h"
#include "ite/itc.h"
#include "ssp/mmp_spi.h"
#include "config.h"
#ifdef __SM32__
#include "openrtos/sm32/port_spr_defs.h"
#include "openrtos/FreeRTOS.h"
#endif

#define MAX_HEADER_SIZE     0x10000
#define HEADER_LEN_OFFSET   12

#ifndef CFG_UPGRADE_IMAGE_POS
#define CFG_UPGRADE_IMAGE_POS 0
#endif

#ifndef _WIN32

#ifdef CFG_UPGRADE_FILE_FOR_NAND_PROGRAMMER
unsigned char gDoReMapFlow = 1;
#endif

extern unsigned int __bootimage_func_start, __bootimage_func_end;

static void __attribute__ ((section (".bootimage_func"), no_instrument_function))
BootImageFunc(register uint32_t* src, register uint32_t len)
{
    register uint32_t* dst = 0;
    register uint32_t i = 0;

    // disable IRQ & MMU
#ifdef __arm__
    __asm__ __volatile__ (
       "mrs     r3, cpsr\n"
       "orr     r3,r3,#0x000000c0\n"
       "msr     cpsr_c,r3\n"
       "mrc     p15,0,r3,c1,c0,0\n"
       "bic     r3,r3,#0x00000001\n"
       "mcr     p15,0,r3,c1,c0,0\n"
       "nop\n"
	   "nop\n"
	   "nop\n"
	   "nop\n"
	   "nop\n"
       :
       :
       : "r3"	/* clobber list */
    );
#elif defined __SM32__
    __asm__ __volatile__ (
        "l.addi   r21, r0, 0x11        \n\t"    /* Get SPR_SR address.   */
        "l.mfspr  r23, r21, 0x0        \n\t"    /* Get SPR_SR.           */
        "l.addi   r25, r0, 0xfffffff9  \n\t"    /* Disable IEE & TEE       */
        "l.and    r27, r23, r25        \n\t"    /* Disable IEE & TEE    */
        "l.mtspr  r21, r27, 0x0        \n\t"    /* Write Back SPR_SR.    */
        :
        :
        : "r21", "r23", "r25", "r27"  /* clobber list */
     );
#elif defined __riscv
    // TODO: RISCV
#else
    #error "Unsupport platform"
#endif // 	__arm__


    // copy image to address 0
    for (i = 0; i < len; i++)
    {
        dst[i] = src[i];
    }

#ifdef __arm__
	__asm__ __volatile__ (
		"mov r3,#0\n"
		"mcr p15,0,r3,c7,c10,0\n"	/* flush d-cache all */
		"mcr p15,0,r3,c7,c10,4\n"	/* flush d-cache write buffer */
		"mcr p15,0,r3,c7,c5,0\n"	/* invalidate i-cache all */
		:
		:
		: "r3"	/* clobber list */
		);

    // reboot
    asm volatile (
        "mov      pc, #0\n"
        );
#elif defined __SM32__
    {
      int ncs, bs;
      int cache_size, cache_line_size;
      int i = 0;

      // Disable DC
      mtspr(SPR_SR, mfspr(SPR_SR) & ~SPR_SR_DCE);

      // Number of cache sets
      ncs = ((mfspr(SPR_DCCFGR) >> 3) & 0xf);

      // Number of cache block size
      bs  = ((mfspr(SPR_DCCFGR) >> 7) & 0x1) + 4;

      // Calc Cache size
      cache_line_size = 1 << bs;
      cache_size      = 1 << (ncs+bs);

      // Flush DC
      do {
         mtspr(SPR_DCBIR, i);
         i += cache_line_size;
      } while(i < cache_size);

      // Enable DC
      mtspr(SPR_SR, mfspr(SPR_SR) | SPR_SR_DCE);
      asm volatile("l.nop 0x0\nl.nop 0x0\nl.nop 0x0\nl.nop 0x0\n");
   }
   {
      int ncs, bs;
      int cache_size, cache_line_size;
      int i = 0;

      // Disable IC
      mtspr(SPR_SR, mfspr(SPR_SR) & ~SPR_SR_ICE);

      // Number of cache sets
      ncs = (mfspr(SPR_ICCFGR) >> 3) & 0xf;

      // Number of cache block size
      bs  = ((mfspr(SPR_ICCFGR) >> 7) & 0x1) + 4;

      // Calc Cache size
      cache_line_size = 1 << bs;
      cache_size      = 1 << (ncs+bs);

      // Flush IC
      do {
         mtspr(SPR_ICBIR, i);
         i += cache_line_size;
      } while (i < cache_size);

      // Enable IC
      mtspr(SPR_SR, mfspr(SPR_SR) | SPR_SR_ICE);
      asm volatile("l.nop 0x0\nl.nop 0x0\nl.nop 0x0\nl.nop 0x0\n");
    }

    // reboot
    asm volatile (
        "l.movhi     r3,hi(_start)\n"
        "l.ori       r3,r3,lo(_start)\n"
        "l.jr        r3\n"
        "l.nop\n"
        :
        :
        : "r3"	/* clobber list */
        );
#elif defined __riscv
    // TODO: RISCV
#else
    #error "Unsupport platform"
#endif // 	__arm__

}

#endif // !_WIN32

static uint32_t imagesize;
static uint8_t* imagebuf;

void* LoadImage(void* arg)
{
    uint32_t ret, headersize, bufsize, alignsize, blockcount, blocksize, contentsize, pos, gapsize;
    uint8_t *rombuf;
    bool compressed;
    int fd = -1;

#if defined(CFG_UPGRADE_IMAGE_NAND)
    fd = open(":nand", O_RDONLY, 0);
    LOG_DBG "nand fd: 0x%X\n", fd LOG_END
#elif defined(CFG_UPGRADE_IMAGE_NOR)
    mmpSpiSetControlMode(SPI_CONTROL_NOR);
    mmpSpiResetControl();
    fd = open(":nor", O_RDONLY, 0);
    LOG_DBG "nor fd: 0x%X\n", fd LOG_END
#elif defined(CFG_UPGRADE_IMAGE_SD0)
    fd = open(":sd0", O_RDONLY, 0);
    LOG_DBG "sd0 fd: 0x%X\n", fd LOG_END
#elif defined(CFG_UPGRADE_IMAGE_SD1)
    fd = open(":sd1", O_RDONLY, 0);
    LOG_DBG "sd1 fd: 0x%X\n", fd LOG_END
#endif

    if (fd == -1)
    {
        LOG_ERR "open device error: %d\n", fd LOG_END
        goto end;
    }

    // mount disks on booting
#ifdef CFG_UPGRADE_FILE_FOR_NAND_PROGRAMMER
    if(!ioctl(ITP_DEVICE_NAND, ITP_IOCTL_NAND_CHECK_REMAP, NULL))
    {
        gDoReMapFlow = 1;
        while(1)
        {            
            if(gDoReMapFlow==0) break;
            usleep(10000);
        }
        printf("NAND remap flow has finished\n");
    }
#endif

    if (ioctl(fd, ITP_IOCTL_GET_BLOCK_SIZE, &blocksize))
    {
        LOG_ERR "get block size error\n" LOG_END
        goto end;
    }

    if (ioctl(fd, ITP_IOCTL_GET_GAP_SIZE, &gapsize))
    {
        LOG_ERR "get gap size error:\n" LOG_END
        goto end;
    }

    // read rom header size
    pos = CFG_UPGRADE_IMAGE_POS / (blocksize + gapsize);
    blockcount = CFG_UPGRADE_IMAGE_POS % (blocksize + gapsize);
    if (blockcount > 0)
    {
        LOG_WARN "rom position 0x%X and block size 0x%X are not aligned; align to 0x%X\n", CFG_UPGRADE_IMAGE_POS, blocksize, (pos * blocksize) LOG_END
    }

    LOG_DBG "blocksize=%d pos=0x%X align=%d\n", blocksize, pos, blockcount LOG_END

    if (lseek(fd, pos, SEEK_SET) != pos)
    {
        LOG_ERR "seek to rom position %d error\n", pos LOG_END
        goto end;
    }

    alignsize = ITH_ALIGN_UP(MAX_HEADER_SIZE, blocksize);
    rombuf = malloc(alignsize);
    if (!rombuf)
    {
        LOG_ERR "out of memory %d\n", alignsize LOG_END
        goto end;
    }

    blockcount = alignsize / blocksize;
    ret = read(fd, rombuf, blockcount);
    if (ret != blockcount)
    {
        LOG_ERR "read rom header error: %d != %d\n", ret, blockcount LOG_END
        goto end;
    }

    headersize = *(uint32_t*)(rombuf + HEADER_LEN_OFFSET);
    headersize = itpBetoh32(headersize);
    LOG_DBG "rom header size: %d\n", headersize LOG_END

    if ((headersize %4) || headersize > 100*1024)
    {
        LOG_ERR "invalid header size: %d != %d\n", headersize LOG_END
        goto end;
    }
    
    imagesize = *(uint32_t*)(rombuf + headersize - 4);
    imagesize = itpBetoh32(imagesize);
    LOG_DBG "rom image size: %d\n", imagesize LOG_END

        imagebuf = rombuf + headersize;
    if (strncmp(imagebuf, "SMAZ", 4) == 0)
    {
        // compressed rom
        contentsize = *(uint32_t*)(rombuf + HEADER_LEN_OFFSET + 4);
        contentsize = itpBetoh32(contentsize);
        LOG_DBG "compress rom, content size: %d\n", contentsize LOG_END
        compressed = true;
    }
    else
    {
        // non-compressed rom
        contentsize = imagesize;
        LOG_DBG "non-compress rom, content size: %d\n", contentsize LOG_END
            compressed = false;
    }

    // read rom image
    bufsize = headersize + contentsize;
    bufsize = ITH_ALIGN_UP(bufsize, blocksize);
    rombuf = realloc(rombuf, bufsize);
    if (!rombuf)
    {
        LOG_ERR "out of memory %d\n", bufsize LOG_END
        goto end;
    }

    blockcount = bufsize / blocksize - blockcount;
#if defined(CFG_UPGRADE_IMAGE_NAND)
    ret = read(fd, rombuf + blocksize, blockcount);
#else
    ret = read(fd, rombuf + alignsize, blockcount);
#endif
    if (ret != blockcount)
    {
        LOG_ERR "read rom image error: %d != %d\n", ret, blockcount LOG_END
        goto end;
    }

    if (compressed)
    {
        imagebuf = malloc(imagesize);
        if (!imagebuf)
        {
            LOG_ERR "out of memory %d\n", imagesize LOG_END
            goto end;
        }

        // hardware decompress
        ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_INIT, NULL);

        if (write(ITP_DEVICE_DECOMPRESS, rombuf + headersize + 8, contentsize) == contentsize)
        {
            ret = read(ITP_DEVICE_DECOMPRESS, imagebuf, imagesize);
        }

        ioctl(ITP_DEVICE_DECOMPRESS, ITP_IOCTL_EXIT, NULL);
    }
    else
    {
        imagebuf = rombuf + headersize;
    }

end:
    if (fd != -1)
        close(fd);

    return NULL;
}

void ReleaseImage(void)
{
    free(imagebuf);
    imagebuf = NULL;
    imagesize = 0;
}

void BootImage(void)
{
#ifndef _WIN32
    uint32_t bootimageFuncSize;
    static void (*bootimageFunc)(uint32_t* src, uint32_t len);
#endif // !_WIN32

#if defined(CFG_UART0_ENABLE) && defined(CFG_UART0_DMA)
	// avoid crash from booting image
	ioctl(ITP_DEVICE_UART0, ITP_IOCTL_UART_STOP_DMA, NULL);
#endif

#if defined(CFG_UART1_ENABLE) && defined(CFG_UART1_DMA)
	// avoid crash from booting image
	ioctl(ITP_DEVICE_UART1, ITP_IOCTL_UART_STOP_DMA, NULL);
#endif

#if defined(CFG_UART2_ENABLE) && defined(CFG_UART2_DMA)
	// avoid crash from booting image
	ioctl(ITP_DEVICE_UART2, ITP_IOCTL_UART_STOP_DMA, NULL);
#endif

#if defined(CFG_UART3_ENABLE) && defined(CFG_UART3_DMA)
	// avoid crash from booting image
	ioctl(ITP_DEVICE_UART3, ITP_IOCTL_UART_STOP_DMA, NULL);
#endif

#if defined(CFG_UART4_ENABLE) && defined(CFG_UART4_DMA)
	// avoid crash from booting image
	ioctl(ITP_DEVICE_UART4, ITP_IOCTL_UART_STOP_DMA, NULL);
#endif

#if defined(CFG_UART5_ENABLE) && defined(CFG_UART5_DMA)
	// avoid crash from booting image
	ioctl(ITP_DEVICE_UART5, ITP_IOCTL_UART_STOP_DMA, NULL);
#endif

    // reset interrupt before booting
    ithIntrReset();

    // boot image
#ifndef _WIN32
    bootimageFuncSize = (unsigned int)&__bootimage_func_end - (unsigned int)&__bootimage_func_start;
    bootimageFunc = malloc(ITH_ALIGN_UP(imagesize, 4) + bootimageFuncSize);
    bootimageFunc = (void*)((uint8_t*)bootimageFunc + ITH_ALIGN_UP(imagesize, 4));
#if defined(CFG_DBG_SWUART_CODEC)
	iteRiscTerminateEngine();  
#endif
    memcpy(bootimageFunc, BootImageFunc, bootimageFuncSize);
	//flush instruction cache
	ithInvalidateICache();
    bootimageFunc((uint32_t*)(imagebuf), ITH_ALIGN_UP(imagesize, 4) / 4);
#endif // !_WIN32
}

#if defined(CFG_ENABLE_UART_CLI)
void BootBin(ITCStream *upgradeFile)
{
	uint32_t bootimageFuncSize;
	uint32_t imagesize;
	static void(*bootimageFunc)(uint32_t* src, uint32_t len);
	uint8_t *imagebuf;

	bootimageFuncSize = (unsigned int)&__bootimage_func_end - (unsigned int)&__bootimage_func_start;

	imagesize = upgradeFile->size;
	imagebuf = malloc(upgradeFile);

	itcStreamRead(upgradeFile, imagebuf, imagesize);
	itcStreamClose(upgradeFile);

	// reset interrupt before booting
	ithIntrReset();

	bootimageFunc = malloc(ITH_ALIGN_UP(imagesize, 4) + bootimageFuncSize);
	bootimageFunc = (void*)((uint8_t*)bootimageFunc + ITH_ALIGN_UP(imagesize, 4));

	memcpy(bootimageFunc, BootImageFunc, bootimageFuncSize);
	ithInvalidateICache();
	bootimageFunc((uint32_t*)(imagebuf), ITH_ALIGN_UP(imagesize, 4) / 4);
}
#endif

#ifdef CFG_BOOT_TESTBIN_ENABLE
ITCStream* OpenTestBin(void)
{
    char pkgFilePath[PATH_MAX];
    static ITCFileStream fileStream;
    ITPDriveStatus* driveStatusTable;
    ITPDriveStatus* driveStatus = NULL;
    int i;

    // try to find the package drive
    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_GET_TABLE, &driveStatusTable);

    for (i = ITP_MAX_DRIVE - 1; i >= 0; i--)
    {
        driveStatus = &driveStatusTable[i];
        if (driveStatus->avail && driveStatus->removable)
        {
            char buf[PATH_MAX], *ptr;

            // get file path from list
            strcpy(buf, CFG_TESTBIN_FILENAME);
            ptr = strtok(buf, " ");
            do
            {
                strcpy(pkgFilePath, driveStatus->name);
                strcat(pkgFilePath, ptr);

                if (itcFileStreamOpen(&fileStream, pkgFilePath, false) == 0)
                {
                    LOG_INFO "Found Test bin file %s\n", pkgFilePath LOG_END
                    return &fileStream.stream;
                }
                else
                {
                    LOG_DBG "try to fopen(%s) fail\n", pkgFilePath LOG_END
                }
            }
            while ((ptr = strtok(NULL, " ")) != NULL);
        }
    }
    LOG_DBG "cannot find Test Bin file.\n" LOG_END
    return NULL;
}

void BootTestBin(ITCStream *testBinFile)
{
	uint32_t bootimageFuncSize;
	uint32_t imagesize;
    static void (*bootimageFunc)(uint32_t* src, uint32_t len);
	uint8_t *imagebuf;

	bootimageFuncSize = (unsigned int)&__bootimage_func_end - (unsigned int)&__bootimage_func_start;

	imagesize = testBinFile->size;
	imagebuf = malloc (imagesize);

	itcStreamRead(testBinFile, imagebuf, imagesize);	
	itcStreamClose(testBinFile);

	// reset interrupt before booting
    ithIntrReset();

	bootimageFunc = malloc(ITH_ALIGN_UP(imagesize, 4) + bootimageFuncSize);
	bootimageFunc = (void*)((uint8_t*)bootimageFunc +ITH_ALIGN_UP(imagesize, 4));

	memcpy(bootimageFunc, BootImageFunc, bootimageFuncSize);
	ithInvalidateICache();
    bootimageFunc((uint32_t*)(imagebuf), ITH_ALIGN_UP(imagesize, 4)/ 4);
}
#endif
