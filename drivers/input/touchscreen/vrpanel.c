/*
    Copyright 2007 Smart Wireless Corporatoin, All rights reserved.

    Filename: virtualpanel.c

    Description:

    ------------------------------------------------------------------------------
    Revision History:
    Date            Author Name         Change ID           Brief Description
    2007-04-03      wangyu              xxxxxxxxx           Draft              
    ------------------------------------------------------------------------------
*/

//------------------------------------------------------------------------------
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/io.h>
#include <linux/vrpanel.h>

//------------------------------------------------------------------------------
//  Local Definitions
//#define VRP_DEBUG 

#ifdef VRP_DEBUG
#define VRPTRACE(a, b)                if (a){printk b;}
#else
#define VRPTRACE(a, b)
#endif

#define ESCAPECODEBASE          100000
#define VRP_CHECK_REG           (ESCAPECODEBASE + 30)
#define VRP_CHECK_INFO          (ESCAPECODEBASE + 31)
#define VRP_CHECK_ENABLE        (ESCAPECODEBASE + 32)

#define VRP_MSG                 1//(s_stVrpRes.stCfg.mEnable)

#define PIX2POS(p)              (p)

#define VRP_INVALID_REGION      (-1)
#define VRP_REG_SUBKEY          (L"HARDWARE\\DEVICEMAP\\TOUCH\\VRP")
#define VRP_REG_GDI_SUBKEY      (L"System\\GDI\\ROTATION")

#define VRP_TREND_TOUCH         1
#define VRP_TREND_SLIDE         2
#define VRP_TREND_UP            1
#define VRP_TREND_DOWN          2
#define VRP_TREND_LEFT          1
#define VRP_TREND_RIGHT         2

#define VRP_TOUCH_HDELTA_DEF    PIX2POS(20)
#define VRP_TOUCH_VDELTA_DEF    PIX2POS(10)
#define VRP_SLIDE_HDELTA_DEF    PIX2POS(30)
#define VRP_SLIDE_VDELTA_DEF    PIX2POS(15)
#define VRP_SLIDE_TIMEOUT_DEF   350
#define VRP_TOUCH_TIMEOUT_DEF   50

#define VRP_DTOUCH_TIMEOUT_DEF  50
#define VRP_FTOUCH_TIMEOUT_DEF  150
#define VRP_STOUCH_TIMEOUT_DEF  1000

#define VRP_SLPF_TIMEOUT_DEF    500
#define VRP_SLPS_TIMEOUT_DEF    100
#define VRP_VIB_TIMEOUT_DEF     75

#define VRP_RAGION_GROUP_DEF    0

#define VRP_TOUCH_HDELTA        (s_stVrpRes.stCfg.thDelta)
#define VRP_TOUCH_VDELTA        (s_stVrpRes.stCfg.tvDelta)
#define VRP_SLIDE_HDELTA        (s_stVrpRes.stCfg.shDelta)
#define VRP_SLIDE_VDELTA        (s_stVrpRes.stCfg.svDelta)
#define VRP_SLIDE_TIMEOUT       (s_stVrpRes.stCfg.sTimeout)
#define VRP_TOUCH_TIMEOUT       (s_stVrpRes.stCfg.tTimeout)

#define VRP_DTOUCH_TIMEOUT      (s_stVrpRes.stCfg.dtTimeout)
#define VRP_FTOUCH_TIMEOUT      (s_stVrpRes.stCfg.ftTimeout)
#define VRP_STOUCH_TIMEOUT      (s_stVrpRes.stCfg.stTimeout)

#define VRP_SLPF_TIMEOUT         (s_stVrpRes.stCfg.slpfTimeout)
#define VRP_SLPS_TIMEOUT         (s_stVrpRes.stCfg.slpsTimeout)
#define VRP_VIB_TIMEOUT         (s_stVrpRes.stCfg.vTimeout)

#define VRP_DTOUCH_ENABLE       (s_stVrpRes.stCfg.dtEnable)
#define VRP_TOUCH_LONGPRESS     (s_stVrpRes.stCfg.tlpEnable)
#define VRP_SLIDE_LONGPRESS     (s_stVrpRes.stCfg.slpEnable)
#define VRP_DUALDIR_SLIDE       (s_stVrpRes.stCfg.ddsEnable)
#define VRP_ROTATION_ANGLE      (s_stVrpRes.stCfg.rAngle)

#define VRP_EXIST_FOCUS_REGION  (VRP_INVALID_REGION!=s_stVrpRes.FocusRegion)
#define VRP_IS_FOCUS_REGION(r)  (r==s_stVrpRes.FocusRegion)
#define VRP_PEN_DOWN(f)         (f&TouchSampleDownFlag)

#define VRP_BINGO_REGION(p,r)   ((p->x>=r.stRect.left)   && \
                                 (p->y>=r.stRect.top)    && \
                                 (p->x<=r.stRect.right)  && \
                                 (p->y<=r.stRect.bottom))
                                 
#define VRP_TREND_TGET(v)       ((v>>0)&0xFF)
#define VRP_TREND_HGET(v)       ((v>>8)&0xFF)
#define VRP_TREND_VGET(v)       ((v>>16)&0xFF)
#define VRP_TREND_TSET(v,t)     {v |= (t&0xFF)<<0;}
#define VRP_TREND_HSET(v,t)     {v |= (t&0xFF)<<8;}
#define VRP_TREND_VSET(v,t)     {v |= (t&0xFF)<<16;}
#define ABS(a)                  ((a) > 0 ? (a): -(a))

typedef enum tagVRP_KEY {
    VRP_KEY_UP,
    VRP_KEY_DOWN,
    VRP_KEY_LEFT,
    VRP_KEY_RIGHT,
    VRP_KEY_ACTION,
    VRP_KEY_LAST,
    VRP_KEY_DIR,
} VRP_KEY ;

typedef enum tagVRP_REG {
    VRP_REG_RANGLE,
    VRP_REG_FIRST,
    VRP_REG_THDELTA = VRP_REG_FIRST,
    VRP_REG_TVDELTA,
    VRP_REG_SHDELTA,
    VRP_REG_SVDELTA,
    VRP_REG_STIMEOUT,
    VRP_REG_TTIMEOUT,
    VRP_REG_DTTIMEOUT,
    VRP_REG_FTTIMEOUT,
    VRP_REG_STTIMEOUT,
    VRP_REG_SLPFTIMEOUT,
    VRP_REG_SLPSTIMEOUT,
    VRP_REG_VTIMEOUT,
    VRP_REG_DTENABLE,
    VRP_REG_MENABLE,
    VRP_REG_TLPENABLE,
    VRP_REG_SLPENABLE,
    VRP_REG_DDSENABLE,
    VRP_REG_RGROUP,
    VRP_REG_LAST,   // LAST VRP REG
}VRP_REG ;

