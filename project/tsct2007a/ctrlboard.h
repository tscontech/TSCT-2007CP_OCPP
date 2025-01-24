/** @file
 * ITE Display Control Board Modules.
 *
 * @author Jim Tan
 * @version 1.0
 * @date 2015
 * @copyright ITE Tech. Inc. All Rights Reserved.
 */
/** @defgroup ctrlboard ITE Display Control Board Modules
 *  @{
 */
#ifndef CTRLBOARD_H
#define CTRLBOARD_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ADDRESS     70

/** @defgroup ctrlboard_audio Audio Player
 *  @{
 */

typedef int (*AudioPlayCallback)(int state);

/**
 * Initializes audio module.
 */
void AudioInit(void);

/**
 * Exits audio module.
 */
void AudioExit(void);

/**
 * Plays the specified wav file.
 *
 * @param filename The specified wav file to play.
 * @param func The callback function.
 * @return 0 for success, otherwise failed.
 */
int AudioPlay(char* filename, AudioPlayCallback func);

/**
 * Stops playing sound.
 */
void AudioStop(void);

int AudioPlayMusic(char* filename, AudioPlayCallback func);

/**
 * Plays keypad sound.
 */
void AudioPlayKeySound(void);
void AudioPauseKeySound(void);
void AudioResumeKeySound(void);

/**
 * Sets the volume of keypad sound.
 *
 * @param level the percentage of volume.
 */
void AudioSetKeyLevel(int level);

/**
 * Mutes all audio.
 */
void AudioMute(void);

/**
 * Un-mutes all audio.
 */
void AudioUnMute(void);

/**
 * Determines whether this audio is muted or not.
 *
 * @return true muted, false otherwise.
 */
bool AudioIsMuted(void);

bool AudioIsPlaying(void);

void AudioSetVolume(int level);
void AudioSetLevel(int level);
int AudioGetVolume(void);

int UpdateImg(void);

void FaultManageInit(void);


/** @} */ // end of ctrlboard_audio

/** @defgroup ctrlboard_config Configuration
 *  @{
 */
/**
 * Language type definition.
 */
typedef enum
{
    LANG_ENG,   ///< English
    LANG_CHT,   ///< Traditional Chinese
    LANG_CHS    ///< Simplified Chinese
} LangType;

typedef enum
{
    WIFI_CLIENT = 0,   ///< Client mode
    WIFI_SOFTAP,   ///< SoftAP mode
} WifiType;

bool disableHotspot;
/**
 * Main menu type definition.
 */
typedef enum
{
    MAINMENU_COVERFLOW,             ///< Cover flow
    MAINMENU_COVERFLOW_REFLECTION,  ///< Cover flow with reflection effect
    MAINMENU_PAGEFLOW_FLIP,         ///< Flip page flow
    MAINMENU_PAGEFLOW_FLIP2,        ///< Flip2 page flow
    MAINMENU_PAGEFLOW_FOLD,         ///< Fold page flow
    MAINMENU_COVERFLOW_RIPPLE       ///< Cover flow with ripple effect
} MainMenuType;


typedef enum {
	STOP_REASON_NONE = 0,
	STOP_REASON_RM,
	STOP_REASON_EVDIS,
	STOP_REASON_SOFTREST,
	STOP_REASON_HARDRESET,
	STOP_REASON_LOCAL,
    STOP_REASON_POWER,
    STOP_REASON_DEAUTH,
} STOP_REASON;


typedef struct {	
    char dhcp;
	//Application
	char configcheck[2];
	char authkey[17];                                                    ///< Kepco authkey
	char applver[9];                                                    ///< Appl Ver
	char applmodel[11];													///<Appl Model
	char timeinit[2];                                                    ///< 시간 동기화후 리셋??? 화면에 시간은 동기화후 리부팅이 되어야 제대로 표시가 된다.
    int mainmenu_type;                                                  ///< Main menu type @see MainMenuType
        // photo
    int       photo_interval;                                           // /< Photo change interval (second)
    char ssid[64];
    char password[256];
    char secumode[3];
    int wifi_mode;
    int wifi_on_off;
    // wifi
    char ap_ssid[64];  // ap mode ssid
    char ap_password[256]; // ap mode password
    // setting
// login
    char user_id[64];
    char user_password[64];

    int touch_calibration;         

	//BasicConfig
	int devchanel;                                                    ///< Chanel
	char devid1[3];                                                    ///< Id
	char devid2[3];                                                    ///< Id
	char siteid[9];                                                   ///< site Id
	char adminpassword[5]; 													//login password
	char gpslat[9];														//gps 정보 8byte?  위도
    char gpslon[10];														// gps 정보	   경도

	int devtype;                                                  ///< Type	
	int ConfirmSelect;													//  인증방법
	int chargingstatus;					// Connector Availablity -> bit 0~7 : connector Id 0~7

    int OperationMode;
    int FreeChargingTime;
    char chkModeMac[13];

    int targetSoc;

    //int UnitPriceType;				// 0 : integer, 1: float
	
	// network
	char chargermac[18];                                                    ///< IP address
    char ipaddr[16];                                                    ///< IP address
    char netmask[16];                                                   ///< Netmask
    char gw[16];                                                        ///< Gateway address
    char dns[16];                                                       ///< DNS address	
	char serverip[MAX_ADDRESS];                                                    ///< Server IP address
	int serverport; 													//serverport

    // ftp server
	char ftpIp[16];                                                    ///< Ftp Server IP address
	char ftpId[10];                                                    ///< Ftp Server User ID 8 charactor
	char ftpPw[16];                                                    ///< Ftp Server Password 16 charactor
    char ftpDns[MAX_ADDRESS];                                                    ///< Ftp Server DNS 40 charactor
    char ftpPort[2];                                                    //< Ftp Port 2 charactor
    char ftpPath[100];                                                  //< Ftp File Path 100 charactor

    // display
    int brightness;                                                     ///< Brightness, the range is 0~9
    int screensaver_time;                                               ///< Time to enter screen saver mode, unit is minute
    int screensaver_type;                                               ///< Screen saver type @see ScreensaverType
    int lang;                                                           ///< Language type @see LangType

    // sound
    char      keysound[PATH_MAX];                                       // /< Key sound file path
    int       keylevel;                                                 // /< Key volume percentage, range is 0~100
    int       audiolevel;                                               // /< Audio volume percentage, range is 0~100

	// packet _only Straffic dsAn 190422
	int setrevh1_flag;													// h1 packet server recv flag ( 0 : no recv , 1 : Set )
} Config;

