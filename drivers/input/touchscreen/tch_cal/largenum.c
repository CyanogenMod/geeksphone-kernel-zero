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

    largenum.c

Abstract:  

    General purpose large number arithmatic/logic operations.

Functions:
LargeNumSet
IsLargeNumNotZero
IsLargeNumNegative
IsLargeNumMagGreaterThan
IsLargeNumMagLessThan
LargeNumMagInc
LargeNumMagAdd
LargeNumMagSub
LargeNumAdd
LargeNumSub
LargeNumMulUint32
LargeNumMulInt32
LargeNumMult
LargeNumSignedFormat
LargeNumRAShift
LargeNumDivInt32
LargeNumBits
LargeNumToAscii

Notes: 
    The constant SIZE_OF_LARGENUM, defined in largenum.h,
    defines the size of a large number.

    Since this constant is determined at compilation time,
    this module is to be included and compiled with other components
    that needs large number operations.

    Not all arithmatic and logic operations are included at this time.

    --- IMPORTANT ---

    These routines are not optimized.  They should not be used in a
    component (or portion) where optimized performance is crucial.

--*/
#include <linux/kernel.h>
#include "touchp.h"
#include "largenum.h"

//
// Function prototypes
//
UINT32
LargeNumSignedFormat(
    PLARGENUM   pNum
    );

//
// Routines start
//

/*++

LargeNumSet:

    Initialized the content of a large integer to a given value.

Arguments:

    pNum    - Points to the large integer to be initializeed.
    n       - the specific signed value used to initialize the large integer.

Return Value:

    Pointer to the large integer

--*/
PLARGENUM
LargeNumSet(
    PLARGENUM   pNum,
    INT32       n
    )
{
    int i;

    if(n < 0){
        pNum->u.s32.u[0] = -n;
        pNum->fNegative = 1;
    } else{
        pNum->u.s32.u[0] = n;
        pNum->fNegative=0;
    }
    for(i=1; i<SIZE_OF_LARGENUM; i++){
        pNum->u.s32.u[i] = 0;
    }
    return pNum;
}

/*++

IsLargeNumNotZero:

    Test the content of a large number for zero.

Arguments:

    pNum    - Points to the large integer to be initializeed.

Return Value:

    TRUE,   if the large number is zero.
    FALSE,  otherwise.

--*/
BOOL
IsLargeNumNotZero(
    PLARGENUM   pNum
    )
{
    int i;

    for(i=0; i<SIZE_OF_LARGENUM; i++){
        if(pNum->u.s32.u[i]){
            return TRUE;
        }
    }
    return FALSE;
}

/*++

IsLargeNumNegative:

    Test the content of a large number for negative value.

Arguments:

    pNum    - Points to the large integer to be initializeed.

Return Value:

    TRUE,   if the large number is negative.
    FALSE,  otherwise.

Note:

    Zero is consider positive.

--*/
BOOL
IsLargeNumNegative(
    PLARGENUM   pNum
    )
{
    return (pNum->fNegative ? TRUE : FALSE);

}

/*++

IsLargeNumMagGreaterThan:

    Compare the contents (magnitude) of two large numbers.

Arguments:

    pNum1   - Points to the first large integer to be compared
    pNum2   - Points to the second large integer to be compared.

Return Value:

    TRUE,   if the first number is greater than the second.
    FALSE,  otherwise.

Note:

    Only the magnitudes of the numbers are compared.  The signs are ignored.

--*/
BOOL
IsLargeNumMagGreaterThan(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2
    )
{
    int i;

    for(i=SIZE_OF_LARGENUM-1; i>=0; i--){
        if(pNum1->u.s32.u[i] > pNum2->u.s32.u[i]){
            return TRUE;
        } else if(pNum1->u.s32.u[i] < pNum2->u.s32.u[i]){
            return FALSE;
        }
    }
    return FALSE;
}

/*++

IsLargeNumMagLessThan:

    Compare the contents (magnitude) of two large numbers.

Arguments:

    pNum1   - Points to the first large integer to be compared
    pNum2   - Points to the second large integer to be compared.

Return Value:

    TRUE,   if the first number is less than the second.
    FALSE,  otherwise.

Note:

    Only the magnitudes of the numbers are compared.  The signs are ignored.

--*/
BOOL
IsLargeNumMagLessThan(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2
    )
{
    int i;

    for(i=SIZE_OF_LARGENUM-1; i>=0; i--){
        if(pNum1->u.s32.u[i] < pNum2->u.s32.u[i]){
            return TRUE;
        } else if(pNum1->u.s32.u[i] > pNum2->u.s32.u[i]){
            return FALSE;
        }
    }
    return FALSE;
}