typedef struct {
    DWORD TickCount;
    BOOL bExsitTouch;
} VRP_DOUBLE_TOUCH;

typedef struct {
    DWORD TickCount;
    BOOL bLongPress;
    DWORD KeyPress;
    BOOL bRepeat;
} VRP_DIR_LONGPRESS;

typedef struct {
    UINT32              KeyPress;
    UINT32              Trend;
    DWORD               TickCount;
    INT32               PrvX;
    INT32               PrvY;
    VRP_DIR_LONGPRESS   DirLongPress;
} VRP_SLIDE_PANEL;

typedef struct {
    const int *pMap;
    UINT32 Num;
} VRP_KEY_MAP;

typedef void (*PFN_VRP_HOT_HANDLE)(DWORD dwFlag, DWORD dwParam1, DWORD dwParam2, const VRP_KEY_MAP *pKeyMap);

typedef struct {
    RECT stRect;
    DWORD dwParam1;
    DWORD dwParam2;
    PFN_VRP_HOT_HANDLE Handle;
    BOOL bActived;
    BOOL bPos;
    BOOL bFocus;
    const VRP_KEY_MAP *pKeyMap;
} VRP_HOT_REGION;

typedef struct {
    VRP_HOT_REGION *pRegion;
    UINT32          Num;
} VRP_REGION_GROUP;

typedef struct {
    INT32 thDelta;
    INT32 tvDelta;
    INT32 shDelta;
    INT32 svDelta;
    INT32 sTimeout;
    INT32 tTimeout;
    INT32 dtTimeout;
    INT32 ftTimeout;
    INT32 stTimeout;
    INT32 slpfTimeout;
    INT32 slpsTimeout;
    INT32 vTimeout;
    BOOL  dtEnable;
    BOOL  mEnable;
    BOOL  tlpEnable;
    BOOL  slpEnable;
    BOOL  ddsEnable;
    DWORD rAngle;
    VRP_REGION_GROUP *rGroup;
} VRP_CFG;

typedef struct {
    BOOL                    bInited;
    PFN_TOUCH_PANEL_CALLBACK pfnCallback;
    struct hrtimer       VibOffTimer;
    INT32                   FocusRegion;
    VRP_DOUBLE_TOUCH        stDoubleTouch;
    VRP_CFG                 stCfg;
    void                    *pReg;
    void                    *hRegNotify[VRP_REG_LAST];
    BOOL                    bUpdateCfg;
    VRP_CHECK_MODE          stCheckMode;
    struct input_dev *input;
} VRP_RES;

void HotHandle(DWORD dwFlag, DWORD dwParam1, DWORD dwParam2, const VRP_KEY_MAP *pKeyMap);
void SlideHandle(DWORD dwFlag, DWORD dwParam1, DWORD dwParam2, const VRP_KEY_MAP *pKeyMap);
void TouchHandle(DWORD dwFlag, DWORD dwParam1, DWORD dwParam2, const VRP_KEY_MAP *pKeyMap);

const char c_szReg_thDelta[]   = ("thDelta");
const char c_szReg_tvDelta[]   = ("tvDelta");
const char c_szReg_shDelta[]   = ("shDelta");
const char c_szReg_svDelta[]   = ("svDelta");
const char c_szReg_sTimeout[]  = ("sTimeout");
const char c_szReg_tTimeout[]  = ("tTimeout");
const char c_szReg_dtTimeout[] = ("dtTimeout");
const char c_szReg_ftTimeout[] = ("ftTimeout");
const char c_szReg_stTimeout[] = ("stTimeout");
const char c_szReg_slpfTimeout[]= ("slpfTimeout");
const char c_szReg_slpsTimeout[]= ("slpsTimeout");
const char c_szReg_vTimeout[]  = ("vTimeout");
const char c_szReg_dtEnable[]  = ("dtEnable");
const char c_szReg_mEnable[]   = ("mEnable");
const char c_szReg_tlpEnable[] = ("tlpEnable");
const char c_szReg_slpEnable[] = ("slpEnable");
const char c_szReg_ddsEnable[] = ("ddsEnable");
const char c_szReg_rAngle[]    = ("Angle");
const char c_szReg_rGroup[]    = ("rGroup");

const char *c_szRegValue[] = {
    c_szReg_rAngle,
    c_szReg_thDelta,
    c_szReg_tvDelta,
    c_szReg_shDelta,
    c_szReg_svDelta,  
    c_szReg_sTimeout,
    c_szReg_tTimeout, 
    c_szReg_dtTimeout,
    c_szReg_ftTimeout,
    c_szReg_stTimeout,
    c_szReg_slpfTimeout,  
    c_szReg_slpsTimeout,  
    c_szReg_vTimeout,
    c_szReg_dtEnable,
    c_szReg_mEnable,
    c_szReg_tlpEnable,
    c_szReg_slpEnable,
    c_szReg_ddsEnable,
    c_szReg_rGroup,
};

static VRP_RES s_stVrpRes;

const int c_KeyMap1[] = {
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ENTER,
};

const int c_KeyMap2[] = {
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_OK,
};

const VRP_KEY_MAP c_KeyMaps[] = {
    {c_KeyMap1, sizeof(c_KeyMap1)/sizeof(int)},
    {c_KeyMap2, sizeof(c_KeyMap2)/sizeof(int)},
};

static VRP_HOT_REGION s_KeyHotRegion1[] = {
    {{PIX2POS(0),  PIX2POS(0),  PIX2POS(320),PIX2POS(480)}, 0,       0,   TouchHandle, FALSE, TRUE,  FALSE, NULL},
    {{PIX2POS(0*4/3),  PIX2POS(410*6/5),PIX2POS(50*4/3), PIX2POS(545)}, KEY_HOME,   1,      HotHandle,   FALSE, FALSE, FALSE, NULL},
    {{PIX2POS(64*4/3), PIX2POS(410*6/5),PIX2POS(114*4/3),PIX2POS(545)}, KEY_MENU,   2,      HotHandle,   FALSE, FALSE, FALSE, NULL},
    {{PIX2POS(128*4/3),PIX2POS(410*6/5),PIX2POS(178*4/3),PIX2POS(545)}, KEY_BACK,      3,      HotHandle,   FALSE, FALSE, FALSE, NULL},
    {{PIX2POS(192*4/3),PIX2POS(410*6/5),PIX2POS(240*4/3),PIX2POS(545)}, KEY_SEARCH,    4,      HotHandle,   FALSE, FALSE, FALSE, NULL},
};

