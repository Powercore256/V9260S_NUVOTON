#ifndef V9260S_H
#define V9260S_H

/*===========================================================================
 * v9260s.h
 * MS51FB9AE @ HIRC 24MHz + V9260S
 *===========================================================================*/

/* --- Dia chi thanh ghi --- */
#define V9260S_REG_SYSCON   0x0180
#define V9260S_REG_BPFPARA  0x0107
#define V9260S_REG_VRMS     0x00D4
#define V9260S_REG_IRMS_A   0x00D3
#define V9260S_REG_POWER_A  0x00D0

/* --- Gia tri khoi tao --- */
#define V9260S_SYSCON_VAL   0x38000000UL
#define V9260S_BPFPARA_VAL  0x806764B6UL

/* --- He so chia (da calibrate) --- */
#define V9260S_V_DIV        2254786UL
#define V9260S_I_DIV        48296503UL
#define V9260S_P_DIV        101378UL

/* --- Struct ket qua --- */
typedef struct {
    unsigned int  voltage_v;      /* Dien ap (V)    */
    unsigned int  current_ma;     /* Dong dien (mA) */
    unsigned int  power_w;        /* Cong suat (W)  */
    unsigned char power_negative; /* 1 = am         */
    unsigned char valid;          /* 1 = du lieu OK */
} V9260S_Data_t;

/* --- API --- */
/* Tra ve 1 = OK, 0 = loi (UART timeout / chip khong phan hoi) */
unsigned char V9260S_Init    (void);
unsigned char V9260S_WriteReg(unsigned int addr, unsigned long val);
unsigned char V9260S_ReadReg (unsigned int addr, unsigned long *val);
unsigned char V9260S_ReadAll (V9260S_Data_t *out);

#endif /* V9260S_H */