/*++

LargeNumMagInc:

    Increment the magnitude of a large number.

Arguments:

    pNum    - Points to the large integer to be incremented

Return Value:

    The same pointer to the large number.

Note:

    Only the magnitude of the number is incremented.
    In other words, if the number is negative, this routine actually
    decrement it (by increment its magnitude).

--*/
PLARGENUM
LargeNumMagInc(
    PLARGENUM   pNum
    )
{
    UINT32  c;
    int     i;

    c = 1;
    for(i=0; i<SIZE_OF_LARGENUM; i++){
        pNum->u.s32.u[i] += c;
        if(pNum->u.s32.u[i]){
            c = 0;
        }
    }
    return pNum;
}

/*++

LargeNumMagAdd:

    Sums up the magnitude of two large numbers.  Signs are ignored.

Arguments:

    pNum1   - Points to the first large integer to be added to.
    pNum2   - Points to the second large integer to be added with.
    pResult - Points to the a large integer buffer to receive the sum.

Return Value:

    The same pointer as pResult.

Note:

    Only the magnitudes of the large numbers are summed.
    Signs are ignored.
    In other words, if the signs are different, this routine actually
    subtracts them (by summing their magnitudes).

--*/
PLARGENUM
LargeNumMagAdd(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2,
    PLARGENUM   pResult
    )
{
    UINT32      c;
    UINT32      i;
    UINT32      a;
    UINT32      b;

    c = 0;
    for(i=0; i<SIZE_OF_LARGENUM; i++){
        a = pNum1->u.s32.u[i];
        b = pNum2->u.s32.u[i];
        pResult->u.s32.u[i] = a + b + c;
        if(c){
            if(pResult->u.s32.u[i] <= a){
                c = 1;
            } else {
                c = 0;
            }

        } else {
            if(pResult->u.s32.u[i] < a){
                c = 1;
            } else {
                c = 0;
            }

        }
    }
    return pResult;
}

/*++

LargeNumMagSub:

    Similar to LargeNumMagAdd except that, instead of sum, the difference is taken.

Arguments:

    pNum1   - Points to the first large integer to be subtracted from.
    pNum2   - Points to the second large integer to be substracted with.
    pResult - Points to the a large integer buffer to receive the difference.

Return Value:

    The same pointer as pResult.

Note:

    Only the magnitudes of the large numbers are summed.
    Signs are ignored.
    In other words, if the signs are different, this routine actually
    adds them (by substracting their magnitudes).

--*/
PLARGENUM
LargeNumMagSub(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2,
    PLARGENUM   pResult
    )
{
    UINT32      c;
    UINT32      i;
    UINT32      a;
    UINT32      b;

    c = 1;
    for(i=0; i<SIZE_OF_LARGENUM; i++){
        a = pNum1->u.s32.u[i];
        b = ~(pNum2->u.s32.u[i]);
        pResult->u.s32.u[i] = a + b + c;
        if(c){
            if(pResult->u.s32.u[i] <= a){
                c = 1;
            } else {
                c = 0;
            }

        } else {
            if(pResult->u.s32.u[i] < a){
                c = 1;
            } else {
                c = 0;
            }

        }
    }
    return pResult;
}

/*++

LargeNumAdd:

    Sums up two large numbers.  Signs are taken into account.

Arguments:

    pNum1   - Points to the first large integer to be added to.
    pNum2   - Points to the second large integer to be added with.
    pResult - Points to the a large integer buffer to receive the sum.

Return Value:

    The same pointer as pResult.

--*/
PLARGENUM
LargeNumAdd(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2,
    PLARGENUM   pResult
    )
{
    BOOL    fNegative1;
    BOOL    fNegative2;

    fNegative1 = IsLargeNumNegative(pNum1);
    fNegative2 = IsLargeNumNegative(pNum2);

    if(fNegative1 != fNegative2){
        if(IsLargeNumMagGreaterThan(pNum1, pNum2)){
            LargeNumMagSub(pNum1, pNum2, pResult);
        } else {
            LargeNumMagSub(pNum2, pNum1, pResult);
            fNegative1 = !fNegative1;
        }
    } else {
        LargeNumMagAdd(pNum1, pNum2, pResult);
    }
    if(!IsLargeNumNotZero(pResult)){
        pResult->fNegative = FALSE;
    } else {
        pResult->fNegative = fNegative1;
    }
    return pResult;
}

