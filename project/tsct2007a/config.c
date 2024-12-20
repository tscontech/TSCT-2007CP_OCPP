#include <sys/ioctl.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "iniparser/iniparser.h"
#include "ite/itp.h"
#include "ctrlboard.h"
#include "scene.h"
#include "tsctcfg.h" // 2019.01.03 Added by Jake.Lee 

#define INI_FILENAME "ctrlboard.ini"

Config theConfig;			// Config Data
ConfigBackup SaveConfig;	// Backup Config Data
static dictionary* cfgIni;
static bool cfgIsSaving;
static int cfgSavingCount;
static pthread_mutex_t cfgMutex  = PTHREAD_MUTEX_INITIALIZER;
bool I1_reset_boot = false;

static char VerifyConfigSave(void);
static char DumpConfigSave(void);	
static char CheckConfigSave(void);
static void* ConfigSaveTask(void* arg);
void ConfigInitSave();
char FilePrefix[6] = {'c','o','s','t','e','l'};
char UnitCost[8] = {'u','n','i','t','c','o','s','t'};  // 0
char UserInfo[8] = {'u','s','e','r','i','n','f','o'};  // 1
char NotSent[7] = {'n','o','t','s','e','n','t'};       // 2
char DayData[7] = {'d','a','y','d','a','t','a'};       // 3
char ChargEnd[8] = {'c','h','a','r','e','e','n','d'};  // 4

/**
 * @brief Save Config Value to Backup Directory
 * 
 * @param bFlashFlush 
 */
static void ConfigSaveBackup(bool bFlashFlush)
{
	FILE* f;
	int ret = 0;
	memcpy(SaveConfig.authkey, theConfig.authkey, 17);	
	memcpy(SaveConfig.devid1, theConfig.devid1, 3);
	memcpy(SaveConfig.siteid, theConfig.siteid, 9);
	SaveConfig.devtype = theConfig.devtype;
	SaveConfig.ConfirmSelect = theConfig.ConfirmSelect;
	memcpy(SaveConfig.chargermac, theConfig.chargermac, 18);	
	memcpy(SaveConfig.ipaddr, theConfig.ipaddr, 16);
	memcpy(SaveConfig.netmask, theConfig.netmask, 16);
	memcpy(SaveConfig.gw, theConfig.gw, 16);
	memcpy(SaveConfig.serverip, theConfig.serverip, 40);
	SaveConfig.serverport = theConfig.serverport;
	memcpy(SaveConfig.ftpIp, theConfig.ftpIp, 16);	
	memcpy(SaveConfig.ftpDns, theConfig.ftpDns, 40);	
	memcpy(SaveConfig.ftpId, theConfig.ftpId, 10);
	memcpy(SaveConfig.ftpPw, theConfig.ftpPw, 16);
	SaveConfig.setrevh1_flag = theConfig.setrevh1_flag;
	f = fopen("c:/"BACKUP_FILENAME, "wb+");
	if (!f)
	{
		printf("cannot open ini file: %s\n", "c:/"BACKUP_FILENAME);		
		system("rm -rf c://backup.ini"); // force to remove the crashed or opened file. 2020.01.13  ktlee.
		CST_EVTFileSend(CST_FILE_INI_BAKCUP, NULL, 0);  // retry to save teh backup file.
		return;
	}
	ret = fwrite((void *)&SaveConfig.authkey[0], 1, sizeof(SaveConfig), f);
	fflush(f);	
	fclose(f);
	if(bFlashFlush)
	{
#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
		ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
	}
	printf(" ret[%d] ConfigSaveBackup :: Type %d, Site Id : %s, Dev Id : %s \n", ret, SaveConfig.devtype, SaveConfig.siteid, SaveConfig.devid1);
}

/**
 * @brief Dump Save Backup Config
 * 
 */
static void ConfigSaveBackupDump(void)
{
	FILE* f;
	int ret = 0;	
	ConfigBackup SavedConfig;
	f = fopen("c:/"BACKUP_FILENAME, "rb");
	if (!f)
	{
		printf("ConfigSaveBackupDump :: cannot open ini file: %s\n", "c:/"BACKUP_FILENAME);	
		CST_EVTFileSend(CST_FILE_INI_BAKCUP, NULL, 0);
		return;
	}
	ret = fread((void *)&SavedConfig.authkey[0], 1, sizeof(SavedConfig), f);	
	fclose(f);
	printf(" ret[%d] ConfigSaveBackupDump ::  Type %d, h1 :%d \n", ret, SavedConfig.devtype, SavedConfig.setrevh1_flag);
	printf(" Site Id : %s, Dev Id : %s \n", SavedConfig.siteid, SavedConfig.devid1);
	printf(" Net :: ip %s, MAC:%s, NetMask:%s , gw:%s \n", SavedConfig.ipaddr, SavedConfig.chargermac, SavedConfig.netmask, SavedConfig.gw);
	printf(" Server : ip %s, port:%d \n", SavedConfig.serverip, SavedConfig.serverport);
	printf(" ftp :: ip %s, id:%s, pw:%s \n", SavedConfig.ftpIp, SavedConfig.ftpId, SavedConfig.ftpPw);
}

/**
 * @brief When Booting Device, Initial Config Value to Saved Config Value from Filesystem
 * 
 */

void init_Data(void) 
{
	shmDataAppInfo.charger_type = CP_TYPE_CAR_AC_NORMAL_SPEED;
	memcpy(shmDataAppInfo.charger_station_id ,theConfig.siteid, sizeof(shmDataAppInfo.charger_station_id));
	memcpy(shmDataAppInfo.charger_id ,theConfig.devid1, sizeof(shmDataAppInfo.charger_id));

	shmDataAppInfo.app_order = APP_ORDER_NONE;

	shmDataAppInfo.eqp_mode[0] = 0x00;
    shmDataAppInfo.eqp_mode[1] = 0x01; 
	memset(&shmDataAppInfo.eqp_status, 0x00, 8); // Clear

	shmDataAppInfo.eqp_status[0] = 0xFF;
	shmDataAppInfo.eqp_status[1] = 0xFF;
	shmDataAppInfo.eqp_status[2] = 0xFF;
	shmDataAppInfo.eqp_status[3] = 0xFF;
	shmDataAppInfo.eqp_status[4] = 0xFF;
	shmDataAppInfo.eqp_status[5] = 0xFF;
	shmDataAppInfo.eqp_status[6] = 0xFF;
	shmDataAppInfo.eqp_status[7] = 0xFF;
}

