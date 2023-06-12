
#ifndef _SMUBASE_H_
#define _SMUBASE_H_

#include "pic24fj.h"
#include "common.h"
#include <stdint.h>

// LED pin definitions
#define LED1                LATDbits.LATD7
#define LED2                LATFbits.LATF0
#define LED3                LATFbits.LATF1

#define LED1_DIR            TRISDbits.TRISD7
#define LED2_DIR            TRISFbits.TRISF0
#define LED3_DIR            TRISFbits.TRISF1

// Tactile switch pin definitions
#define SW1                 PORTCbits.RC15
#define SW1_DIR             TRISCbits.TRISC15

// Power supply pin definitions
#define ENA12V              LATBbits.LATB3
#define ENA12V_DIR          TRISBbits.TRISB3

// Digital header pin definitions
#define RD0_                LATDbits.LATD0
#define RD1_                LATDbits.LATD1
#define RD2_                LATDbits.LATD2
#define RD3_                LATDbits.LATD3
#define RD4_                LATDbits.LATD4
#define RD5_                LATDbits.LATD5
#define RD6_                LATDbits.LATD6
#define RE0_                LATEbits.LATE0
#define RE1_                LATEbits.LATE1
#define RE2_                LATEbits.LATE2
#define RE3_                LATEbits.LATE3
#define RE4_                LATEbits.LATE4
#define RE5_                LATEbits.LATE5
#define RE6_                LATEbits.LATE6

#define RD0_DIR             TRISDbits.TRISD0
#define RD1_DIR             TRISDbits.TRISD1
#define RD2_DIR             TRISDbits.TRISD2
#define RD3_DIR             TRISDbits.TRISD3
#define RD4_DIR             TRISDbits.TRISD4
#define RD5_DIR             TRISDbits.TRISD5
#define RD6_DIR             TRISDbits.TRISD6
#define RE0_DIR             TRISEbits.TRISE0
#define RE1_DIR             TRISEbits.TRISE1
#define RE2_DIR             TRISEbits.TRISE2
#define RE3_DIR             TRISEbits.TRISE3
#define RE4_DIR             TRISEbits.TRISE4
#define RE5_DIR             TRISEbits.TRISE5
#define RE6_DIR             TRISEbits.TRISE6

// DAC16 (DAC8565) pin definitions
#define DAC_CSN             LATDbits.LATD8
#define DAC_SCK             LATDbits.LATD11
#define DAC_MOSI            LATDbits.LATD10
#define DAC_MISO            PORTDbits.RD9

#define DAC_CSN_DIR         TRISDbits.TRISD8
#define DAC_SCK_DIR         TRISDbits.TRISD11
#define DAC_MOSI_DIR        TRISDbits.TRISD10
#define DAC_MISO_DIR        TRISDbits.TRISD9

#define DAC_CSN_RP          2
#define DAC_SCK_RP          12
#define DAC_MOSI_RP         3
#define DAC_MISO_RP         4

// ADC24 (ADS1292) pin definitions
#define ADC_CSN             LATFbits.LATF5
#define ADC_SCK             LATBbits.LATB15
#define ADC_MOSI            LATFbits.LATF4
#define ADC_MISO            PORTBbits.RB14
#define ADC_START           LATGbits.LATG8
#define ADC_DRDY            PORTFbits.RF3
#define ADC_CLKSEL          LATCbits.LATC12
#define ADC_CLK             LATBbits.LATB4

#define ADC_CSN_DIR         TRISFbits.TRISF5
#define ADC_SCK_DIR         TRISBbits.TRISB15
#define ADC_MOSI_DIR        TRISFbits.TRISF4
#define ADC_MISO_DIR        TRISBbits.TRISB14
#define ADC_START_DIR       TRISGbits.TRISG8
#define ADC_DRDY_DIR        TRISFbits.TRISF3
#define ADC_CLKSEL_DIR      TRISCbits.TRISC12
#define ADC_CLK_DIR         TRISBbits.TRISB4

#define ADC_CSN_RP          17
#define ADC_SCK_RP          29
#define ADC_MOSI_RP         10
#define ADC_MISO_RP         14
#define ADC_START_RP        19
#define ADC_DRDY_RP         16
#define ADC_CLK_RP          28

// BLE module (RN4871) pin definitions
#define BLE_RX_IND          LATBbits.LATB12
#define BLE_RST_N           LATEbits.LATE7
#define BLE_RX              PORTBbits.RB1
#define BLE_TX              PORTBbits.RB2
#define BLE_RTS             PORTBbits.RB5
#define BLE_CTS             PORTGbits.RG6

#define BLE_RX_IND_DIR      TRISBbits.TRISB12
#define BLE_RST_N_DIR       TRISEbits.TRISE7
#define BLE_RX_DIR          TRISBbits.TRISB1
#define BLE_TX_DIR          TRISBbits.TRISB2
#define BLE_RTS_DIR         TRISBbits.TRISB5
#define BLE_CTS_DIR         TRISGbits.TRISG6

#define BLE_RX_RP           1
#define BLE_TX_RP           13
#define BLE_RTS_RP          18
#define BLE_CTS_RP          21

// Peripheral remappable pin definitions
#define INT1_RP             1
#define INT2_RP             2
#define INT3_RP             3
#define INT4_RP             4