/*++

LargeNumSub:

    Substracts a large numbers from another.  Signs are taken into account.

Arguments:

    pNum1   - Points to the first large integer to be subtracted from.
    pNum2   - Points to the second large integer to be substracted with.
    pResult - Points to the a large integer buffer to receive the difference.

Return Value:

    The same pointer as pResult.

--*/
PLARGENUM
LargeNumSub(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2,
    PLARGENUM   pResult
    )
{
    BOOL    fNegative1;
    BOOL    fNegative2;

    fNegative1 = IsLargeNumNegative(pNum1);
    fNegative2 = IsLargeNumNegative(pNum2);

    if(fNegative1 == fNegative2){
        if(IsLargeNumMagGreaterThan(pNum1, pNum2)){
            LargeNumMagSub(pNum1, pNum2, pResult);
        } else {
            LargeNumMagSub(pNum2, pNum1, pResult);
            fNegative1 = !fNegative1;
        }
    } else {
        LargeNumMagAdd(pNum1, pNum2, pResult);
    }
    if(!IsLargeNumNotZero(pResult)){
        pResult->fNegative = FALSE;
    } else {
        pResult->fNegative = fNegative1;
    }
    return pResult;
}

//
//
//
#if SIZE_OF_LARGENUM < 2
#error  SIZE_OF_LARGENUM must be at least 2
#endif

/*++

LargeNumMulUint32:

    Multiply two unsigned 32-bit numbers.

Arguments:

    a       - the 32-bit multipliant.
    b       - the 32-bit multiplier.
    pResult - Points to the a large integer buffer to receive the difference.

Return Value:

    The same pointer as pResult.

Note:

    The sign of the returned large number is set to positive.

--*/
PLARGENUM
LargeNumMulUint32(
    UINT32      a,
    UINT32      b,
    PLARGENUM   pResult
    )
{
    UINT32  a1, a0;
    UINT32  b1, b0;
    UINT32  r0;
    UINT32  r1;
    UINT32  r2;
    UINT32  c;
    int     i;

    a1 = a >> 16;
    a0 = a & 0xffff;
    b1 = b >> 16;
    b0 = b & 0xffff;

    r0 = a0 * b0;
    r1 = a1 * b0 + a0 * b1;
    r2 = a1 * b1;

    pResult->u.s32.u[0] = (r1 << 16) + r0;
    if(pResult->u.s32.u[0] < r0){
        c = 1;
    } else {
        c = 0;
    }
    pResult->u.s32.u[1] = r2 + (r1 >> 16) + c;
    for(i=2; i<SIZE_OF_LARGENUM; i++){
        pResult->u.s32.u[i] = 0;
    }
    pResult->fNegative = 0;

    return pResult;
}

/*++

LargeNumMulInt32:

    Multiply two signed 32-bit numbers.

Arguments:

    a       - the 32-bit multipliant.
    b       - the 32-bit multiplier.
    pResult - Points to the a large integer buffer to receive the difference.

Return Value:

    The same pointer as pResult.

Note:

    The sign of the returned large number is set according to the
    multiplication result.

--*/
PLARGENUM
LargeNumMulInt32(
    INT32       a,
    INT32       b,
    PLARGENUM   pResult
    )
{
    BOOL        fNegativeA;
    BOOL        fNegativeB;

    if(a < 0){
        fNegativeA = TRUE;
        a = -a;
    } else {
        fNegativeA = FALSE;
    }

    if(b < 0){
        fNegativeB = TRUE;
        b = -b;
    } else {
        fNegativeB = FALSE;
    }

    LargeNumMulUint32(a, b, pResult);

    if(!IsLargeNumNotZero(pResult)){
        pResult->fNegative = FALSE;
    } else {
        if(fNegativeA != fNegativeB){
            pResult->fNegative = TRUE;
        }
    }
    return pResult;
}

