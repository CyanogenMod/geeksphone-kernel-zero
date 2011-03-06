//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:  
    calibrat.c

Abstract:  
    This module contains code for accurate two-dimensional
    touch device calibration for PeRP.


Functions:
TouchPanelSetCalibration
ComputeMatrix33
TouchPanelCalibrateAPoint
ErrorAnalysis

Notes: 
    This new two-dimensional calibration takes care of all the
    two-dimensional mapping, including transposition, roration,
    scaling, and shearing.
    A detail discussion on the approach and its mathmatical background,
    which explains why all the computation in this module, is
    given in the Appendix of the touch driver design specification.

--*/

#include <linux/kernel.h>
#include "touchp.h"
#include "largenum.h"

#define MAX_POINT_ERROR 5
#define DEBUGMSG(a,b) if(a){printk b;}
#define RETAILMSG(a,b) if(a){printk b;}
#define __TEXT
//
// Global Varibales and Private Declarations
//
typedef struct {
    PLARGENUM   pa11, pa12, pa13;
    PLARGENUM   pa21, pa22, pa23;
    PLARGENUM   pa31, pa32, pa33;
}   MATRIX33, *PMATRIX33;

typedef struct {
    INT32   a1;
    INT32   b1;
    INT32   c1;
    INT32   a2;
    INT32   b2;
    INT32   c2;
    INT32   delta;
}   CALIBRATION_PARAMETER, *PCALIBRATION_PARAMETER;

static BOOL                     v_Calibrated = FALSE;
static CALIBRATION_PARAMETER    v_CalcParam;

//
// Function Prototypes
//
BOOL
ErrorAnalysis(
    INT32   cCalibrationPoints,     //@PARM The number of calibration points
    INT32   *pScreenXBuffer,        //@PARM List of screen X coords displayed
    INT32   *pScreenYBuffer,        //@PARM List of screen Y coords displayed
    INT32   *pUncalXBuffer,         //@PARM List of X coords collected
    INT32   *pUncalYBuffer          //@PARM List of Y coords collected
    );

VOID
ComputeMatrix33(
    PLARGENUM   pResult,
    PMATRIX33   pMatrix
    );