static VRP_HOT_REGION s_KeyHotRegion2[] = {
    {{PIX2POS(0),  PIX2POS(0),  PIX2POS(320),PIX2POS(480)}, 0,       0,   TouchHandle, FALSE, TRUE,  FALSE, NULL},
    {{PIX2POS(0*4/3),  PIX2POS(410*6/5),PIX2POS(70*4/3), PIX2POS(565)}, KEY_HOME,   1,      HotHandle,   FALSE, FALSE, FALSE, NULL},
    {{PIX2POS(84*4/3), PIX2POS(410*6/5),PIX2POS(156*4/3),PIX2POS(565)}, KEY_MENU,   2,      HotHandle,   FALSE, FALSE, FALSE, NULL},
    {{PIX2POS(170*4/3),PIX2POS(410*6/5),PIX2POS(240*4/3),PIX2POS(565)}, KEY_BACK,      3,      HotHandle,   FALSE, FALSE, FALSE, NULL},
};

static VRP_HOT_REGION s_KeyHotRegion3[] = {
    {{PIX2POS(0),  PIX2POS(0),  PIX2POS(480),PIX2POS(800)}, 0,       0,   TouchHandle, FALSE, TRUE,  FALSE, NULL},
    {{PIX2POS(0),  PIX2POS(810),PIX2POS(120), PIX2POS(900)}, KEY_HOME,   1,      HotHandle,   FALSE, FALSE, FALSE, NULL},
    {{PIX2POS(130), PIX2POS(810),PIX2POS(240),PIX2POS(900)}, KEY_MENU,   2,      HotHandle,   FALSE, FALSE, FALSE, NULL},
    {{PIX2POS(250),PIX2POS(810),PIX2POS(360),PIX2POS(900)}, KEY_BACK,      3,      HotHandle,   FALSE, FALSE, FALSE, NULL},
    {{PIX2POS(370),PIX2POS(810),PIX2POS(480),PIX2POS(900)}, KEY_SEARCH,    4,      HotHandle,   FALSE, FALSE, FALSE, NULL},
};

static VRP_HOT_REGION s_KeyHotRegion4[] = {
    {{PIX2POS(0),  PIX2POS(0),  PIX2POS(480),PIX2POS(800)}, 0,       0,   TouchHandle, FALSE, TRUE,  TRUE, NULL},
    {{PIX2POS(0),  PIX2POS(810),PIX2POS(100), PIX2POS(900)}, KEY_BACK,   1,      HotHandle,   FALSE, FALSE, FALSE, NULL},
    {{PIX2POS(110), PIX2POS(810),PIX2POS(240),PIX2POS(900)}, KEY_MENU,   2,      HotHandle,   FALSE, FALSE, FALSE, NULL},
    {{PIX2POS(250),PIX2POS(810),PIX2POS(380),PIX2POS(900)}, KEY_HOME,      3,      HotHandle,   FALSE, FALSE, FALSE, NULL},
    {{PIX2POS(390),PIX2POS(810),PIX2POS(480),PIX2POS(900)}, KEY_SEARCH,    4,      HotHandle,   FALSE, FALSE, FALSE, NULL},
};



static VRP_REGION_GROUP s_RegionGroup[] = {
    {s_KeyHotRegion1, sizeof(s_KeyHotRegion1)/sizeof(VRP_HOT_REGION)},
    {s_KeyHotRegion2, sizeof(s_KeyHotRegion2)/sizeof(VRP_HOT_REGION)},
    {s_KeyHotRegion3, sizeof(s_KeyHotRegion3)/sizeof(VRP_HOT_REGION)},
    {s_KeyHotRegion4, sizeof(s_KeyHotRegion4)/sizeof(VRP_HOT_REGION)},
};

static enum hrtimer_restart CallOffVib(struct hrtimer *timer)
{    
    VRPTRACE(VRP_MSG, ("[VRP]Vib On\n"));
    return HRTIMER_NORESTART;
}

static void CallOnVib(void)
{    
    VRPTRACE(VRP_MSG, ("[VRP]Vib Off\n"));
}

void SetVibOffTimer(void)
{
    hrtimer_start(&s_stVrpRes.VibOffTimer, ktime_set(0, VRP_VIB_TIMEOUT), HRTIMER_MODE_REL);
}

void KillVibOffTimer(void)
{
    hrtimer_cancel(&s_stVrpRes.VibOffTimer);
}

void VibEvent(void) {
    KillVibOffTimer();
    CallOffVib(&s_stVrpRes.VibOffTimer);
    CallOnVib();
    SetVibOffTimer();
}

void KeyEvent (
    int Vk, 
    int Scan, 
    DWORD dwKeyFlags,
    BOOL bVib)
{
    if (bVib) {
        //VibEvent();
    }
    input_report_key(s_stVrpRes.input, Vk, !dwKeyFlags);
    input_sync(s_stVrpRes.input);
}

