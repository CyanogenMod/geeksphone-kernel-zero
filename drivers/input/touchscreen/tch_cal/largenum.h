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

    largenum.h

Abstract:  

    General purpose large number arithmatic/logic operations.


Functions:

Notes: 

    The constant SIZE_OF_LARGENUM defines the size of a large number.
    Since this constant is determined at compilation time,
    this module is to be included and compiled with other components
    that needs large number operations.

--*/
#ifndef _LARGENUM_H_
#define _LARGENUM_H_

#define SIZE_OF_LARGENUM    3   // Number of UINT32 in a LARGE_NUM

typedef struct {
    BOOL    fNegative;
    union {
        struct {
            UINT16  s[2*SIZE_OF_LARGENUM];
        }   s16;
        struct {
            UINT32  u[SIZE_OF_LARGENUM];
        }   s32;
    }   u;
} LARGENUM, *PLARGENUM;

//
// Function prototypes
//
PLARGENUM
LargeNumSet(
    PLARGENUM   pNum,
    INT32       n
    );

BOOL
IsLargeNumNotZero(
    PLARGENUM   pNum
    );

BOOL
IsLargeNumNegative(
    PLARGENUM   pNum
    );

BOOL
IsLargeNumMagGreaterThan(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2
    );

BOOL
IsLargeNumMagLessThan(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2
    );

PLARGENUM
LargeNumMagInc(
    PLARGENUM   pNum
    );

char *
LargeNumToAscii(
    PLARGENUM   pNum
    );


PLARGENUM
LargeNumMagAdd(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2,
    PLARGENUM   pResult
    );

PLARGENUM
LargeNumMagSub(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2,
    PLARGENUM   pResult
    );

PLARGENUM
LargeNumAdd(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2,
    PLARGENUM   pResult
    );

PLARGENUM
LargeNumSub(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2,
    PLARGENUM   pResult
    );

PLARGENUM
LargeNumMulUint32(
    UINT32      a,
    UINT32      b,
    PLARGENUM   pResult
    );

PLARGENUM
LargeNumMulInt32(
    INT32       a,
    INT32       b,
    PLARGENUM   pResult
    );

PLARGENUM
LargeNumMult(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2,
    PLARGENUM   pResult
    );

VOID
LargeNumRAShift(
    PLARGENUM   pNum,
    INT32       count
    );

UINT32
LargeNumDivInt32(
    PLARGENUM   pNum,
    INT32       divisor,
    PLARGENUM   pResult
    );

INT32
LargeNumBits(
    PLARGENUM   pNum
    );

#endif  // #ifndef _LARGENUM_H_
