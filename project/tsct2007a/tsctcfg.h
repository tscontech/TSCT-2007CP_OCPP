/** @file
 * Costel Product infomations.
 *
 * @author JC KIM
 * @version 1.0
 * @date 2021
 * @copyright TSCT,Ltd. All Rights Reserved.
 */
/** @defgroup costel product infomations
 *  @{
		Version History : 1.0.1 : Network init & Router connect timing modify
 
 */
 
#ifndef COSTELCFG_H
#define COSTELCFG_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "ctrlboard.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ctrlboard_audio Audio Player
 *  @{
 */

/* define devtype...*/
#define 	B_TYPE		(8001)  // 1 ch
#define 	C_TYPE		(8002)  //  1 ch EV Charger [no doorlock]
#define 	BC_TYPE		(8003)  // use 1 ch but 2 charger 
#define 	BB_TYPE		(8004)  // 2 ch	
#define 	HBC_TYPE	(8005)  // Home BC_Type Charger  [no doorlock]
#define 	HC_TYPE		(8006)  //  1 ch Home Charger [no doorlock]
#define 	BC2_TYPE	(8007)  //  2 ch Stand Charger BC Type

// #define	sHC_MODEL 		"TSCT-1007A"  // RFID Home C Type 
// #define	sHBC_MODEL 		"TSCT-2007B"  // RFID Home BC Type 

// #define	sB_MODEL 		"TSCT-1007E"  // RFID Stand B Type 
// #define	sC_MODEL 		"TSCT-1007B"  // RFID Stand C Type 
// #define	sBC_MODEL 		"TSCT-2007CO"  // RFID Stand BC Type 
// #define	sBB_MODEL 		"TSCT-2007F"  // RFID Stand BB Type 
// #define	s2BC_MODEL 		"TSCT-2007G"  // RFID Stand 2ch BB Type 

#define SECC_MODEL	"TSCT-2007CP"

char StrModelName[20];

//[application]	
#define DEFAULT_AUTHKEY	"1234567890123456"
//#define	SW_VERSION 		"V1.0.1"
//#define	SW_VERSION 		"V1.0.3"  // 2019.11.29 iksung is-200 _dsAn
//#define	SW_VERSION 		"V1.0.5"  // 2020.01.29 tesla issue and init config issue by ktlee
								  // 2020.02.26 RFID error dialog
#define	SW_VERSION 		"V2.2.2" // watthourmeter noise driver & checksum 2020.10.12 srkim
#define	CERTI_SW_VERSION 		"01.00.00" // 20200928 ktlee for certification

#if defined(OBD_MODEL)
	#define SW_MODEL			OBD_MODEL
	#define DEFAULT_DEVTYPE 	BC_TYPE	

#elif defined(SECC_MODEL)
	#define SW_MODEL			SECC_MODEL
	#define DEFAULT_DEVTYPE 	BC_TYPE	
#endif



//[basicconfig]		
#define DEFAULT_DEVCHANNEL		(1)
#define DEFAULT_DEVID1 			"00"
#define DEFAULT_DEVID2 			"05"
#define DEFAULT_SITEID			"00000000"
#define DEFAULT_ADMINPW			"7188"
#define DEFAULT_GPSLAT			"37.64070"
#define DEFAULT_GPSLON			"122.57323"

// [tcpip] - network
#define DEFAULT_NETMAC			"F0:8B:FE:0F:00:33"
#define DEFAULT_NETIP			"192.168.001.240"
#define DEFAULT_NETMASK			"255.255.255.000"
#define DEFAULT_NETGW			"192.168.001.001"
#define DEFAULT_NETDNS			"192.168.001.001"
#define DEFAULT_NETSVRIP		"com-me.tscontech.com"//"192.168.001.222"
#define DEFAULT_NETSVRPORT		(5000)

#define DEFAULT_NETFTPIP		"192.168.001.222"
#define DEFAULT_NETFTPID		"tsct"
#define DEFAULT_NETFTPPW		"tsct01340p@"

#define DEFAULT_DSIPBL		(8)
#define DEFAULT_DISPSSTIME	(1)

#define DEFAULT_SNDKEY	"key1.wav"
#define DEFAULT_SNDLEVEL	(15)
#define DEFAULT_SNDAULEVEL	(15)

#define DEFAULT_TOUCHCAL	(0)
#define DEFAULT_SETREVH1	(0)


#ifdef __cplusplus
}
#endif

#endif /* COSTELCFG_H */
/** @} */ // end of costelcfg