#define TSCT_DEVVERSION "dev13_230414"

#define TSCT_BUFSIZE   (4096)
#define NotTxBuffSize  (TSCT_BUFSIZE -25-2-8-4-2)

#define DEFAULT_UNITPRICE   24000
typedef struct 
{	
	unsigned int count;	
	unsigned int size;
	unsigned char data[NotTxBuffSize];
}CSTNotTxData;

CSTNotTxData sTrafficNoTxData;

typedef struct
{	
	char authkey[17]; 

	char devid1[3];        
	char siteid[9];        

	int devtype;       
	int ConfirmSelect;	

	char chargermac[18];
    char ipaddr[16];    
    char netmask[16];   
    char gw[16];        
	char serverip[40];  
	int serverport; 	
	char ftpIp[16];  
    char ftpDns[40];         
	char ftpId[10];      
	char ftpPw[16];
	
	// packet _only Straffic dsAn 190422
	int setrevh1_flag;													// h1 packet server recv flag ( 0 : no recv , 1 : Set )	
} ConfigBackup;

typedef enum{
    MEM_EMPTY = 0,
    MEM_MEBER = 1,
    MEM_NON,
    MEM_VIP
} MEM_TYPE;

#define  CST_DOOR_CLOSE				(0)  	// Close 1
#define  CST_OUT_OVER_VOLTAGE		(10)	// Bit[10] 출력 과전압 Err o, Ok 1
#define  CST_OUT_OVER_CURRENT		(11)	// Bit[11] 출력 과전류 Err o, Ok 1
#define  CST_EMG_SW					(14)	// Bit[14] 비상 스위치 On o, Ok 1
#define  CST_IN_UNDER_VOLTAGE		(26)	// Bit[26] 입력 저전압 Err o, Ok 1
#define  CST_COM_WHM				(29)	// Bit[29] communication with power meter Err 0, Ok 1	
#define  CST_MEMORY					(30)	// Bit[30] memory space 	 Bad 0, Ok 1
#define  CST_SELF_TEST 				(32)	// Bit[32] self test result  Bad 0, Ok 1
#define  CST_TIMEOUT_SEQUENCY		(36)	// Bit[36] sequency time out Err 0, Ok 1

// Error Code
#define ERR_SERVER_DISCON           (0)     // Server Disconnect 
#define ERR_AMI_DISCON              (2)     // AMI Disconnect
#define ERR_PLC_DISCON              (3)     // PLC Disconnect 24.11.28 JGLEE
#define ERR_OV_VOLT                 (23)    // Over Voltage
#define ERR_OV_CURT                 (24)    // OVer Current
#define ERR_UD_VOLT                 (25)    // Under Voltage
#define PAY_ERR                     (61)    // PAY Error
#define ERR_TOUCH                   (62)
#define ERR_RFID                    (63)
#define ERR_AMI                     (64)
#define ERR_CHARGE                  (70)


