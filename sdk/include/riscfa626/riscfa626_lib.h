/*
* @brief riscfa626 LIB Driver
*
* @note
* Copyright(C) CHIPSBRAIN CO., Ltd., 1999 ~ 2016
 * All rights reserved.
*
* File Name 	: riscfa626_lib.h
*
* Version	: V1.17
* Date 		: 2015.08.07
* Description : riscfa626 LIB Header
*/
#ifndef __riscfa626_LIB_H_
#define __riscfa626_LIB_H_

#include<stdint.h>
#include<stdbool.h>

#define MCU_TYPE_8BIT		0
#define MCU_TYPE_16BIT	1
#define MCU_TYPE_32BIT	2

#define MCU_TYPE	MCU_TYPE_32BIT

#ifndef CONST
#define CONST           const
#endif

#ifndef uint8_t
typedef unsigned          char uint8_t;
#endif

#ifndef uint16_t
typedef unsigned short     int uint16_t;
#endif

#ifndef NULL
#define NULL	(void*)0
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
	riscfa626_STATUS_OK = 0,
	riscfa626_STATUS_FAILED = 1,
	riscfa626_NOT_INITIALIZE_LIB = 2,
	riscfa626_NOT_SUPPORT_MODE = 3,
	riscfa626_NOT_SUPPORT_PAGE_INDEX = 4,
	riscfa626_NOT_SUPPORT_BLOCK_INDEX = 5,
	riscfa626_NOT_SUPPORT_WRITE_PAGE = 6,
	riscfa626_NOT_SUPPORT_READ_PAGE = 7,
	riscfa626_NOT_SUPPORT_CHANGE_PASSWORD_IN_PAGE = 8,
	riscfa626_FAILED_CHANGE_PASSWORD = 9,
	riscfa626_ERROR_SW_AES_ENC_DEC	= 10,
	riscfa626_NOT_MACHED_AES_KEY_SIZE = 11,
	riscfa626_NULL_POINT_EXCEPTION = 12,
	riscfa626_NOT_SUPPORT_BLOCK = 13,
	riscfa626_ERROR_DOUBLE_BLOCK_EMPTY	= 14,
	riscfa626_ERROR_AUTHENTICATION	= 15,
	riscfa626_OK_AUTHENTICATION	= 16,
	riscfa626_NOT_SUPPORT_BLOCK_SIZE = 17,
	riscfa626_FAILED_PASSWORD = 18,
} riscfa626_STATUS_T;

#define riscfa626_SHUTDOWN				0
#define riscfa626_DELAY_SHUTDOWN	1

#define AES_REQ_ENCODING				0
#define AES_REQ_DECODING				1

#define SET_AES_KEY_SIZE_128			0
#define SET_AES_KEY_SIZE_192			1
#define SET_AES_KEY_SIZE_256			2

#define REQ_AES_NONE_WRITE			0
#define REQ_AES_ENCODING_WRITE	1

#define REQ_AES_NONE_READ			0
#define REQ_AES_ENCODING_READ	1


#define MAX_EEPROM_BLOCK_NUM	16
#define EEPROM_BANK_LEN				64
#define MAX_AES_BUFFER_SIZE			16