void ConfigInit(void)
{
    cfgIni = iniparser_load(CFG_PUBLIC_DRIVE ":/" INI_FILENAME);
    if (!cfgIni)
    {
#if 1 // remove the file was crashed... test... 20200109 Kt.lee
		system("rm -rf d://ctrlboard.ini");
#endif    
        cfgIni = dictionary_new(0);
        assert(cfgIni);
		dictionary_set(cfgIni, "application", NULL);
		dictionary_set(cfgIni, "basicconfig", NULL);
        dictionary_set(cfgIni, "tcpip", NULL);
        dictionary_set(cfgIni, "ctrlboard", NULL);
		ConfigInitSave();		
    }
	strncpy(theConfig.configcheck, iniparser_getstring(cfgIni, "application:configcheck", "0"), 2);
	strncpy(theConfig.authkey, iniparser_getstring(cfgIni, "application:authkey", DEFAULT_AUTHKEY), strlen (DEFAULT_AUTHKEY) + 1); // not used
	strncpy(theConfig.timeinit, iniparser_getstring(cfgIni, "application:timeinit", "1"), 2);
	theConfig.devtype = iniparser_getint(cfgIni, "basicconfig:devtype", DEFAULT_DEVTYPE);
	sprintf(theConfig.applver, SW_VERSION, strlen(SW_VERSION)); 
	if(theConfig.devtype == C_TYPE) 			strcpy(StrModelName, SW_MODEL);		
	else
	{
		theConfig.devtype = DEFAULT_DEVTYPE;
		strcpy(StrModelName, SW_MODEL);		
		printf(" ConfigInit   theConfig.devtype %d ==> %s\n", theConfig.devtype,  StrModelName );
	}

	theConfig.devchanel = iniparser_getint(cfgIni, "basicconfig:devchanel", DEFAULT_DEVCHANNEL);
	strncpy(theConfig.devid1, iniparser_getstring(cfgIni, "basicconfig:devid1", DEFAULT_DEVID1), strlen (DEFAULT_DEVID1) + 1);
	strncpy(theConfig.devid2, iniparser_getstring(cfgIni, "basicconfig:devid2", DEFAULT_DEVID2), strlen (DEFAULT_DEVID2) + 1);
	strncpy(theConfig.siteid, iniparser_getstring(cfgIni, "basicconfig:siteid", DEFAULT_SITEID), strlen (DEFAULT_SITEID) + 1);
	strncpy(theConfig.adminpassword, iniparser_getstring(cfgIni, "basicconfig:adminpassword", DEFAULT_ADMINPW), strlen (DEFAULT_ADMINPW) + 1);
	strncpy(theConfig.gpslat, iniparser_getstring(cfgIni, "basicconfig:gpslat", DEFAULT_GPSLAT), strlen (DEFAULT_GPSLAT) + 1);
	strncpy(theConfig.gpslon, iniparser_getstring(cfgIni, "basicconfig:gpslon", DEFAULT_GPSLON), strlen (DEFAULT_GPSLON) + 1);
	theConfig.chargingstatus = iniparser_getint(cfgIni, "basicconfig:chargingstatus", 2); // not used
	theConfig.ConfirmSelect = iniparser_getint(cfgIni, "basicconfig:ConfirmSelect", USER_AUTH_NET);	//UserAuthType : Card
	theConfig.OperationMode = iniparser_getint(cfgIni, "basicconfig:OperationMode", OP_NORMAL_MODE);	//
	theConfig.FreeChargingTime = iniparser_getint(cfgIni, "basicconfig:FreeChargingTime", 0);	//
	strncpy(theConfig.chkModeMac, iniparser_getstring(cfgIni, "basicconfig:chkModeMac", "9012A100CB12"), sizeof(theConfig.chkModeMac));
	theConfig.targetSoc = iniparser_getint(cfgIni, "basicconfig:targetSoc", 100);	//
    
	// Stop Transaction
	StopTsConfig.Connector_No = iniparser_getint(cfgIni, "application:con_no", (0));
	StopTsConfig.TrId = iniparser_getint(cfgIni, "application:trid", (0));
	strncpy(StopTsConfig.Time_Stamp, iniparser_getstring(cfgIni, "application:time_stamp", "2023-10-23T14:05:04.899Z"), sizeof(StopTsConfig.Time_Stamp));
	StopTsConfig.MeterStop_Val = iniparser_getint(cfgIni, "application:meter_stop", (0));
	strncpy(StopTsConfig.IdTag, iniparser_getstring(cfgIni, "application:idtag", "0000000000000000"), sizeof(StopTsConfig.IdTag));
	StopTsConfig.Stop_Reason = iniparser_getint(cfgIni, "application:stop_reason", (0));

	// Local List

	LocalListVer = iniparser_getint(cfgIni, "ocpp:local_list_ver", 0);

	LocalListVal[0].Status = iniparser_getint(cfgIni, "application:local_list_1_status", (0));	
	strncpy(LocalListVal[0].Id, iniparser_getstring(cfgIni, "ocpp:local_list_1_id", "00000000000000000000"), strlen ("00000000000000000000") + 1);

	LocalListVal[1].Status = iniparser_getint(cfgIni, "application:local_list_2_status", (0));
	strncpy(LocalListVal[1].Id, iniparser_getstring(cfgIni, "ocpp:local_list_2_id", "00000000000000000000"), strlen ("00000000000000000000") + 1);

	LocalListVal[2].Status = iniparser_getint(cfgIni, "application:local_list_3_status", (0));	
	strncpy(LocalListVal[2].Id, iniparser_getstring(cfgIni, "ocpp:local_list_3_id", "00000000000000000000"), strlen ("00000000000000000000") + 1);

	LocalListVal[3].Status = iniparser_getint(cfgIni, "application:local_list_4_status", (0));
	strncpy(LocalListVal[3].Id, iniparser_getstring(cfgIni, "ocpp:local_list_4_id", "00000000000000000000"), strlen ("00000000000000000000") + 1);

	LocalListVal[4].Status = iniparser_getint(cfgIni, "application:local_list_5_status", (0));
	strncpy(LocalListVal[4].Id, iniparser_getstring(cfgIni, "ocpp:local_list_5_id", "00000000000000000000"), strlen ("00000000000000000000") + 1);


	// network
	theConfig.dhcp = iniparser_getint(cfgIni, "tcpip:dhcp", (1));
	//strncpy(theConfig.dhcp, iniparser_getstring(cfgIni, "tcpip:dhcp", 'y'), 1);
	strncpy(theConfig.chargermac, iniparser_getstring(cfgIni, "tcpip:chargermac", DEFAULT_NETMAC), sizeof(theConfig.chargermac));
	strncpy(theConfig.ipaddr, iniparser_getstring(cfgIni, "tcpip:ipaddr", DEFAULT_NETIP), sizeof(theConfig.ipaddr));
	strncpy(theConfig.netmask, iniparser_getstring(cfgIni, "tcpip:netmask", DEFAULT_NETMASK), sizeof(theConfig.netmask));
	strncpy(theConfig.gw, iniparser_getstring(cfgIni, "tcpip:gw", DEFAULT_NETGW), sizeof(theConfig.gw));
	strncpy(theConfig.dns, iniparser_getstring(cfgIni, "tcpip:dns", DEFAULT_NETDNS), sizeof(theConfig.dns));
	strncpy(theConfig.serverip, iniparser_getstring(cfgIni, "tcpip:serverip", DEFAULT_NETSVRIP), sizeof(theConfig.serverip));	
	theConfig.serverport = iniparser_getint(cfgIni, "tcpip:serverport", DEFAULT_NETSVRPORT);
	strncpy(theConfig.ftpIp, iniparser_getstring(cfgIni, "tcpip:ftpIp", DEFAULT_NETFTPIP), sizeof(theConfig.ftpIp));	
	strncpy(theConfig.ftpDns, iniparser_getstring(cfgIni, "tcpip:ftpDns", "csms-ftp.tscontech.com"), sizeof(theConfig.ftpDns));
	strncpy(theConfig.ftpId, iniparser_getstring(cfgIni, "tcpip:ftpId", "cptsct"), sizeof(theConfig.ftpId));	
	strncpy(theConfig.ftpPw, iniparser_getstring(cfgIni, "tcpip:ftpPw", DEFAULT_NETFTPPW), sizeof(theConfig.ftpPw));	

    // display
    theConfig.lang = iniparser_getint(cfgIni, "ctrlboard:lang", LANG_ENG);
	theConfig.brightness = iniparser_getint(cfgIni, "ctrlboard:brightness", DEFAULT_DSIPBL);
	theConfig.screensaver_time = iniparser_getint(cfgIni, "ctrlboard:screensaver_time", DEFAULT_DISPSSTIME);	 
    theConfig.screensaver_type = iniparser_getint(cfgIni, "ctrlboard:screensaver_type", SCREENSAVER_BLANK);
   // theConfig.mainmenu_type = iniparser_getint(cfgIni, "ctrlboard:mainmenu_type", MAINMENU_COVERFLOW);

    // sound
    strcpy(theConfig.keysound, iniparser_getstring(cfgIni, "ctrlboard:keysound", DEFAULT_SNDKEY));
	theConfig.keylevel = iniparser_getint(cfgIni, "ctrlboard:keylevel", DEFAULT_SNDLEVEL);
	theConfig.audiolevel = iniparser_getint(cfgIni, "ctrlboard:audiolevel", DEFAULT_SNDAULEVEL);

    // photo
	theConfig.setrevh1_flag = iniparser_getint(cfgIni, "ctrlboard:setrevh1", DEFAULT_SETREVH1);

	if((theConfig.devtype == B_TYPE) || (theConfig.devtype == HC_TYPE) || (theConfig.devtype == C_TYPE))
	{
		theConfig.devchanel = 1;
	}
	else  // BC_TYPE, BB_TYPE, HBC_TYPE, HBC_TYPE BC2_TYPE
	{
		theConfig.devchanel = 2;
	}
	// login
	printf(" ConfigInit :: Type %d, ch : %d \n", theConfig.devtype, theConfig.devchanel );
    // strncpy(theConfig.user_password, iniparser_getstring(cfgIni, "ctrlboard:user_password", "admin"), sizeof (theConfig.user_password) - 1);
	DumpConfigSave();
	memset((void *)&CstFileEvtQ.head, 0, sizeof(CstFileEvtQ));

	if(theConfig.configcheck[0] == 0x32)	
		ConfigSaveBackupDump();
	bTestModeOn = false;  // Display Test Mode .... default value is false.... 20190319.
}

