#include "ite/itu.h"

extern bool LogoOnEnter(ITUWidget* widget, char* param);
extern bool StandbyOnEnter(ITUWidget* widget, char* param);
extern bool StandbyOnLeave(ITUWidget* widget, char* param);
extern bool StandbyScreenOnPress(ITUWidget* widget, char* param);
extern bool MainOnEnter(ITUWidget* widget, char* param);
extern bool MainOnLeave(ITUWidget* widget, char* param);
extern bool setting2Layer(ITUWidget* widget, char* param);
extern bool MainStartOnPress(ITUWidget* widget, char* param);
extern bool Ch2OnEnter(ITUWidget* widget, char* param);
extern bool Ch2OnLeave(ITUWidget* widget, char* param);
extern bool Ch2LeftStartOnPress(ITUWidget* widget, char* param);
extern bool Ch2RightStartOnPress(ITUWidget* widget, char* param);
extern bool AuthUserOnEnter(ITUWidget* widget, char* param);
extern bool AuthUserOnLeave(ITUWidget* widget, char* param);
extern bool MemCardOnPress(ITUWidget* widget, char* param);
extern bool AuthTypeSelectOnPress(ITUWidget* widget, char* param);
extern bool RemoteAuthOnPress(ITUWidget* widget, char* param);
extern bool RfidCardOnEnter(ITUWidget* widget, char* param);
extern bool RfidCardOnLeave(ITUWidget* widget, char* param);
extern bool B_nextLayer(ITUWidget* widget, char* param);
extern bool CardNumOnEnter(ITUWidget* widget, char* param);
extern bool CardNumOnLeave(ITUWidget* widget, char* param);
extern bool CardNumCancelOnPress(ITUWidget* widget, char* param);
extern bool CardNumOkOnPress(ITUWidget* widget, char* param);
extern bool remoteOnEnter(ITUWidget* widget, char* param);
extern bool remoteOnLeave(ITUWidget* widget, char* param);
extern bool QRCancelOnPress(ITUWidget* widget, char* param);
extern bool ConnectOnEnter(ITUWidget* widget, char* param);
extern bool ConnectOnLeave(ITUWidget* widget, char* param);
extern bool B_nextLayer2(ITUWidget* widget, char* param);
extern bool CardWaitEnter(ITUWidget* widget, char* param);
extern bool CardWaitLeave(ITUWidget* widget, char* param);
extern bool ChargeOnEnter(ITUWidget* widget, char* param);
extern bool ChargeOnLeave(ITUWidget* widget, char* param);
extern bool ChargeStopOnPress(ITUWidget* widget, char* param);
extern bool Ch2FinishOnEnter(ITUWidget* widget, char* param);
extern bool Ch2FinishOnLeave(ITUWidget* widget, char* param);
extern bool Ch2FinishConfirmOnPress(ITUWidget* widget, char* param);
extern bool DisconnectOnEnter(ITUWidget* widget, char* param);
extern bool DisconnectOnLeave(ITUWidget* widget, char* param);
extern bool ThanksOnEnter(ITUWidget* widget, char* param);
extern bool ThanksOnLeave(ITUWidget* widget, char* param);
extern bool ThanksOkOnPress(ITUWidget* widget, char* param);
extern bool PassNumOnEnter(ITUWidget* widget, char* param);
extern bool PassNumOnLeave(ITUWidget* widget, char* param);
extern bool AdminLoginOkOnPress(ITUWidget* widget, char* param);
extern bool AdminLoginCancelOnPress(ITUWidget* widget, char* param);
extern bool NumKeypad1OnPress(ITUWidget* widget, char* param);
extern bool NumKeypad2OnPress(ITUWidget* widget, char* param);
extern bool NumKeypad3OnPress(ITUWidget* widget, char* param);
extern bool NumKeypad4OnPress(ITUWidget* widget, char* param);
extern bool NumKeypad5OnPress(ITUWidget* widget, char* param);
extern bool NumKeypad6OnPress(ITUWidget* widget, char* param);
extern bool NumKeypad7OnPress(ITUWidget* widget, char* param);
extern bool NumKeypad8OnPress(ITUWidget* widget, char* param);
extern bool NumKeypad9OnPress(ITUWidget* widget, char* param);
extern bool NumKeypadResetOnPress(ITUWidget* widget, char* param);
extern bool NumKeypad0OnPress(ITUWidget* widget, char* param);
extern bool NumKeypadDelOnPress(ITUWidget* widget, char* param);
extern bool OkDialogOkOnPress(ITUWidget* widget, char* param);
extern bool OkCancelDialogCancelOnPress(ITUWidget* widget, char* param);
extern bool OkCancelDialogOkOnPress(ITUWidget* widget, char* param);
extern bool ipAllLayer_Enter(ITUWidget* widget, char* param);
extern bool ipAllLayer_Leave(ITUWidget* widget, char* param);
extern bool IpKeypadOnPress(ITUWidget* widget, char* param);
extern bool exit1Layer(ITUWidget* widget, char* param);
extern bool MacAddressCancelOnPress(ITUWidget* widget, char* param);
extern bool MacAddressOkOnPress(ITUWidget* widget, char* param);
extern bool ChargerGateWayCancelOnPress(ITUWidget* widget, char* param);
extern bool gatewayOkOnPress(ITUWidget* widget, char* param);
extern bool aNumKeypadDotOnPress(ITUWidget* widget, char* param);
extern bool aNumKeypadDelOnPress(ITUWidget* widget, char* param);
extern bool aNumKeypad0OnPress(ITUWidget* widget, char* param);
extern bool aNumKeypad9OnPress(ITUWidget* widget, char* param);
extern bool aNumKeypad8OnPress(ITUWidget* widget, char* param);
extern bool aNumKeypad7OnPress(ITUWidget* widget, char* param);
extern bool aNumKeypad6OnPress(ITUWidget* widget, char* param);
extern bool aNumKeypad5OnPress(ITUWidget* widget, char* param);
extern bool aNumKeypad4OnPress(ITUWidget* widget, char* param);
extern bool aNumKeypad3OnPress(ITUWidget* widget, char* param);
extern bool aNumKeypad2OnPress(ITUWidget* widget, char* param);
extern bool aNumKeypad1OnPress(ITUWidget* widget, char* param);
extern bool serverportcancelOnPress(ITUWidget* widget, char* param);
extern bool serverportOkOnPress(ITUWidget* widget, char* param);
extern bool chargeripsetCancelOnPress(ITUWidget* widget, char* param);
extern bool chargeripsetOkOnPress(ITUWidget* widget, char* param);
extern bool subnetmaskCancelOnPress(ITUWidget* widget, char* param);
extern bool subnetmaskOkOnPress(ITUWidget* widget, char* param);
extern bool ServeripOkCancelPress(ITUWidget* widget, char* param);
extern bool ServeripOkOnPress(ITUWidget* widget, char* param);
extern bool adminsetipEnter(ITUWidget* widget, char* param);
extern bool adminsetipLeave(ITUWidget* widget, char* param);
extern bool SetNetFuncOnPress(ITUWidget* widget, char* param);
extern bool ToggleDHCP(ITUWidget* widget, char* param);
extern bool exitLayer(ITUWidget* widget, char* param);
extern bool SetIpSubMenu(ITUWidget* widget, char* param);
extern bool adminsetchargerEnter(ITUWidget* widget, char* param);
extern bool adminsetchargerLeave(ITUWidget* widget, char* param);
extern bool exitaLayer(ITUWidget* widget, char* param);
extern bool SetChargerSubMenu(ITUWidget* widget, char* param);
extern bool SetCharger2SelectItem(ITUWidget* widget, char* param);
extern bool SelectDevSetFunc(ITUWidget* widget, char* param);
extern bool adminChargerTestEnter(ITUWidget* widget, char* param);
extern bool adminChargerTestLeave(ITUWidget* widget, char* param);
extern bool exitcLayer(ITUWidget* widget, char* param);
extern bool ChargerTestSubMenu(ITUWidget* widget, char* param);
extern bool AdminSetTestSelectItem(ITUWidget* widget, char* param);
extern bool layer_chargersetEnter(ITUWidget* widget, char* param);
extern bool layer_chargersetLeave(ITUWidget* widget, char* param);
extern bool KeypadOnPress_del(ITUWidget* widget, char* param);
extern bool KeypadOnPress_next(ITUWidget* widget, char* param);
extern bool KeypadOnPress(ITUWidget* widget, char* param);
extern bool bNumKeypad1OnPress(ITUWidget* widget, char* param);
extern bool exit2Layer(ITUWidget* widget, char* param);
extern bool CancelONpress(ITUWidget* widget, char* param);
extern bool TargetSocOkOnPress(ITUWidget* widget, char* param);
extern bool SelectTargetSocSetPress(ITUWidget* widget, char* param);
extern bool SelectEvccMacSetPress(ITUWidget* widget, char* param);
extern bool SelectFreeTimeSetPress(ITUWidget* widget, char* param);
extern bool SelectModeSetPress(ITUWidget* widget, char* param);
extern bool ModeOkOnPress(ITUWidget* widget, char* param);
extern bool PassChangeOkOnPress(ITUWidget* widget, char* param);
extern bool SelectDateTimeSet(ITUWidget* widget, char* param);
extern bool SelectLcdTimeSetPress(ITUWidget* widget, char* param);
extern bool lcdOnOffOkOnPress(ITUWidget* widget, char* param);
extern bool SelSoundSetPress(ITUWidget* widget, char* param);
extern bool chargerid2OkOnPress(ITUWidget* widget, char* param);
extern bool stationIdOkOnPress(ITUWidget* widget, char* param);
extern bool DevIdTwoPress(ITUWidget* widget, char* param);
extern bool DevIdOnePress(ITUWidget* widget, char* param);
extern bool chargerid1OkOnPress(ITUWidget* widget, char* param);
extern bool layer_charger2setEnter(ITUWidget* widget, char* param);
extern bool Layer_charger2setLeave(ITUWidget* widget, char* param);
extern bool exit3Layer(ITUWidget* widget, char* param);
extern bool cNumKeypaddelOnPress(ITUWidget* widget, char* param);
extern bool cNumKeypadjjumOnPress(ITUWidget* widget, char* param);
extern bool cNumKeypad0OnPress(ITUWidget* widget, char* param);
extern bool cNumKeypad9OnPress(ITUWidget* widget, char* param);
extern bool cNumKeypad8OnPress(ITUWidget* widget, char* param);
extern bool cNumKeypad7OnPress(ITUWidget* widget, char* param);
extern bool cNumKeypad6OnPress(ITUWidget* widget, char* param);
extern bool cNumKeypad5OnPress(ITUWidget* widget, char* param);
extern bool cNumKeypad4OnPress(ITUWidget* widget, char* param);
extern bool cNumKeypad3OnPress(ITUWidget* widget, char* param);
extern bool cNumKeypad2OnPress(ITUWidget* widget, char* param);
extern bool cNumKeypad1OnPress(ITUWidget* widget, char* param);
extern bool cNumKeypadFOnPress(ITUWidget* widget, char* param);
extern bool cNumKeypadEOnPress(ITUWidget* widget, char* param);
extern bool cNumKeypadDOnPress(ITUWidget* widget, char* param);
extern bool cNumKeypadCOnPress(ITUWidget* widget, char* param);
extern bool cNumKeypadBOnPress(ITUWidget* widget, char* param);
extern bool cNumKeypadAOnPress(ITUWidget* widget, char* param);
extern bool ChargerSelectCancelOnPress(ITUWidget* widget, char* param);
extern bool ChargerSelectOkOnPress(ITUWidget* widget, char* param);
extern bool SystemInitcancelOnPress(ITUWidget* widget, char* param);
extern bool SystemInitOkOnPress(ITUWidget* widget, char* param);
extern bool GpsLongitudetCancelOnPress(ITUWidget* widget, char* param);
extern bool GpsLongitudetOkOnPress(ITUWidget* widget, char* param);
extern bool GpsLatitudeCancelOnPress(ITUWidget* widget, char* param);
extern bool GpsLatitudeOkOnPress(ITUWidget* widget, char* param);
extern bool AdmintestEnter(ITUWidget* widget, char* param);
extern bool AdmintestLeave(ITUWidget* widget, char* param);
extern bool exit4Layer(ITUWidget* widget, char* param);
extern bool FullTestStartOnPress(ITUWidget* widget, char* param);
extern bool BacklightOff_OnPress(ITUWidget* widget, char* param);
extern bool Mc2teststartOnPress(ITUWidget* widget, char* param);
extern bool Led1teststartOnPress(ITUWidget* widget, char* param);
extern bool Mc1teststartOnPress(ITUWidget* widget, char* param);
extern bool PingTest_OnPress(ITUWidget* widget, char* param);
extern bool Ami2Test_OnPress(ITUWidget* widget, char* param);
extern bool Ami1Test_OnPress(ITUWidget* widget, char* param);
extern bool RfReaderTest_OnPress(ITUWidget* widget, char* param);
extern bool FtpFwUpdateEnter(ITUWidget* widget, char* param);
extern bool FtpFwUpdateLeave(ITUWidget* widget, char* param);
extern bool FtpKeypadOnPress(ITUWidget* widget, char* param);
extern bool FtpFwUpdatCancelOnPress(ITUWidget* widget, char* param);
extern bool FtpInfoSelect(ITUWidget* widget, char* param);
extern bool FtpFwUpdatOnPress(ITUWidget* widget, char* param);
extern bool AdminchargerlistEnter(ITUWidget* widget, char* param);
extern bool AdminchargerlistLeave(ITUWidget* widget, char* param);
extern bool exitdLayer(ITUWidget* widget, char* param);
extern bool ChargerListSubMenu(ITUWidget* widget, char* param);
extern bool GoUserCardManage(ITUWidget* widget, char* param);
extern bool ChkChangeDevType(ITUWidget* widget, char* param);
extern bool press_Send_h1(ITUWidget* widget, char* param);
extern bool FtpFWupdate(ITUWidget* widget, char* param);
extern bool qualifTestEnter(ITUWidget* widget, char* param);
extern bool qualifTestLeave(ITUWidget* widget, char* param);
extern bool tMc2teststartOnPress(ITUWidget* widget, char* param);
extern bool tMc1teststartOnPress(ITUWidget* widget, char* param);
extern bool tExitLayer(ITUWidget* widget, char* param);
extern bool backBtn_press(ITUWidget* widget, char* param);
extern bool homeBtn_press(ITUWidget* widget, char* param);

