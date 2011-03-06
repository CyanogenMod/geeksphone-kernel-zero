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

    touchp.h

Abstract:  

Notes: 

--*/

#ifndef _TOUCHP_H_
#define _TOUCHP_H_

#ifndef INT
#define INT int
#endif

#ifndef INT16
#define INT16 s16
#endif

#ifndef UINT16
#define UINT16 u16
#endif

#ifndef UINT32
#define UINT32 u32
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

#ifndef VOID
#define VOID void
#endif

#ifndef BOOL
#define BOOL int
#endif

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

#ifdef  _cplusplus
#define C_FUNC_DECL extern "C"
#else
#define C_FUNC_DECL
#endif


#define ADC_PRECISION       12          // Precision of ADC output (in bits)
#define MAX_TERM_PRECISION  27          // Reserve 1 bit for sign and two bits for
                                        //  three terms (there are three terms in
                                        //  each of x and y mapping functions.)

//
// All a1, a2, b1, and b2 must have less than MAX_COEFF_PRECISION bits since
//  they all are multiplied with either an X or a Y to form a term.
// Both c1 and c2 can have up to MAX_TERM_PRECISION since they each alone
//  forms a term.
//
#define MAX_COEFF_PRECISION (MAX_TERM_PRECISION - ADC_PRECISION)

C_FUNC_DECL
BOOL
TouchPanelSetCalibration(
    INT32   cCalibrationPoints,     //@PARM The number of calibration points
    INT32   *pScreenXBuffer,        //@PARM List of screen X coords displayed
    INT32   *pScreenYBuffer,        //@PARM List of screen Y coords displayed
    INT32   *pUncalXBuffer,         //@PARM List of X coords collected
    INT32   *pUncalYBuffer          //@PARM List of Y coords collected
    );

C_FUNC_DECL
void
TouchPanelCalibrateAPoint(
    INT32   UncalX,     //@PARM The uncalibrated X coordinate
    INT32   UncalY,     //@PARM The uncalibrated Y coordinate
    INT32   *pCalX,     //@PARM The calibrated X coordinate
    INT32   *pCalY      //@PARM The calibrated Y coordinate
    );

#endif  //#ifndef   _TOUCHP_H_