// Not Use
void ConfigRecoverFromBackup(void)
{
	FILE* f;
	int ret = 0;	 
	ConfigBackup SavedConfig;
    // // Password
	f = fopen("c:/"BACKUP_FILENAME, "rb");
	if (!f)
	{
		 printf("ConfigSaveBackupDump :: cannot open ini file: %s\n", "c:/"BACKUP_FILENAME);
		 ConfigInitSave();
		 return;
	}
	ret = fread((void *)&SavedConfig.authkey[0], 1, sizeof(SavedConfig), f);
	fclose(f);
	printf(" ret[%d] ConfigSaveBackupDump :: \n Type %d, Site Id : %s, Dev Id : %s \n", ret, SavedConfig.devtype, SavedConfig.siteid, SavedConfig.devid1);  
	printf(" Net :: ip %s, MAC:%s, NetMask:%s , gw:%s \n", SavedConfig.ipaddr, SavedConfig.chargermac, SavedConfig.netmask, SavedConfig.gw);
    // // Security mode
	printf(" Server : ip %s, port:%d \n", SavedConfig.serverip, SavedConfig.serverport);	 
	printf(" ftp :: ip %s, id:%s, pw:%s \n", SavedConfig.ftpIp, SavedConfig.ftpId, SavedConfig.ftpPw);

	memcpy(theConfig.authkey, SavedConfig.authkey, 17);	
	memcpy(theConfig.devid1, SavedConfig.devid1, 3);
	memcpy(theConfig.siteid, SavedConfig.siteid, 9);
    // // wifi switch on/off
	theConfig.devtype = SavedConfig.devtype;
	theConfig.ConfirmSelect = SavedConfig.ConfirmSelect;

	memcpy(theConfig.chargermac, SavedConfig.chargermac, 18);	
	memcpy(theConfig.ipaddr, SavedConfig.ipaddr, 16);
	memcpy(theConfig.netmask, SavedConfig.netmask, 16);
	memcpy(theConfig.gw, SavedConfig.gw, 16);
	memcpy(theConfig.serverip, SavedConfig.serverip, 40);
	theConfig.serverport = SavedConfig.serverport;
	memcpy(theConfig.ftpIp, SavedConfig.ftpIp, 16);	
	memcpy(theConfig.ftpDns, SavedConfig.ftpDns, 40);	
	memcpy(theConfig.ftpId, SavedConfig.ftpId, 10);
	memcpy(theConfig.ftpPw, SavedConfig.ftpPw, 16);
	theConfig.setrevh1_flag = SavedConfig.setrevh1_flag;
	if((theConfig.devtype == B_TYPE) || (theConfig.devtype == HC_TYPE) || (theConfig.devtype == C_TYPE))
	{
		theConfig.devchanel = 1;
	}
	else  // BC_TYPE, BB_TYPE, HBC_TYPE
	{
		theConfig.devchanel = 2;
	}
	sprintf(theConfig.applver, SW_VERSION, strlen(SW_VERSION)); 
	
	printf(" ConfigInit   theConfig.devtype %d ==> %d\n", theConfig.devtype,  DEFAULT_DEVTYPE );
	theConfig.devtype = DEFAULT_DEVTYPE;
	strcpy(StrModelName, SW_MODEL);
	
	strncpy(theConfig.adminpassword, DEFAULT_ADMINPW, strlen (DEFAULT_ADMINPW) + 1);
	strncpy(theConfig.gpslat, DEFAULT_GPSLAT, strlen (DEFAULT_GPSLAT) + 1);
	strncpy(theConfig.gpslon, DEFAULT_GPSLON, strlen (DEFAULT_GPSLON) + 1);
	theConfig.chargingstatus = 2; // not used
    theConfig.lang = LANG_ENG;
	theConfig.brightness = DEFAULT_DSIPBL;
	theConfig.screensaver_time = DEFAULT_DISPSSTIME;	 
	theConfig.screensaver_type = SCREENSAVER_BLANK; 
    // // AP mode Password
    memcpy((void *)&theConfig.keysound[0], DEFAULT_SNDKEY, sizeof(DEFAULT_SNDKEY));
	theConfig.keysound[sizeof(DEFAULT_SNDKEY)] = '\0';
	theConfig.keylevel = DEFAULT_SNDLEVEL;
	theConfig.audiolevel = DEFAULT_SNDAULEVEL;

	printf(" ConfigRecoverFromBackup :: Type %d, %s , ch : %d \n", theConfig.devtype, StrModelName, theConfig.devchanel );

	CST_EVTFileSend(CST_FILE_INI_SAVE, NULL, 0);
}

// Use Server from Reset
void I1_Reset_Set(void) // Only Straffic _190411 dsAn
{
	gDailyData.count |= 0x8000;	
	CST_EVTFileSend(CST_FILE_DAYLY_SAVE, (char *)&gDailyData.date, sizeof(gDailyData));
	printf("\n Reset Bit Set \n" );
}

// Use Server from Reset
void I1_Reset_Clear(void) // Only Straffic _190411 dsAn
{
	I1_reset_boot = false;
	gDailyData.count ^= 0x8000;		
	CST_EVTFileSend(CST_FILE_DAYLY_SAVE, (char *)&gDailyData.date, sizeof(gDailyData));
	printf("\n I1_Reset_Clear \n");
}

// Use with Server
bool CstCheckDayDataDate(unsigned int curDate)
{
	printf("CstCheckDayDataDate :: the gDailyData is set to init.. [[[save date %d, curDate :%d ]]] \n", gDailyData.date, curDate);
	if(curDate == gDailyData.date)
	{		
		return false;
	}
	memset((char *)&(gDailyData.date), 0, sizeof(gDailyData));
	gDailyData.date = curDate;  // date update.....
	return true;	
}