void CalcTrend(INT32 DeltaX, INT32 DeltaY, VRP_SLIDE_PANEL *pstSlide) {
    UINT32 Trend;
    
    VRPTRACE(VRP_MSG, ("[VRP]DeltaX:%d, DeltaY:%d\n", DeltaX, DeltaY));
    
    Trend = VRP_TREND_TGET(pstSlide->Trend);        
    pstSlide->Trend = 0;

    if (VRP_DTOUCH_ENABLE) {
        if (ABS(DeltaX) < VRP_SLIDE_HDELTA &&
            ABS(DeltaY) < VRP_SLIDE_VDELTA) {
            VRP_TREND_TSET(pstSlide->Trend, VRP_TREND_TOUCH);
        }
    }
    else {
        if (VRP_TREND_SLIDE == Trend){
            VRP_TREND_TSET(pstSlide->Trend, VRP_TREND_SLIDE);
        }
        else if (ABS(DeltaX) <= VRP_TOUCH_HDELTA &&
                 ABS(DeltaY) <= VRP_TOUCH_VDELTA) {
            VRP_TREND_TSET(pstSlide->Trend, VRP_TREND_TOUCH);
        }
        else {
            VRP_TREND_TSET(pstSlide->Trend, VRP_TREND_SLIDE);
        }
    }

    if (ABS(DeltaX) >= VRP_SLIDE_HDELTA) {
        if (DeltaX < 0) {
            VRP_TREND_HSET(pstSlide->Trend, VRP_TREND_LEFT);
        }
        else {
            VRP_TREND_HSET(pstSlide->Trend, VRP_TREND_RIGHT);
        }
    }

    if (ABS(DeltaY) >= VRP_SLIDE_VDELTA) {
        if (DeltaY < 0) {
            VRP_TREND_VSET(pstSlide->Trend, VRP_TREND_UP);
        }
        else {
            VRP_TREND_VSET(pstSlide->Trend, VRP_TREND_DOWN);
        }
    }
    
    if (!VRP_DUALDIR_SLIDE &&
        VRP_TREND_HGET(pstSlide->Trend) &&
        VRP_TREND_VGET(pstSlide->Trend)) 
    {
        if (ABS(DeltaX) > ABS(DeltaY)) {
            pstSlide->Trend &= ~(0xFF<<16); //remove vertical trend
        }
        else {
            pstSlide->Trend &= ~(0xFF<<8); // remove horizontal trend
        }
    }
}

void GetDoubleTouchKey(DWORD dwFlag, UINT32 *pKeyPress, VRP_SLIDE_PANEL *pstSlide) {
    INT32 Count = 0;
    UINT32 Trend;
    BOOL bFirstTouch = FALSE;
    
    Trend = VRP_TREND_TGET(pstSlide->Trend);            
    if (!VRP_PEN_DOWN(dwFlag)) {
        VRPTRACE(VRP_MSG, ("[VRP]+bExsitTouch:%d\n", s_stVrpRes.stDoubleTouch.bExsitTouch));
        if ((VRP_TREND_TOUCH == Trend)  &&
            (0 == *pKeyPress)) 
        {
            if (!s_stVrpRes.stDoubleTouch.bExsitTouch) {
                Count = jiffies_to_msecs(jiffies) - pstSlide->TickCount;
                VRPTRACE(VRP_MSG, ("[VRP]SlideTickCount:%d\n", Count));
                if (ABS(Count) < VRP_FTOUCH_TIMEOUT &&
                    ABS(Count) > VRP_DTOUCH_TIMEOUT) {
                    s_stVrpRes.stDoubleTouch.bExsitTouch = TRUE;
                    s_stVrpRes.stDoubleTouch.TickCount = jiffies_to_msecs(jiffies);
                    bFirstTouch = TRUE;
                }
            }
        }

        if (!bFirstTouch) {
            memset(&s_stVrpRes.stDoubleTouch, 0, sizeof(VRP_DOUBLE_TOUCH));
        }
        VRPTRACE(VRP_MSG, ("[VRP]-bExsitTouch:%d\n", s_stVrpRes.stDoubleTouch.bExsitTouch));
    }
    else {
        if ((VRP_TREND_TOUCH == Trend)  &&
            (0 == *pKeyPress)) 
        {
            if (s_stVrpRes.stDoubleTouch.bExsitTouch) {
                Count = jiffies_to_msecs(jiffies) - s_stVrpRes.stDoubleTouch.TickCount;
                VRPTRACE(VRP_MSG, ("[VRP]DoubleTickCount:%d\n", Count));
                if (ABS(Count) < VRP_STOUCH_TIMEOUT) {
                    *pKeyPress |= 1 << VRP_KEY_ACTION;
                }
                else {
                    memset(&s_stVrpRes.stDoubleTouch, 0, sizeof(VRP_DOUBLE_TOUCH));                    
                }
            }
        }
    }
}

void GetActKey(DWORD dwFlag, UINT32 *pKeyPress, VRP_SLIDE_PANEL *pstSlide) {
    INT32 Count;
    UINT32 Trend, PrvKey;

    if (VRP_DTOUCH_ENABLE) {
        GetDoubleTouchKey(dwFlag, pKeyPress, pstSlide);
    }
    else {
        if (VRP_PEN_DOWN(dwFlag)) {
            if (VRP_TOUCH_LONGPRESS) {
                Trend = VRP_TREND_TGET(pstSlide->Trend);        
                PrvKey = (~(1 << VRP_KEY_ACTION)) & pstSlide->KeyPress; // ����ACTION ������м�
                if ((VRP_TREND_TOUCH == Trend)  &&
                    (0 == PrvKey)               &&
                    (0 == *pKeyPress)) 
                {
                    Count = jiffies_to_msecs(jiffies) - pstSlide->TickCount;
                    if (ABS(Count) > VRP_SLIDE_TIMEOUT)
                        *pKeyPress |= 1 << VRP_KEY_ACTION;
                }
            }
        }
        else {
            Trend = VRP_TREND_TGET(pstSlide->Trend);        
            if (!pstSlide->KeyPress && 
                (VRP_TREND_TOUCH == Trend))
            {
                Count = jiffies_to_msecs(jiffies) - pstSlide->TickCount;
                if (ABS(Count) > VRP_TOUCH_TIMEOUT)
                    *pKeyPress |= 1 << VRP_KEY_ACTION;
            }
        }
    }
}

VRP_KEY RotationDirKey (VRP_KEY Key) {
    const VRP_KEY c_DirRotation90Map[4] = {
        VRP_KEY_RIGHT,
        VRP_KEY_LEFT,
        VRP_KEY_UP,
        VRP_KEY_DOWN,
    };
    
    const VRP_KEY c_DirRotation270Map[4] = {
        VRP_KEY_LEFT,
        VRP_KEY_RIGHT,
        VRP_KEY_DOWN,
        VRP_KEY_UP,
    };
    
    switch (VRP_ROTATION_ANGLE) {
        case 90:
            Key = c_DirRotation90Map[Key];
            break;
        case 270: 
            Key = c_DirRotation270Map[Key];
            break;
    }

    return Key;
}

