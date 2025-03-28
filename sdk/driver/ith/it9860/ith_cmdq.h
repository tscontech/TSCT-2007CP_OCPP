﻿#ifndef ITE_ITH_CMDQ_H
#define ITE_ITH_CMDQ_H

#include "ite/ith.h"
#include "ith/ith_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* Initializes command queue
*
* @param cmdQ the command queue instance
*/
void ithCmdQInit(ITHCmdQ* cmdQ, ITHCmdQPortOffset portOffset);

/**
* Terminates command queue
*/
void ithCmdQExit(ITHCmdQPortOffset portOffset);

/**
* Resets command queue hardware
*/
void ithCmdQReset(ITHCmdQPortOffset portOffset);

/**
* Waits specified command queue size.
*
* @param size the specified size to wait.
*/
uint32_t* ithCmdQWaitSize(uint32_t size, ITHCmdQPortOffset portOffset);

/**
* Flushes command queue buffer.
*
* @param ptr the end of command queue buffer.
*/
void ithCmdQFlush(uint32_t* ptr, ITHCmdQPortOffset portOffset);

/**
* Hardware flip LCD.
*
* @param index the index to flip. 0 is A, 1 is B, 2 is C.
*/
void ithCmdQFlip(unsigned int index, ITHCmdQPortOffset portOffset);

/**
* Waits command queue empty.
*/
int ithCmdQWaitEmpty(ITHCmdQPortOffset portOffset);

/**
* Waits the interrupt of command queue.
*/
int ithCmdQWaitInterrupt(void);

/**
* Enable command queue clock.
*/
void ithCmdQEnableClock(void);

/**
* Disable command queue clock.
*/
void ithCmdQDisableClock(void);

/**
* Enables specified command queue controls.
*
* @param ctrl the controls to enable.
*/
void ithCmdQCtrlEnable(ITHCmdQCtrl ctrl, ITHCmdQPortOffset portOffset);

/**
* Disables specified command queue controls.
*
* @param ctrl the controls to disable.
*/
void ithCmdQCtrlDisable(ITHCmdQCtrl ctrl, ITHCmdQPortOffset portOffset);

#if defined(CMDQ_IRQ_ENABLE) 
/**
* Clears the interrupt of command queue.
*/
void ithCmdQClearIntr(ITHCmdQPortOffset portOffset);

/**
* enable the interrupt of command queue.
*/
void ithCmdQEnableIntr(ITHCmdQPortOffset portOffset);
#endif

#ifdef __cplusplus
}
#endif

#endif