int CstFileRead(int nNo, char *rBuf, char *sFileName)
{
    FILE* f;	
	int ret=0, length=0;
	char* mem;
 	if(rBuf == NULL)		return -1;
	f = fopen(sFileName, "rb");
	if (!f)
	{
		printf("cannot open dat file2: %s\n", sFileName);
		return -1;
	}
	fseek(f, 0, SEEK_END);
    length = ftell(f);
 	fseek(f, 0, SEEK_SET);
    mem = malloc(length);
	ret = fread(mem, 1, length, f);
	fclose(f);
	switch(nNo)
	{
		case 0:
			if(ret > (sizeof(FilePrefix)+sizeof(UnitCost)))
			{
				if(strncmp(mem, FilePrefix, sizeof(FilePrefix)) == 0)
				{
					if(strncmp(&mem[ret-sizeof(UnitCost)], UnitCost, sizeof(UnitCost)) == 0)
					{
						memcpy(rBuf, &mem[sizeof(FilePrefix)], ret-sizeof(FilePrefix)-sizeof(UnitCost));
						free(mem);
						return ret-sizeof(FilePrefix)-sizeof(UnitCost);
					}
				}
			}
			break;
		case 1:
			if(ret > (sizeof(FilePrefix)+sizeof(UserInfo)))
			{
				if(strncmp(mem, FilePrefix, sizeof(FilePrefix)) == 0)
				{
					if(strncmp(&mem[ret-sizeof(UserInfo)], UserInfo, sizeof(UserInfo)) == 0)
					{
						memcpy(rBuf, &mem[sizeof(FilePrefix)], ret-sizeof(FilePrefix)-sizeof(UserInfo));
						free(mem);
						return ret-sizeof(FilePrefix)-sizeof(UserInfo);
					}
				}
			}
			break;
		case 2:
			if(ret > (sizeof(FilePrefix)+sizeof(NotSent)))
			{
				if(strncmp(mem, FilePrefix, sizeof(FilePrefix)) == 0)
				{
					if(strncmp(&mem[ret-sizeof(NotSent)], NotSent, sizeof(NotSent)) == 0)
					{
						memcpy(rBuf, &mem[sizeof(FilePrefix)], ret-sizeof(FilePrefix)-sizeof(NotSent));
						free(mem);
						return ret-sizeof(FilePrefix)-sizeof(NotSent);
					}
				}
			}
			break;
		case 3:
			if(ret > (sizeof(FilePrefix)+sizeof(DayData)))
			{
				if(strncmp(mem, FilePrefix, sizeof(FilePrefix)) == 0)
				{
					if(strncmp(&mem[ret-sizeof(DayData)], DayData, sizeof(DayData)) == 0)
					{
						memcpy(rBuf, &mem[sizeof(FilePrefix)], ret-sizeof(FilePrefix)-sizeof(DayData));
						free(mem);
						return ret-sizeof(FilePrefix)-sizeof(DayData);
					}
				}
			}
			break;	
		case 4:
		if(ret > (sizeof(FilePrefix)+sizeof(ChargEnd)))
		{
			if(strncmp(mem, FilePrefix, sizeof(FilePrefix)) == 0)
			{
				if(strncmp(&mem[ret-sizeof(ChargEnd)], ChargEnd, sizeof(ChargEnd)) == 0)
				{
					memcpy(rBuf, &mem[sizeof(FilePrefix)], ret-sizeof(FilePrefix)-sizeof(ChargEnd)-1);
					gChargedDataFrame.nCount = mem[ret-sizeof(ChargEnd)-1] +1;
					printf("\n ===> gChargedDataFrame.nCount = [%02x] <=== \n", gChargedDataFrame.nCount);
					free(mem);
					return ret-sizeof(FilePrefix)-sizeof(ChargEnd)-1;
				}
			}
		}
		break;	
	}	
	free(mem);
	return -1;
}
int CstFileSave(char nNo, char bAdd, char *wBuf, int length, char *sFileName)
{
    FILE* f;	
	int ret =0;
	char* mem;
	printf("CstFileSave : %s\n", sFileName);
	if(bAdd)
	{
		f = fopen(sFileName, "wb+");
	}
	else
	{
		f = fopen(sFileName, "wb");
		fseek(f, 0, SEEK_SET);
	}
	if (!f)
	{
		printf("cannot open dat file: %s\n", sFileName);
		return -1;
	}
	switch(nNo)
	{
		case 0:
			if(bAdd)
			{
				int fLeng;
				mem = malloc(length+sizeof(UnitCost));
				memcpy(mem, wBuf, length);
				memcpy(&mem[length], UnitCost, sizeof(UnitCost));
				fseek(f, 0, SEEK_END);
				fLeng = ftell(f);
				fseek(f, fLeng-sizeof(UnitCost), SEEK_SET);
				ret = fwrite(mem, 1, length+sizeof(UnitCost), f);
			}
			else
			{
				mem = malloc(length+sizeof(FilePrefix)+sizeof(UnitCost));
				memcpy(mem, FilePrefix, sizeof(FilePrefix));
				memcpy(&mem[sizeof(FilePrefix)], wBuf, length);
				memcpy(&mem[sizeof(FilePrefix)+length], UnitCost, sizeof(UnitCost));			
		    	ret = fwrite(mem, 1, length+sizeof(FilePrefix)+sizeof(UnitCost), f);
			}				
			break;
		case 1:
			if(bAdd)
			{
				int fLeng;
				
				mem = malloc(length+sizeof(UserInfo));
				memcpy(mem, wBuf, length);
				memcpy(&mem[length], UserInfo, sizeof(UserInfo));
				
				fseek(f, 0, SEEK_END);
				fLeng = ftell(f);
				fseek(f, fLeng-sizeof(UserInfo), SEEK_SET);
				
				ret = fwrite(mem, 1, length+sizeof(UserInfo), f);
			}
			else
			{
				mem = malloc(length+sizeof(FilePrefix)+sizeof(UserInfo));
				memcpy(mem, FilePrefix, sizeof(FilePrefix));
				memcpy(&mem[sizeof(FilePrefix)], wBuf, length);
				memcpy(&mem[sizeof(FilePrefix)+length], UserInfo, sizeof(UserInfo));			
		    	ret = fwrite(mem, 1, length+sizeof(FilePrefix)+sizeof(UserInfo), f);
			}		
			break;
		case 2:
			if(bAdd)
			{
				int fLeng;
				
				mem = malloc(length+sizeof(NotSent));
				memcpy(mem, wBuf, length);
				memcpy(&mem[length], NotSent, sizeof(NotSent));
				
				fseek(f, 0, SEEK_END);
				fLeng = ftell(f);
				fseek(f, fLeng-sizeof(NotSent), SEEK_SET);
				
				ret = fwrite(mem, 1, length+sizeof(NotSent), f);
			}
			else
			{
				mem = malloc(length+sizeof(FilePrefix)+sizeof(NotSent));
				memcpy(mem, FilePrefix, sizeof(FilePrefix));
				memcpy(&mem[sizeof(FilePrefix)], wBuf, length);
				memcpy(&mem[sizeof(FilePrefix)+length], NotSent, sizeof(NotSent));			
		    	ret = fwrite(mem, 1, length+sizeof(FilePrefix)+sizeof(NotSent), f);
			}	
			break;
			
		case 3:
			if(bAdd)
			{
				int fLeng;
				
				mem = malloc(length+sizeof(DayData));
				memcpy(mem, wBuf, length);
				memcpy(&mem[length], DayData, sizeof(DayData));
				
				fseek(f, 0, SEEK_END);
				fLeng = ftell(f);
				fseek(f, fLeng-sizeof(DayData), SEEK_SET);
				
				ret = fwrite(mem, 1, length+sizeof(DayData), f);
			}
			else
			{
				mem = malloc(length+sizeof(FilePrefix)+sizeof(DayData));
				memcpy(mem, FilePrefix, sizeof(FilePrefix));
				memcpy(&mem[sizeof(FilePrefix)], wBuf, length);
				memcpy(&mem[sizeof(FilePrefix)+length], DayData, sizeof(DayData));			
		    	ret = fwrite(mem, 1, length+sizeof(FilePrefix)+sizeof(DayData), f);
			}	
			break;	

			case 4:
		{
			mem = malloc(length+sizeof(FilePrefix)+sizeof(ChargEnd)+1);
			memcpy(mem, FilePrefix, sizeof(FilePrefix));
			memcpy(&mem[sizeof(FilePrefix)], wBuf, length);

			mem[length+sizeof(FilePrefix)] = gChargedDataFrame.nCount;
			
			memcpy(&mem[sizeof(FilePrefix)+length+1], ChargEnd, sizeof(ChargEnd));
						
	    	ret = fwrite(mem, 1, length+sizeof(FilePrefix)+sizeof(ChargEnd)+1, f);

			printf(" [[ %d, %d :: real data count +1 plz]]\n", mem[length+sizeof(FilePrefix)], gChargedDataFrame.nCount);
			gChargedDataFrame.nCount++;  // prepare a Next F1
			
		}	
		break;			
	}
	
	fflush(f);
	fclose(f);
	free(mem);
	
	pthread_mutex_lock(&cfgMutex);

#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
	ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif

	pthread_mutex_unlock(&cfgMutex);
	
	usleep(200*1000);
	
	return ret;	
}
int CstDwFileSave(char *wBuf, int length)
{
    FILE* f;	
	int ret =0;

	//printf("CstDwFileSave : %s\n", CFG_PUBLIC_DRIVE":/"CST_FIRMWARE_NAME);

	//f = fopen(CFG_PUBLIC_DRIVE":/"CST_FIRMWARE_NAME, "wb+");		

	printf("CstDwFileSave : %s\n", CFG_PUBLIC_DRIVE":/"TSCT_FIRMWARE_NAME);

	f = fopen(CFG_PUBLIC_DRIVE":/"TSCT_FIRMWARE_NAME, "wb+");
	
	if (!f)
	{
		//printf("cannot open dat file: %s\n", CFG_PUBLIC_DRIVE":/"CST_FIRMWARE_NAME);
		printf("cannot open dat file: %s\n", CFG_PUBLIC_DRIVE":/"TSCT_FIRMWARE_NAME);
		return -1;
	}

	ret = fwrite(wBuf, 1, length, f);

	fflush(f);

    fclose(f);
	pthread_mutex_lock(&cfgMutex);

#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
	ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif

	pthread_mutex_unlock(&cfgMutex);
	
	usleep(3*1000*1000);
	
	CST_EVTFileSend(CST_FILE_INI_BAKCUP, NULL, 0);
	return ret;	
}

void ConfigExit(void)
{
    iniparser_freedict(cfgIni);
    cfgIni = NULL;
}

/**
 * @brief Save Config Value to 'ctrlboard.ini' / Call Save Backup Function
 * 
 */