typedef struct 
{	
    unsigned char send_date[7];                     // 전송일시
    unsigned char charger_type;                     // 장비타입
    unsigned char charger_station_id[8];            // 충전소 ID
    unsigned char charger_id[2];                    // 충전기 ID
    unsigned char eqp_mode[2];                      // 장비모드
    unsigned char eqp_status[8];                    // 장비상태
    unsigned char eqp_watt[4];         	        	// 충전기 사용전력량
    unsigned char card_no[21];                      // 회원카드번호
    unsigned char charge_request_type;              // 충전요구량 선택
    unsigned char charge_request_watt[4];           // 충전요구 전력량
    unsigned char charge_request_money[4];          // 충전요구 금액
    unsigned char payment_type;                     // 결제방법
    unsigned char charge_type;                      // 충전유형
    unsigned char battery_status;                   // 배터리 상태
    unsigned char battery_soc[2];                   // SOC 배터리량
    unsigned char battery_volume[4];                // 배터리 전체용량
    unsigned char battery_watt[4];                  // 현재 배터리 잔량
    unsigned char battery_voltage[4];               // 현재 배터리 전압
    unsigned char bms_sw_ver[4];                    // BMS S/W 버전
    unsigned char charge_comp_expect[2];            // 충전종료 예상 일시
    unsigned char supply_current[4];                // 공급 전류량
    unsigned char charge_watt[4];                   // 충전전력량 (완료 or 실시간)
	unsigned char charge_provide_time[3];           // 충전공급 시간
    unsigned char charge_money[4];                  // 충전금액 (완료 or 실시간)
    unsigned char charge_comp_status;               // 충전완료 상태
	unsigned char charge_price[4];                  // 충전단가 (완료 or 실시간)
	unsigned char charge_price_credit[4];           // 비회원 충전단가 (완료 or 실시간)
    unsigned char battery_current[4];               // 현재 배터리 전류
    unsigned char battery_temperature[4];           // 현재 배터리 온도
    unsigned char charge_remain_time[2];            // 충전 남은시간
    unsigned char app_order;                        // app order
    unsigned char van_server_ip[4];                 // van ip (ex "222.222.222.222")
	unsigned char van_server_port[4];					// van port
	unsigned char admin_tel[16];                    // 관리자 전화번호
    unsigned char charger_gps[25];                  // 충전기 GPS	

    unsigned char auth_type[1];					    // auth type   
	unsigned char sOri_price[4];			   		// 할인되기 전의 오리지날 금액
    unsigned char charge_start_time[7];               // Charge Starting Time
    MEM_TYPE member_type;			   		        // using meber type
    
}SHM_DATA_APP_INFO;

typedef struct 
{
	// Current price
    unsigned char ver_info[8];                      // 버전정보 ASC
    unsigned char cp_type;                          // 충전기 Type
    unsigned char apply_date[3][7];                 // 적용일자 BCD

	unsigned char unit_cost[6][24][4];              // 충전요금 단가
    unsigned char connect_status;                   // 서버 연결상태
    unsigned char card_auth;                        // ID카드 인증결과
	unsigned char card_no[21];                      // M2M에서 받은 회원카드번호

	unsigned char DB_specific_info_code[2];         //DB고유정보 코드
    unsigned short sno;                             // 전송번호

	bool is_date_finish;							// 일 마감 처리 플래그

    double OCPP_iUnitprice;

}SHM_DATA_IF_INFO;


/////// sTraffic operation mode  /// SHM_DATA_APP_INFO.Mode1
#define CP_MODE_OPERATION	(1<<0)
#define CP_MODE_STOP_OPER	(1<<1)
#define CP_MODE_WAITCHARG	(1<<2)
#define CP_MODE_CHARGING	(1<<3)
#define CP_MODE_CHECKING	(1<<4)
#define CP_MODE_TESTING		(1<<5)
#define CP_MODE_POWEROFF	(1<<6)
#define CP_MODE_AUTOAUTH	(1<<7)

typedef struct 
{
    unsigned char nCnt;                // 카드 넘버 count
	char sCardNum[10][16];			   // 저장된 카드 번호
}CST_USER_INFO;

CST_USER_INFO localUserInfo;

typedef struct 
{
	unsigned int 	date;  			// 1byte day, 1byte month, 2Byte year
	unsigned short 	count;			// 일마감 총 충전건수  2 byte
	float			watt;           // 일마감 총 충전전력량 4 byte
    unsigned int 	money;          // 일마감 총 충전금액    4 byte
	unsigned short 	dc_count;		// 일마감 총 할인 건수   2 byte
	unsigned int 	dc_money;		// 일마감 총 할인 충전 금액  4 byte
}CST_DATA_DAY_ALL;

CST_DATA_DAY_ALL gDailyData;

typedef struct 
{	
	unsigned char chargePrice[4]; 			// 결제진행 충전금
	unsigned char chargeWatt[4];  			// 결제진행 전력량
	unsigned char payDate[7];	  			// 거래일시
	unsigned char pay_approvalCode[11]; 	// 거래승인번호 
	unsigned char pay_PayNumber[12];		// 거래 고유번호
	unsigned char pay_ResCode;		// 단말기 응답코드
	unsigned char pay_ResMsg[32];		// 응답메시지 
	unsigned char pay_CardRes[2];		// 카드사 응답코드
	
}CREDIT_PAY_STRUCT;

/**
 * Credite card patment type.
 */
typedef enum
{
    PRE_PAYMENT = 0x01,   ///<// ?????? 1
	REAL_PAYMENT,    //// ???????    2
    PRE_PAYMENT_CANCEL,    ///< ?????? ???   3
    FAIL_PRE_PAYMENT_CANCEL,   ///< connect timeout pre-pay cancel 4
    LOW_PAYMENT_CANCEL    ///<  100?????? ???? pre-pay cancel - charge layer ???? ??.  5    
} PAYMENT_TYPE;


/**
 * Select Charge Type by user.
 */
