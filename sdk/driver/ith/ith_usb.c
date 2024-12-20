/*
 * Copyright (c) 2011 ITE Tech. Inc. All Rights Reserved.
 */
/** @file
 * HAL USB functions.
 *
 * @author Jim Tan
 * @version 1.0
 */
#include <unistd.h>
#include "ith_cfg.h"


void ithUsbSuspend(ITHUsbModule usb)
{
    ithWriteRegMaskA(usb + 0x3C, 0x0, 0x14);
    ithSetRegBitA(usb + ITH_USB_HC_MISC_REG, ITH_USB_HOSTPHY_SUSPEND_BIT);
}

void ithUsbResume(ITHUsbModule usb)
{
    ithWriteRegMaskA(usb + 0x3C, 0x10, 0x14);
    ithClearRegBitA(usb + ITH_USB_HC_MISC_REG, ITH_USB_HOSTPHY_SUSPEND_BIT);
}

void ithUsbEnableClock(void)
{
    ithWriteRegMaskA(ITH_HOST_BASE + ITH_USB_CLK_REG, 0xA, 0xA);
}

void ithUsbDisableClock(void)
{
    ithWriteRegMaskA(ITH_HOST_BASE + ITH_USB_CLK_REG, 0x0, 0xA);
}

void ithUsbReset(void)
{
    // for 9860
    ithWriteRegA(ITH_HOST_BASE + ITH_USB_CLK_REG, 0xC0A80000);
    usleep(5*1000);
    ithWriteRegA(ITH_HOST_BASE + ITH_USB_CLK_REG, 0x00A80000);
    usleep(5*1000);
}

void ithUsbInterfaceSel(ITHUsbInterface intf)
{
    #if 0 // wrap default setting fail
    if (intf == ITH_USB_AMBA)
        ithWriteRegMaskA(ITH_USB0_BASE + 0x34, 0x1, 0x1);
    else
        ithWriteRegMaskA(ITH_USB0_BASE + 0x34, 0x0, 0x1);
    #else
    //uint32_t reg = 0x422A0800;
    uint32_t reg = 0x442A0800;
    if (intf == ITH_USB_AMBA)
        reg |= 0x1;
    else
        reg &= ~0x1;
    ithWriteRegA(ITH_USB0_BASE + 0x34, reg);
    #endif
}

void ithUsbPhyPowerOn(ITHUsbModule usb)
{
    ithWriteRegA(usb + 0x3C, 0x00031907);
}

int ithUsbDevicePresent(ITHUsbModule usb)
{
    return ithReadRegA(usb + 0x30) & 0x1;
}

void ithUsbForceDeviceMode(ITHUsbModule usb, bool enable)
{
    if (enable)
        ithWriteRegMaskA(usb + 0x38, (0x1 << 9), (0x1 << 9));
    else
        ithWriteRegMaskA(usb + 0x38, 0, (0x1 << 9));
}

bool ithUsbIsDeviceMode(ITHUsbModule usb)
{
    if (ithReadRegA(usb + 0x80) & (0x1 << 21))
        return true;
    else
        return false;
}

#define USB_WRAP_REG						0x34
#define USB_WRAP_FLUSH_EN				(0x1 << 22)
#define USB_WRAP_FLUSH_FIRE_END (0x1 << 23)

void ithUsbWrapFlush(void)
{
    uint32_t reg = ithReadRegA(ITH_USB0_BASE + 0x34);
    /* only for wrap path */
    if (!(reg & 0x1)) {

        ithEnterCritical();

        /* flush usb's ahb wrap */
        ithWriteRegMaskA(ITH_USB0_BASE + USB_WRAP_REG, USB_WRAP_FLUSH_EN, USB_WRAP_FLUSH_EN);
        ithWriteRegMaskA(ITH_USB0_BASE + USB_WRAP_REG, USB_WRAP_FLUSH_FIRE_END, USB_WRAP_FLUSH_FIRE_END);

        // wait AHB Wrap flush finish!
        while ((ithReadRegA(ITH_USB0_BASE + USB_WRAP_REG) & USB_WRAP_FLUSH_FIRE_END));

        ithWriteRegMaskA(ITH_USB0_BASE + USB_WRAP_REG, 0x0, USB_WRAP_FLUSH_EN);

        /* flush axi2 wrap */
        if(ithMemWrapFlush(ITH_MEM_AXI2WRAP))
            ithPrintf("+\n");

        ithExitCritical();
    }
}
