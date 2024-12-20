/*****************************************************************************
 * riscfa626 Interface driver
 *
 *
 * Copyright(C) CHIPSBRAIN GLOBAL CO., Ltd.
 * All rights reserved.
 *
 * File Name    : riscfa626_interface.h
 *
 * Version      : V0.3
 * Date         : 2015.09.08
 * Description  : riscfa626 Interface Header
 ****************************************************************************/

#ifndef __riscfa626_INTERFACE_H_
#define __riscfa626_INTERFACE_H_

#ifdef __cplusplus
extern "C"
{
#endif

  uint8_t riscfa626_read_data( uint16_t sub_addr, int read_len, uint8_t * r_data );
  uint8_t riscfa626_write_data( uint16_t sub_addr, uint8_t * w_data, int write_len );
  void riscfa626_delay ( uint32_t wait_time );
  uint8_t riscfa626_power_on( void );
#ifdef __cplusplus
}

#endif

#endif /* __riscfa626_INTERFACE_H_ */