ITUActionFunction actionFunctions[] =
{
    "LogoOnEnter", LogoOnEnter,
    "StandbyOnEnter", StandbyOnEnter,
    "StandbyOnLeave", StandbyOnLeave,
    "StandbyScreenOnPress", StandbyScreenOnPress,
    "MainOnEnter", MainOnEnter,
    "MainOnLeave", MainOnLeave,
    "setting2Layer", setting2Layer,
    "MainStartOnPress", MainStartOnPress,
    "Ch2OnEnter", Ch2OnEnter,
    "Ch2OnLeave", Ch2OnLeave,
    "Ch2LeftStartOnPress", Ch2LeftStartOnPress,
    "Ch2RightStartOnPress", Ch2RightStartOnPress,
    "AuthUserOnEnter", AuthUserOnEnter,
    "AuthUserOnLeave", AuthUserOnLeave,
    "MemCardOnPress", MemCardOnPress,
    "AuthTypeSelectOnPress", AuthTypeSelectOnPress,
    "RemoteAuthOnPress", RemoteAuthOnPress,
    "RfidCardOnEnter", RfidCardOnEnter,
    "RfidCardOnLeave", RfidCardOnLeave,
    "B_nextLayer", B_nextLayer,
    "CardNumOnEnter", CardNumOnEnter,
    "CardNumOnLeave", CardNumOnLeave,
    "CardNumCancelOnPress", CardNumCancelOnPress,
    "CardNumOkOnPress", CardNumOkOnPress,
    "remoteOnEnter", remoteOnEnter,
    "remoteOnLeave", remoteOnLeave,
    "QRCancelOnPress", QRCancelOnPress,
    "ConnectOnEnter", ConnectOnEnter,
    "ConnectOnLeave", ConnectOnLeave,
    "B_nextLayer2", B_nextLayer2,
    "CardWaitEnter", CardWaitEnter,
    "CardWaitLeave", CardWaitLeave,
    "ChargeOnEnter", ChargeOnEnter,
    "ChargeOnLeave", ChargeOnLeave,
    "ChargeStopOnPress", ChargeStopOnPress,
    "Ch2FinishOnEnter", Ch2FinishOnEnter,
    "Ch2FinishOnLeave", Ch2FinishOnLeave,
    "Ch2FinishConfirmOnPress", Ch2FinishConfirmOnPress,
    "DisconnectOnEnter", DisconnectOnEnter,
    "DisconnectOnLeave", DisconnectOnLeave,
    "ThanksOnEnter", ThanksOnEnter,
    "ThanksOnLeave", ThanksOnLeave,
    "ThanksOkOnPress", ThanksOkOnPress,
    "PassNumOnEnter", PassNumOnEnter,
    "PassNumOnLeave", PassNumOnLeave,
    "AdminLoginOkOnPress", AdminLoginOkOnPress,
    "AdminLoginCancelOnPress", AdminLoginCancelOnPress,
    "NumKeypad1OnPress", NumKeypad1OnPress,
    "NumKeypad2OnPress", NumKeypad2OnPress,
    "NumKeypad3OnPress", NumKeypad3OnPress,
    "NumKeypad4OnPress", NumKeypad4OnPress,
    "NumKeypad5OnPress", NumKeypad5OnPress,
    "NumKeypad6OnPress", NumKeypad6OnPress,
    "NumKeypad7OnPress", NumKeypad7OnPress,
    "NumKeypad8OnPress", NumKeypad8OnPress,
    "NumKeypad9OnPress", NumKeypad9OnPress,
    "NumKeypadResetOnPress", NumKeypadResetOnPress,
    "NumKeypad0OnPress", NumKeypad0OnPress,
    "NumKeypadDelOnPress", NumKeypadDelOnPress,
    "OkDialogOkOnPress", OkDialogOkOnPress,
    "OkCancelDialogCancelOnPress", OkCancelDialogCancelOnPress,
    "OkCancelDialogOkOnPress", OkCancelDialogOkOnPress,
    "ipAllLayer_Enter", ipAllLayer_Enter,
    "ipAllLayer_Leave", ipAllLayer_Leave,
    "IpKeypadOnPress", IpKeypadOnPress,
    "exit1Layer", exit1Layer,
    "MacAddressCancelOnPress", MacAddressCancelOnPress,
    "MacAddressOkOnPress", MacAddressOkOnPress,
    "ChargerGateWayCancelOnPress", ChargerGateWayCancelOnPress,
    "gatewayOkOnPress", gatewayOkOnPress,
    "aNumKeypadDotOnPress", aNumKeypadDotOnPress,
    "aNumKeypadDelOnPress", aNumKeypadDelOnPress,
    "aNumKeypad0OnPress", aNumKeypad0OnPress,
    "aNumKeypad9OnPress", aNumKeypad9OnPress,
    "aNumKeypad8OnPress", aNumKeypad8OnPress,
    "aNumKeypad7OnPress", aNumKeypad7OnPress,
    "aNumKeypad6OnPress", aNumKeypad6OnPress,
    "aNumKeypad5OnPress", aNumKeypad5OnPress,
    "aNumKeypad4OnPress", aNumKeypad4OnPress,
    "aNumKeypad3OnPress", aNumKeypad3OnPress,
    "aNumKeypad2OnPress", aNumKeypad2OnPress,
    "aNumKeypad1OnPress", aNumKeypad1OnPress,
    "serverportcancelOnPress", serverportcancelOnPress,
    "serverportOkOnPress", serverportOkOnPress,
    "chargeripsetCancelOnPress", chargeripsetCancelOnPress,
    "chargeripsetOkOnPress", chargeripsetOkOnPress,
    "subnetmaskCancelOnPress", subnetmaskCancelOnPress,
    "subnetmaskOkOnPress", subnetmaskOkOnPress,
    "ServeripOkCancelPress", ServeripOkCancelPress,
    "ServeripOkOnPress", ServeripOkOnPress,
    "adminsetipEnter", adminsetipEnter,
    "adminsetipLeave", adminsetipLeave,
    "SetNetFuncOnPress", SetNetFuncOnPress,
    "ToggleDHCP", ToggleDHCP,
    "exitLayer", exitLayer,
    "SetIpSubMenu", SetIpSubMenu,
    "adminsetchargerEnter", adminsetchargerEnter,
    "adminsetchargerLeave", adminsetchargerLeave,
    "exitaLayer", exitaLayer,
    "SetChargerSubMenu", SetChargerSubMenu,
    "SetCharger2SelectItem", SetCharger2SelectItem,
    "SelectDevSetFunc", SelectDevSetFunc,
    "adminChargerTestEnter", adminChargerTestEnter,
    "adminChargerTestLeave", adminChargerTestLeave,
    "exitcLayer", exitcLayer,
    "ChargerTestSubMenu", ChargerTestSubMenu,
    "AdminSetTestSelectItem", AdminSetTestSelectItem,
    "layer_chargersetEnter", layer_chargersetEnter,
    "layer_chargersetLeave", layer_chargersetLeave,
    "KeypadOnPress_del", KeypadOnPress_del,
    "KeypadOnPress_next", KeypadOnPress_next,
    "KeypadOnPress", KeypadOnPress,
    "bNumKeypad1OnPress", bNumKeypad1OnPress,
    "exit2Layer", exit2Layer,
    "CancelONpress", CancelONpress,
    "TargetSocOkOnPress", TargetSocOkOnPress,
    "SelectTargetSocSetPress", SelectTargetSocSetPress,
    "SelectEvccMacSetPress", SelectEvccMacSetPress,
    "SelectFreeTimeSetPress", SelectFreeTimeSetPress,
    "SelectModeSetPress", SelectModeSetPress,
    "ModeOkOnPress", ModeOkOnPress,
    "PassChangeOkOnPress", PassChangeOkOnPress,
    "SelectDateTimeSet", SelectDateTimeSet,
    "SelectLcdTimeSetPress", SelectLcdTimeSetPress,
    "lcdOnOffOkOnPress", lcdOnOffOkOnPress,
    "SelSoundSetPress", SelSoundSetPress,
    "chargerid2OkOnPress", chargerid2OkOnPress,
    "stationIdOkOnPress", stationIdOkOnPress,
    "DevIdTwoPress", DevIdTwoPress,
    "DevIdOnePress", DevIdOnePress,
    "chargerid1OkOnPress", chargerid1OkOnPress,
    "layer_charger2setEnter", layer_charger2setEnter,
    "Layer_charger2setLeave", Layer_charger2setLeave,
    "exit3Layer", exit3Layer,
    "cNumKeypaddelOnPress", cNumKeypaddelOnPress,
    "cNumKeypadjjumOnPress", cNumKeypadjjumOnPress,
    "cNumKeypad0OnPress", cNumKeypad0OnPress,
    "cNumKeypad9OnPress", cNumKeypad9OnPress,
    "cNumKeypad8OnPress", cNumKeypad8OnPress,
    "cNumKeypad7OnPress", cNumKeypad7OnPress,
    "cNumKeypad6OnPress", cNumKeypad6OnPress,
    "cNumKeypad5OnPress", cNumKeypad5OnPress,
    "cNumKeypad4OnPress", cNumKeypad4OnPress,
    "cNumKeypad3OnPress", cNumKeypad3OnPress,
    "cNumKeypad2OnPress", cNumKeypad2OnPress,
    "cNumKeypad1OnPress", cNumKeypad1OnPress,
    "cNumKeypadFOnPress", cNumKeypadFOnPress,
    "cNumKeypadEOnPress", cNumKeypadEOnPress,
    "cNumKeypadDOnPress", cNumKeypadDOnPress,
    "cNumKeypadCOnPress", cNumKeypadCOnPress,
    "cNumKeypadBOnPress", cNumKeypadBOnPress,
    "cNumKeypadAOnPress", cNumKeypadAOnPress,
    "ChargerSelectCancelOnPress", ChargerSelectCancelOnPress,
    "ChargerSelectOkOnPress", ChargerSelectOkOnPress,
    "SystemInitcancelOnPress", SystemInitcancelOnPress,
    "SystemInitOkOnPress", SystemInitOkOnPress,
    "GpsLongitudetCancelOnPress", GpsLongitudetCancelOnPress,
    "GpsLongitudetOkOnPress", GpsLongitudetOkOnPress,
    "GpsLatitudeCancelOnPress", GpsLatitudeCancelOnPress,
    "GpsLatitudeOkOnPress", GpsLatitudeOkOnPress,
    "AdmintestEnter", AdmintestEnter,
    "AdmintestLeave", AdmintestLeave,
    "exit4Layer", exit4Layer,
    "FullTestStartOnPress", FullTestStartOnPress,
    "BacklightOff_OnPress", BacklightOff_OnPress,
    "Mc2teststartOnPress", Mc2teststartOnPress,
    "Led1teststartOnPress", Led1teststartOnPress,
    "Mc1teststartOnPress", Mc1teststartOnPress,
    "PingTest_OnPress", PingTest_OnPress,
    "Ami2Test_OnPress", Ami2Test_OnPress,
    "Ami1Test_OnPress", Ami1Test_OnPress,
    "RfReaderTest_OnPress", RfReaderTest_OnPress,
    "FtpFwUpdateEnter", FtpFwUpdateEnter,
    "FtpFwUpdateLeave", FtpFwUpdateLeave,
    "FtpKeypadOnPress", FtpKeypadOnPress,
    "FtpFwUpdatCancelOnPress", FtpFwUpdatCancelOnPress,
    "FtpInfoSelect", FtpInfoSelect,
    "FtpFwUpdatOnPress", FtpFwUpdatOnPress,
    "AdminchargerlistEnter", AdminchargerlistEnter,
    "AdminchargerlistLeave", AdminchargerlistLeave,
    "exitdLayer", exitdLayer,
    "ChargerListSubMenu", ChargerListSubMenu,
    "GoUserCardManage", GoUserCardManage,
    "ChkChangeDevType", ChkChangeDevType,
    "press_Send_h1", press_Send_h1,
    "FtpFWupdate", FtpFWupdate,
    "qualifTestEnter", qualifTestEnter,
    "qualifTestLeave", qualifTestLeave,
    "tMc2teststartOnPress", tMc2teststartOnPress,
    "tMc1teststartOnPress", tMc1teststartOnPress,
    "tExitLayer", tExitLayer,
    "backBtn_press", backBtn_press,
    "homeBtn_press", homeBtn_press,
    NULL, NULL
};