void GetDirKey(DWORD dwFlag, UINT32 *pKeyPress, VRP_SLIDE_PANEL *pstSlide) {
    UINT32 Trend;
    
    Trend = VRP_TREND_HGET(pstSlide->Trend);
    if (VRP_TREND_LEFT == Trend) {
        *pKeyPress |= 1 << RotationDirKey(VRP_KEY_LEFT);
    }
    if (VRP_TREND_RIGHT == Trend) {
        *pKeyPress |= 1 << RotationDirKey(VRP_KEY_RIGHT);        
    }
    Trend = VRP_TREND_VGET(pstSlide->Trend);
    if (VRP_TREND_UP == Trend) {
        *pKeyPress |= 1 << RotationDirKey(VRP_KEY_UP);        
    }
    if (VRP_TREND_DOWN == Trend) {
        *pKeyPress |= 1 << RotationDirKey(VRP_KEY_DOWN);        
    }
    if (*pKeyPress)
        *pKeyPress |= 1 << VRP_KEY_DIR;
}

void GetDirLongPress(DWORD dwFlag, UINT32 *pKeyPress, VRP_SLIDE_PANEL *pstSlide) {
    INT32 Count = 0;

    if (VRP_PEN_DOWN(dwFlag)) {
        if (pstSlide->DirLongPress.bLongPress) {
            Count = jiffies_to_msecs(jiffies) - pstSlide->DirLongPress.TickCount;
            VRPTRACE(VRP_MSG, ("[VRP]DirLongPressTickCount:%d\n", Count));
            if (pstSlide->DirLongPress.bRepeat) {
                if (ABS(Count) > VRP_SLPS_TIMEOUT) {
                    *pKeyPress = pstSlide->DirLongPress.KeyPress;
                    pstSlide->DirLongPress.TickCount = jiffies_to_msecs(jiffies);
                }
            }
            else if (ABS(Count) > VRP_SLPF_TIMEOUT) {
                *pKeyPress = pstSlide->DirLongPress.KeyPress;
                pstSlide->DirLongPress.bRepeat = TRUE;
                pstSlide->DirLongPress.TickCount = jiffies_to_msecs(jiffies);
            }
        }
        else {
            GetDirKey(dwFlag, pKeyPress, pstSlide);
            if (*pKeyPress) {
                pstSlide->DirLongPress.TickCount = jiffies_to_msecs(jiffies);
                pstSlide->DirLongPress.bLongPress = TRUE;
                pstSlide->DirLongPress.KeyPress = *pKeyPress;
            }
        }
    }
    
}
    
void GetKeyPress(DWORD dwFlag, UINT32 *pKeyPress, VRP_SLIDE_PANEL *pstSlide) {

    *pKeyPress = 0;
    if ((1 << VRP_KEY_DIR) & pstSlide->KeyPress) {
        *pKeyPress |= 1 << VRP_KEY_DIR;
    }
    if (VRP_PEN_DOWN(dwFlag)) {
        if (!VRP_SLIDE_LONGPRESS) {
            GetDirKey(dwFlag, pKeyPress, pstSlide);
        }
    }
    if (VRP_SLIDE_LONGPRESS) {
        GetDirLongPress(dwFlag, pKeyPress, pstSlide);
    }
    GetActKey(dwFlag, pKeyPress, pstSlide);
}

void SetKeyEvent(DWORD dwFlag, UINT32 *pKeyPress, VRP_SLIDE_PANEL *pstSlide, const VRP_KEY_MAP *pKeyMap) {
    UINT32 i;
    int vk;
    UINT32 PrvKey;
    UINT32 CurKey;
    
    for (i = 0; i < pKeyMap->Num; i++) {
        PrvKey = (pstSlide->KeyPress & (1 << i));
        CurKey = (*pKeyPress & (1 << i));
        vk = pKeyMap->pMap[i];
        if (CurKey &&
            PrvKey &&
            VRP_KEY_ACTION == i) {
            continue;
        }
        if (CurKey || CurKey != PrvKey) {
            if (KEY_ENTER == vk) {
                VRPTRACE(VRP_MSG, ("[VRP]Bingo! VK:0x%04X, KEY:0x%04X\n", KEY_OK, CurKey));
                CurKey ? KeyEvent(KEY_OK, 0, 0, FALSE) : KeyEvent(KEY_OK, 0, KEYEVENTF_KEYUP, FALSE);
            }
            VRPTRACE(VRP_MSG, ("[VRP]Bingo! VK:0x%04X, KEY:0x%04X\n", vk, CurKey));
            CurKey ? KeyEvent(vk, 0, 0, FALSE) : KeyEvent(vk, 0, KEYEVENTF_KEYUP, FALSE);
        }
        if ((CurKey)    &&
            (!PrvKey)   &&
            !VRP_PEN_DOWN(dwFlag) &&
            (VRP_KEY_ACTION == i)) {
            VRPTRACE(VRP_MSG, ("[VRP]Bingo! VK:0x%04X, KEY:0x%04X\n", vk, 0));
            KeyEvent(vk, 0, KEYEVENTF_KEYUP, FALSE);
            if (KEY_ENTER == vk) {
                VRPTRACE(VRP_MSG, ("[VRP]Bingo! VK:0x%04X, KEY:0x%04X\n", KEY_OK, 0));
                KeyEvent(KEY_OK, 0, KEYEVENTF_KEYUP, FALSE);
            }
            *pKeyPress &= ~(1 << VRP_KEY_ACTION);
        }
    }
    pstSlide->KeyPress = *pKeyPress;
}
    
void HotHandle(DWORD dwFlag, DWORD dwParam1, DWORD dwParam2, const VRP_KEY_MAP *pKeyMap)
{
    static DWORD dwCount;
    static DWORD dwVk;
    DWORD dwKeyFlag = KEYEVENTF_KEYUP;

    if (VRP_PEN_DOWN(dwFlag)) {
        dwKeyFlag = 0;
    }
    
    switch (dwParam1) {
    case 0x3A: // FOR CIT
    case 0x3B: // FOR CIT
    case KEY_HOME:
    case KEY_MENU:
    case KEY_BACK:
    case KEY_SEARCH:
        VRPTRACE(VRP_MSG, ("[VRP]Bingo:0x%04X\n", dwParam1));
        if (dwVk != dwParam1) {
            dwCount++;
            dwVk = dwParam1;
            KeyEvent(dwParam1, dwParam2, dwKeyFlag, TRUE);
        }
        else if (dwKeyFlag == KEYEVENTF_KEYUP) {
            dwCount = 0;
            dwVk = 0;
            KeyEvent(dwParam1, dwParam2, dwKeyFlag, FALSE);
        }
        else if (dwCount > 50){
            dwCount = 0;
            dwVk = dwParam1;
            KeyEvent(dwParam1, dwParam2, dwKeyFlag, FALSE);
        }
        else {
            dwCount++;
            dwVk = dwParam1;
        }
        break;
    }
}