static void ConfigSavePublic(void)
{
    FILE* f;
    char buf[20];
    iniparser_set(cfgIni, "application:configcheck", theConfig.configcheck);
	iniparser_set(cfgIni, "application:authkey", theConfig.authkey);

	// Stop TS
	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", StopTsConfig.Connector_No);
	iniparser_set(cfgIni, "application:con_no", buf);	

	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%lld", StopTsConfig.TrId);
	iniparser_set(cfgIni, "application:trid", buf);	

	iniparser_set(cfgIni, "application:time_stamp", StopTsConfig.Time_Stamp);

	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%ld", StopTsConfig.MeterStop_Val);
	iniparser_set(cfgIni, "application:meter_stop", buf);	

	iniparser_set(cfgIni, "application:idtag", StopTsConfig.IdTag);

	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", StopTsConfig.Stop_Reason);
	iniparser_set(cfgIni, "application:stop_reason", buf);	



	// local list

	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", LocalListVer);
	iniparser_set(cfgIni, "ocpp:local_list_ver", buf);	

	if(LocalListVer > 0) {

		memset(buf,0x00,sizeof(buf));
		sprintf(buf, "%d", LocalListVal[0].Status);
		iniparser_set(cfgIni, "ocpp:local_list_1_status", buf);
		memset(buf,0x00,sizeof(buf));
		sprintf(buf, "%d", LocalListVal[1].Status);
		iniparser_set(cfgIni, "ocpp:local_list_2_status", buf);
		memset(buf,0x00,sizeof(buf));
		sprintf(buf, "%d", LocalListVal[2].Status);
		iniparser_set(cfgIni, "ocpp:local_list_3_status", buf);
		memset(buf,0x00,sizeof(buf));
		sprintf(buf, "%d", LocalListVal[3].Status);
		iniparser_set(cfgIni, "ocpp:local_list_4_status", buf);
		memset(buf,0x00,sizeof(buf));
		sprintf(buf, "%d", LocalListVal[4].Status);
		iniparser_set(cfgIni, "ocpp:local_list_5_status", buf);

		if(LocalListVal[0].Status)
			iniparser_set(cfgIni, "ocpp:local_list_1_id", LocalListVal[0].Id);
		if(LocalListVal[1].Status)
			iniparser_set(cfgIni, "ocpp:local_list_2_id", LocalListVal[1].Id);
		if(LocalListVal[2].Status)
			iniparser_set(cfgIni, "ocpp:local_list_3_id", LocalListVal[2].Id);
		if(LocalListVal[3].Status)
			iniparser_set(cfgIni, "ocpp:local_list_4_id", LocalListVal[3].Id);
		if(LocalListVal[4].Status)
			iniparser_set(cfgIni, "ocpp:local_list_5_id", LocalListVal[4].Id);

	}


	// auth cache

    // network
	iniparser_set(cfgIni, "basicconfig:devid1", theConfig.devid1);
	iniparser_set(cfgIni, "basicconfig:devid2", theConfig.devid2);
	iniparser_set(cfgIni, "basicconfig:siteid", theConfig.siteid);
	iniparser_set(cfgIni, "basicconfig:adminpassword", theConfig.adminpassword);
	iniparser_set(cfgIni, "basicconfig:gpslat", theConfig.gpslat);
	iniparser_set(cfgIni, "basicconfig:gpslon", theConfig.gpslon);		
	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", theConfig.devtype);	
   	iniparser_set(cfgIni, "basicconfig:devtype", buf);
	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", theConfig.devchanel);	
   	iniparser_set(cfgIni, "basicconfig:devchanel", buf);
	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", theConfig.chargingstatus);	
    iniparser_set(cfgIni, "basicconfig:chargingstatus", buf);	
	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", theConfig.ConfirmSelect);
    iniparser_set(cfgIni, "basicconfig:ConfirmSelect", buf);
	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", theConfig.OperationMode);
    iniparser_set(cfgIni, "basicconfig:OperationMode", buf);
	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", theConfig.FreeChargingTime);
    iniparser_set(cfgIni, "basicconfig:FreeChargingTime", buf);
	iniparser_set(cfgIni, "basicconfig:chkModeMac", theConfig.chkModeMac);
	iniparser_set(cfgIni, "basicconfig:targetSoc", theConfig.targetSoc);


	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", theConfig.dhcp);
	iniparser_set(cfgIni, "tcpip:dhcp", buf);	
	iniparser_set(cfgIni, "tcpip:chargermac", theConfig.chargermac);	
    iniparser_set(cfgIni, "tcpip:ipaddr", theConfig.ipaddr);
    iniparser_set(cfgIni, "tcpip:netmask", theConfig.netmask);
    iniparser_set(cfgIni, "tcpip:gw", theConfig.gw);
    iniparser_set(cfgIni, "tcpip:dns", theConfig.dns);
	iniparser_set(cfgIni, "tcpip:serverip", theConfig.serverip);

	memset(buf,0x00,sizeof(buf));
	sprintf(buf, "%d", theConfig.serverport);
    iniparser_set(cfgIni, "tcpip:serverport", buf);
	iniparser_set(cfgIni, "tcpip:ftpIp", theConfig.ftpIp);
	iniparser_set(cfgIni, "tcpip:ftpDns", theConfig.ftpDns);
	iniparser_set(cfgIni, "tcpip:ftpId", theConfig.ftpId);
	iniparser_set(cfgIni, "tcpip:ftpPw", theConfig.ftpPw);
    // display
    sprintf(buf, "%d", theConfig.lang);
    iniparser_set(cfgIni, "ctrlboard:lang", buf);

    sprintf(buf, "%d", theConfig.brightness);
    iniparser_set(cfgIni, "ctrlboard:brightness", buf);
    sprintf(buf, "%d", theConfig.screensaver_time);
    iniparser_set(cfgIni, "ctrlboard:screensaver_time", buf);
    sprintf(buf, "%d", theConfig.screensaver_type);
    iniparser_set(cfgIni, "ctrlboard:screensaver_type", buf);


    // sound
    iniparser_set(cfgIni, "ctrlboard:keysound", theConfig.keysound);

    sprintf(buf, "%d", theConfig.keylevel);
    iniparser_set(cfgIni, "ctrlboard:keylevel", buf);

    sprintf(buf, "%d", theConfig.audiolevel);
    iniparser_set(cfgIni, "ctrlboard:audiolevel", buf);

	if(theConfig.setrevh1_flag != 1)	theConfig.setrevh1_flag = 0;

    // Wifi SSID
	sprintf(buf, "%d", theConfig.setrevh1_flag);
    iniparser_set(cfgIni, "ctrlboard:setrevh1", buf);

	ConfigSaveBackup(false);

    // save to file
    f = fopen(CFG_PUBLIC_DRIVE ":/" INI_FILENAME, "wb");
	if (!f)
    {
	    printf("cannot open ini file: %s\n", CFG_PUBLIC_DRIVE ":/" INI_FILENAME);
        return;
    }

    iniparser_dump_ini(cfgIni, f);
	fflush(f);
    fclose(f);
	printf(" ConfigSavePublic :: Type %d, ch : %d \n", theConfig.devtype, theConfig.devchanel );
}

/**
 * @brief Call Saving Config Function
 * 
 * @param arg 
 * @return void* 
 */
static void* ConfigSaveTask(void* arg)
{
    char* filepath = CFG_PUBLIC_DRIVE ":/" INI_FILENAME;

	//char* filepath_qr = CFG_PUBLIC_DRIVE ":/media/qr-code-test1.png";

	static char nErrorCnt = 0;
	char ret = 0;

    ConfigSavePublic();
    pthread_mutex_lock(&cfgMutex);


#ifdef CFG_CHECK_FILES_CRC_ON_BOOTING
        UpgradeSetFileCrc(filepath);
#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
        ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
#ifndef CFG_FS_LFS
        BackupSave();
#endif
#else
#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
        ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
#endif

	pthread_mutex_unlock(&cfgMutex);

	usleep(100*1000);

	ret = VerifyConfigSave();

	if( ret != 0 )
	{				
		nErrorCnt++;
		printf(" ConfigSaveTask :: Error !! %d, ErrCnt : %d \n", ret, nErrorCnt );
	}
/*
	nErrorCnt = 0;
	ret = 0;

    ConfigSavePublic();
    pthread_mutex_lock(&cfgMutex);


#ifdef CFG_CHECK_FILES_CRC_ON_BOOTING
	UpgradeSetFileCrc(filepath_qr);
#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
	ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
#ifndef CFG_FS_LFS
	BackupSave();
#endif
#else
#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
	ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
#endif

	pthread_mutex_unlock(&cfgMutex);

	usleep(100*1000);

	ret = VerifyConfigSave();

	if( ret != 0 )
	{				
		nErrorCnt++;
		printf(" ConfigSaveTask :: Error !! %d, ErrCnt : %d \n", ret, nErrorCnt );
	}
*/
    return NULL;
}

void ConfigUpdateCrc(char* filepath)
{
#ifdef CFG_CHECK_FILES_CRC_ON_BOOTING
    pthread_mutex_lock(&cfgMutex);

    if (filepath)
        UpgradeSetFileCrc(filepath);
    else
        UpgradeSetFileCrc(CFG_PUBLIC_DRIVE ":/" INI_FILENAME);

#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif

    pthread_mutex_unlock(&cfgMutex);
#endif // CFG_CHECK_FILES_CRC_ON_BOOTING
}

/**
 * @brief Call Saving Config Function
 * 
 */
void ConfigSave(void)
{
	bool bSave = VerifyConfigSave();
	printf(" ConfigSave :: cfgSavingCount -> Save [%d]\n", bSave);	 
	if(bSave)
		ConfigSaveTask(NULL);
}

/**
 * @brief When Initial Config Value, Set Default Value.
 * 
 */
