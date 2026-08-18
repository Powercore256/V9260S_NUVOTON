/* ================================================================
 * FILE: V9260S.c
 * MS51FB9AE @ HIRC 24MHz + V9260S
 * Delay can chinh theo datasheet Table 6-1
 * ================================================================ */
#include "MS51_16K.h"
#include "v9260s.h"

/* -- Delay theo datasheet ------------------------------------------
 *  DLY_AFTER_SEND_MS  = tRTD_max(20) + tTF(9.2ms) + margin(5) = 35ms
 *  DLY_BETWEEN_REG_MS = tTRD_min(1)  + margin(5)              =  5ms
 * ------------------------------------------------------------------ */
#define DLY_FLUSH_MS        10   /* Xoa buffer truoc moi lenh - KHONG giam */
#define DLY_AFTER_SEND_MS   35   /* Cho chip xu ly + tra ve response       */
#define DLY_BETWEEN_REG_MS   5   /* Giua 2 lenh lien tiep - KHONG giam     */

/* Timeout cho TxByte: ~5000 vong lap ~ vai ms @ 24MHz
 * Du lon de UART kip gui xong 1 byte, nhung khong treo mai mai */
#define TX_TIMEOUT          50000U

/* ------------------------------------------------------------------ */

static void prv_Dms(unsigned int ms)
{
    unsigned int i; unsigned char j;
    for (i = 0; i < ms; i++) for (j = 0; j < 120; j++);
}

static void prv_RxReset(void)
{
    P0M1 &= ~SET_BIT6;
    P0M2 |=  SET_BIT6;
    P06 = 0;
    prv_Dms(80);
    P06 = 1;
    prv_Dms(35);
}

static void prv_UartInit(void)
{
    TR1 = 0;
    PCON &= ~0x80;
    P0M1 &= ~SET_BIT6; P0M2 |=  SET_BIT6;
    P0M1 &= ~SET_BIT7; P0M2 &= ~SET_BIT7;
    TMOD = (TMOD & 0x0F) | 0x20;
    TH1 = 0xD9; TL1 = 0xD9;
    TR1 = 1;
    SCON = 0xD0;
    TI = 0; RI = 0;
}

/* Tra ve 1 = gui OK, 0 = timeout (UART bi loi) */
static unsigned char prv_TxByte(unsigned char d)
{
    unsigned int t;
    unsigned char p = d;
    p ^= (p >> 4); p ^= (p >> 2); p ^= (p >> 1);
    TB8 = (p & 1) ^ 1;
    SBUF = d;
    for (t = 0; t < TX_TIMEOUT; t++) {
        if (TI) { TI = 0; return 1; }
    }
    return 0; /* timeout */
}

static unsigned char prv_CalcCS(unsigned char *f, unsigned char len)
{
    unsigned char s = 0, i;
    for (i = 0; i < len; i++) s += f[i];
    return (unsigned char)(~s) + 0x33;
}

static void prv_FlushRx(void)
{
    unsigned char dummy;
    prv_Dms(DLY_FLUSH_MS);
    while (RI) { dummy = SBUF; RI = 0; }
    dummy = dummy; /* tranh warning C275 */
}

/* ------------------------------------------------------------------ */

/* Tra ve 1 = OK, 0 = loi TX */
unsigned char V9260S_WriteReg(unsigned int addr, unsigned long val)
{
    unsigned char f[8], i;
    unsigned char addrMSB = (unsigned char)((addr >> 8) & 0x0F);

    prv_FlushRx();
    f[0] = 0x7D;
    f[1] = (addrMSB << 4) | 0x00;
    f[2] = (unsigned char)(addr & 0xFF);
    f[3] = (unsigned char)(val);
    f[4] = (unsigned char)(val >> 8);
    f[5] = (unsigned char)(val >> 16);
    f[6] = (unsigned char)(val >> 24);
    f[7] = prv_CalcCS(f, 7);

    for (i = 0; i < 8; i++) {
        if (!prv_TxByte(f[i])) return 0; /* TX timeout -> thoat ngay */
    }
    prv_Dms(DLY_AFTER_SEND_MS);
    prv_FlushRx();
    return 1;
}

/* Tra ve 1 = doc OK, 0 = loi TX hoac khong nhan du byte */
unsigned char V9260S_ReadReg(unsigned int addr, unsigned long *val)
{
    unsigned char f[8], rx[8], i, n, got;
    unsigned int  t;
    unsigned char j2;
    unsigned char addrMSB = (unsigned char)((addr >> 8) & 0x0F);

    prv_FlushRx();
    f[0] = 0x7D;
    f[1] = (addrMSB << 4) | 0x01;
    f[2] = (unsigned char)(addr & 0xFF);
    f[3] = 0x01;
    f[4] = 0x00; f[5] = 0x00; f[6] = 0x00;
    f[7] = prv_CalcCS(f, 7);

    for (i = 0; i < 8; i++) {
        if (!prv_TxByte(f[i])) return 0; /* TX timeout -> thoat ngay */
    }
    prv_Dms(DLY_AFTER_SEND_MS);

    n = 0;
    while (n < 8) {
        got = 0;
        for (t = 0; t < 3000 && !got; t++)
            for (j2 = 0; j2 < 120; j2++)
                if (RI) { rx[n++] = SBUF; RI = 0; got = 1; break; }
        if (!got) break;
    }

    if (n >= 7 && rx[0] == 0x7D) {
        *val = ((unsigned long)rx[6] << 24)
             | ((unsigned long)rx[5] << 16)
             | ((unsigned long)rx[4] <<  8)
             | (unsigned long)rx[3];
        return 1;
    }
    return 0;
}

/* Tra ve 1 = Init OK, 0 = loi (WriteReg that bai) */
unsigned char V9260S_Init(void)
{
    unsigned long dummy;
    unsigned char ok = 1;

    prv_RxReset();
    prv_UartInit();
    prv_Dms(200);

    /* Doc thu SYSCON de flush pipeline */
    V9260S_ReadReg(V9260S_REG_SYSCON, &dummy);
    prv_Dms(DLY_BETWEEN_REG_MS);

    if (!V9260S_WriteReg(V9260S_REG_SYSCON,  V9260S_SYSCON_VAL))  ok = 0;
    prv_Dms(DLY_BETWEEN_REG_MS);

    if (!V9260S_WriteReg(V9260S_REG_BPFPARA, V9260S_BPFPARA_VAL)) ok = 0;

    prv_Dms(2000); /* Cho chip on dinh - KHONG giam */
    return ok;
}

unsigned char V9260S_ReadAll(V9260S_Data_t *out)
{
    unsigned long raw;
    long          p_signed;
    unsigned long p_abs;

    out->valid = 0;

    if (!V9260S_ReadReg(V9260S_REG_VRMS, &raw)) return 0;
    out->voltage_v = (unsigned int)(raw / V9260S_V_DIV);
    prv_Dms(DLY_BETWEEN_REG_MS);

    if (!V9260S_ReadReg(V9260S_REG_IRMS_A, &raw)) return 0;
    out->current_ma = (unsigned int)(raw / (V9260S_I_DIV / 1000UL));
    prv_Dms(DLY_BETWEEN_REG_MS);

    if (!V9260S_ReadReg(V9260S_REG_POWER_A, &raw)) return 0;
    p_signed            = (long)raw;
    out->power_negative = (p_signed < 0) ? 1 : 0;
    p_abs               = out->power_negative ? (unsigned long)(-p_signed)
                                              : (unsigned long)(p_signed);
    out->power_w        = (unsigned int)(p_abs / V9260S_P_DIV);

    out->valid = 1;
    return 1;
}