typedef enum
{
    PRICE_TYPE = 0,   ///<// price 0
	ENERGY_TYPE,    //// Electric Energy  1
    FULL_CHARGE    ///< Full Charge   2    
} CHARGE_WAY;

/**
 * Select Charge Type by user.
 */
typedef enum
{
    TOUCH_CARD = 1,   // 1: RFID reading 
	INPUT_CARDNUM,    // 2 : input card num
    INPUT_CARNUM,     // 3 : input car num 
    PW_INPUT_CARNUM,  // 4 : input carnum pass
    CHECK_CARNUM,	  //5 : ? 
    SERVER_AUTH,       // 6 : Server Auth
    CONNECT_AUTH,       // 7 : Connection First
} CpAuthType;

//unsigned char card_auth;						

/**
 * card auth state definition.
 */
typedef enum
{
    CARD_AUTH_WAIT = 0,   	// 0 : ?????? 
	CARD_AUTH_OK,    		// 1 : ??????? ????
    CARD_AUTH_OK_AFTER,     // 2 : ??????? ???? ????, ?????? ??
    CARD_AUTH_FAILD,  		// 3 : ??????? ????.
    CARD_AUTH_MAX  			// 4 : 
} CardAuthStates;


/**
 * User Auth type definition.
 */
typedef enum
{
    USER_AUTH_NONE = 0, // 0: ????.
    USER_AUTH_NET,   	// 1: server
    USER_AUTH_CARD,		// 2: card
    USER_AUTH_PASSWORD,	// 3: button
	USER_AUTH_MAX		// 4: 
} UserAuthType;

typedef struct
{
    // network
    int dhcp;                                                           ///< Enable DHCP or not
    char ipaddr[16];                                                    ///< IP address
    char netmask[16];                                                   ///< Netmask
    char gw[16];                                                        ///< Gateway address
    char dns[16];                                                       ///< DNS address

    // display
    int brightness;                                                     ///< Brightness, the range is 0~9
    int screensaver_time;                                               ///< Time to enter screen saver mode, unit is minute
    int screensaver_type;                                               ///< Screen saver type @see ScreensaverType
    int lang;                                                           ///< Language type @see LangType
    int mainmenu_type;                                                  ///< Main menu type @see MainMenuType

    // sound
    char      keysound[PATH_MAX];                                       // /< Key sound file path
    int       keylevel;                                                 // /< Key volume percentage, range is 0~100
    int       audiolevel;                                               // /< Audio volume percentage, range is 0~100

    // photo
    int       photo_interval;                                           // /< Photo change interval (second)

    // setting
    int touch_calibration;                                              ///< Need to do the touch calibration or not

    // wifi
    char ssid[64];
    char password[256];
    char secumode[3];
    int wifi_mode;
    int wifi_on_off;
    // wifi
    char ap_ssid[64];  // ap mode ssid
    char ap_password[256]; // ap mode password
    

	// login
    char user_id[64];
    char user_password[64];

	int order[2][6];
} Config_old;

/**
 * Global instance variable of configuration.
 */
extern Config theConfig;
struct tm tInfo;

extern CHARGE_WAY sel_type; // 0 : charge type 'price', 1 : charge type 'Electric Energy',  2 : charge type 'Full Charge'
CREDIT_PAY_STRUCT credit_Pay[2];
SHM_DATA_IF_INFO shmDataIfInfo;

SHM_DATA_APP_INFO shmDataAppInfo;
int chager_Temperature; 
int chager_Humidity;
bool EmgControl; // 모든 동작을 정지 시키고 대기 화면으로 이동 시킨다. 충전중에는 뒤로 가서 대기 화면으로 간다.
bool sleepOnCheck;
bool sleepOn1chCheck;
bool sChCharging;// = false;
bool TwoChOnlyEmg;
uint32_t gAmiStartWatt;
uint32_t gAmiChargeWatt;
bool gChargeFinal[2];
bool screenOff;  // 절전모드. 화면이 backlight off 되었을 시 터치 y/n

uint32_t QRImage;
uint32_t QRImage_size;

#if (!defined(sTraffic_Server) && !defined(KoChargerSvc_Server))
#endif
#define CST_NO_RTC 
#ifndef EN_CERTI  // ?????? ???α??. ?????? ?? ????? disable.
#define AUDIO_SUPPORT 
#endif
//#define SAMWONFA_CD_TERMINAL

//#define HBC_TYPE_CHARGER  //  no DoorLock, No emmergency 2ch 
//#define HC_TYPE_CHARGER  // 1ch no DoorLock 
//#define B_TYPE_CHARGER  // 1ch no DoorLock 
//#define C_TYPE_CHARGER  // 1ch no DoorLock 
#define BC_TYPE_CHARGER  // 1ch no DoorLock 
//#define BB_TYPE_CHARGER  // 2ch  all function 
// #define BC2_TYPE_CHARGER  // 2ch  all function 
bool mp3Testflag;
int mp3TestSelect;

char admintimeStamp[7];

void itpRtcInit1(void);
long _extRtcGetTime1(void);
void itpRtcSetTime1(long sec, long usec);
bool timeSelectControl_;
/**
 * Loads configuration file.
 */
void ConfigInit(void);
void ConfigRecoverFromBackup(void);
void init_Data(void);