void ConfigInitSave()// ctrlboard.ini의 default 값과 동일 해야한다. 
{//default
	theConfig.configcheck[0]='2';
	theConfig.devid1[0]='0';  theConfig.devid1[1]='0';                                                 
	theConfig.devid2[0]='0';  theConfig.devid2[1]='4';  

	theConfig.siteid[0] =0x30;theConfig.siteid[1] =0x30;theConfig.siteid[2] =0x30;
	theConfig.siteid[3] =0x30;theConfig.siteid[4] =0x30;theConfig.siteid[5] =0x30;
	theConfig.siteid[6] =0x30;theConfig.siteid[7] =0x30;

	theConfig.devtype = DEFAULT_DEVTYPE;
	theConfig.devchanel = 2;
	theConfig.adminpassword[0] = '2'; theConfig.adminpassword[1] = '8'; theConfig.adminpassword[2] = '4'; theConfig.adminpassword[3] = '3'; 	
	theConfig.gpslat[0]='3';	theConfig.gpslat[1]='7';	theConfig.gpslat[2]='.';	theConfig.gpslat[3]='7';
	theConfig.gpslat[4]='6';	theConfig.gpslat[5]='5';	theConfig.gpslat[6]='5';	theConfig.gpslat[7]='5';
	theConfig.gpslon[0]='1';	theConfig.gpslon[1]='2';	theConfig.gpslon[2]='8';	theConfig.gpslon[3]='.';
	theConfig.gpslon[4]='4';	theConfig.gpslon[5]='5';	theConfig.gpslon[6]='4';	theConfig.gpslon[7]='5';
	theConfig.gpslon[8]='4';	
	theConfig.chargermac[0] = 'F'; theConfig.chargermac[1] = '0'; theConfig.chargermac[2] = ':';    
	theConfig.chargermac[3] = '8'; theConfig.chargermac[4] = 'B'; theConfig.chargermac[5] = ':'; 
	theConfig.chargermac[6] = 'F'; theConfig.chargermac[7] = 'E'; theConfig.chargermac[8] = ':'; 
	theConfig.chargermac[9] = '0'; theConfig.chargermac[10] = 'F'; theConfig.chargermac[11] = ':'; 
	theConfig.chargermac[12] = '0'; theConfig.chargermac[13] = '0'; theConfig.chargermac[14] = ':'; 
	theConfig.chargermac[15] = '0'; theConfig.chargermac[16] = '0'; theConfig.chargermac[17] = '\0';

    theConfig.ipaddr[0] = 0x31 ; theConfig.ipaddr[1] = 0x39; theConfig.ipaddr[2] = 0x32; theConfig.ipaddr[3] = '.' ;
	theConfig.ipaddr[4] = 0x31 ;theConfig.ipaddr[5] = 0x36 ;theConfig.ipaddr[6] = 0x38 ;theConfig.ipaddr[7] = '.' ;
	theConfig.ipaddr[8] = 0x30;theConfig.ipaddr[9] = 0x30 ;theConfig.ipaddr[10] = 0x31 ;theConfig.ipaddr[11] = '.' ;
	theConfig.ipaddr[12] = 0x32 ;theConfig.ipaddr[13] = 0x30 ;theConfig.ipaddr[14] = 0x38;	theConfig.ipaddr[15]='\0';

	theConfig.netmask[0] = '2' ; theConfig.netmask[1] = '5'; theConfig.netmask[2] = '5'; theConfig.netmask[3] = '.' ;
	theConfig.netmask[4] = '2' ;theConfig.netmask[5] = '5' ;theConfig.netmask[6] = '5' ;theConfig.netmask[7] = '.' ;
	theConfig.netmask[8] = '2';theConfig.netmask[9] = '5' ;theConfig.netmask[10] = '5' ;theConfig.netmask[11] = '.' ;
	theConfig.netmask[12] = '0' ;theConfig.netmask[13] = '0' ;theConfig.netmask[14] = '0' ;	

	theConfig.gw[0]=0x31;	theConfig.gw[1]=0x39;	theConfig.gw[2]=0x32;	theConfig.gw[3]='.';
	theConfig.gw[4]=0x31;	theConfig.gw[5]=0x36;	theConfig.gw[6]=0x38;	theConfig.gw[7]='.';
	theConfig.gw[8]=0x30;	theConfig.gw[9]=0x30;	theConfig.gw[10]=0x31;	theConfig.gw[11]='.';
	theConfig.gw[12]=0x30;	theConfig.gw[13]=0x30;	theConfig.gw[14]=0x31;	theConfig.gw[15]='\0';

	sprintf(theConfig.serverip, DEFAULT_NETSVRIP);

	theConfig.serverport = 5000; 

	theConfig.ftpIp[0]='1';		theConfig.ftpIp[1]='9';		theConfig.ftpIp[2]='2';		theConfig.ftpIp[3]='.';
	theConfig.ftpIp[4]='1';		theConfig.ftpIp[5]='6';		theConfig.ftpIp[6]='8';		theConfig.ftpIp[7]='.';
	theConfig.ftpIp[8]='0';		theConfig.ftpIp[9]='.';		theConfig.ftpIp[10]='1';	theConfig.ftpIp[11]='0';
	theConfig.ftpIp[12]='0';	theConfig.ftpIp[13]='\0';	theConfig.ftpIp[14]=' ';	theConfig.ftpIp[15]=' ';

	sprintf(theConfig.ftpDns,"csms-ftp.tscontech.com");

	sprintf(theConfig.ftpId,"cptsct");

	sprintf(theConfig.ftpPw,"cp01340");

	theConfig.setrevh1_flag = 0;
	theConfig.ConfirmSelect = USER_AUTH_NET; ///mod
	theConfig.screensaver_time = 10;
	theConfig.screensaver_type = 2;
		
	theConfig.dhcp = 1;

	theConfig.OperationMode = 0;

	theConfig.FreeChargingTime = 0;

	theConfig.targetSoc = 100;
	
	sprintf(theConfig.chkModeMac,"9012A100CB12");

	StopTsConfig.Connector_No = 0;

	StopTsConfig.TrId = 0;

	sprintf(StopTsConfig.Time_Stamp,"2023-10-23T14:05:04.899Z");

	StopTsConfig.MeterStop_Val = 0;

	sprintf(StopTsConfig.IdTag,"0000000000000000");

	StopTsConfig.Stop_Reason = 0;

	// Local List

	LocalListVer = 0;

	LocalListVal[0].Status = 0;	
	sprintf(LocalListVal[0].Id, "00000000000000000000");

	LocalListVal[1].Status = 0;	
	sprintf(LocalListVal[1].Id, "00000000000000000000");

	LocalListVal[2].Status = 0;	
	sprintf(LocalListVal[2].Id, "00000000000000000000");

	LocalListVal[3].Status = 0;	
	sprintf(LocalListVal[3].Id, "00000000000000000000");

	LocalListVal[4].Status = 0;	
	sprintf(LocalListVal[4].Id, "00000000000000000000");

	strcpy(StrModelName, SW_MODEL);
	CST_EVTFileSend(CST_FILE_INI_SAVE, NULL, 0);

	printf("==============  ConfigInitSave =================\n");
}

/**
 * @brief Verify
 * 
 * @return char 
 */