/*++

LargeNumMult:

    Multiply two large numbers.

Arguments:

    pNum1   - points to the large multipliant.
    b       - points to the large multiplier.
    pResult - Points to the a large integer buffer to receive the difference.

Return Value:

    The same pointer as pResult.

Note:

    The sign of the returned large number is set according to the
    multiplication result.

--*/
PLARGENUM
LargeNumMult(
    PLARGENUM   pNum1,
    PLARGENUM   pNum2,
    PLARGENUM   pResult
    )
{
    LARGENUM    lNumTemp;
    LARGENUM    lNumSum;
    LARGENUM    lNumCarry;
    int         i;
    int         j;

    LargeNumSet(&lNumCarry, 0);
    for(i=0; i<SIZE_OF_LARGENUM; i++){
        LargeNumSet(&lNumSum, 0);
        for(j=0; j<=i; j++){
            LargeNumMulUint32(pNum1->u.s32.u[j], pNum2->u.s32.u[i-j], &lNumTemp);
            LargeNumMagAdd(&lNumTemp, &lNumSum, &lNumSum);
        }
        LargeNumMagAdd(&lNumCarry, &lNumSum, &lNumSum);
        for(j=0; j<SIZE_OF_LARGENUM-1; j++){
            lNumCarry.u.s32.u[j] = lNumSum.u.s32.u[j+1];
        }
        pResult->u.s32.u[i] = lNumSum.u.s32.u[0];
    }

    if(!IsLargeNumNotZero(pResult)){
        pResult->fNegative = FALSE;
    } else {
        pResult->fNegative = (pNum1->fNegative != pNum2->fNegative);
    }
    return pResult;
}

/*++

LargeNumSignedFormat:

    Convert a LARGENUM into signed 2's complement format.

Arguments:

    pNum    - points to the large number to be shifted.

Return Value:

    0xffffffff  : if the number is negative
    0           : if the number is zero or positive

Note:

    The result is in 2's completement notation and the vacant
    bits are sign extended.

    -- Important ---

    *pNum is detroyed by this operation and should not be used as LARGENUM
    after returned from this routine.

--*/
UINT32
LargeNumSignedFormat(
    PLARGENUM   pNum
    )
{
    int     i;
    UINT32  c;

    if(IsLargeNumNegative(pNum)){
        c = 1;
        for(i=0; i<SIZE_OF_LARGENUM; i++){
            pNum->u.s32.u[i] = ~(pNum->u.s32.u[i]) + c;
            if(pNum->u.s32.u[i]){
                c = 0;
            }
        }
        return 0xffffffff;
    } else {
        return 0;
    }
}

/*++

LargeNumRAShift:

    Arithmatic shift a given large number right by a specific number
    of positions.

Arguments:

    pNum    - points to the large number to be shifted.
    count   - number of pistion to shift.

Return Value:

    -- none --

Note:

    The result is in 2's completement notation and the vacant
    bits are sign extended.

    -- Important ---

    *pNum is detroyed by this operation and should not be used as LARGENUM
    after returned from this routine.

--*/
VOID
LargeNumRAShift(
    PLARGENUM   pNum,
    INT32       count
    )
{
    INT32   shift32;
    INT32   countLeft;
    UINT32  filler;
    int     i;
    int     j;

    filler = LargeNumSignedFormat(pNum);

    shift32 = count / 32;

    if(shift32 > (SIZE_OF_LARGENUM - 1)){
        for(i=0; i<SIZE_OF_LARGENUM; i++){
            pNum->u.s32.u[i] = filler;
        }
        return;
    }

    count %= 32;
    countLeft = 32 - count;
    for(i=0, j=shift32;;){
        pNum->u.s32.u[i] = (pNum->u.s32.u[j] >> count);
        if(j<(SIZE_OF_LARGENUM-1)){
            j++;            
            if (countLeft < 32) {
                // Shifting by >= 32 is undefined.
                pNum->u.s32.u[i] |= pNum->u.s32.u[j] << countLeft;
            }
            i++;
        } else {
            if (countLeft < 32) {
                // Shifting by >= 32 is undefined.
                pNum->u.s32.u[i] |= filler << countLeft;
            }
            i++;
            break;
        }
    }

    for(; i<SIZE_OF_LARGENUM; i++){
        pNum->u.s32.u[i] = filler;
    }
}