/**
 * Exits configuration.
 */
void ConfigExit(void);

/**
 * Updates CRC files.
 *
 * @param filepath The file path to update the CRC value. NULL for ini file on public drive.
 */
void ConfigUpdateCrc(char* filepath);

/**
 * Saves the public part of configuration to file.
 */
void ConfigSave(void);

/** @} */ // end of ctrlboard_config

/** @defgroup ctrlboard_external External
 *  @{
 */
#define EXTERNAL_BUFFER_SIZE 64 ///< External buffer size

typedef enum
{
    EXTERNAL_SHOW_MSG,  ///< Show message
    EXTERNAL_TEST0,     ///< Test #0
    EXTERNAL_TEST1,     ///< Test #1
    EXTERNAL_TEST2,     ///< Test #2
    EXTERNAL_TEST3,     ///< Test #3
    EXTERNAL_TEST4,     ///< Test #4
    EXTERNAL_TEST5      ///< Test #5
} ExternalEventType;

typedef struct
{
    ExternalEventType type;
    int arg1;
    int arg2;
    int arg3;
    uint8_t buf1[EXTERNAL_BUFFER_SIZE];

} ExternalEvent;

/**
 * Initializes external module.
 */
void ExternalInit(void);

/**
 * Exits external module.
 */
void ExternalExit(void);

/**
 * Receives external module event.
 *
 * @param ev The external event.
 * @return 0 for no event yet, otherwise for event occured.
 */
int ExternalReceive(ExternalEvent* ev);

/**
 * Sends external module event.
 *
 * @param ev The external event.
 * @return 0 for success, otherwise for failure.
 */
int ExternalSend(ExternalEvent* ev);

/**
 * Initializes external process module.
 */
void ExternalProcessInit(void);

/**
 * Processes external module event.
 *
 * @param ev The external event.
 * @return 0 for no event yet, otherwise for event occured.
 */
void ExternalProcessEvent(ExternalEvent* ev);

/** @} */ // end of ctrlboard_external

/** @defgroup ctrlboard upgarde_uart
 *  @{
 */
#if defined(CFG_UPGRADE_FROM_UART_RUN_TIME)
#if defined(CFG_UPGRADE_UART0)
#define UPGRADE_UART_PORT	ITP_DEVICE_UART0
#elif defined(CFG_UPGRADE_UART1)
#define UPGRADE_UART_PORT	ITP_DEVICE_UART1
#else
#define UPGRADE_UART_PORT	ITP_DEVICE_UART0
#endif
#define UPGRADE_PATTERN				0x1A

#define ACK20						0x14
#define ACK50						0x32
#define ACK100						0x64
#define ACK150						0x96
#define ACK200						0xC8
#define ACK210						0xD2
#define ACK211						0xD3
#define ACK220						0xDC
#define ACK221						0xDD

//the total check times is CHECK_NUM or CHECK_NUM+1
#define CHECK_NUM			4		
#define RETRY_SIZE			5
#define RETRY_CHECKSUM		1
#define RETRY_DATA			1

/**
 * Initializes Upgrade Fw by Uart module.
 */
void UpgradeUartInit(void);

/**
 * Exits Upgrade Fw by Uart module.
 */
void UpgradeUartExit(void);
#endif
/** @} */ // end of ctrlboard upgarde_uart

/** @defgroup ctrlboard_network Network
 *  @{
 */
/**
 * Initializes network module.
 */
void NetworkInit(void);
void NetworkWifiModeSwitch(void);

/**
 * Network pre-setting.
 */
void NetworkPreSetting(void);
void NetworkWifiPreSetting(void);

/**
 * Network function process.
 */
void NetworkEthernetProcess(void);
void NetworkWifiProcess(void);

/**
 * Exits network module.
 */
void NetworkExit(void);
bool NetworkIsExit(void);

/**
 * Resets network module.
 */
void NetworkReset(void);

/**
 * Determines whether the network(Ethernet) is ready or not.
 *
 * @return true for ready; false for net ready yet.
 */
bool NetworkIsReady(void);

/**
 * Determines whether the network(Ethernet) and Center Server are both ready or not.
 *
 * @return true for ready; false for net ready yet.
 */
bool NetworkServerIsReady(void);

/**
 * Determines whether the network(WIFI) is ready or not.
 *
 * @return true for ready; false for net ready yet.
 */
bool NetworkWifiIsReady(void);

/** @} */ // end of ctrlboard_network

/** @defgroup ctrlboard_photo Photo Loader
 *  @{
 */
bool GetNetworkReset(void);
void NetworkExit(void);

typedef void (*PhotoLoadCallback)(uint8_t* data, int size);

void PhotoInit(void);

void PhotoExit(void);

int PhotoLoad(char* filename, PhotoLoadCallback func);

/** @} */ // end of ctrlboard_photo

/** @defgroup ctrlboard_screen Screen
 *  @{
 */
/**
 * Screensaver type definition.
 */
typedef enum
{
    SCREENSAVER_NONE,   ///< No screensaver
    SCREENSAVER_CLOCK,  ///< Clock sreensaver
    SCREENSAVER_BLANK,  ///< Black screen screensaver
    SCREENSAVER_PHOTO   ///< Photo screensaver
} ScreensaverType;

