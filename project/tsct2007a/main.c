#include <sys/ioctl.h>
#include <unistd.h>
#include "libxml/parser.h"
#include "SDL/SDL.h"
#include "ite/itp.h"
#include "ctrlboard.h"
#include "scene.h"
#include "network_config.h"
#include "tsctcommon.h"
#include "tsctcfg.h"

#ifdef _WIN32
    #include <crtdbg.h>
#else
    #include "openrtos/FreeRTOS.h"
    #include "openrtos/task.h"
#endif
bool pingtestcheck = false;
bool booting_set = false;

extern void BackupInit(void);
extern void BackupRestore(void);
extern void BackupSyncFile(void);
extern void BackupDestroy(void);

int SDL_main(int argc, char *argv[]){
    int ret = 0;
    int restryCount = 0;


#if !defined(CFG_LCD_INIT_ON_BOOTING) && !defined(CFG_BL_SHOW_LOGO)
    ScreenClear();

#endif
    CtLogMagenta("[SDL_main]Start \r\n");

#ifdef CFG_CHECK_FILES_CRC_ON_BOOTING
    #ifdef CFG_FS_LFS
        ret = UpgradeInit();
        if (ret)
            goto end;
    #else
        BackupInit();
    retry_backup:
        ret = UpgradeInit();
        if (ret)
        {
            if (++restryCount > 2)
            {
                printf("cannot recover from backup....\n");
                goto end;
            }
            BackupRestore();
            goto retry_backup;
        }
        BackupSyncFile();
    #endif
#else
    CtLogMagenta("[SDL_main]Check Upgrade USB \r\n");
    ret = UpgradeInit();
    if (ret)  goto end;
#endif

#ifdef	CFG_DYNAMIC_LOAD_TP_MODULE
	//This function must be in front of SDL_Init().
	DynamicLoadTpModule();
#endif
    CtLogMagenta("[SDL_main] SDL_Init \r\n");
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());

    CtLogMagenta("[SDL_main] ConfigInit \r\n");
    ConfigInit();

    init_Data();
    
    CtLogMagenta("[SDL_main] NetworkInit \r\n");
    xNetworkInit(); 
    sleep(2);
    // ClientInit();
    WsClientInit();

        // WebServerInit();
        // WebServerInit();

    // CtLogMagenta("[SDL_main] BuzzerInit \r\n");usleep(100000);
    BuzzerInit();
    CtLogMagenta("[SDL_main] ScreenInit \r\n");
    ScreenInit();
    CtLogMagenta("[SDL_main] ExternalInit \r\n");
    ExternalInit();
#if defined(CFG_UPGRADE_FROM_UART_RUN_TIME)
    CtLogMagenta("[SDL_main] UpgradeUartInit \r\n");usleep(100000);
	UpgradeUartInit();
#endif
    CtLogMagenta("[SDL_main] StorageInit \r\n");
    StorageInit();

    // CtLogMagenta("[SDL_main] AudioInit \r\n");usleep(100000);
    AudioInit();

    CtLogMagenta("[SDL_main] PhotoInit \r\n");
    PhotoInit();

    // CtLogMagenta("[SDL_main] SceneLoad \r\n");usleep(100000);
    // SceneLoad();
    itpRtcInit(); printf("\n-----rtc init-----\n");

    // CtLogMagenta("[SDL_main] Ac220Init \r\n");
    // Ac220Init();

    BacklightInit();
	LEDInit();
	WattHourMeterInit();
    
    // CstGfciInit();
    ControlPilotInit();

    SeccInit();

    CardReaderInit(); 

    // ObdInit();

    OderSequenceInit(); printf("\n-----OderSequenceInit-----\n ");

    FaultManageInit();  printf("-----FaultManageInit-----\n ");

    Temperature_Humidity_Init();

    CtLogMagenta("[SDL_main] SceneInit \r\n");
    SceneInit();
    CtLogMagenta("[SDL_main] SceneRun \r\n");
    SceneLoad();
    ret = SceneRun();
    while (1) {
        sleep(1);
    }
    SceneExit();

    ioctl(ITP_DEVICE_DRIVE, ITP_IOCTL_EXIT, NULL);

    PhotoExit();
    AudioExit();
#if defined(CFG_UPGRADE_FROM_UART_RUN_TIME)
	UpgradeUartExit();
#endif
    ExternalExit();

#ifdef CFG_NET_ENABLE
    if (ret != QUIT_UPGRADE_WEB)
        WebServerExit();

    xmlCleanupParser();
#endif // CFG_NET_ENABLE

    ConfigExit();
    SDL_Quit();

#ifdef CFG_CHECK_FILES_CRC_ON_BOOTING
    BackupDestroy();
#endif

#ifdef _WIN32
    _CrtDumpMemoryLeaks();
#else
    if (0)
    {
    #if (configUSE_TRACE_FACILITY == 1)
        static signed char buf[2048];
        vTaskList(buf);
        puts(buf);
    #endif
        malloc_stats();

    #ifdef CFG_DBG_RMALLOC
        Rmalloc_stat(__FILE__);
    #endif
    }
#endif // _WIN32

end:
    ret = UpgradeProcess(ret);
    itp_codec_standby();
    exit(ret);
    return ret;
}
