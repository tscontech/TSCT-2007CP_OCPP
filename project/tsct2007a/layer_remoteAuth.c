#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ite/itp.h"
#include "ite/itu.h"
#include "SDL/SDL.h"
#include "tsctqrencode/qrenc.h"
#include "scene.h"
#include "ctrlboard.h"

#define REMOTE_TIMEOUT	90

static ITUBackground* sremouteBackground;
static ITUText* sQRGentxt1;
static ITUIcon* sQR_icon;
char qrcode_buf[120];

void Tsct_GenQR(void)
{
    ituWidgetSetVisible(sQRGentxt1, false);
    ituWidgetSetVisible(sQR_icon, true);
}

void QRGotoSatartLayer(void)
{
    GotoStartLayer();
}

bool remoteOnEnter(ITUWidget* widget, char* param)
{
 	CtLogRed("Enter Remote Auth layer..\n");
    shmDataAppInfo.app_order = APP_ORDER_KAKAO_QR;

    if (!sremouteBackground)
    {
        sremouteBackground = ituSceneFindWidget(&theScene, "remouteBackground");
        assert(sremouteBackground);
        sQRGentxt1 = ituSceneFindWidget(&theScene, "QRGentxt1");
		assert(sQRGentxt1);
        sQR_icon = ituSceneFindWidget(&theScene, "QR_icon");
		assert(sQR_icon);	
    }
    int randomNumber = rand() % 100;
    sprintf(CsConfigVal.SecureNo, "%02d", randomNumber);
    sprintf(qrcode_buf, "https://tscs.tscontech.com/api/kakaopay/call_one?cs_id=%s&cp_id=%s&cnctr_id=%02d&no=%s", theConfig.siteid, theConfig.devid1, bDevChannel+1, CsConfigVal.SecureNo);
    qrencode(qrcode_buf, strlen(qrcode_buf), CFG_PUBLIC_DRIVE ":/media/qren-code.png");
    ituIconLoadPngFileSync(sQR_icon, CFG_PUBLIC_DRIVE ":/media/qren-code.png");

    ituWidgetSetVisible(sQRGentxt1, false);
    ituWidgetSetVisible(sQR_icon, true);
    CsConfigVal.bQREventFlg = 1;

    TopHomeBtnVisible(false);
    TopBackBtnVisible(false);

    TopSetTimer(REMOTE_TIMEOUT, QRGotoSatartLayer);
 
    return true;
}

bool QRCancelOnPress(ITUWidget* widget, char* param)
{
    GotoStartLayer();
	return true;
}

bool remoteOnLeave(ITUWidget* widget, char* param)
{
    CtLogBlue("Leave the remoteAuth Layer ..\n");
    if(CsConfigVal.bReqRmtStartTsNo == 0)
    {
        CsConfigVal.bQREventFlg = 2;
    }

    TopCloseTimer();
    return true;
}

