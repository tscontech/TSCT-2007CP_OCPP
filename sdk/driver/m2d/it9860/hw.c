﻿/*
 * Copyright (c) 2014 ITE Corp. All Rights Reserved.
 */
/** @file hw.c
 *  GFX hardware layer API function file.
 *
 * @author Awin Huang
 * @version 1.0
 * @date 2014-05-28
 */

#include <pthread.h>
#include "hw.h"
#include "ite/ith.h"
#include "msg.h"
#include "../../include/ite/itp.h"

//=============================================================================
//                              Compile Option
//=============================================================================
#ifndef CFG_CHIP_FT
#define GFX_USE_CMDQ
#endif
//#define GFX_USE_COMQ_BURST
//#define GFX_USE_INTERRUPT

//=============================================================================
//                              Extern Reference
//=============================================================================

//=============================================================================
//                              Macro Definition
//=============================================================================

//=============================================================================
//                              Structure Definition
//=============================================================================

//=============================================================================
//                              Global Data Definition
//=============================================================================
static GFX_HW_DEVICE GfxHwDevice = {
    {
        {GFX_REG_SRC_ADDR,      0},
        {GFX_REG_SRCXY_ADDR,    0},
        {GFX_REG_SHWR_ADDR,     0},
        {GFX_REG_SPR_ADDR,      0},
        {GFX_REG_MASK_ADDR,     0},
        {GFX_REG_MASKXY_ADDR,   0},
        {GFX_REG_MHWR_ADDR,     0},
        {GFX_REG_MPR_ADDR,      0},
        {GFX_REG_DST_ADDR,      0},
        {GFX_REG_DSTXY_ADDR,    0},
        {GFX_REG_DHWR_ADDR,     0},
        {GFX_REG_DPR_ADDR,      0},
        {GFX_REG_PXCR_ADDR,     0},
        {GFX_REG_PYCR_ADDR,     0},
        {GFX_REG_LNEXY_ADDR,    0},
        {GFX_REG_ITMR00_ADDR,   0},
        {GFX_REG_ITMR01_ADDR,   0},
        {GFX_REG_ITMR02_ADDR,   0},
        {GFX_REG_ITMR10_ADDR,   0},
        {GFX_REG_ITMR11_ADDR,   0},
        {GFX_REG_ITMR12_ADDR,   0},
        {GFX_REG_ITMR20_ADDR,   0},
        {GFX_REG_ITMR21_ADDR,   0},
        {GFX_REG_ITMR22_ADDR,   0},
        {GFX_REG_FGCOLOR_ADDR,  0},
        {GFX_REG_LINE_P1_ADDR,  0},
        {GFX_REG_CAR_ADDR,      0},
        {GFX_REG_SAFE_ADDR,     0},
        {GFX_REG_CR1_ADDR,      0},
        {GFX_REG_CR2_ADDR,      0},
        {GFX_REG_CR3_ADDR,      0},
        {GFX_REG_CMD_ADDR,      0},
        {GFX_REG_ICR_ADDR,      0},
        {GFX_REG_ISR_ADDR,      0},
        {GFX_REG_ID1_ADDR,      0},
        {GFX_REG_ID2_ADDR,      0},
        {GFX_REG_ID3_ADDR,      0},
        {GFX_REG_IDM1_ADDR,     0},
        {GFX_REG_IDM2_ADDR,     0},
        {GFX_REG_IDM3_ADDR,     0}
    }};

//=============================================================================
//                              Private Data Declaration
//=============================================================================
static volatile bool __hwCRC_en     = false;
static volatile bool __hwCRC_update = false;
#ifdef GFX_USE_INTERRUPT
static sem_t    gfxMutex;
#endif

static gfxBlitIdCallback gppfCallback[LAST_GFX_ID] = { 0 };
static int               gbEnableIdInterrupt = 0;

//=============================================================================
//                              Private Function Declaration
//=============================================================================
bool
_HwEngineMmioFire(
    GFX_HW_DEVICE*  dev);