/**
 * Initializes screen module.
 */
void ScreenInit(void);

/**
 * Turns off screen.
 */
void ScreenOff(void);

/**
 * Turns on screen.
 */
void ScreenOn(void);

/**
 * Is the screen off or on.
 *
 * @return true for off; false for on.
 */
bool ScreenIsOff(void);

/**
 * Sets the brightness.
 *
 * @param value The brightness value.
 */
void ScreenSetBrightness(int value);

/**
 * Gets the maximum brightness level.
 *
 * @return the maximum brightness level.
 */
int ScreenGetMaxBrightness(void);

/**
 * Re-counts the time to enter screensaver.
 */
void ScreenSaverRefresh(void);

/**
 * Checks whether it is about time to enter screensaver mode.
 *
 * @return 0 for not yet, otherwise for entering screensaver mode currently.
 */
int ScreenSaverCheck(void);

/**
 * Is on screensaver mode or not.
 */
bool ScreenSaverIsScreenSaving(void);

/**
 * Takes a screenshot to USB drive.
 *
 * @param lcdSurf The LCD surface widget.
 */
void Screenshot(void* lcdSurf);

/**
 * Clears screen.
 *
 */
void ScreenClear(void);

/** @} */ // end of ctrlboard_screen

/** @defgroup ctrlboard_storage Storage
 *  @{
 */

typedef enum
{
    STORAGE_NONE = -1,
    STORAGE_USB,
    STORAGE_SD,
    STORAGE_INTERNAL,

    STORAGE_MAX_COUNT
} StorageType;

typedef enum
{
    STORAGE_UNKNOWN,
    STORAGE_SD_INSERTED,
    STORAGE_SD_REMOVED,
    STORAGE_USB_INSERTED,
    STORAGE_USB_REMOVED,
    STORAGE_USB_DEVICE_INSERTED,
    STORAGE_USB_DEVICE_REMOVED
} StorageAction;

/**
 * Initializes storage module.
 */
void StorageInit(void);

StorageAction StorageCheck(void);
StorageType StorageGetCurrType(void);
void StorageSetCurrType(StorageType type);
char* StorageGetDrivePath(StorageType type);
bool StorageIsInUsbDeviceMode(void);

/** @} */ // end of ctrlboard_storage

/** @defgroup ctrlboard_string String
 *  @{
 */
/**
 * Guard sensors definition.
 */
typedef enum
{
    GUARD_EMERGENCY,    ///< Emergency sensor
    GUARD_INFRARED,     ///< Infrared sensor
    GUARD_DOOR,         ///< Door sensor
    GUARD_WINDOW,       ///< Window sensor
    GUARD_SMOKE,        ///< Smoke sensor
    GUARD_GAS,          ///< Gas sensor
    GUARD_AREA,         ///< Area sensor
    GUARD_ROB,          ///< Rob sensor

    GUARD_SENSOR_COUNT  ///< Total sensor count
} GuardSensor;

/**
 * Gets the description of guard sensor.
 *
 * @param sensor The guard sensor.
 * @return the string of guard sensor.
 */
const char* StringGetGuardSensor(GuardSensor sensor);

/**
 * Gets the description of WiFi connected.
 *
 * @return the string of WiFi connected.
 */
const char* StringGetWiFiConnected(void);

/** @} */ // end of ctrlboard_string

/** @defgroup ctrlboard_upgrade Upgrade
 *  @{
 */
/**
 * Quit value definition.
 */
typedef enum
{
    QUIT_NONE,                  ///< Do not quit
    QUIT_DEFAULT,               ///< Quit for nothing
    QUIT_RESET_FACTORY,         ///< Quit to reset to factory setting
    QUIT_UPGRADE_FIRMWARE,      ///< Quit to upgrade firmware
    QUIT_UPGRADE_WEB,           ///< Quit to wait web upgrading
    QUIT_RESET_NETWORK,         ///< Quit to reset network
    QUIT_UPGRADE_UART			///< Quit to upgrade firmware by Uart
} QuitValue;

/**
 * Initializes upgrade module.
 *
 * @return 0 for initializing success, non-zero for initializing failed and the value will be the QuitValue.
 */
int UpgradeInit(void);

/**
 * Sets the CRC value of the specified file path.
 *
 * @param filepath The file path to update the CRC value.
 */
void UpgradeSetFileCrc(char* filepath);

/**
 * Sets the URL to upgrade.
 *
 * @param url The url to download and upgrade.
 */
void UpgradeSetUrl(char* url, char* id, char* pw);

/**
 * Sets the stream to upgrade.
 *
 * @param stream The stream to upgrade.
 */
void UpgradeSetStream(void* stream);

/**
 * Processes the upgrade procedure by QuitValue.
 *
 * @param code The QuitValue.
 * @return 0 for process success; otherwise failed.
 */
int UpgradeProcess(int code);

/**
 * Is upgrading ready or not.
 *
 * @return true for ready; otherwise not ready yet.
 */
bool UpgradeIsReady(void);