extern uint8_t riscfa626_read_data( uint16_t sub_addr, int read_len, uint8_t * r_data );
extern uint8_t riscfa626_write_data( uint16_t sub_addr, uint8_t * w_data, int write_len );
extern void riscfa626_delay( uint32_t wait_time );
extern uint8_t riscfa626_power_on( void );

	/**
	 * name : uint8_t riscfa626_init(uint8_t *r_seral_data)
	 * @brief	: riscfa626 Initialize LIB
	 * @param	 r_seral_data	: Pointer to serial number of riscfa626
	 * @return	On success returns riscfa626_STATUS_OK.
	 *
	 */
	uint8_t riscfa626_init( uint8_t * r_seral_data );

	/**
	 * name : uint8_t get_lib_version(uint8_t *pVer, uint8_t *len);
	 * @brief	: get information of LIB
	 * @param	 pVer : Pointer to version information of LIB
	 * @param	 len	: length to version information of LIB
	 * @return	On success returns riscfa626_STATUS_OK.
	 *
	 */
	uint8_t get_lib_version( uint8_t * pVer, uint8_t  *len );


	/**
	 * name : uint8_t riscfa626_eeprom_read( uint8_t page, uint8_t *r_data, uint8_t encrytion )
	 * @brief	: read EEPROM page data
	 * @param	 page : page index to read EEPROM data (0 ~ 29)
	 * @param	 r_data : pointer to read EEPROM ( mininum buffer size are 64)
	 * @param	 encrytion : read data are required aes decryption.
	 *                                      0 : require raw data
	 *                                      1 : require decrypted data.
	 * @return	On success returns riscfa626_STATUS_OK.
	 *            On not initialize lib returns riscfa626_NOT_INITIALIZE_LIB
	 *            On page_index greate than 29 returns riscfa626_NOT_SUPPORT_PAGE_INDEX
	 *            On Support Inkjet Mode and page_index less than Range Inkjet Area than returns riscfa626_NOT_SUPPORT_READ_PAGE
	 *
	 */

	uint8_t riscfa626_eeprom_read( uint32_t password, uint8_t page, uint8_t *r_data, uint8_t encrytion );

	/**
	 * name : uint8_t riscfa626_eeprom_write( uint8_t page, uint8_t *w_data, uint8_t encrytion )
	 * @brief	: write EEPROM page data
	 * @param	 uint8_t riscfa626_eeprom_write( uint8_t page, uint8_t *w_data, uint8_t encrytion ) : page index to write EEPROM data (0 ~ 29)
	 * @param	 w_data : pointer to read EEPROM( buffer size are 64)
	 * @param	 encrytion : write data are required aes encryption.
	 *                                      0 : require raw data
	 *                                      1 : require encryption data.
	 * @return	On success returns riscfa626_STATUS_OK.
	 *            On not initialize lib returns riscfa626_NOT_INITIALIZE_LIB
	 *            On page_index greate than 29 returns riscfa626_NOT_SUPPORT_PAGE_INDEX
	 *            On Support Inkjet Mode and page_index less than Range Inkjet Area than returns riscfa626_NOT_SUPPORT_WRITE_PAGE
	 *
	 */
	uint8_t riscfa626_eeprom_write( uint32_t password, uint8_t page, uint8_t *w_data, uint8_t encrytion );

	/**
	 * name : riscfa626_eeprom_pwchg( uint8_t page, uint32_t old_password, uint32_t new_password )
	 * @brief	: change password of EEPROM Block
	 * @param	 page : index to EEPROM Block(0 ~ 29)
   * @param	 old_password : old password
	 * @param	 new_password : new password
	 * @return	On success returns riscfa626_STATUS_OK.
	 *            On not initialize lib returns riscfa626_NOT_INITIALIZE_LIB
	 *            On block_index greate than 14 returns riscfa626_NOT_SUPPORT_BLOCK_INDEX
	 *            On Support Inkjet Mode and block_index less than Range Inkjet Area than returns riscfa626_NOT_SUPPORT_CHANGE_PASSWORD_IN_PAGE
	 *
	 */
	uint8_t riscfa626_eeprom_pwchg( uint8_t page, uint32_t old_password, uint32_t new_password );



	/**
	 * name : uint8_t riscfa626_req_enc_dec(uint8_t *req_data, uint8_t *result_data, uint8_t mode);
	 * @brief	: Request Encryption and Decryption data
	 * @param	 req_data : request data for encryption or decryption (must be buffer length is 16)
	 * @param	 result_data : return result data for encryption or decryption (must be buffer length is 16)
	 * @param	 mode : 	AES_REQ_ENCODING
	 * 				AES_REQ_DECODING
	 * @return	On success returns riscfa626_STATUS_OK.
	 *            On not initialize lib returns riscfa626_NOT_INITIALIZE_LIB
	 *            On is_req_encryption_to_write greate than 2 returns riscfa626_NOT_SUPPORT_MODE
	 *
	 */
	uint8_t riscfa626_req_enc_dec( uint8_t * req_data, uint8_t * result_data, uint8_t mode );

	/**
	 * name : uint8_t riscfa626_bypass_mode(uint8_t *req_bypass_data,  uint8_t *bypassed_data);
	 * @brief	: Request Bypass Mode
	 * @param	 req_bypass_data : request source data for Bypass (must be buffer length is 16)
	 * @param	 bypassed_data : return result data for Bypass (must be buffer length is 16)
	 * @return	On success returns riscfa626_STATUS_OK.
	 *            On not initialize lib returns riscfa626_NOT_INITIALIZE_LIB
	 *
	 */
	uint8_t riscfa626_bypass_mode( uint8_t * req_bypass_data, uint8_t * bypassed_data );

	/**
	 * name : uint8_t riscfa626_set_aes_key_size(uint8_t aes_key_size);
	 * @brief	: Request Set Aes Key Size
	 * @param	 aes_key_size : 	SET_AES_KEY_SIZE_128
	 * 						SET_AES_KEY_SIZE_192
	 *						SET_AES_KEY_SIZE_256
	 * @return	On success returns riscfa626_STATUS_OK.
	 *            On not initialize lib returns riscfa626_NOT_INITIALIZE_LIB
	 *            On mode greate than 1 returns riscfa626_NOT_SUPPORT_MODE
	 *
	 */
	uint8_t riscfa626_set_aes_key_size( uint8_t aes_key_size );

	/**
	 * name : uint8_t riscfa626_read_inkjet_counter(uint16_t *counter_value);
	 * @brief	: read inkjet counter
	 * @param	 counter_value : inkjet counter
	 * @return	On success returns riscfa626_STATUS_OK.
	 *            On not initialize lib returns riscfa626_NOT_INITIALIZE_LIB
	 *            On not enabled Inkjet mode returns riscfa626_NOT_SUPPORT_MODE
	 *
	 */
	uint8_t riscfa626_read_inkjet_counter( uint16_t * counter_value );

	/**
	 * name : uint8_t riscfa626_inc_inkjet_counter(void);
	 * @brief	: increment inkjet counter
	 * @return	On success returns riscfa626_STATUS_OK.
	 *            On not initialize lib returns riscfa626_NOT_INITIALIZE_LIB
	 *            On not enabled Inkjet mode returns riscfa626_NOT_SUPPORT_MODE
	 *
	 */
	uint8_t riscfa626_inc_inkjet_counter( void );

	/**
	 * name : uint8_t riscfa626_powersave(uint8_t cmd, uint32_t time)
	 * @brief	: power off
	 * @param	 cmd : riscfa626_SHUTDOWN, riscfa626_DELAY_SHUTDOWN
	 * @param	 time : if(cmd == riscfa626_SHUTDOWN) NONE
	 *                if(cmd == riscfa626_DELAY_SHUTDOWN) valid(20 ~ 1400) / unit : ms
	 * @return	On success returns riscfa626_STATUS_OK.
	 *
	 */
	uint8_t riscfa626_powersave(uint8_t cmd, uint32_t time);

	/**
	 * name : void riscfa626_srand(uint32_t seed);
	 * @brief	: set random seed
	 * @param	 seed : random seed
	 * @return	NONE.
	 *
	 */
	void riscfa626_srand( uint32_t seed );

	/**
	 * name : vuint8_t riscfa626_random( void );
	 * @brief	: random value generation
	 * @return :	random value.
	 *
	 */
	uint8_t riscfa626_random( void );

	/**
	 * name : uint8_t riscfa626_authorization(uint8_t aes_key_size, uint32_t seed, uint8_t *raw_data);
	 * @brief	: authorization
	 * @param	 aes_key_size :
     *                      SET_AES_KEY_SIZE_128
	 * 			         SET_AES_KEY_SIZE_192
	 *			             SET_AES_KEY_SIZE_256
	 * @param	 seed : random seed
	 *                             0 : it is not used as  random SEED value.
	 *                Non-zero : it is used as random SEED value.
     * @param	 raw_data : input data(must be buffer length is 16)
     * @return	TRUE : success
     *               FALSE : Fail
	 *
	 */
   uint8_t riscfa626_authentication(uint8_t aes_key_size, uint32_t seed, uint8_t *indata);

#ifdef __cplusplus
}

#endif

#endif /* __riscfa626_LIB_H_ */