void SlideHandle(DWORD dwFlag, DWORD dwParam1, DWORD dwParam2, const VRP_KEY_MAP *pKeyMap)
{
    static VRP_SLIDE_PANEL stSlide;
    static BOOL bVib;
    INT32 DeltaX = 0, DeltaY = 0;
    INT32 CurX, CurY;
    UINT32 KeyPress;
    INT32 Count;
    
    CurX = (INT32)dwParam1;
    CurY = (INT32)dwParam2; 

    if (0 == stSlide.TickCount) {
        stSlide.TickCount = jiffies_to_msecs(jiffies);
    }
    
    Count = jiffies_to_msecs(jiffies) - stSlide.TickCount;
    if (ABS(Count) > VRP_TOUCH_TIMEOUT &&
        !bVib) {
        VibEvent();
        bVib = TRUE;
    }
    
    if (stSlide.PrvX) {      
        DeltaX = CurX - stSlide.PrvX;
    } else {
        stSlide.PrvX = CurX;
    }
        
    if (stSlide.PrvY) { 
        DeltaY = CurY - stSlide.PrvY;
    } else {
        stSlide.PrvY = CurY;
    }

    CalcTrend(DeltaX, DeltaY, &stSlide);
    GetKeyPress(dwFlag, &KeyPress, &stSlide);
    SetKeyEvent(dwFlag, &KeyPress, &stSlide, pKeyMap);

    if (VRP_PEN_DOWN(dwFlag)) {
        if (VRP_TREND_HGET(stSlide.Trend)) {
            stSlide.PrvX = CurX;
        }
        if (VRP_TREND_VGET(stSlide.Trend)) {
            stSlide.PrvY = CurY;
        }
    }
    else {
        VRPTRACE(VRP_MSG, ("[VRP]Release\n"));
        memset(&stSlide, 0, sizeof(VRP_SLIDE_PANEL));
        bVib = FALSE;
    }
}

void TouchHandle(DWORD dwFlag, DWORD dwParam1, DWORD dwParam2, const VRP_KEY_MAP *pKeyMap) {
    if (s_stVrpRes.pfnCallback) {
        s_stVrpRes.pfnCallback(dwFlag, dwParam1, dwParam2);
    }
}

void ReleaseRegion(POINT *pHotPoint, DWORD dwFlag, VRP_HOT_REGION *pstRegion, DWORD Region)
{
    pstRegion->bActived = FALSE;
    dwFlag = TouchSampleValidFlag | TouchSamplePreviousDownFlag;
    pstRegion->Handle(dwFlag, pstRegion->dwParam1, pstRegion->dwParam2, pstRegion->pKeyMap);
}

INT32 ActiveRegion(POINT *pHotPoint, DWORD dwFlag, VRP_HOT_REGION *pstRegion, DWORD Region)
{
    INT32 NextFocusRegion = s_stVrpRes.FocusRegion;

    do {
        if (VRP_PEN_DOWN(dwFlag)) {
            if (!VRP_IS_FOCUS_REGION(Region) && 
                VRP_EXIST_FOCUS_REGION) {
                break;
            }
            pstRegion->bActived = TRUE;
            if (pstRegion->bFocus) {
                NextFocusRegion = Region;
            }
        }
        else {
            if (VRP_EXIST_FOCUS_REGION) {
                if (VRP_IS_FOCUS_REGION(Region) && s_stVrpRes.stCfg.rGroup->pRegion[s_stVrpRes.FocusRegion].bPos) {
                    s_stVrpRes.stCfg.rGroup->pRegion[s_stVrpRes.FocusRegion].dwParam1 = pHotPoint->x;
                    s_stVrpRes.stCfg.rGroup->pRegion[s_stVrpRes.FocusRegion].dwParam2 = pHotPoint->y;
                }
                s_stVrpRes.stCfg.rGroup->pRegion[s_stVrpRes.FocusRegion].bActived = FALSE;
                s_stVrpRes.stCfg.rGroup->pRegion[s_stVrpRes.FocusRegion].Handle(
                    dwFlag, 
                    s_stVrpRes.stCfg.rGroup->pRegion[s_stVrpRes.FocusRegion].dwParam1, 
                    s_stVrpRes.stCfg.rGroup->pRegion[s_stVrpRes.FocusRegion].dwParam2,
                    s_stVrpRes.stCfg.rGroup->pRegion[s_stVrpRes.FocusRegion].pKeyMap);
                NextFocusRegion = VRP_INVALID_REGION;
                break;
            }
            pstRegion->bActived = FALSE;
        }
        if (pstRegion->bPos) {
            pstRegion->Handle(dwFlag, pHotPoint->x, pHotPoint->y, pstRegion->pKeyMap);
            pstRegion->dwParam1 = pHotPoint->x;
            pstRegion->dwParam2 = pHotPoint->y;
        }
        else {
            pstRegion->Handle(dwFlag, pstRegion->dwParam1, pstRegion->dwParam2, pstRegion->pKeyMap);
        }
    }
    while(0);
    
    return NextFocusRegion;
}

void LoadDefCfg(VRP_CFG *pstCfg) {
    
    pstCfg->thDelta = VRP_TOUCH_HDELTA_DEF;
    pstCfg->tvDelta = VRP_TOUCH_VDELTA_DEF;
    pstCfg->shDelta = VRP_SLIDE_HDELTA_DEF;
    pstCfg->svDelta = VRP_SLIDE_VDELTA_DEF;
    pstCfg->sTimeout= VRP_SLIDE_TIMEOUT_DEF;
    pstCfg->tTimeout= VRP_TOUCH_TIMEOUT_DEF;
    
    pstCfg->dtTimeout = VRP_DTOUCH_TIMEOUT_DEF;
    pstCfg->ftTimeout = VRP_FTOUCH_TIMEOUT_DEF;
    pstCfg->stTimeout = VRP_STOUCH_TIMEOUT_DEF;
    
    pstCfg->slpfTimeout = VRP_SLPF_TIMEOUT_DEF;
    pstCfg->slpsTimeout = VRP_SLPS_TIMEOUT_DEF;
    pstCfg->vTimeout = VRP_VIB_TIMEOUT_DEF;

    pstCfg->dtEnable = (0);
    pstCfg->mEnable = (0);
    pstCfg->tlpEnable = (0);
    pstCfg->slpEnable = (1);
    pstCfg->ddsEnable = (0);
    
#ifdef CONFIG_TOUCHSCREEN_VRPANEL_THREE_KEY
    pstCfg->rGroup = &s_RegionGroup[1];
#endif

#ifdef CONFIG_TOUCHSCREEN_VRPANEL_FOUR_KEY
    pstCfg->rGroup = &s_RegionGroup[0]; 
#endif

#ifdef CONFIG_TOUCHSCREEN_VRPANEL_WVGA_FOUR_KEY
    pstCfg->rGroup = &s_RegionGroup[2]; 
#endif

#ifdef CONFIG_TOUCHSCREEN_VRPANEL_SHARP_WVGA_FOUR_KEY
    pstCfg->rGroup = &s_RegionGroup[3]; 
#endif


}