static char VerifyConfigSave(void)
{
	char ret = 0;	
    dictionary* cfgIni2 = iniparser_load(CFG_PUBLIC_DRIVE ":/" INI_FILENAME);
	Config theConfig2;
    if (!cfgIni2)
    {
	    printf("VerifyConfigSavev :: cannot open ini file: %s\n", CFG_PUBLIC_DRIVE ":/" INI_FILENAME);
        return -1;
    }
	strncpy(theConfig2.configcheck, iniparser_getstring(cfgIni2, "application:configcheck", "0"), 2);
	strncpy(theConfig2.timeinit, iniparser_getstring(cfgIni2, "application:timeinit", "1"), 2);
	theConfig2.ConfirmSelect = iniparser_getint(cfgIni2, "basicconfig:ConfirmSelect", 1);
	strncpy(theConfig2.authkey, iniparser_getstring(cfgIni2, "application:authkey", DEFAULT_AUTHKEY), strlen (DEFAULT_AUTHKEY) + 1); // not used	
	theConfig2.devchanel = iniparser_getint(cfgIni2, "basicconfig:devchanel", DEFAULT_DEVCHANNEL);
	strncpy(theConfig2.devid1, iniparser_getstring(cfgIni2, "basicconfig:devid1", DEFAULT_DEVID1), strlen (DEFAULT_DEVID1) + 1);
	strncpy(theConfig2.devid2, iniparser_getstring(cfgIni2, "basicconfig:devid2", DEFAULT_DEVID2), strlen (DEFAULT_DEVID2) + 1);
	strncpy(theConfig2.siteid, iniparser_getstring(cfgIni2, "basicconfig:siteid", DEFAULT_SITEID), strlen (DEFAULT_SITEID) + 1);
//	strncpy(theConfig2.devtype, iniparser_getstring(cfgIni2, "basicconfig:devtype", DEFAULT_DEVTYPE), strlen (DEFAULT_DEVTYPE) + 1);
	theConfig2.devtype = iniparser_getint(cfgIni2, "basicconfig:devtype", 4); 

	strncpy(theConfig2.adminpassword, iniparser_getstring(cfgIni2, "basicconfig:adminpassword", DEFAULT_ADMINPW), strlen (DEFAULT_ADMINPW) + 1);
	strncpy(theConfig2.gpslat, iniparser_getstring(cfgIni2, "basicconfig:gpslat", DEFAULT_GPSLAT), strlen (DEFAULT_GPSLAT) + 1);
	strncpy(theConfig2.gpslon, iniparser_getstring(cfgIni2, ":gpslon", DEFAULT_GPSLON), strlen (DEFAULT_GPSLON) + 1);
	theConfig2.chargingstatus = iniparser_getint(cfgIni2, "basicconfig:chargingstatus", 2); // not used
	
	// [tcpip] - network
	theConfig2.dhcp = iniparser_getint(cfgIni, "tcpip:dhcp", (1));
	//strncpy(theConfig.dhcp, iniparser_getstring(cfgIni, "tcpip:dhcp", 'y'), 1);	
	strncpy(theConfig2.chargermac, iniparser_getstring(cfgIni2, "tcpip:chargermac", DEFAULT_NETMAC), strlen (DEFAULT_NETMAC) + 1);
	strncpy(theConfig2.ipaddr, iniparser_getstring(cfgIni2, "tcpip:ipaddr", DEFAULT_NETIP), strlen (DEFAULT_NETIP) + 1);
	strncpy(theConfig2.netmask, iniparser_getstring(cfgIni2, "tcpip:netmask", DEFAULT_NETMASK), strlen (DEFAULT_NETMASK) + 1);
	strncpy(theConfig2.gw, iniparser_getstring(cfgIni2, "tcpip:gw", DEFAULT_NETGW), strlen (DEFAULT_NETGW) + 1);
	strncpy(theConfig2.dns, iniparser_getstring(cfgIni2, "tcpip:dns", DEFAULT_NETDNS), strlen (DEFAULT_NETDNS) + 1);
	strncpy(theConfig2.serverip, iniparser_getstring(cfgIni2, "tcpip:serverip", DEFAULT_NETSVRIP), strlen (DEFAULT_NETSVRIP) + 1);	
	theConfig2.serverport = iniparser_getint(cfgIni2, "tcpip:serverport", DEFAULT_NETSVRPORT);

	strncpy(theConfig2.ftpIp, iniparser_getstring(cfgIni2, "tcpip:ftpIp", DEFAULT_NETFTPIP), strlen (DEFAULT_NETFTPIP) + 1);	
	strncpy(theConfig2.ftpIp, iniparser_getstring(cfgIni2, "tcpip:ftpId", DEFAULT_NETFTPID), strlen (DEFAULT_NETFTPID) + 1);	
	strncpy(theConfig2.ftpIp, iniparser_getstring(cfgIni2, "tcpip:ftpPw", DEFAULT_NETFTPPW), strlen (DEFAULT_NETFTPPW) + 1);	

	// [ctrlboard] - display
	theConfig2.lang = iniparser_getint(cfgIni2, "ctrlboard:lang", LANG_ENG);
	theConfig2.brightness = iniparser_getint(cfgIni2, "ctrlboard:brightness", DEFAULT_DSIPBL);
	theConfig2.screensaver_time = iniparser_getint(cfgIni2, "ctrlboard:screensaver_time", DEFAULT_DISPSSTIME);	 
	theConfig2.screensaver_type = iniparser_getint(cfgIni2, "ctrlboard:screensaver_type_home", SCREENSAVER_BLANK); 
	
	// [ctrlboard] - sound
	strcpy(theConfig2.keysound, iniparser_getstring(cfgIni2, "ctrlboard:keysound", DEFAULT_SNDKEY));
	theConfig2.keylevel = iniparser_getint(cfgIni2, "ctrlboard:keylevel", DEFAULT_SNDLEVEL);
	theConfig2.audiolevel = iniparser_getint(cfgIni2, "ctrlboard:audiolevel", DEFAULT_SNDAULEVEL);	

// Reading the ctrboard.ini is completed.. //////////////////////////////////////////////////////////////////////////////////////////////

	if (theConfig2.dhcp != theConfig.dhcp)
	{
		printf("different dhcp  %d, %d\n", theConfig.dhcp, theConfig2.dhcp);
        ret = 1;
	}

	if ( strcmp(theConfig2.configcheck, theConfig.configcheck) )    
	{
	    printf("different configcheck  %20s, %20s\n", theConfig.configcheck, theConfig2.configcheck);
        ret = 1;
    }

	if ( theConfig2.devtype != theConfig.devtype )    
	{
	    printf("different devtype  %d, %d\n", theConfig.devtype, theConfig2.devtype);
        ret = 1;
    }

	/*if ( strcmp(theConfig2.timeinit, theConfig.timeinit) )    
	{
	    printf("different timeinit  %20s, %20s\n", theConfig.timeinit, theConfig2.timeinit);
       ret = 1;
    }*/

	if ( theConfig2.ConfirmSelect != theConfig.ConfirmSelect )    
	{
	    printf("different ConfirmSelect  %d, %d\n", theConfig.ConfirmSelect, theConfig2.ConfirmSelect);
       ret = 1;
    }

	if ( strcmp(theConfig2.authkey, theConfig.authkey) )    
	{
	    printf("different authkey  %20s, %20s\n", theConfig.authkey, theConfig2.authkey);
       ret = 1;
    }	

	if ( strcmp(theConfig2.adminpassword, theConfig.adminpassword) )    
	{
	    printf("different adminpassword  %20s, %20s\n", theConfig.adminpassword, theConfig2.adminpassword);
        ret = 1;
    }

	if ( strcmp(theConfig2.chargermac, theConfig.chargermac) )    
	{
	    printf("different chargermac  %20s, %20s\n", theConfig.chargermac, theConfig2.chargermac);
        ret = 1;
    }

	if ( strcmp(theConfig2.ipaddr, theConfig.ipaddr) )    
	{
	    printf("different ipaddr  %20s, %20s\n", theConfig.ipaddr, theConfig2.ipaddr);
        ret = 1;
    }

	if ( strcmp(theConfig2.netmask, theConfig.netmask) )    
	{
	    printf("different netmask  %20s, %20s\n", theConfig.netmask, theConfig2.netmask);
        ret = 1;
    }

	if ( strcmp(theConfig2.gw, theConfig.gw) )    
	{
	    printf("different gw  %20s, %20s\n", theConfig.gw, theConfig2.gw);
       ret = 1;
    }

	if ( strcmp(theConfig2.serverip, theConfig.serverip) )    
	{
	    printf("different serverip  %20s, %20s\n", theConfig.serverip, theConfig2.serverip);
        ret = 1;
    }

	if ( theConfig2.serverport != theConfig.serverport )    
	{
	    printf("different serverport  %d, %d\n", theConfig.serverport, theConfig2.serverport);
        ret = 1;
    }

	if ( strcmp(theConfig2.ftpIp, theConfig.ftpIp) )    
	{
	    printf("different ftpIp  %20s, %20s\n", theConfig.ftpIp, theConfig2.ftpIp);
        ret = 1;
    }
	if ( strcmp(theConfig2.ftpDns, theConfig.ftpDns) )    
	{
	    printf("different ftpIp  %41s, %41s\n", theConfig.ftpDns, theConfig2.ftpDns);
        ret = 1;
    }

	if ( strcmp(theConfig2.ftpId, theConfig.ftpId) )    
	{
	    printf("different ftpId  %20s, %20s\n", theConfig.ftpId, theConfig2.ftpId);
        ret = 1;
    }

	if ( strcmp(theConfig2.ftpPw, theConfig.ftpPw) )    
	{
	    printf("different ftpPw  %16s, %16s\n", theConfig.ftpPw, theConfig2.ftpPw);
        ret = 1;
    }	

	/*if ( strcmp(theConfig2.dhcp, theConfig.dhcp) )    
	{
	    printf("different ftpPw  %c, %c\n", theConfig.dhcp, theConfig2.dhcp);
        ret = 1;
    }*/

	printf(" VerifyConfigSave :: Type %d, ch : %d \n", theConfig.devtype, theConfig.devchanel );

    return ret;
}

