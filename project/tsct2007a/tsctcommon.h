/**
*       @file
*               tsctcommon.h
*       @brief
*   		Copyright (c) Costel All rights reserved. <br>
*               date: 2017.12.05 <br>
*               author: dyhwang <br>
*               description: <br>
*/
#ifndef __CSTCOMMON_H__
#define __CSTCOMMON_H__

//-----------------------------------------------------------------------
// MACRO
//-----------------------------------------------------------------------

typedef enum {
	CH1 = 0,
	CH2 = 1,
	MaxCH = 2,
} ChannelType;

typedef enum {
	INPUT = 0,
	OUTPUT = 1,
} GPIO_InOut;

typedef enum {
	SET_LOW = 0,
	SET_HIGH = 1,
} GPIO_SET;

bool bAdminExit;
bool bErrorcheck1ShowHide;
bool bErrorcheck2ShowHide;
extern bool bEmbCheckShowHide;
bool rfredercheck;
extern bool pingtestcheck;
bool bGloAdminStatus;
bool bRouterPower;

// EV 충전??종류 구분
#define CP_TYPE_TOTAL                                   0xFF    //??체 ??비
#define CP_TYPE_CAR_DC_HIGH_SPEED                       0x01    // ??용??DC급속
#define CP_TYPE_CAR_AC_NORMAL_SPEED                     0x02    // ??용??AC??속
#define CP_TYPE_CAR_NO_TOUCH                            0x03    // ??용??비접??
#define CP_TYPE_CAR_AC_HIGH_SPEED                       0x04    // ??용??AC급속
#define CP_TYPE_BUS_DC_HIGH_SPEED                       0x11    // 버스 DC급속
#define CP_TYPE_BUS_AC_NORMAL_SPEED                     0x12    // 버스 AC??속
#define CP_TYPE_BUS_NO_TOUCH                            0x13    // 버스 비접??
#define CP_TYPE_BUS_AC_HIGH_SPEED                       0x14    // 버스 AC급속
#define CP_TYPE_MOTORCYCLE_HIGH_SPEED                   0x21    // ??륜??급속
#define CP_TYPE_MOTORCYCLE_NORMAL_SPEED                 0x22    // ??륜????속
#define CP_TYPE_CAR_HIGH_SPEED_AND_BUS_HIGH_SPEED       0x31    // ??용??급속+버스 급속



// Common
#define GPIO_BUZZER			45
#define GPIO_PILOT_PWR1		22
#define GPIO_PILOT_PWR2		19

#define GPIO_BACKLIGHT		38
#define GPIO_AC220_GFCI		26

#define	GPIO_MAGNET_CTL1	15
#define	GPIO_ROUTER_CTL	16

#define GPIO_LED_RAMP1		44
#define GPIO_LED_RAMP2		44

#define GPIO_EMBUTTON		24

#define GPIO_CP1_RELAY_CTL	22

#define GPIO_PLC_PWR_RELAY_CTL	19

#define GPIO_RS485_EN1		11
#define RS485_DEV1			ITH_RS485_2
#define RS485_IPT1			ITP_DEVICE_UART2

#define SECC_DEV ITP_DEVICE_UART3

#define GPIO_RS485_EN2		49
#define RS485_DEV2			ITH_RS485_3
#define RS485_IPT2			ITP_DEVICE_RS485_3

#define GPIO_DOOR_OPEN0		-1
#define GPIO_DOOR_CTR0		-1

#define GPIO_DOOR_OPEN1		-1
#define GPIO_DOOR_CTR1		-1

// #define GPIO_AUDIO_EN		25 
#define GPIO_AUDIO_PWM		-1

#define GPIO_RFID_TX		41
#define GPIO_RFID_RX		42

#define AUDIO_PWM_NUM		ITH_PWM3

#define CARD_READER_DEV		ITP_DEVICE_UART1

#define OBD_READER_DEV		ITP_DEVICE_UART4

#define	CST_CP0_PWM				ITH_PWM2
#define	CST_GPIO_CP0			23

/* GPIO Function Mode Setting..*/
#define GPIO_SET_IN		0
#define GPIO_SET_OUT	1

#define CHx_GFCI			0
#define CHx_RFID			1
#define CH1_WHM				2
#define CH1_AC220_IN		3
#define CH1_AC220_OUT		4
#define CH2_WHM				5
#define CH2_AC220_IN		6
#define CH2_AC220_OUT		7
extern int gSystemErrorCode;