bool
_HwEngineCmdQFire(
    GFX_HW_DEVICE*  dev);

#ifdef GFX_USE_INTERRUPT
static void
_IntrHandler(
    void* arg);

bool
_IntrCmdQSingleFire(
    void);
#endif

static void
_BlitIdIntrHandler(void* arg)
{
    uint32_t regVal = ithReadRegA(GFX_REG_ISR_ADDR);//clear

    //ID 0
    if (regVal & 0x1)
    {
        if (gppfCallback[0])
        {
            (*gppfCallback[0])();
        }
    }
    //ID 1
    if (regVal & 0x2)
    {
        if (gppfCallback[1])
        {
            (*gppfCallback[1])();
        }
    }
    //ID 1
    if (regVal & 0x4)
    {
        if (gppfCallback[2])
        {
            (*gppfCallback[2])();
        }
    }
}


//=============================================================================
//                              Public Function Definition
//=============================================================================
GFX_HW_DEVICE*
gfxHwInitialize()
{
    int i;
    uint32_t reg = 0;

#ifdef GFX_USE_INTERRUPT

    ithIntrDisableIrq(ITH_INTR_2D);
    ithReadRegA(GFX_REG_ISR_ADDR);//clear
    ithIntrClearIrq(ITH_INTR_2D);

    ithIntrRegisterHandlerIrq(ITH_INTR_2D, _IntrHandler, NULL);
    ithIntrEnableIrq(ITH_INTR_2D);

    // interrupt enable
    ithWriteRegA(GFX_REG_ICR_ADDR, 0x1);

    sem_init(&gfxMutex, 0, 0);
#endif

    // reset 2D, enable 2D clock
    reg = ithReadRegA(ITH_HOST_BASE + ITH_CQ_CLK_REG);
    //printf("read ITH_CQ_CLK_REG:0x%x\n",reg);
    ithWriteRegA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 0x42af8000 | reg);
    //printf("ITH_CQ_CLK_REG:0x%x\n", ithReadRegA(ITH_HOST_BASE + ITH_CQ_CLK_REG));
#ifndef WIN32
    for (i = 0; i<100; i++) asm volatile("");
#endif
    ithWriteRegA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 0x02af8000 | reg);
    //printf("ITH_CQ_CLK_REG:0x%x\n", ithReadRegA(ITH_HOST_BASE + ITH_CQ_CLK_REG));

    return &GfxHwDevice;
}

void
gfxHwRegisterBlitIdCallback(int id, gfxBlitIdCallback pfCallback)
{
    if (id < LAST_GFX_ID)
    {
        gppfCallback[id] = pfCallback;
    }

    if (!gbEnableIdInterrupt)
    {
        ithIntrDisableIrq(ITH_INTR_2D);
        ithReadRegA(GFX_REG_ISR_ADDR);//clear
        ithIntrClearIrq(ITH_INTR_2D);

        ithIntrRegisterHandlerIrq(ITH_INTR_2D, _BlitIdIntrHandler, NULL);
        ithIntrEnableIrq(ITH_INTR_2D);
        gbEnableIdInterrupt = 1;
    }
}

void
gfxHwTerminate(
    GFX_HW_DEVICE* dev)
{
    // disable 2D clock
    ithWriteRegMaskA(ITH_HOST_BASE + ITH_CQ_CLK_REG, 0, (0x1 << 15) | (0x1 << 17) | (0x1 << 19) | (0x1 << 23));
}