/*++

LargeNumDivInt32:

    Divide a large number with a signed 32-bit number.

Arguments:

    pNum    - points to the large dividant.
    divisor - the 32-bit divisor.
    pResult - Points to the a large integer buffer to receive the difference.

Return Value:

    The remainder.

Note:

    If divisor is zero, the result will be filled with 0xffffffff and
    the remainder returned is 0xffffffff.

    Also, the remainder is adjusted so that it is always positive and
    is within the range between 0 and the absolute value of divisor.

--*/
UINT32
LargeNumDivInt32(
    PLARGENUM   pNum,
    INT32       divisor,
    PLARGENUM   pResult
    )
{
    UINT32  s[2*SIZE_OF_LARGENUM];
    UINT32  r;
    UINT32  q;
    UINT32  d;
    BOOL    sd;
    int     i;

    for(i=0; i<2*SIZE_OF_LARGENUM; i++){
        s[i] = pNum->u.s16.s[i];
    }

    if(divisor < 0){
        divisor = -divisor;
        sd = TRUE;
    } else if(divisor == 0){
        //
        // This is a divide-by-zero error
        //
        for(i=0; i<SIZE_OF_LARGENUM; i++){
            pResult->u.s32.u[i] = 0xffffffff;
        }
        return 0xffffffff;
    } else {
        sd = FALSE;
    }

    r = 0;
    for(i=(2*SIZE_OF_LARGENUM-1); i>=0; i--){
        d = (r << 16) + s[i];
        q = d / divisor;
        r = d - q * divisor;
        s[i] = q;
    }

    for(i=0; i<2*SIZE_OF_LARGENUM; i++){
        pResult->u.s16.s[i] = s[i];
    }

    if(pNum->fNegative){
        LargeNumMagInc(pResult);
        r = divisor - r;
        if(sd == 0 && IsLargeNumNotZero(pResult)){
            pResult->fNegative = TRUE;
        } else {
            pResult->fNegative = FALSE;
        }

    } else {
        if(sd && IsLargeNumNotZero(pResult)){
            pResult->fNegative = TRUE;
        } else {
            pResult->fNegative = FALSE;
        }
    }

    return r;
}

/*++

LargeNumBits:

    Finds the number of significant bits in a large number.  Basically, it
    is the position of the highest one bit plus 1.

Arguments:

    pNum    - points to the large number.

Return Value:

    The number of significant bits.

--*/
INT32
LargeNumBits(
    PLARGENUM   pNum
    )
{
    static  UINT32 LargeNumMask[32] = {
        0x00000001,
        0x00000002,
        0x00000004,
        0x00000008,
        0x00000010,
        0x00000020,
        0x00000040,
        0x00000080,
        0x00000100,
        0x00000200,
        0x00000400,
        0x00000800,
        0x00001000,
        0x00002000,
        0x00004000,
        0x00008000,
        0x00010000,
        0x00020000,
        0x00040000,
        0x00080000,
        0x00100000,
        0x00200000,
        0x00400000,
        0x00800000,
        0x01000000,
        0x02000000,
        0x04000000,
        0x08000000,
        0x10000000,
        0x20000000,
        0x40000000,
        0x80000000,
        };

    int     i;
    int     j;
    UINT32  u;

    for(i=(SIZE_OF_LARGENUM-1); i>=0; i--){
        u = pNum->u.s32.u[i];
        if(u){
            for(j=31; j>=0; j--){
                if(u & (LargeNumMask[j])){
                    return i * 32 + j + 1;
                }
            }
        }
    }
    return 0;
}

/*++

LargeNumToAscii:

    Converts a large integer into ASCIIZ string.

Arguments:

    pNum    - points to the large number.

Return Value:

    Pointer to the ASCIIZ string.

Note:

    Since the string is a static area inside the routine, this routine is
    not re-entrant and, most importantly, once it is called, the previous
    content is gone.

--*/
char *
LargeNumToAscii(
    PLARGENUM   pNum
    )
{
    static  char    buf[SIZE_OF_LARGENUM * 10 + 2];
    LARGENUM        lNum;
    char            *p;
    char            *q;
    UINT32          r;
    int             s;

    p = buf + sizeof(buf) - 1;
    *p= 0;

    lNum = *pNum;

    s = pNum->fNegative;
    lNum.fNegative = 0;

    while(IsLargeNumNotZero(&lNum)){
        r = LargeNumDivInt32(&lNum, 10, &lNum);
        p--;
        *p = r + '0';
    }

    q = buf;

    if(s){
        *q++='-';
    }
    while(*p){
        //assert(q <= p);
        *q++ = *p++;
    }

    if((q == buf) || (s && q == &(buf[1]))){
        *q++ = '0';
    }
    *q = 0;
    return buf;
}