void LoadRegCfg(VRP_CFG *pstCfg) {    
#if 0    
    BOOL bResult;
    UINT32 Group = 0;
    pstCfg->rGroup = &s_RegionGroup[0];
    while (s_stVrpRes.pReg) {
        bResult = s_stVrpRes.pReg->OpenOrCreateRegKey(HKEY_LOCAL_MACHINE, VRP_REG_SUBKEY);
        if (!bResult) {
            LoadDefCfg(stCfg);
            break;
        }
        Group = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_RGROUP], VRP_RAGION_GROUP_DEF);
        
        if (Group < (sizeof(s_RegionGroup)/sizeof(VRP_REGION_GROUP))) {
            stCfg.rGroup = &s_RegionGroup[Group];
        }
        
        stCfg.thDelta = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_THDELTA], VRP_TOUCH_HDELTA_DEF);
        stCfg.tvDelta = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_TVDELTA], VRP_TOUCH_VDELTA_DEF);
        stCfg.shDelta = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_SHDELTA], VRP_SLIDE_HDELTA_DEF);
        stCfg.svDelta = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_SVDELTA], VRP_SLIDE_VDELTA_DEF);
        stCfg.sTimeout= s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_STIMEOUT], VRP_SLIDE_TIMEOUT_DEF);
        stCfg.tTimeout= s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_TTIMEOUT], VRP_TOUCH_TIMEOUT_DEF);
        
        stCfg.dtTimeout = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_DTTIMEOUT], VRP_DTOUCH_TIMEOUT_DEF);
        stCfg.ftTimeout = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_FTTIMEOUT], VRP_FTOUCH_TIMEOUT_DEF);
        stCfg.stTimeout = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_STTIMEOUT], VRP_STOUCH_TIMEOUT_DEF);
        
        stCfg.slpfTimeout = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_SLPFTIMEOUT], VRP_SLPF_TIMEOUT_DEF);
        stCfg.slpsTimeout = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_SLPSTIMEOUT], VRP_SLPS_TIMEOUT_DEF);
        stCfg.vTimeout = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_VTIMEOUT], VRP_VIB_TIMEOUT_DEF);

        stCfg.dtEnable = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_DTENABLE], 0);
        stCfg.mEnable = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_MENABLE], 0);
        stCfg.tlpEnable = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_TLPENABLE], 0);
        stCfg.slpEnable = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_SLPENABLE], 1);
        stCfg.ddsEnable = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_DDSENABLE], 1);

        if (stCfg.slpEnable) {
            stCfg.ddsEnable = 0;
        }

        bResult = s_stVrpRes.pReg->OpenOrCreateRegKey(HKEY_LOCAL_MACHINE, VRP_REG_GDI_SUBKEY);
        if (bResult) {
            stCfg.rAngle = s_stVrpRes.pReg->ValueDW(c_szRegValue[VRP_REG_RANGLE], 0);
        }
        
        VRPTRACE(VRP_MSG, ("[VRP]rGroup:%d\n", Group));
        VRPTRACE(VRP_MSG, ("[VRP]thDelta:%d\n", stCfg.thDelta));
        VRPTRACE(VRP_MSG, ("[VRP]tvDelta:%d\n", stCfg.tvDelta));
        VRPTRACE(VRP_MSG, ("[VRP]shDelta:%d\n", stCfg.shDelta));
        VRPTRACE(VRP_MSG, ("[VRP]svDelta:%d\n", stCfg.svDelta));
        VRPTRACE(VRP_MSG, ("[VRP]sTimeout:%d\n", stCfg.sTimeout));
        VRPTRACE(VRP_MSG, ("[VRP]tTimeout:%d\n", stCfg.tTimeout));
        VRPTRACE(VRP_MSG, ("[VRP]dtTimeout:%d\n", stCfg.dtTimeout));
        VRPTRACE(VRP_MSG, ("[VRP]ftTimeout:%d\n", stCfg.ftTimeout));
        VRPTRACE(VRP_MSG, ("[VRP]stTimeout:%d\n", stCfg.stTimeout));
        VRPTRACE(VRP_MSG, ("[VRP]slpfTimeout:%d\n", stCfg.slpfTimeout));
        VRPTRACE(VRP_MSG, ("[VRP]slpsTimeout:%d\n", stCfg.slpsTimeout));
        VRPTRACE(VRP_MSG, ("[VRP]vTimeout:%d\n", stCfg.vTimeout));
        VRPTRACE(VRP_MSG, ("[VRP]dtEnable:%d\n", stCfg.dtEnable));
        VRPTRACE(VRP_MSG, ("[VRP]mEnable:%d\n", stCfg.mEnable));
        VRPTRACE(VRP_MSG, ("[VRP]tlpEnable:%d\n", stCfg.tlpEnable));
        VRPTRACE(VRP_MSG, ("[VRP]slpEnable:%d\n", stCfg.slpEnable));
        VRPTRACE(VRP_MSG, ("[VRP]ddsEnable:%d\n", stCfg.ddsEnable));
        VRPTRACE(VRP_MSG, ("[VRP]rAngle:%d\n", stCfg.rAngle));
        break;
    }
#else
    LoadDefCfg(pstCfg);
#endif
}

void RegNotifyCallback(void)
{
#if 0
    if (dwUserData < VRP_REG_LAST) {
        s_stVrpRes.bUpdateCfg = TRUE;
    }
#endif
}

BOOL AddRegNotify(void) {
    return TRUE;
}

