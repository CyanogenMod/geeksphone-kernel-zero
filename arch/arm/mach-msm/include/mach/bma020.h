#ifndef __ASM_ARCH_BAM020_H__
#define __ASM_ARCH_BAM020_H__


#define SCALE2G         15.6        //mg
#define SCALE4G         31.2        //mg
#define SCALE8G         62.4        //mg

#define I2C_ADDRSIZE    0x07    

#define XOUTL           0x0
#define XOUTH           0x1
#define YOUTL           0x2
#define YOUTH           0x3
#define ZOUTL           0x4
#define ZOUTH           0x5
#define XOUT8           0x6
#define YOUT8           0x7
#define ZOUT8           0x8
#define STATUS          0x9
#define DETSRC          0xA
#define MMA_ID          0xD
#define XOFFL           0x10
#define XOFFH           0x11
#define YOFFL           0x12
#define YOFFH           0x13
#define ZOFFL           0x14
#define ZOFFH           0x15
#define MCTL            0x16
#define INTRST          0x17
#define CTL1            0x18
#define CTL2            0x19
#define LDTL            0x1A
#define LDTH            0x1B
#define PW              0x1C
#define LT              0x1D
#define TW              0x1E

#define BMA_ID          0x0
#define BMA_CONTROL1    0x15
#define BMA_CONTROL2    0x14
#define BMA_STATUS      0xA

#define STATUS_RDY      (1<<0)
#define STATUS_DOVR     (1<<0)
#define STATUS_PERR     (1<<0)

#define MCTL_MODE_SB    (0<<0)
#define MCTL_MODE_MM    (1<<0)
#define MCTL_MODE_LDM   (2<<0)
#define MCTL_MODE_PDM   (3<<0)
#define MCTL_MODE_MASK  (3<<0)

#define MCTL_GVAL4G     (2<<2)
#define MCTL_GVAL2G     (1<<2)
#define MCTL_GVAL8G     (0<<2)
#define MCTL_GVAL_MASK  (3<<2)

#define MCTL_STON       (1<<4)
#define MCTL_SPI3W      (1<<5)
#define MCTL_DRPD       (1<<6)

#define INTRST_CLRINT1  (1<<0)
#define INTRST_CLRINT2  (1<<1)

#define CTL1_INTPIN     (1<<0)
#define CTL1_INT_LD     (1<<1)
#define CTL1_INT_PD     (2<<1)
#define CTL1_INT_SPD    (3<<1)
#define CTL1_XDA        (1<<3)
#define CTL1_YDA        (1<<4)
#define CTL1_ZDA        (1<<5)
#define CTL1_THOPT      (1<<6)
#define CTL1_DFBW       (1<<7)

#define CTL2_LDPL       (1<<0)
#define CTL2_PDPL       (1<<1)
#define CTL2_DRVO       (1<<2)



typedef struct
{
    signed   XVal       :10;    // bit0~9
    signed   YVal       :10;    // bit10~19
    signed   ZVal       :10;    // bit20~29
    unsigned Resverde1  :1;     // bit 30
    unsigned Resverde2  :1;     // bit 31
}   MON_XYZ_VALBITS, *PMON_XYZ_VALBITS;


typedef struct
{
	signed short		XVal;
	signed short		YVal;
	signed short		ZVal;
}   MON_XYZ_INT, *PMON_XYZINT;

typedef struct
{
	MON_XYZ_INT			xyz;
	signed short		scalex;
	signed short		scaley;
	signed short		scalez;
	unsigned int		resvered;
}   MON_CAL_VAL, *PMON_CAL_VAL;


union Data16bit{
    short  Sdata16;
    unsigned short Udata16;
    struct {           // Bank	0
        unsigned char   LByte;  // 0000
        unsigned char   HByte;  // 0002
    };
    struct {
        unsigned signbit        :1;    // bit1
        unsigned databit        :15;    // bit1
    };
};




#define MOTION_GETXYZ8 	_IOW('V', 0x2, unsigned long)
#define MOTION_DOCAL 	_IOW('V', 0x3, unsigned long)
#define MOTION_GETXYZ 	_IOW('V', 0x4, unsigned long)
#define MOTION_SETCAL 	_IOW('V', 0x5, unsigned long)
#define MOTION_DOCAL_STEP1 	_IOW('V', 0x6, unsigned long)
#define MOTION_DOCAL_STEP2 	_IOW('V', 0x7, unsigned long)


#define MOTACC_PATH		"/etc/accel_caldata"
#define ABS(a)                ((a) > 0 ? (a): -(a))
#define BMA020CAL_SCALE 181
#define BMA020CAL_MAX_OFFSET	64

#endif