/**
 * Is upgrading finished or not.
 *
 * @return true for finished; otherwise not finish yet.
 */
bool UpgradeIsFinished(void);

/**
 * Gets the upgrading result.
 *
 * @return 0 for success, failed otherwise.
 */
int UpgradeGetResult(void);

/** @} */ // end of ctrlboard_upgrade

/** @defgroup ctrlboard_webserver Web Server
 *  @{
 */
/**
 * Initializes web server module.
 */
void WebServerInit(void);

/**
 * Exits web server module.
 */
void WebServerExit(void);

/**
 * @brief Upgrade with FTP Server
 * 
 * @return int 
 */
int UpgradePackage(void);

/** @} */ // end of ctrlboard_webserver

// Audio File Define...
#define AUDIO_NONE			0
#define AUDIO_WELCOM		1
#define AUDIO_OPENING		2
#define AUDIO_RFIDCARD		3
#define AUDIO_INPUTNUM		4
#define AUDIO_STARTCHARGE	5
#define AUDIO_TRYCHARGE		6
#define AUDIO_TRYCONNECT	7
#define AUDIO_ENDGHARGE		8
#define AUDIO_HANGCONNECT	9
#define AUDIO_THANKYOU		10
#define AUDIO_TESTAUDIO		11
#define AUDIO_INPUTCARNUM	12
#define AUDIO_SELECTAUTHTYPE   13
#define AUDIO_INPUTPASSWORD    14
#define AUDIO_SELECTCHARGETYPE 15
#define AUDIO_INPUTCHARGEPRICE 16
#define AUDIO_INPUTCHARGEWATT  17
#define AUDIO_SELECTCARNUM	   18

// Audio file define end....

typedef enum {
   APP_ORDER_NONE = 0,         // 
   APP_ORDER_WAIT,             // 1 >>>>>>>>>>>>>>>>>
   APP_ORDER_AUTH_METHOD,      // 2>>>>>>>>>>>>>>>>>>>>>>>     
   APP_ORDER_KAKAO_QR,
   APP_ORDER_CARD_READER,      // 3
   APP_ORDER_REMOTE_CHECK,
   APP_ORDER_CUSTOMER_AUTH,    // 4
   APP_ORDER_CONNECTOR_SELECT, // 5 
   APP_ORDER_CONNECT_IN,       // 6
   APP_ORDER_PAYMENT_METHOD,   // 7
   APP_ORDER_CHARGE_INPUT,     // 8
   APP_ORDER_CHARGE_READY,     //9>>>>>>>>>>>>>>>>>>>>>>>
   APP_ORDER_CHARGING,         //10>>>>>>>>>>>
   APP_ORDER_CHARGING_STOP,   //11  >>>>>>>>>
   APP_ORDER_CHARGE_END,       //12  >>>>>>>
   APP_ORDER_CUSTOMER_CONFIRM, //13	
   APP_ORDER_PAYMENT,          //14
   APP_ORDER_CONNECT_OUT,      //15  >>>>>>>>>>>>>>>>.
   APP_ORDER_FINISH,            //16
   APP_ORDER_CAR_NUMAUTH,		//17 straffic add 190321 _daAn
   APP_ORDER_CANCEL_PREPAY,		// 18 straffic add 190529 _daAn 
   APP_ORDER_ERR_REBOOT         //19 Error Occure go Reset
}APP_ORDER;
void PlayVideo(int x, int y, int width, int height, int bgColor, int volume);
void WaitPlayVideoFinish(void);

void PlayMjpeg(int x, int y, int width, int height, int bgColor, int volume);
void WaitPlayMjpegFinish(void);
void TimeOutPageOut1(void);
void I1_Reset_Set(void) ;
void I1_Reset_Clear(void);
void UnitcostFileInit(void);
int CstFileRead(int nNo, char *rBuf, char *sFileName);
int CstFileSave(char nNo, char bAdd, char *wBuf, int length, char *sFileName);
bool CstCheckDayDataDate(unsigned int curDate);
int CstUpgradeProcess(void);
int CstDwFileSave(char *wBuf, int length);
void ShowWhmErrorDialogBox(unsigned short Ecode);

extern void CstFwUpdateMsg(void);

extern void GroundFaultListenerOnCharge(void);

bool amicomunication1;
bool amicomunication2;
float gChargeWatt[2] ;
int setipselect;
int adminsetchargerselect1;
int adminsetchargerselect2;
int adminsettestselect;

unsigned long long int start_time_i;
unsigned long long int end_time_i;

bool bTestModeOn;
bool bDevChannel;  // false is C Type(left, CH1), true is B type(Right, CH2)
bool SAMWONRev4G;
bool SAMWONRev4L;
bool flag_4G4L;
bool Insert_Timeout_t1; // If the credit card insertion time is exceeded, it is transmitted.
bool Insert_Timeout_u1;
time_t stime;  // ktlee

typedef enum  {
	bBlcoking_Rec_OK = 0x00,
	bBlocking_a1 = 0x01,
	bBlocking_b1,
	bBlocking_c1,
	bBlocking_d1,
	bBlocking_e1,
	bBlocking_f1,
	bBlocking_g1
}bBlocking_Code;
bBlocking_Code bBlocking;