bool
gfxHwEngineFire(
    GFX_HW_DEVICE* dev)
{
    GFX_FUNC_ENTRY;

    if (1) {
        GFX_HW_REGS*    regs = &dev->regs;

        // Disable merge for testing
        //regs->regSAFE.data |= 0x00f00000;
        //regs->regSAFE.data |= 0x00000800;

//#if (CFG_CHIP_FAMILY == 9860) //for 9860 test chip
        // Disable read reorder
        regs->regSAFE.data |= GFX_REG_SAFE_RDREORDER_EN;
//        // Disable write reorder
//        regs->regSAFE.data |= GFX_REG_SAFE_WRREORDER_EN;
//#endif
        //Enable extend by zero for color depth extend
        regs->regCR1.data |= 0x00004000;
    }

    if (__hwCRC_en)
    {
        GFX_HW_REGS* regs = &dev->regs;
        if (__hwCRC_update)
        {
            regs->regCR1.data |= (GFX_REG_CR1_RDCRC_EN | GFX_REG_CR1_RDCRC_UPD |
                                  GFX_REG_CR1_WRCRC_EN | GFX_REG_CR1_WRCRC_UPD |
                                  GFX_REG_CR1_ALUCRC_EN | GFX_REG_CR1_ALUCRC_UPD );
        }
        else
        {
            regs->regCR1.data |= (GFX_REG_CR1_RDCRC_EN | GFX_REG_CR1_WRCRC_EN | GFX_REG_CR1_ALUCRC_EN);
        }
        __hwCRC_update = false;
    }

#ifndef GFX_USE_CMDQ
    return _HwEngineMmioFire(dev);
#else
    return _HwEngineCmdQFire(dev);
#endif
    GFX_FUNC_LEAVE;
}

void
gfxHwReset(
    GFX_HW_DEVICE* dev)
{
    GFX_HW_REGS*    regs    = &dev->regs;
    GFX_HW_REG*     currReg = &regs->regSRC;
    GFX_HW_REG*     endReg  = &regs->regIDM3;

    while(currReg <= endReg)
    {
        currReg->data = 0;
        currReg++;
    }

    regs->regITMR00.data = 0x80000;
    regs->regITMR11.data = 0x80000;
    regs->regITMR22.data = 0x80000;
}

bool
gfxwaitEngineIdle(
    void)
{
#ifndef CFG_CHIP_FT

    uint32_t timeout = 5000;

    if (1) {  // FIXME

#ifdef GFX_USE_INTERRUPT
        _IntrCmdQSingleFire();

        int result = 0;

        result = itpSemWaitTimeout(&gfxMutex, 20);
        if ( result )
        {
            /* Time out, fail! */
            printf("Time out, fail!\n");
            return false;
        }
        else
        {
            printf("gfxwaitEngineIdle success!\n");
            return true;
        }
#else

#ifdef GFX_USE_CMDQ
        ithCmdQWaitEmpty(ITH_CMDQ0_OFFSET);
#endif
        // Wait engine idle
        while((ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_BUSY) && timeout != 0) {
            usleep(200);
            timeout--;
        }
#endif
    }
    else //for test gfx CRC
    {
        while (1)
        {
            if (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + ITH_CMDQ0_OFFSET) & (0x1 << ITH_CMDQ_ALLIDLE_BIT))
                break;

            usleep(1000);
        }

        while (ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_BUSY) {
            usleep(200);
        }
    }

    if (timeout == 0)
    {
        printf("2D Wait engine idle timeout\n");
        return false;
    }
    else
        return true;
#else
    return true;
#endif
}

bool
gfxHwCRCInitialize()
{
    uint32_t status;
    status = ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_CRC32_IMPL;

    if (status)
    {
        __hwCRC_update = true;
        __hwCRC_en     = true;
        return true;
    }
    else
    {
        __hwCRC_update = false;
        __hwCRC_en     = false;
        return false;
    }
}

//#ifndef CFG_CHIP_FT
bool
gfxHWCRCGetValue(
    uint32_t* rdcrc,
    uint32_t* wrcrc,
    uint32_t* alucrc)
{
    uint32_t status;
    status = ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_CRC32_IMPL;

    if (status && __hwCRC_en)
    {
        //gfxwaitEngineIdle();
        while (1)
        {
            if (ithReadRegA(ITH_CMDQ_BASE + ITH_CMDQ_SR1_REG + ITH_CMDQ0_OFFSET) & (0x1 << ITH_CMDQ_ALLIDLE_BIT))
                break;

            usleep(1000);
        }

        while (ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_BUSY) {
            usleep(200);
        }

        *rdcrc = ithReadRegA(GFX_REG_RDCRC_ADDR);
        *wrcrc = ithReadRegA(GFX_REG_WRCRC_ADDR);
        *alucrc = ithReadRegA(GFX_REG_ALUCRC_ADDR);

        return true;
    }
    else
    {
        return false;
    }
}
//#endif

