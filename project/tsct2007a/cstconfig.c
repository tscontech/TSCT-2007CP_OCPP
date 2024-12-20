#include <sys/ioctl.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "iniparser/iniparser.h"
#include "ite/itp.h"
#include "ctrlboard.h"
#include "scene.h"

#define DEFAULT_BUFFER_SIZE 1024
#define INI_FILENAME_SET "c:\boardConfig.ini"
char m_pFileBuffer[1024*2];
char pData[DEFAULT_BUFFER_SIZE];


int SetDataValue(char *chTotalString, char *chStartString, char *chEndString, char *chModifyString, char *chModifyValue)
{
	char *chTemp;
	chTemp = NULL;
	char *chLastFile;
	chLastFile = NULL;
	char chStartString1[DEFAULT_BUFFER_SIZE];

	int nTotalStringSize = strlen(chTotalString);
	int nStartStringSize = strlen(chStartString);

	int nStartPos = 0;
	int nEndStringSize = 0;
	int nModifyStringSize = strlen(chModifyString);

	int nModifyValueSize = 0;
	chTemp = strstr(chTotalString, chStartString);
	nStartPos = chTemp - chTotalString + nStartStringSize;

	memset(chStartString1,0,DEFAULT_BUFFER_SIZE);
	memcpy(chStartString1,chTotalString,nStartPos);
	chLastFile = strstr(chTemp,chEndString);
	nEndStringSize = strlen(chLastFile);	
	memset(chModifyValue,0,DEFAULT_BUFFER_SIZE);
	memcpy(chModifyValue,chStartString1,nStartPos);
	memcpy(chModifyValue+nStartPos,chModifyString,nModifyStringSize);
	memcpy(chModifyValue+nStartPos+nModifyStringSize,chLastFile,nEndStringSize);

	nModifyValueSize = strlen(chModifyValue);		
	memset(m_pFileBuffer,0,DEFAULT_BUFFER_SIZE);
	memcpy(m_pFileBuffer,chModifyValue,nModifyValueSize);
	chTemp = NULL;
	chLastFile = NULL;	

	return nModifyValueSize;
}
int GetDataValue(char *chTotalString, char *chStartString, char *chEndString, char *chSearchString)
{
	char *chTemp;
	chTemp = NULL;
	char *chLastFile;
	chLastFile = NULL;
	int nTotalStringSize = strlen(chTotalString);
	int nStartStringSize = strlen(chStartString);
	int nSearchStringSize = 0;
	chTemp = strstr(chTotalString, chStartString);
	chLastFile = strstr(chTemp, chEndString);
	nSearchStringSize = chLastFile - chTemp - nStartStringSize;
	memcpy(chSearchString, chTemp+nStartStringSize, nSearchStringSize);
	return nSearchStringSize;		
}

void DefaultConfigData()
{
	FILE* pFile;
	pFile = fopen(INI_FILENAME_SET,"wb");
	if(pFile == NULL)
	{
		printf(" Default Config Data Write Fail \n");
		return ;
	}
	
	fprintf(pFile,"<Network> \r\n");
	fprintf(pFile,"IP Type=1 \r\n");
	
	fclose(pFile);
	
}

void TestConfigInit(void)
{	
	FILE *pFile;
	char chDataBuf[1024];
	char chModifyBuf[1024];	
	int retval;
	int nIPType ;
	printf(" ==========GetConfigData Failed =======================\n");
	long nDataSize = 0;

	pFile = fopen(INI_FILENAME_SET,"rb");
	
	if(pFile == NULL)
	{
		DefaultConfigData();
		pFile = fopen(INI_FILENAME_SET,"rb");
		if(pFile == NULL)
		{
			printf("GetConfigData Failed \n");
			return ;
		}	
	}
	
	memset(chDataBuf,0,sizeof(chDataBuf));
	GetDataValue(pFile,"IP Type="," \r\n",chDataBuf);
	nIPType = atoi(chDataBuf);
	printf(" ==============  nIPType 1 [%d]\n",nIPType);
	nIPType++;
	itoa(chDataBuf,nIPType,2);
	printf(" ==============  nIPType 2 [%d]\n",nIPType);
	SetDataValue(m_pFileBuffer, "IP Type=", " \r\n", chDataBuf, chModifyBuf);
}