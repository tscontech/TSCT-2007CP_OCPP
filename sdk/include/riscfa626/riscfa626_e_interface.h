/*****************************************************************************
 * riscfa626 I2C Bus Driver Header
 *
 *
 * Copyright(C) CHIPSBRAIN GLOBAL CO., Ltd.
 * All rights reserved.
 *
 * File Name    : riscfa626_e_interface.h
 *
 * Version      : V0.3
 * Date         : 2015.09.08
 * Description  : riscfa626 I2C Bus Driver
 ****************************************************************************/

#ifndef _RISCFA626_E_H
#define _RISCFA626_E_H

#include <stdint.h>
#include <stdbool.h>

/*  Conditional extern definition */
/*  ============================================================================= */
/* ****************************************************************************** */

extern uint8_t RISC626Write (uint8_t bDevAddr, uint8_t *pbAddr, uint8_t wAddrLen, uint8_t *pbData, uint8_t wData);
extern uint8_t RISC626Read ( uint8_t bDevAddr, uint8_t *pbAddr, uint8_t wAddrLen, uint8_t *pbData, uint8_t wData);
#endif