//=============================================================================
//                              Private Function Definition
//=============================================================================
bool
_HwEngineMmioFire(
    GFX_HW_DEVICE*  dev)
{
    GFX_HW_REGS*    regs = &dev->regs;
    GFX_HW_REG*     currReg = &regs->regSRC;
    GFX_HW_REG*     endReg = &regs->regICR;

    // Wait engine idle
    while ((ithReadRegA(GFX_REG_ST1_ADDR) & GFX_REG_ST1_BUSY));

    while(currReg < endReg)
    {
        ithWriteRegA(currReg->addr, currReg->data);
        //ithPrintf("0x%08x, 0x%08x\n", currReg->addr, currReg->data);
        currReg++;
    }

    return true;
}

#ifndef CFG_CHIP_FT
bool
_HwEngineCmdQSingleFire(
    GFX_HW_DEVICE*  dev)
{
    bool            result = true;
    GFX_HW_REGS*    regs = &dev->regs;
    GFX_HW_REG*     currReg = &regs->regSRC;
    GFX_HW_REG*     endReg = &regs->regCMD;
    uint32_t        requestCmdQLen = 0;
    uint32_t*       cmdqAddr = NULL;

    ithCmdQLock(ITH_CMDQ0_OFFSET);

    requestCmdQLen = ((uint32_t)endReg - (uint32_t)currReg) * 2;
    cmdqAddr = ithCmdQWaitSize(requestCmdQLen, ITH_CMDQ0_OFFSET);

    while(currReg <= endReg)
    {
        ITH_CMDQ_SINGLE_CMD(cmdqAddr, currReg->addr, currReg->data);
        //ithPrintf("0x%08x, 0x%08x\n", currReg->addr, currReg->data);
        //ithPrintf("0x%08x, 0x%08x\n", currReg->addr, ithReadRegA(currReg->addr));
        currReg++;
    }

    ithCmdQFlush(cmdqAddr, ITH_CMDQ0_OFFSET);
    ithCmdQUnlock(ITH_CMDQ0_OFFSET);

    return result;
}
#endif

bool
_HwEngineCmdQBurstFire(
    GFX_HW_DEVICE*  dev)
{
    bool result = false;

    return result;
}

#ifndef CFG_CHIP_FT
bool
_HwEngineCmdQFire(
    GFX_HW_DEVICE*  dev)
{
    bool result = false;

#ifndef GFX_USE_COMQ_BURST
    result = _HwEngineCmdQSingleFire(dev);
#else
    result = _HwEngineCmdQBurstFire(dev);
#endif

    return result;
}
#endif

#ifdef GFX_USE_INTERRUPT
static void
_IntrHandler(void* arg)
{
    ithReadRegA(GFX_REG_ISR_ADDR);//clear
    itpSemPostFromISR(&gfxMutex);
    //printf("GFX isr!\n");
}

#ifndef CFG_CHIP_FT
bool
_IntrCmdQSingleFire(
    void)
{
    bool            result = true;
    uint32_t        requestCmdQLen = 0;
    uint32_t*       cmdqAddr = NULL;

    ithCmdQLock();

    cmdqAddr = ithCmdQWaitSize(8, ITH_CMDQ0_OFFSET);

    ITH_CMDQ_SINGLE_CMD(cmdqAddr, GFX_REG_ID1_ADDR, 0x1);

    ithCmdQFlush(cmdqAddr, ITH_CMDQ0_OFFSET);

    ithCmdQUnlock();

    return result;
}
#endif

#endif