typedef enum ChargeStep_t {
	STEP_UNKNOWN = 0,
	STEP_START,
	STEP_AUTH_USER,
    STEP_REQ_CONNECT,
    STEP_REQ_CLOSE,
	STEP_CHARGE,
	STEP_FINISH,
	STEP_DISCONNECT,
	STEP_THANKS,
	STEP_FTPFWUP,
	STEP_REBOOT,	
} ChargeStep;

#define READ_QUEUE_SIZE			64

typedef struct {
	char queue[READ_QUEUE_SIZE];
	int rear;
	int front;
} ReadQueue;


typedef struct {
	char NumCard[16];			// HEX	Card num
	char AuthKey[30];			// ASC	Authorize Key
	char Watt[4];				// Hex	Charged Watt
	char nTime[3];			// BCD	Charged Time
	char nPrice[4]; 	/// HEX Charged Money 
	char byPay; 			// Hex	How to Paid...
	char bCharge;			// HEX	 Status of Charging.
	unsigned char endTime[7];		// BCD the Time of charging end.
	char nRate[2];			// HEX rate of discount...
	char dcPrice[4];		// HEX discount price...
	char unitCostType;		// HEX discount price...	
	char startTime[7];		// HEX discount price...
} ChargeEndData;

typedef struct {
	unsigned char nCount;
	ChargeEndData nData[12];
} ChargedSavData;

ChargedSavData gChargedDataFrame; 

void DumpBuffer(char* msg, char* data, int size);
void ReadQueueInit(ReadQueue *q);
int ReadQueueIsEmpty(ReadQueue *q);
int ReadQueueIsFull(ReadQueue *q);
void ReadQueueEnqueue(ReadQueue *q, char c);
char ReadQueueDequeue(ReadQueue *q);
int GetQueueDataNum(ReadQueue *q);

typedef void (*NumkeyInputListener)(char);
typedef void (*sNumkeyInputListener)(char);
typedef void (*PasskeyInputListener)(char);
typedef void (*TopTimerTimeoutListener)(void);
typedef void (*DialogTimerTimeoutListener)(void);
typedef void (*GroundFaultListener)(void);

void StartPwm(ChannelType ch);
void StopPwm(ChannelType ch);

void CstSetUserActiveChannel(ChannelType ch);
ChannelType CstGetUserActiveChannel();
void CstSetUsedEnergy(float energy, int time);
void CstGpioSetModeState(unsigned int nGpio, int bOut, int bSet);
extern void ConfigInitSave();
unsigned int CharToInt(char *p, unsigned char len);
void AudioAmpPowerSet(int bSet);
unsigned int FourByteOrder(unsigned char* buf );
void ValueOrderFourByte(unsigned char* buf, unsigned int nValue);

void CstGfciInit(void);

void ComMsgQueueInit(void);
void ComMsgQueueAdd(int ch, int nCom);
void ComMsgQueueRemove(int ch, int nCom);
void GetCurComMsg(int *pCh, int *pCom);
void DumpMsg(const char *msgType, int ch, const int msg_len, const unsigned char *msgBuf);
bool CstCheckDoorLock(int ch);
bool CstCheckDoorClosed(int ch);
void CstSetEpqStatus(unsigned int nBit, bool bSet);
bool CstGetEpqStatus(unsigned int nBit);
void CstEUCKR2UTF8(char *UTF8, char *EUKkr);
void CstEUCKR2UTF8Test(void);
void CstTimeValueToBCD(char *pBuf, unsigned int Value);
void CstFwUpdateMsg(void);
void FtpIMGUpdate_func(void);
void FtpFwUpdate_func(void);
time_t CstGetTime();
time_t CstGetTime_Msec();
time_t CstGetTime_Msec_test();
void CurrentDateTime(unsigned char* buf);
unsigned short CalcuCRC16(const char *data, const int datacnt);

int RS232Write(char *data, size_t size);
int RS232Read(char *data, size_t size);
time_t evc_get_time();
void StartStopCharging(bool bSet, int ch);

extern void CstAriaTest(void);
extern void ARIA_test(void) ;

extern void CstAriaEncrypt(char *pInput, char * pOut);
extern void CstAriaDecrypt(char *pInput, char * pOut);

void CstGetUsedEnergy(float *energy, int *time);

bool GetHomeLayer(void);
void SetHomeLayer(bool bset);

uint16_t TsctCrc16(uint8_t *buffer, uint16_t buffer_length);
void RouterPowerCheck(void);

#endif


#define card_text