#define MOSI1_RP            7
#define SCK1OUT_RP          8
#define MOSI2_RP            10
#define SCK2OUT_RP          11

#define MISO1_RP            40
#define SCK1IN_RP           41
#define MISO2_RP            44
#define SCK2IN_RP           45

#define OC1_RP              18
#define OC2_RP              19
#define OC3_RP              20
#define OC4_RP              21
#define OC5_RP              22
#define OC6_RP              23
#define OC7_RP              24
#define OC8_RP              25
#define OC9_RP              35

#define U1TX_RP             3
#define U1RTS_RP            4
#define U2TX_RP             5
#define U2RTS_RP            6
#define U3TX_RP             28
#define U3RTS_RP            29
#define U4TX_RP             30
#define U4RTS_RP            31

#define U1RX_RP             36
#define U1CTS_RP            37
#define U2RX_RP             38
#define U2CTS_RP            39
#define U3RX_RP             35
#define U3CTS_RP            43
#define U4RX_RP             54
#define U4CTS_RP            55

// Convenience boolean values definitions
#define FALSE               0
#define TRUE                1

#define OFF                 0
#define ON                  1

#define OUT                 0
#define IN                  1

// ADS1292 SPI commands
#define ADC24_CMD_WAKEUP    0x02
#define ADC24_CMD_STANDBY   0x04
#define ADC24_CMD_RESET     0x06
#define ADC24_CMD_START     0x08
#define ADC24_CMD_STOP      0x0A
#define ADC24_CMD_OFFSETCAL 0x1A
#define ADC24_CMD_RDATAC    0x10
#define ADC24_CMD_SDATAC    0x11
#define ADC24_CMD_RDATA     0x12
#define ADC24_CMD_RREG      0x20
#define ADC24_CMD_WREG      0x40

// ADS1292 register definitions
#define ADC24_REG_ID        0x00
#define ADC24_REG_CONFIG1   0x01
#define ADC24_REG_CONFIG2   0x02
#define ADC24_REG_LOFF      0x03
#define ADC24_REG_CH1SET    0x04
#define ADC24_REG_CH2SET    0x05
#define ADC24_REG_RLD_SENS  0x06
#define ADC24_REG_LOFF_SENS 0x07
#define ADC24_REG_LOFF_STAT 0x08
#define ADC24_REG_RESP1     0x09
#define ADC24_REG_RESP2     0x0A
#define ADC24_REG_GPIO      0x0B

#define U1TX_BUFFER_LENGTH  1024
#define U1RX_BUFFER_LENGTH  1024

typedef struct {
    uint8_t *data;
    uint16_t length;
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} RINGBUFFER;

extern RINGBUFFER U1TXbuffer, U1RXbuffer;
extern uint8_t U1TX_buffer[];
extern uint8_t U1RX_buffer[];
extern uint16_t U1TXthreshold;

void init_smu_base(void);

void init_adc16(void);
void adc16_calibrate(void);
int16_t adc16_meas_ch1_raw(void);
int16_t adc16_meas_ch2_raw(void);
int16_t adc16_meas_ch1(void);
int16_t adc16_meas_ch2(void);
int16_t adc16_meas_ch1_avg(void);
int16_t adc16_meas_ch2_avg(void);
int16_t adc16_get_offset(void);
uint16_t adc16_get_max_val(void);

void init_dac16(void);
uint16_t dac16_get_dac0(void);
void dac16_set_dac0(uint16_t val);
uint16_t dac16_get_dac1(void);
void dac16_set_dac1(uint16_t val);
uint16_t dac16_get_dac2(void);
void dac16_set_dac2(uint16_t val);
uint16_t dac16_get_dac3(void);
void dac16_set_dac3(uint16_t val);
void dac16_set_ch1(uint16_t pos, uint16_t neg);
void dac16_set_ch2(uint16_t pos, uint16_t neg);

void init_adc24(void);
void adc24_calibrate(void);
void adc24_command(uint8_t cmd);
void adc24_write_reg(uint8_t reg, uint8_t val);
uint8_t adc24_read_reg(uint8_t reg);
void adc24_read_data(int32_t *ch1val, int32_t *ch2val);
void adc24_meas_both(int32_t *ch1val, int32_t *ch2val);
void adc24_meas_both_avg(int32_t *ch1val, int32_t *ch2val);
void adc24_meas_both_raw(int32_t *ch1val, int32_t *ch2val);
void adc24_set_ch1offset(int32_t val);
int32_t adc24_get_ch1offset(void);
void adc24_set_ch2offset(int32_t val);
int32_t adc24_get_ch2offset(void);

void init_ble(void);
uint16_t ble_in_waiting(void);
void ble_putc(uint8_t ch);
uint8_t ble_getc(void);
void ble_puts(uint8_t *str);

uint16_t dummy_in_waiting(void);
void dummy_putc(uint8_t ch);
uint8_t dummy_getc(void);
void dummy_puts(uint8_t *str);

uint16_t U1inWaiting(void);
void U1flushTxBuffer(void);
void U1putc(uint8_t ch);
uint8_t U1getc(void);
void U1puts(uint8_t *str);
void U1gets(uint8_t *str, uint16_t len);
void U1gets_term(uint8_t *str, uint16_t len);

#endif