/*++

Routine Description:

    Initializes the calibration info which is used to convert uncalibrated
    points to calibrated points.


Autodoc Information:

    @doc INTERNAL DRIVERS MDD TOUCH_DRIVER
    @func VOID | TouchPanelSetCalibration |
    Initializes the calibration info which is used to convert uncalibrated
    points to calibrated points.

    @comm
    The information being initialized in vCalcParam is:<nl>

        a1<nl>
        b1<nl>
        c1<nl>
        a2<nl>
        b2<nl>
        c2<nl>
        delta<nl>

    @devnote

    This is the 2D version of the transform.  A simpler 1D transform
    can be found in the mdd code.  The preprocessor variable
    TWO_DIMENSIONAL_CALIBRATION can be used to select which of
    the transforms will be used.<nl>

    The 2D version is more complicated,
    but is more accurate since it does not make the simplification
    of assuming independent X and Y values.

--*/
BOOL
TouchPanelSetCalibration(
    INT32   cCalibrationPoints,     //@PARM The number of calibration points
    INT32   *pScreenXBuffer,        //@PARM List of screen X coords displayed
    INT32   *pScreenYBuffer,        //@PARM List of screen Y coords displayed
    INT32   *pUncalXBuffer,         //@PARM List of X coords collected
    INT32   *pUncalYBuffer          //@PARM List of Y coords collected
    )
{
    LARGENUM    a11;
    LARGENUM    a21, a22;
    LARGENUM    a31, a32, a33;
    LARGENUM    b11, b12, b13;
    LARGENUM    b21, b22, b23;
    LARGENUM    lnScreenX;
    LARGENUM    lnScreenY;
    LARGENUM    lnTouchX;
    LARGENUM    lnTouchY;
    LARGENUM    lnTemp;
    LARGENUM    delta;
    LARGENUM    a1, b1, c1;
    LARGENUM    a2, b2, c2;
    MATRIX33    Matrix;
    INT32       cShift;
    INT32       minShift;
    int         i;


    DEBUGMSG(1,(__TEXT("calibrating %d point set\r\n"), cCalibrationPoints));

     //
     // If the calibration data is being cleared, set the flag so
     // that the conversion operation is a noop.
     //

    if ( cCalibrationPoints == 0 )
    {
        v_Calibrated = FALSE;
        return TRUE;
    }

    //
    // Compute these large numbers
    //
    LargeNumSet(&a11, 0);
    LargeNumSet(&a21, 0);
    LargeNumSet(&a31, 0);
    LargeNumSet(&a22, 0);
    LargeNumSet(&a32, 0);
    LargeNumSet(&a33, cCalibrationPoints);
    LargeNumSet(&b11, 0);
    LargeNumSet(&b12, 0);
    LargeNumSet(&b13, 0);
    LargeNumSet(&b21, 0);
    LargeNumSet(&b22, 0);
    LargeNumSet(&b23, 0);
    for(i=0; i<cCalibrationPoints; i++){
        LargeNumSet(&lnTouchX, pUncalXBuffer[i]);
        LargeNumSet(&lnTouchY, pUncalYBuffer[i]);
        LargeNumSet(&lnScreenX, pScreenXBuffer[i]);
        LargeNumSet(&lnScreenY, pScreenYBuffer[i]);
        LargeNumMult(&lnTouchX, &lnTouchX, &lnTemp);
        LargeNumAdd(&a11, &lnTemp, &a11);
        LargeNumMult(&lnTouchX, &lnTouchY, &lnTemp);
        LargeNumAdd(&a21, &lnTemp, &a21);
        LargeNumAdd(&a31, &lnTouchX, &a31);
        LargeNumMult(&lnTouchY, &lnTouchY, &lnTemp);
        LargeNumAdd(&a22, &lnTemp, &a22);
        LargeNumAdd(&a32, &lnTouchY, &a32);
        LargeNumMult(&lnTouchX, &lnScreenX, &lnTemp);
        LargeNumAdd(&b11, &lnTemp, &b11);
        LargeNumMult(&lnTouchY, &lnScreenX, &lnTemp);
        LargeNumAdd(&b12, &lnTemp, &b12);
        LargeNumAdd(&b13, &lnScreenX, &b13);
        LargeNumMult(&lnTouchX, &lnScreenY, &lnTemp);
        LargeNumAdd(&b21, &lnTemp, &b21);
        LargeNumMult(&lnTouchY, &lnScreenY, &lnTemp);
        LargeNumAdd(&b22, &lnTemp, &b22);
        LargeNumAdd(&b23, &lnScreenY, &b23);
    }

    Matrix.pa11 = &a11;
    Matrix.pa21 = &a21;
    Matrix.pa31 = &a31;
    Matrix.pa12 = &a21;
    Matrix.pa22 = &a22;
    Matrix.pa32 = &a32;
    Matrix.pa13 = &a31;
    Matrix.pa23 = &a32;
    Matrix.pa33 = &a33;
    ComputeMatrix33(&delta, &Matrix);

    Matrix.pa11 = &b11;
    Matrix.pa21 = &b12;
    Matrix.pa31 = &b13;
    ComputeMatrix33(&a1, &Matrix);

    Matrix.pa11 = &a11;
    Matrix.pa21 = &a21;
    Matrix.pa31 = &a31;
    Matrix.pa12 = &b11;
    Matrix.pa22 = &b12;
    Matrix.pa32 = &b13;
    ComputeMatrix33(&b1, &Matrix);

    Matrix.pa12 = &a21;
    Matrix.pa22 = &a22;
    Matrix.pa32 = &a32;
    Matrix.pa13 = &b11;
    Matrix.pa23 = &b12;
    Matrix.pa33 = &b13;
    ComputeMatrix33(&c1, &Matrix);

    Matrix.pa13 = &a31;
    Matrix.pa23 = &a32;
    Matrix.pa33 = &a33;
    Matrix.pa11 = &b21;
    Matrix.pa21 = &b22;
    Matrix.pa31 = &b23;
    ComputeMatrix33(&a2, &Matrix);

    Matrix.pa11 = &a11;
    Matrix.pa21 = &a21;
    Matrix.pa31 = &a31;
    Matrix.pa12 = &b21;
    Matrix.pa22 = &b22;
    Matrix.pa32 = &b23;
    ComputeMatrix33(&b2, &Matrix);

    Matrix.pa12 = &a21;
    Matrix.pa22 = &a22;
    Matrix.pa32 = &a32;
    Matrix.pa13 = &b21;
    Matrix.pa23 = &b22;
    Matrix.pa33 = &b23;
    ComputeMatrix33(&c2, &Matrix);

#if 1
    {
        LARGENUM    halfDelta;
        //
        // Take care of possible truncation error in later mapping operations
        //
        if(IsLargeNumNegative(&delta)){
            LargeNumDivInt32(&delta, -2, &halfDelta);
        } else {
            LargeNumDivInt32(&delta, 2, &halfDelta);
        }
        LargeNumAdd(&c1, &halfDelta, &c1);
        LargeNumAdd(&c2, &halfDelta, &c2);
    }
#endif

    //
    // All the numbers are determined now.
    // Let's scale them back to 32 bit world
    //
    minShift = 0;
    cShift = LargeNumBits(&a1) - MAX_COEFF_PRECISION;
    if(cShift > minShift){
        minShift = cShift;
    }
    cShift = LargeNumBits(&b1) - MAX_COEFF_PRECISION;
    if(cShift > minShift){
        minShift = cShift;
    }
    cShift = LargeNumBits(&a2) - MAX_COEFF_PRECISION;
    if(cShift > minShift){
        minShift = cShift;
    }
    cShift = LargeNumBits(&b2) - MAX_COEFF_PRECISION;
    if(cShift > minShift){
        minShift = cShift;
    }
    cShift = LargeNumBits(&c1) - MAX_TERM_PRECISION;
    if(cShift > minShift){
        minShift = cShift;
    }
    cShift = LargeNumBits(&c2) - MAX_TERM_PRECISION;
    if(cShift > minShift){
        minShift = cShift;
    }
    cShift = LargeNumBits(&delta) - 31;
    if(cShift > minShift){
        minShift = cShift;
    }

    //
    // Now, shift count is determined, shift all the numbers
    //  right to obtain the 32-bit signed values
    //
    if(minShift){
        LargeNumRAShift(&a1, minShift);
        LargeNumRAShift(&a2, minShift);
        LargeNumRAShift(&b1, minShift);
        LargeNumRAShift(&b2, minShift);
        LargeNumRAShift(&c1, minShift);
        LargeNumRAShift(&c2, minShift);
        LargeNumRAShift(&delta, minShift);
    }
    v_CalcParam.a1      = a1.u.s32.u[0];
    v_CalcParam.b1      = b1.u.s32.u[0];
    v_CalcParam.c1      = c1.u.s32.u[0];
    v_CalcParam.a2      = a2.u.s32.u[0];
    v_CalcParam.b2      = b2.u.s32.u[0];
    v_CalcParam.c2      = c2.u.s32.u[0];
    v_CalcParam.delta   = delta.u.s32.u[0];

     // Don't allow delta to be zero, since it gets used as a divisor
    if( ! v_CalcParam.delta )
    {
        RETAILMSG(1,(__TEXT("TouchPanelSetCalibration: delta of 0 invalid\r\n")));
        RETAILMSG(1,(__TEXT("\tCalibration failed.\r\n")));
        v_CalcParam.delta = 1;  // any non-zero value to prevents DivByZero traps later
        v_Calibrated = FALSE;
    }
    else
        v_Calibrated = TRUE;

    return ErrorAnalysis(
                    cCalibrationPoints,
                    pScreenXBuffer,
                    pScreenYBuffer,
                    pUncalXBuffer,
                    pUncalYBuffer
                );
}