static char DumpConfigSave(void)
{
// Reading the ctrboard.ini is completed.. //////////////////////////////////////////////////////////////////////////////////////////////

	printf("Display %s file  ############################\n", INI_FILENAME);

	printf("configcheck  : %s\n", theConfig.configcheck);
	printf("timeinit   : %s\n", theConfig.timeinit);
	printf("ConfirmSelect :  %d\n", theConfig.ConfirmSelect);
	
	printf("authkey  : %s\n", theConfig.authkey);

	printf("devchanel : %d\n", theConfig.devchanel);
	printf("devid1 : %s\n", theConfig.devid1);
	printf("devid2 : %s\n", theConfig.devid2);
	printf("siteid : %s\n", theConfig.siteid);
	printf("devtype :%d\n", theConfig.devtype);
	
	printf("adminpassword : %s\n", theConfig.adminpassword);
	printf("chargermac : %s\n", theConfig.chargermac);
	printf("ipaddr  : %s\n", theConfig.ipaddr);
	printf("netmask : %s\n", theConfig.netmask);
	printf("gateway : %s\n", theConfig.gw);
	printf("serverip :%s\n", theConfig.serverip);
	printf("serverport : %d\n", theConfig.serverport);
	printf("ftpIp :%s\n", theConfig.ftpIp);
	printf("ftpDns :%s\n", theConfig.ftpDns);
	printf("ftpId :%s\n", theConfig.ftpId);
	printf("DHCP : %d\n", theConfig.dhcp);
	printf("######################################### \n");
    return 0;
}

static char CheckConfigSave(void)
{
	if( strlen(theConfig.adminpassword)  < 3)
	{
		ConfigInitSave();
	}
#if 1	
	printf("##### theConfig.adminpassword length : < %d > ##### \n", strlen(theConfig.adminpassword));	

	printf("devid1 : %s\n", theConfig.devid1);
	printf("devid2 : %s\n", theConfig.devid2);
	printf("siteid : %s\n", theConfig.siteid);
	printf("devtype :%d\n", theConfig.devtype);
	
	printf("chargermac : %s\n", theConfig.chargermac);
	printf("ipaddr  : %s\n", theConfig.ipaddr);
	printf("netmask : %s\n", theConfig.netmask);
	printf("gateway : %s\n", theConfig.gw);
	printf("serverip :%s\n", theConfig.serverip);
	printf("serverport : %d\n", theConfig.serverport);
	printf("ftpIp :%s\n", theConfig.ftpIp);
	printf("ftpDns :%s\n", theConfig.ftpDns);
	printf("ftpId :%s\n", theConfig.ftpId);
	printf("dhcp : %d\n", theConfig.dhcp);
	printf("######################################### \n");
#endif	
    return 0;
}

static void DoFileSaveEvent(CST_FileEvent evt)
{
	printf("[[DoFileSaveEvent]] ========> [0x%x] ,", evt.type);

	switch(evt.type)
	{			
		case CST_FILE_INI_SAVE:			
			printf(" CST_FILE_INI_SAVE \n");
			ConfigSave();
			break;	

		case CST_FILE_INI_BAKCUP:			
			printf(" CST_FILE_INI_BAKCUP \n");
			ConfigSaveBackup(true);
			usleep(3);
			ConfigSaveBackupDump();
			break;	

		case CST_FILE_DAYLY_SAVE: 		
			printf(" CST_FILE_DAYLY_SAVE [0x%x, %d ] \n", (uint16_t)evt.Buff, evt.Length);				
			CstFileSave(3, 0, evt.Buff, evt.Length, CFG_PUBLIC_DRIVE":/"DAYDATA_FILENAME );
			break;	

		case CST_FILE_F1_SAVE:			
			printf(" CST_FILE_F1_SAVE [0x%x, %d ] \n", (uint16_t)evt.Buff, evt.Length);			
			CstFileSave(4, 0, evt.Buff, evt.Length, CFG_PUBLIC_DRIVE":/"CHARGEND_FILENAME );
			break;	

		case CST_FILE_USER_DATA_SAVE:			
			printf(" CST_FILE_USER_DATA_SAVE [0x%x, %d ] \n", (uint16_t)evt.Buff, evt.Length);
			break;	

		case CST_FILE_UNIT_COST_SAVE:			
			printf(" CST_FILE_UNIT_COST_SAVE [0x%x, %d ] \n", (uint16_t)evt.Buff, evt.Length);				
			CstFileSave(0, 0, evt.Buff, evt.Length, CFG_PUBLIC_DRIVE":/"UNITCOST_FILENAME ); 
			break;	

		case CST_FILE_NO_SENT_SAVE:			
			printf(" CST_FILE_NO_SENT_SAVE [0x%x, %d ] \n", (uint16_t)evt.Buff, evt.Length);			
			CstFileSave(2, 0, evt.Buff, evt.Length, CFG_PUBLIC_DRIVE":/"NOTSENT_FILENAME );
			break;				
	}
}

/* Add an event to the event queue -- called with the queue locked */
static void CST_AddFileEvent(CST_FileEvent * event)
{
    int tail;

    tail = (CstFileEvtQ.tail + 1) % CST_FileEvtMax;
    if (tail == CstFileEvtQ.head) {
        /* Overflow, drop event */      
    } else {		
        CstFileEvtQ.event[CstFileEvtQ.tail].type = event->type;
		CstFileEvtQ.event[CstFileEvtQ.tail].Buff = event->Buff;
		CstFileEvtQ.event[CstFileEvtQ.tail].Length = event->Length;
        CstFileEvtQ.tail = tail;			
    }
}

static int CST_GetFileEvent(CST_FileEvent * event)
{
	int ret =0;

	if(CstFileEvtQ.head == CstFileEvtQ.tail ) 	return ret;

	if((CstFileEvtQ.event[CstFileEvtQ.head].type > CST_FILE_INIT) && 
		(CstFileEvtQ.event[CstFileEvtQ.head].type < CST_FILE_EVT_NONE))
	{
		event->type = CstFileEvtQ.event[CstFileEvtQ.head].type;
		event->Buff = CstFileEvtQ.event[CstFileEvtQ.head].Buff;				
		event->Length = CstFileEvtQ.event[CstFileEvtQ.head].Length;		
		ret = 1;
	}
	
    return ret;
}

static void CST_RemoveFileEvent(void)
{
//	printf("CST_RemoveEvent :: head %d, tail %d ===> ", CST_EventQ.head, CST_EventQ.tail);
	
	if((CstFileEvtQ.event[CstFileEvtQ.head].type > CST_FILE_INIT) && 
		(CstFileEvtQ.event[CstFileEvtQ.head].type < CST_FILE_EVT_NONE))
	{
		CstFileEvtQ.event[CstFileEvtQ.head].type = CST_FILE_EVT_NONE;
		CstFileEvtQ.event[CstFileEvtQ.head].Buff = NULL;
		CstFileEvtQ.event[CstFileEvtQ.head].Length = 0;
		
		CstFileEvtQ.head = (CstFileEvtQ.head+1) % CST_FileEvtMax;						
	}
}

void CST_EVTFileSend(uint16_t nEvent, uint8_t *pBuf, uint8_t len)
{
	if(CheckClientInit())
	{
		CST_FileEvent addEvt;

		addEvt.type = nEvent;
		addEvt.Buff = pBuf;
		addEvt.Length = len;
		
		printf("CST_EVTFileSend => addEvt.type : evt 0x%x, buf 0x%x length %d \n", nEvent, addEvt.type, addEvt.Length);

		CST_AddFileEvent(&addEvt);	
	}
	else
	{
		if( nEvent == CST_FILE_INI_SAVE )			
		{
			printf(" CST_FILE_INI_SAVE \n");
				ConfigSave();
		}
		else if( nEvent == CST_FILE_INI_BAKCUP )			
		{
			printf(" CST_FILE_INI_BAKCUP \n");
			ConfigSaveBackup(true);
			usleep(3);
			ConfigSaveBackupDump();
		}		
		//printf("CST_EVTFileSend => Don't be done a ClientInit() evt 0x%x, buf 0x%x length %d \n", nEvent, pBuf, len);
	}
}

static uint16_t nSleepCount = 0;
void CstNetFileFunc(void)
{
    CST_FileEvent ev;
	
    if((nSleepCount == 0)&&(CST_GetFileEvent(&ev) == 1))
    {
	 	DoFileSaveEvent(ev);	
		CST_RemoveFileEvent();
    }	
#if 0	
	if(nSleepCount > 1)		
	{
		nSleepCount--;
	//	printf("Wait for saving a file : nSleepCount :: %d \n", nSleepCount);				
	}
	else if(nSleepCount == 1)
	{
		nSleepCount = 0;
//		ShowFileSaveWait_Dialog(false);
	}
#endif
}
