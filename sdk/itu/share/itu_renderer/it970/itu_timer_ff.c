#include <assert.h>
#include <pthread.h>
#include <time.h>

#include "ite/itv.h"
#include "ite/itu.h"

#define MAX_UI_BUF_CNT     3
#define timer_ff_x         100
#define timer_ff_y         150
#define timer_ff_w         512
#define timer_ff_wp        timer_ff_w * 4
#define timer_ff_h         128
#define timer_ff_fs        64

static uint32_t        g_ui_buff_addr[MAX_UI_BUF_CNT];
static ITUSurface      *gITUSurface[MAX_UI_BUF_CNT];
static char            date_0[20];
static char            date_1[20];
static bool            g_timer_ff_init;
static pthread_mutex_t TIMER_FF_MUTEX = PTHREAD_MUTEX_INITIALIZER;

static void _get_rtc_time(char *date_0, char *date_1)
{
    time_t current_time = 0;

    struct tm s_time = {0};

    time(&current_time);

    if (gmtime_r(&current_time, &s_time))
    {
        sprintf(date_0, "%04d% / %02d / %02d", (1900 + s_time.tm_year), (1 + s_time.tm_mon), s_time.tm_mday);
        sprintf(date_1, "%02d% : %02d : %02d", s_time.tm_hour, s_time.tm_min, s_time.tm_sec);
    }
}

void timer_ff_drawing(void)
{
    int             i          = 0;
    ITV_UI_PROPERTY ui_prop    = {0};
    uint32_t        ui_buf_ptr = 0;
    ITUColor        color_r    = {0, 255, 0, 0};

    pthread_mutex_lock(&TIMER_FF_MUTEX);

    if (!g_timer_ff_init) goto end;

    ui_buf_ptr = (uint32_t)itv_get_osdbuf_anchor(0);

    if (!ui_buf_ptr) goto end;

    for (i = 0; i < MAX_UI_BUF_CNT; i++)
        if (gITUSurface[i]->addr == ui_buf_ptr)
            break;

    if (i == MAX_UI_BUF_CNT)
    {
        printf("[TimerFF] gITUSurface addr fail ...\n");
        goto end;
    }

    ituColorFill(gITUSurface[i], 0, 0, gITUSurface[i]->width, gITUSurface[i]->height, &color_r);
    ituFtSetFontStyle(0);
    ituFtSetFontSize(timer_ff_fs, timer_ff_fs);
    _get_rtc_time(date_0, date_1);
    ituFtDrawText(gITUSurface[i], 0, 0, date_0);
    ituFtDrawText(gITUSurface[i], 0, timer_ff_fs, date_1);
    
    //gfxwaitEngineIdle();

    ui_prop.startX       = 0;
    ui_prop.startY       = 0;
    ui_prop.width        = timer_ff_w;
    ui_prop.height       = timer_ff_h;
    ui_prop.pitch        = timer_ff_wp;
    ui_prop.colorKeyR    = 0xFF;
    ui_prop.colorKeyG    = 0xFF;
    ui_prop.colorKeyB    = 0xFF;
    ui_prop.EnAlphaBlend = true;
    ui_prop.constAlpha   = 0xFF;
    itv_update_osdbuf_anchor(0, &ui_prop);

end:
    pthread_mutex_unlock(&TIMER_FF_MUTEX);
}

void timer_ff_init(void)
{
    int i = 0;

    pthread_mutex_lock(&TIMER_FF_MUTEX);

    g_ui_buff_addr[0] = itpVmemAlloc(timer_ff_wp * timer_ff_h * MAX_UI_BUF_CNT);

    assert(g_ui_buff_addr[0]);

    itv_osd_enable(0, true);
    itv_osd_setup_base(0, 0, (uint8_t *)g_ui_buff_addr[0]);
    for (i = 1; i < MAX_UI_BUF_CNT; i++)
    {
        g_ui_buff_addr[i] = g_ui_buff_addr[i-1] + timer_ff_wp * timer_ff_h;
        itv_osd_setup_base(0, i, (uint8_t *)g_ui_buff_addr[i]);
    }

    for (i = 0; i < MAX_UI_BUF_CNT; i++)
        gITUSurface[i] = ituCreateSurface(timer_ff_w, timer_ff_h, timer_ff_wp, ITU_ARGB8888, g_ui_buff_addr[i], ITU_STATIC);

    g_timer_ff_init = true;

    pthread_mutex_unlock(&TIMER_FF_MUTEX);
}

void timer_ff_deinit(void)
{
    int i = 0;

    pthread_mutex_lock(&TIMER_FF_MUTEX);

    itv_flush_osdbuf(0);
    itv_osd_enable(0, false);

    for (i = 0; i < MAX_UI_BUF_CNT; i++)
        ituDestroySurface(gITUSurface[i]);

    if (g_ui_buff_addr[0])
    {
        itpVmemFree(g_ui_buff_addr[0]);
        g_ui_buff_addr[0] = 0;
    }

    g_timer_ff_init = false;

    pthread_mutex_unlock(&TIMER_FF_MUTEX);
}