VOID
ComputeMatrix33(
    PLARGENUM   pResult,
    PMATRIX33   pMatrix
    )
{
    LARGENUM    lnTemp;

    LargeNumMult(pMatrix->pa11, pMatrix->pa22, &lnTemp);
    LargeNumMult(pMatrix->pa33, &lnTemp, pResult);
    LargeNumMult(pMatrix->pa21, pMatrix->pa32, &lnTemp);
    LargeNumMult(pMatrix->pa13, &lnTemp, &lnTemp);
    LargeNumAdd(pResult, &lnTemp, pResult);
    LargeNumMult(pMatrix->pa12, pMatrix->pa23, &lnTemp);
    LargeNumMult(pMatrix->pa31, &lnTemp, &lnTemp);
    LargeNumAdd(pResult, &lnTemp, pResult);
    LargeNumMult(pMatrix->pa13, pMatrix->pa22, &lnTemp);
    LargeNumMult(pMatrix->pa31, &lnTemp, &lnTemp);
    LargeNumSub(pResult, &lnTemp, pResult);
    LargeNumMult(pMatrix->pa12, pMatrix->pa21, &lnTemp);
    LargeNumMult(pMatrix->pa33, &lnTemp, &lnTemp);
    LargeNumSub(pResult, &lnTemp, pResult);
    LargeNumMult(pMatrix->pa23, pMatrix->pa32, &lnTemp);
    LargeNumMult(pMatrix->pa11, &lnTemp, &lnTemp);
    LargeNumSub(pResult, &lnTemp, pResult);
}

