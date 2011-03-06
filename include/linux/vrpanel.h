#ifndef _VRPANEL_H_
#define _VRPANEL_H_

#ifndef INT
#define INT int
#endif

#ifndef INT32
#define INT32 int
#endif

#ifndef UINT32
#define UINT32 u32
#endif

#ifndef DWORD
#define DWORD int
#endif

#ifndef LONG
#define LONG long
#endif

#ifndef HANDLE
#define HANDLE (void*)
#endif

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

#ifndef POINT
typedef struct tagPOINT
{
    INT  x;
    INT  y;
} POINT;
#endif

#ifndef RECT
typedef struct tagRECT 
{
    INT left;
    INT top;
    INT right;
    INT bottom;
} RECT;
#endif


#ifndef TOUCH_PANEL_SAMPLE_FLAGS
typedef UINT32 TOUCH_PANEL_SAMPLE_FLAGS;
#endif

#ifndef PFN_TOUCH_PANEL_CALLBACK
typedef BOOL (*PFN_TOUCH_PANEL_CALLBACK)(
    TOUCH_PANEL_SAMPLE_FLAGS    Flags,
    INT X,
    INT Y
    );
#endif

#ifndef KEYEVENTF_KEYUP
#define KEYEVENTF_KEYUP 1
#endif 

enum enumTouchPanelSampleFlags
{
    TouchSampleValidFlag            = 0x01,     //@EMEM The sample is valid.
    TouchSampleDownFlag             = 0x02,     //@EMEM The finger/stylus is down.
    TouchSampleIsCalibratedFlag     = 0x04,     //@EMEM The XY data has already been calibrated.
    TouchSamplePreviousDownFlag     = 0x08,     //@EMEM The state of the previous valid sample.
    TouchSampleIgnore               = 0x10,     //@EMEM Ignore this sample.

    TouchSampleMouse                = 0x40000000    //  reserved
};

typedef struct {
    UINT32 Size;
    UINT32 RegionNum;
    RECT Regions[10];
} VRP_INFO;

typedef struct {
    BOOL bEnable;
} VRP_CHECK_MODE;

typedef void(*PFN_CHECK_INFO)(VRP_INFO *pInfo);
typedef void(*PFN_CHECK_ENABLE)(VRP_CHECK_MODE *pMode);

typedef struct {
    PFN_CHECK_INFO pFnCheckInfo;
    PFN_CHECK_ENABLE pFnCheckEnable;
} VRP_CHECK_IF;

BOOL VrpCallback(
    TOUCH_PANEL_SAMPLE_FLAGS    Flags,
    INT X,
    INT Y
    );
void VrpInit(struct input_dev *input_dev, PFN_TOUCH_PANEL_CALLBACK pfnCallback);
void VrpDeinit(void);
void VrpCheckInfo(VRP_INFO *pInfo);
void VrpCheckEnable(VRP_CHECK_MODE *pMode);

#endif