void CloseRegNotify(void) {
}

BOOL VrpScanHotRegion(POINT *pHotPoint, DWORD dwFlag, VRP_HOT_REGION *pHotRegion, INT32 dwNum) {
    INT32 i, BingoRegion = VRP_INVALID_REGION;
    INT32 NextFocusRegion;
    BOOL Bingo = FALSE;

    NextFocusRegion = s_stVrpRes.FocusRegion;
    
    for (i = 0; i < dwNum; i++) {
        
        if (VRP_BINGO_REGION(pHotPoint, pHotRegion[i]))
        {
            BingoRegion = i;
            Bingo = TRUE;
            NextFocusRegion = ActiveRegion(pHotPoint, dwFlag, pHotRegion + i, i);
            break;
        }
    }

    if (!VRP_EXIST_FOCUS_REGION) {
        for (i = 0; i < dwNum; i++) {
            
            if (i != BingoRegion && pHotRegion[i].bActived)
            {
                ReleaseRegion(pHotPoint, dwFlag, pHotRegion + i, i);
            }
        }
    }
    s_stVrpRes.FocusRegion = NextFocusRegion;

    return Bingo;
}

BOOL VrpCheckMode(POINT *pHotPoint, DWORD dwFlag, VRP_HOT_REGION *pHotRegion, INT32 dwNum)
{
    BOOL bResult = FALSE;
    if (VRP_BINGO_REGION(pHotPoint, pHotRegion[0])) {
        bResult = VrpScanHotRegion(pHotPoint, dwFlag, s_stVrpRes.stCfg.rGroup->pRegion, s_stVrpRes.stCfg.rGroup->Num);
    }
    else {
        //VRPTRACE(1, ("[VRP]hWnd:0x%04X, HotPoint.x:%d, HotPoint.y:%d\n", s_stVrpRes.stCheckMode.hWnd, pHotRegion->x, pHotRegion->y));
        //bResult = PostMessage(s_stVrpRes.stCheckMode.hWnd, WM_USER, HotPoint.x, HotPoint.y);
    }
    return bResult;
}

BOOL VrpCallback(
    TOUCH_PANEL_SAMPLE_FLAGS    Flags,
    INT X,
    INT Y
    ) 
{
    BOOL bResult = FALSE;
    POINT HotPoint;
    
    if (s_stVrpRes.bInited) {
        if (Flags & TouchSampleValidFlag) {
            HotPoint.x = X;
            HotPoint.y = Y;             
            if (s_stVrpRes.bUpdateCfg) {
                LoadRegCfg(&s_stVrpRes.stCfg);
                s_stVrpRes.bUpdateCfg = FALSE;
            }
            VRPTRACE(VRP_MSG, ("[VRP]FocusRegion:%d, HotPoint.x:%d, HotPoint.y:%d\n", s_stVrpRes.FocusRegion, HotPoint.x, HotPoint.y));
            if (s_stVrpRes.stCheckMode.bEnable) {
                bResult = VrpCheckMode(&HotPoint, Flags, s_stVrpRes.stCfg.rGroup->pRegion, s_stVrpRes.stCfg.rGroup->Num);
            }
            else {
                bResult = VrpScanHotRegion(&HotPoint, Flags, s_stVrpRes.stCfg.rGroup->pRegion, s_stVrpRes.stCfg.rGroup->Num);
            }
        }
    }

    return bResult;
}

//------------------------------------------------------------------------------
//
//  Function:  VrpInit
//
//  Called by device manager to initialize device
//
void VrpInit(struct input_dev *input_dev, PFN_TOUCH_PANEL_CALLBACK pfnCallback)
{    
    VRP_CHECK_IF stIf;
    
    memset(&s_stVrpRes, 0, sizeof(VRP_RES));

    set_bit(KEY_HOME, input_dev->keybit);
    set_bit(KEY_MENU, input_dev->keybit);
    set_bit(KEY_BACK, input_dev->keybit);
    set_bit(KEY_SEARCH, input_dev->keybit);
    set_bit(KEY_SEND, input_dev->keybit);
    set_bit(KEY_END, input_dev->keybit);
    set_bit(KEY_OK, input_dev->keybit);
    set_bit(KEY_ENTER, input_dev->keybit);
    s_stVrpRes.input = input_dev;
    s_stVrpRes.pfnCallback = pfnCallback;

    AddRegNotify();
    LoadRegCfg(&s_stVrpRes.stCfg);

    stIf.pFnCheckEnable = VrpCheckEnable;
    stIf.pFnCheckInfo = VrpCheckInfo;
    
    // ExtEscape(hDC, VRP_CHECK_REG, sizeof(VRP_CHECK_IF), (LPCSTR)&stIf, 0, 0);
    s_stVrpRes.VibOffTimer.function = CallOffVib;
    hrtimer_init(&s_stVrpRes.VibOffTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

    s_stVrpRes.FocusRegion = VRP_INVALID_REGION;
    s_stVrpRes.bInited = TRUE;    
}

//------------------------------------------------------------------------------
//
//  Function:  VrpDeinit
//
//  Called by device manager to uninitialize device.
//
void VrpDeinit(void)
{    
    CloseRegNotify();
}

void VrpCheckInfo(VRP_INFO *pInfo) {
    UINT32 i;
    UINT32 MaxNum;
    VRP_HOT_REGION *pRegion = s_stVrpRes.stCfg.rGroup->pRegion;
    if (pInfo->Size) {
        MaxNum = pInfo->Size - sizeof(UINT32) - sizeof(UINT32);
        MaxNum /= sizeof(RECT);
        if (MaxNum < s_stVrpRes.stCfg.rGroup->Num) {
            pInfo->RegionNum = MaxNum;
        }
        else {
            pInfo->RegionNum = s_stVrpRes.stCfg.rGroup->Num;
        }
#if 0
        // first region is touch pannel
        pRegion = s_stVrpRes.stCfg.rGroup->pRegion + 1;
        if (pInfo->RegionNum > 0)
        {
            pInfo->RegionNum--;
        }
#endif
        for (i = 0; i < pInfo->RegionNum; i++) {
            memcpy(&(pInfo->Regions[i]), &(pRegion->stRect), sizeof(RECT));
            pRegion++;
        }
    }
}

void VrpCheckEnable(VRP_CHECK_MODE *pMode) {    
    s_stVrpRes.stCheckMode = *pMode;    
}