/*++

Routine Description:

    Convert uncalibrated points to calibrated points.


Autodoc Information:

    @doc INTERNAL DRIVERS MDD TOUCH_DRIVER
    @func VOID | TouchPanelCalibrateAPoint |
    Convert uncalibrated points to calibrated points.

    @comm
    The transform coefficients are already in vCalcParam.<nl>


    @devnote

--*/


void
TouchPanelCalibrateAPoint(
    INT32   UncalX,     //@PARM The uncalibrated X coordinate
    INT32   UncalY,     //@PARM The uncalibrated Y coordinate
    INT32   *pCalX,     //@PARM The calibrated X coordinate
    INT32   *pCalY      //@PARM The calibrated Y coordinate
    )
{
    INT32   x, y;

    if ( !v_Calibrated ){
        *pCalX = UncalX;
        *pCalY = UncalY;
        return;
    }

    x = (v_CalcParam.a1 * UncalX + v_CalcParam.b1 * UncalY +
         v_CalcParam.c1) / v_CalcParam.delta;
    y = (v_CalcParam.a2 * UncalX + v_CalcParam.b2 * UncalY +
         v_CalcParam.c2) / v_CalcParam.delta;
    if ( x < 0 ){
        x = 0;
    }

    if ( y < 0 ){
        y = 0;
    }

    *pCalX = x;
    *pCalY = y;
}

/*++

    @doc INTERNAL DRIVERS MDD TOUCH_DRIVER
    @func VOID | ErrorAnalysis |
    Display info on accuracy of the calibration.


--*/
    
BOOL
ErrorAnalysis(
    INT32   cCalibrationPoints,     //@PARM The number of calibration points
    INT32   *pScreenXBuffer,        //@PARM List of screen X coords displayed
    INT32   *pScreenYBuffer,        //@PARM List of screen Y coords displayed
    INT32   *pUncalXBuffer,         //@PARM List of X coords collected
    INT32   *pUncalYBuffer          //@PARM List of Y coords collected
    )
{
    int     i;
    UINT32  maxErr, err;
    INT32   x,y;
    INT32   dx,dy;
    UINT32  errThreshold = MAX_POINT_ERROR;  // Can be overridden by registry entry

    RETAILMSG(1,(__TEXT("Maximum Allowed Error %d:\r\n"),
                errThreshold));
    DEBUGMSG(1,(__TEXT("Calibration Results:\r\n")));

    maxErr = 0;
    DEBUGMSG(1,(__TEXT("   Screen    =>    Mapped\r\n")));
    for(i=0; i<cCalibrationPoints; i++){
        TouchPanelCalibrateAPoint(  pUncalXBuffer[i],
                                    pUncalYBuffer[i],
                                    &x,
                                    &y
                                    );

        DEBUGMSG(1,(__TEXT("(%4d, %4d) => (%4d, %4d)\r\n"),
                pScreenXBuffer[i],
                pScreenYBuffer[i],
                x,
                y
                ));
        dx = x - pScreenXBuffer[i];
        dy = y - pScreenYBuffer[i];
        err = dx * dx + dy * dy;
        if(err > maxErr){
            maxErr = err;
        }
    }
    DEBUGMSG(1,(__TEXT("Maximum error (square of Euclidean distance in screen units) = %u\r\n"),
            maxErr
            ));

    if( maxErr < (errThreshold * errThreshold) ){
       return TRUE;
    } else {
       RETAILMSG(1,(__TEXT("Maximum error %u exceeds calibration threshold %u\r\n"),
               maxErr, errThreshold
               ));
       return FALSE;
    }

}