enum CHARGER_OPERATION_MODE_CODE {
	OP_NORMAL_MODE = 0x00,
	OP_CHECK_MODE = 0x01,
	OP_FREE_MODE
};

// 0 is idel, 1 wait for ftp connection, 2 start to downlod by ftp. 3 upgrade to nor flash  4 verfying 5 waiting reset.
// Error Mask 0x80, highest bit is error bit.. 
unsigned char bFTPupgrade;  

#define FTP_FW_UD_IDLE		(0)
#define FTP_FW_UD_WAIT_CON	(1)
#define FTP_FW_UD_DOWNLOAD	(2)
#define FTP_FW_UD_WRITE		(3)
#define FTP_FW_UD_VERIFY	(4)
#define FTP_FW_UD_WAIT_RST	(5)
#define FTP_FW_UD_ERR_MASK	(0x80)


#define INI_FILENAME "ctrlboard.ini"
#define UNITCOST_FILENAME "unitcost.dat"
#define NOTSENT_FILENAME "notsent.dat"
#define USERINFO_FILENAME "userinfo.dat"
#define DAYDATA_FILENAME "daydata.dat"
#define CHARGEND_FILENAME "chargend.dat"

#define BACKUP_FILENAME "backup.ini"

#define TSCT_FIRMWARE_NAME  "ITEPKG03.PKG"
#define CST_FIRMWARE_NAME   "COSTELSW.PKG"
#define STRAFFIC_FOLDER  	"straffic"
#define HAPPYCHARGER_FOLDER "happy_char"
#define AllOpen_FOLDER  	"AllOpen"
#define HlfOpen_FOLDER  	"HalfOpen"
typedef struct
{
    uint16_t type;  /**< Event type, shared with all events */
    uint8_t *Buff;  /**< App event data */
	uint8_t Length;	/**< App event data length*/
} CST_FileEvent;

typedef enum
{
    CST_FILE_INIT     = 0,     /**< Unused (do not remove) */
	CST_FILE_INI_SAVE	= 0x100,
	CST_FILE_INI_BAKCUP	= 0x101,		
	CST_FILE_SYS_REBOOT = 0x102,    
	CST_FILE_DAYLY_SAVE	= 0x200,
	CST_FILE_F1_SAVE	= 0x300,
	CST_FILE_USER_DATA_SAVE	= 0x400,
	CST_FILE_UNIT_COST_SAVE = 0x500,
	CST_FILE_NO_SENT_SAVE 	= 0x600,    	
    CST_FILE_EVT_NONE    = 0xFFFF
} CST_FileEventType;

#define CST_FileEvtMax	32
static struct
{
    int head;
    int tail;
    CST_FileEvent event[CST_FileEvtMax];   
} CstFileEvtQ;
#include "tsctcommon.h"
#include "cstcontrolpilot.h"
#include "cstwatthourmeter.h"
#include "cstmagneticcontactor.h"
#include "cstemergencybutton.h"
#include "cstled.h"
#include "cstbuzzer.h"
#include "cstac220.h"
#include "cstbacklight.h"
#include "cstlog.h"
// #include "tsctclient.h"
#include "tsctwsclient.h"
#include "cstordersequence.h"
#include "cstping.h"
#include "layer_common.h"
#include "tsctsecc.h"

#ifdef SAMWONFA_CD_TERMINAL
#include "cstsamwonfa.h"

//int card_waittxt; // 0 : default, 1 : pre-pay, 2 : real pay, 3 : pre-pay cancel
typedef enum {
   PAYMENT_NONE = 0,         // 
   PAYMENT_PRE,             // 1 >>>>>>>>>>>>>>>>>
   PAYMENT_REAL,      		// 2>>>>>>>>>>>>>>>>>>>>>>>     
   PAYMENT_PRE_CANCEL,      // 3
   PAYMENT_PRE_CANCEL_FAIL,	// 4
   PAYMENT_LOW_COST_CANCEL, // 5
   PAYMENT_MAX,    			// 6
}PAYMENT_STATES;
#else
#include "cstcardreader.h"
#endif
#include "tsctobd.h"

//// Layer common function.
void AdminSetupMenuExit(bool bSaveCfg);
extern bool ChangeSetupSubMenu(int nSelect);
extern void CstPlayAudioMsg(int nAudioMsg);
extern void CheckCurrentIsZero(int ch, int *tCurZeroTime, int tChargedTime, float nCurrent);

void CstGotoAuthOrNextLayer(void);
void StopCharge2ch(int ch);
void StopCharge(void);
void UpdateStopGui();
int CstGetActiveCh(void);
bool CstGetMcstatus(void);


void CST_EVTFileSend(uint16_t nEvent, uint8_t *pBuf, uint8_t len);
void CstNetFileFunc(void);

uint16_t TSCTGetAMIVolt(void);
uint16_t TSCTGetAMICurrent(void);

int checkUpdate(void);
void clearUpdate(void);

bool GetServerCon(void);
bool GetMeterCon(void);

#ifdef	CFG_DYNAMIC_LOAD_TP_MODULE
void DynamicLoadTpModule(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CTRLBOARD_H */
/** @} */ // end of ctrlboard