#include "smu_base.h"

int16_t adc16_offset;
int32_t adc16_max_val;

uint16_t dac16_dac0, dac16_dac1, dac16_dac2, dac16_dac3;

int32_t adc24_ch1offset, adc24_ch2offset;

RINGBUFFER U1TXbuffer, U1RXbuffer;
uint8_t U1TX_buffer[U1TX_BUFFER_LENGTH];
uint8_t U1RX_buffer[U1RX_BUFFER_LENGTH];
uint16_t U1TXthreshold;

void init_smu_base(void) {
    CLKDIV = 0x0100;        // RCDIV = 001 (4MHz, div2),
                            // CPDIV = 00 (FOSC = 32MHz, FCY = 16MHz)

    OSCTUN = 0x9000;        // enable FRC self tuning with USB host clock

    // Make all pins digital I/Os
    ANSB = 0;
    ANSC = 0;
    ANSD = 0;
    ANSF = 0;
    ANSG = 0;

    ANSBbits.ANSB0 = 1;     // configure RB0 (AN0) for analog function
    TRISBbits.TRISB0 = 1;   // tristate RB0's output driver

    ANSGbits.ANSG9 = 1;     // configure RG9 (DAC1) for analog function
    TRISGbits.TRISG9 = 1;   // tristate RG9's output driver
    DAC1CON = 0x8081;       // enable DAC1, no trigger, and reference is DREF+
    DAC1DAT = 0;

    ANSBbits.ANSB13 = 1;    // configure RB13 (DAC2) for analog function
    TRISBbits.TRISB13 = 1;  // tristate RB13's output driver
    DAC2CON = 0x8081;       // enable DAC2, no trigger, and reference is DREF+
    DAC2DAT = 0;

    // Configure LED pins as outputs, set to low (off)
    LED1 = OFF; LED1_DIR = OUT;
    LED2 = OFF; LED2_DIR = OUT;
    LED3 = OFF; LED3_DIR = OUT;

    // Configure SW pin as inputs
    SW1_DIR = IN;

    // Configure ENA12V pin as an output, set to low (off)
    ENA12V = OFF; ENA12V_DIR = OUT;

    // Configure digital header pins as outputs, set to low
    RD0_DIR = OUT; RD0_ = 0;
    RD1_DIR = OUT; RD1_ = 0;
    RD2_DIR = OUT; RD2_ = 0;
    RD3_DIR = OUT; RD3_ = 0;
    RD4_DIR = OUT; RD4_ = 0;
    RD5_DIR = OUT; RD5_ = 0;
    RD6_DIR = OUT; RD6_ = 0;

    RE0_DIR = OUT; RE0_ = 0;
    RE1_DIR = OUT; RE1_ = 0;
    RE2_DIR = OUT; RE2_ = 0;
    RE3_DIR = OUT; RE3_ = 0;
    RE4_DIR = OUT; RE4_ = 0;
    RE5_DIR = OUT; RE5_ = 0;
    RE6_DIR = OUT; RE6_ = 0;

    init_adc16();
    init_dac16();
    init_adc24();
    init_ble();
}

// Functions for measuring with the 16-bit sigma-delta ADC
void init_adc16() {
    // Configure 16-bit sigma-delta ADC for a data rate of 0.9765625 kS/s
    // (i.e., sample clock frequency of 4 MHz and an OSR of 1024x), which is 
    // a sample interval of 1.024ms.  Note that averaging 16 consecutive 
    // samples should reslut in a partial cancellation of 60-Hz noise.
    //
    SD1CON1 = 0x00D1;       // configure sigma-delta ADC with
                            //     SDGAIN = 000 (1x)
                            //     DITHER = 11 (high dither)
                            //     VOSCAL = 1 (measure offset error)
                            //     SDREFN = 0 (SVSS is neg ref)
                            //     SDREFP = 0 (SVDD is pos ref)
                            //     PWRLVL = 1 (2x bandwidth)
    SD1CON2 = 0xF110;       //     CHOP = 11 (chopping enabled)
                            //     SDINT = 11 (int on every samp clock)
                            //     SDWM = 01 (result updated every int)
                            //     RNDRES = 10 (round result to 16 bits)
    SD1CON3 = 0x4000;       //     SDDIV = 010 (clock divider is 4)
                            //     SDOSR = 000 (OSR is 1024)
                            //     SDCS = 00 (clock src is sys clock, FCY)
                            //     SDCH = 000 (select CH0)

    SD1CON1bits.SDON = 1;

    adc16_calibrate();
}

void adc16_calibrate(void) {
    uint16_t i;
    int32_t offset;

    // Configure sigma-delta ADC for offset calibration
    SD1CON1bits.VOSCAL = 1;

    // Measure sigma-delta ADC internal offset
    for (i = 0; i < 5; i++) {
        IFS6bits.SDA1IF = 0;
        while (IFS6bits.SDA1IF == 0) {}
    }
    offset = (int32_t)SD1RESH;
    for (i = 0; i < 15; i++) {
        IFS6bits.SDA1IF = 0;
        while (IFS6bits.SDA1IF == 0) {}
        offset += (int32_t)SD1RESH;
    }
    offset = offset / 16;
    adc16_offset = (int16_t)offset;

    // Configure sigma-delta ADC for normal operation
    SD1CON1bits.VOSCAL = 0;

    // Measure the sigma-delta ADC positive reference for gain calibration
    SD1CON3bits.SDCH = 3;
    for (i = 0; i < 5; i++) {
        IFS6bits.SDA1IF = 0;
        while (IFS6bits.SDA1IF == 0) {}
    }
    adc16_max_val = (int32_t)SD1RESH - (int32_t)adc16_offset;

    // Configure sigma-delta ADC to measure CH0
    SD1CON3bits.SDCH = 0;
}

int16_t adc16_meas_ch1_raw(void) {
    uint16_t i;

    SD1CON3bits.SDCH = 0;
    for (i = 0; i < 5; i++) {
        IFS6bits.SDA1IF = 0;
        while (IFS6bits.SDA1IF == 0) {}
    }
    return (int16_t)SD1RESH;
}

int16_t adc16_meas_ch2_raw(void) {
    uint16_t i;

    SD1CON3bits.SDCH = 1;
    for (i = 0; i < 5; i++) {
        IFS6bits.SDA1IF = 0;
        while (IFS6bits.SDA1IF == 0) {}
    }
    return (int16_t)SD1RESH;
}

int16_t adc16_meas_ch1(void) {
    int32_t val;
    uint16_t i;

    SD1CON3bits.SDCH = 0;
    for (i = 0; i < 5; i++) {
        IFS6bits.SDA1IF = 0;
        while (IFS6bits.SDA1IF == 0) {}
    }
    val = (int32_t)SD1RESH - (int32_t)adc16_offset;
//    val = ((int32_t)32767 * val) / adc16_max_val;

    return (int16_t)val;
}

int16_t adc16_meas_ch2(void) {
    int32_t val;
    uint16_t i;

    SD1CON3bits.SDCH = 1;
    for (i = 0; i < 5; i++) {
        IFS6bits.SDA1IF = 0;
        while (IFS6bits.SDA1IF == 0) {}
    }
    val = (int32_t)SD1RESH - (int32_t)adc16_offset;
//    val = ((int32_t)32767 * val) / adc16_max_val;

    return (int16_t)val;
}

int16_t adc16_meas_ch1_avg16(void) {
    int32_t val;
    uint16_t i;

    SD1CON3bits.SDCH = 0;
    for (i = 0; i < 5; i++) {
        IFS6bits.SDA1IF = 0;
        while (IFS6bits.SDA1IF == 0) {}
    }
    val = (int32_t)SD1RESH;
    for (i = 0; i < 15; i++) {
        IFS6bits.SDA1IF = 0;
        while (IFS6bits.SDA1IF == 0) {}
        val += (int32_t)SD1RESH;
    }
    val = val / 16;
    val -= (int32_t)adc16_offset;
//    val = ((int32_t)32767 * val) / adc16_max_val;

    return (int16_t)val;
}

int16_t adc16_meas_ch2_avg16(void) {
    int32_t val;
    uint16_t i;

    SD1CON3bits.SDCH = 1;
    for (i = 0; i < 5; i++) {
        IFS6bits.SDA1IF = 0;
        while (IFS6bits.SDA1IF == 0) {}
    }
    val = (int32_t)SD1RESH;
    for (i = 0; i < 15; i++) {
        IFS6bits.SDA1IF = 0;
        while (IFS6bits.SDA1IF == 0) {}
        val += (int32_t)SD1RESH;
    }
    val = val / 16;
    val -= (int32_t)adc16_offset;
//    val = ((int32_t)32767 * val) / adc16_max_val;

    return (int16_t)val;
}

int16_t adc16_get_offset(void) {
    return adc16_offset;
}

uint16_t adc16_get_max_val(void) {
    return (uint16_t)adc16_max_val;
}

// Functions for interfacing with the quad 16-bit DAC (DAC8564)
void init_dac16(void) {
    uint8_t *RPOR, *RPINR;

    // Configure DAC16 pins and SPI peripheral (SPI1)
    DAC_CSN_DIR = OUT; DAC_CSN = 1;
    DAC_SCK_DIR = OUT; DAC_SCK = 1;
    DAC_MOSI_DIR = OUT; DAC_MOSI = 0;
    DAC_MISO_DIR = IN;

    RPOR = (uint8_t *)&RPOR0;
    RPINR = (uint8_t *)&RPINR0;

    __builtin_write_OSCCONL(OSCCON & 0xBF);
    RPINR[MISO1_RP] = DAC_MISO_RP;
    RPOR[DAC_MOSI_RP] = MOSI1_RP;
    RPOR[DAC_SCK_RP] = SCK1OUT_RP;
    __builtin_write_OSCCONL(OSCCON | 0x40);

    SPI1CON1 = 0x017B;      // SPI1 mode = 2, SCK freq = 2 MHz
    SPI1CON2 = 0;
    SPI1STAT = 0x8000;

    dac16_dac0 = 0;
    dac16_dac1 = 0;
    dac16_dac2 = 0;
    dac16_dac3 = 0;
}

uint16_t dac16_get_dac0(void) {
    return dac16_dac0;
}

void dac16_set_dac0(uint16_t val) {
    uint16_t temp;

    dac16_dac0 = val;

    DAC_CSN = 0;

    // Write to buffer with data and load DAC0
    SPI1BUF = 0b00010000;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write high byte of DAC0 value
    SPI1BUF = dac16_dac0 >> 8;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write low byte of DAC0 value
    SPI1BUF = dac16_dac0 & 0xFF;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    DAC_CSN = 1;
}

uint16_t dac16_get_dac1(void) {
    return dac16_dac1;
}

void dac16_set_dac1(uint16_t val) {
    uint16_t temp;

    dac16_dac1 = val;

    DAC_CSN = 0;

    // Write to buffer with data and load DAC1
    SPI1BUF = 0b00010010;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write high byte of DAC1 value
    SPI1BUF = dac16_dac1 >> 8;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write low byte of DAC1 value
    SPI1BUF = dac16_dac1 & 0xFF;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    DAC_CSN = 1;
}

uint16_t dac16_get_dac2(void) {
    return dac16_dac2;
}

void dac16_set_dac2(uint16_t val) {
    uint16_t temp;

    dac16_dac2 = val;

    DAC_CSN = 0;

    // Write to buffer with data and load DAC2
    SPI1BUF = 0b00010100;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write high byte of DAC2 value
    SPI1BUF = dac16_dac2 >> 8;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write low byte of DAC2 value
    SPI1BUF = dac16_dac2 & 0xFF;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    DAC_CSN = 1;
}

uint16_t dac16_get_dac3(void) {
    return dac16_dac3;
}

void dac16_set_dac3(uint16_t val) {
    uint16_t temp;

    dac16_dac3 = val;

    DAC_CSN = 0;

    // Write to buffer with data and load DAC3
    SPI1BUF = 0b00010110;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write high byte of DAC3 value
    SPI1BUF = dac16_dac3 >> 8;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write low byte of DAC3 value
    SPI1BUF = dac16_dac3 & 0xFF;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    DAC_CSN = 1;
}

void dac16_set_ch1(uint16_t pos, uint16_t neg) {
    uint16_t temp;

    dac16_dac0 = neg;

    DAC_CSN = 0;

    // Write to buffer 0 with data
    SPI1BUF = 0b00000000;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write high byte of DAC0 value
    SPI1BUF = dac16_dac0 >> 8;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write low byte of DAC0 value
    SPI1BUF = dac16_dac0 & 0xFF;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    DAC_CSN = 1;

    dac16_dac1 = pos;

    DAC_CSN = 0;

    // Write to buffer 1 with data and load all DACs simultaneously
    SPI1BUF = 0b00100010;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write high byte of DAC1 value
    SPI1BUF = dac16_dac1 >> 8;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write low byte of DAC1 value
    SPI1BUF = dac16_dac1 & 0xFF;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    DAC_CSN = 1;
}

void dac16_set_ch2(uint16_t pos, uint16_t neg) {
    uint16_t temp;

    dac16_dac2 = neg;

    DAC_CSN = 0;

    // Write to buffer 2 with data
    SPI1BUF = 0b00000100;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write high byte of DAC2 value
    SPI1BUF = dac16_dac2 >> 8;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write low byte of DAC2 value
    SPI1BUF = dac16_dac2 & 0xFF;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    DAC_CSN = 1;

    dac16_dac3 = pos;

    DAC_CSN = 0;

    // Write to buffer 3 with data and load all DACs simultaneously
    SPI1BUF = 0b00100110;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write high byte of DAC3 value
    SPI1BUF = dac16_dac3 >> 8;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    // Write low byte of DAC3 value
    SPI1BUF = dac16_dac3 & 0xFF;
    while (SPI1STATbits.SPIRBF == 0) {}
    temp = SPI1BUF;

    DAC_CSN = 1;
}

// Functions for interfacing with the 2-channel, 24-bit sigma-delta ADC (ADS1292)
void init_adc24(void) {
    uint8_t *RPOR, *RPINR;
    uint16_t i;

    // Configure ADC24 pins, SPI peripheral (SPI2), and clock (OC1)
    ADC_CSN_DIR = OUT; ADC_CSN = 1;
    ADC_SCK_DIR = OUT; ADC_SCK = 0;
    ADC_MOSI_DIR = OUT; ADC_MOSI = 0;
    ADC_MISO_DIR = IN;
    ADC_START_DIR = OUT; ADC_START = 0;
    ADC_DRDY_DIR = IN;
    ADC_CLKSEL_DIR = OUT; ADC_CLKSEL = 0;
    ADC_CLK_DIR = OUT; ADC_CLK = 0;

    RPOR = (uint8_t *)&RPOR0;
    RPINR = (uint8_t *)&RPINR0;

    __builtin_write_OSCCONL(OSCCON & 0xBF);
    RPINR[MISO2_RP] = ADC_MISO_RP;
    RPOR[ADC_MOSI_RP] = MOSI2_RP;
    RPOR[ADC_SCK_RP] = SCK2OUT_RP;
    __builtin_write_OSCCONL(OSCCON | 0x40);

    SPI2CON1 = 0x0032;      // SPI2 mode = 1, SCK freq = 1 MHz
    SPI2CON2 = 0;
    SPI2STAT = 0x8000;

    __builtin_write_OSCCONL(OSCCON & 0xBF);
    RPOR[ADC_CLK_RP] = OC1_RP;
    __builtin_write_OSCCONL(OSCCON | 0x40);

    OC1CON1 = 0x1C06;       // Configure OC1 to produce a 551.7 kHz, 50% duty
    OC1CON2 = 0x001F;       //   cycle PWM output.  With OSR = 256, we get a 
    OC1RS = 28;             //   sample rate of 538.8 S/s and 9 sample times 
    OC1R = 14;              //   is very close to 16.67 ms (i.e., 1/60 s).

    adc24_ch1offset = 0;
    adc24_ch2offset = 0;

    // Wait for 20 ms to allow ADS1292 to start up
    for (i = 64000; i; i--) {}

    adc24_command(ADC24_CMD_RESET);

    // Wait for 36 clock periods (9 TMOD = 65.25 µs) for reset to complete
    for (i = 208; i; i--) {}

    adc24_command(ADC24_CMD_SDATAC);

    // Configure for continuous conversion mode, OSR = 256 (about 500 S/s)
    adc24_write_reg(ADC24_REG_CONFIG1, 0b00000010);

    // Set GPIO pins as outputs with low values
    adc24_write_reg(ADC24_REG_GPIO, 0b00000000);

    // Configure CH1 for normal operation, PGA gain = 1
    adc24_write_reg(ADC24_REG_CH1SET, 0b000010000);

    // Configure CH2 for normal operation, PGA gain = 1
    adc24_write_reg(ADC24_REG_CH2SET, 0b000010000);

    adc24_calibrate();
}

void adc24_calibrate(void) {
    uint16_t i;
    int32_t ch1val, ch2val;

    // Configure CH1 for normal operation, PGA gain = 1, inputs shorted to
    //   measure CH1 offset
    adc24_write_reg(ADC24_REG_CH1SET, 0b000010001);

    // Configure CH2 for normal operation, PGA gain = 1, inputs shorted to
    //   measure CH2 offset
    adc24_write_reg(ADC24_REG_CH2SET, 0b000010001);

    // Measure the offset of both channels
    adc24_ch1offset = 0;
    adc24_ch2offset = 0;

    ADC_START = 1;

    while (ADC_DRDY == 1) {}
    adc24_read_data(&ch1val, &ch2val);

    for (i = 0; i < 9; i++) {
        while (ADC_DRDY == 1) {}
        adc24_read_data(&ch1val, &ch2val);

        adc24_ch1offset += ch1val;
        adc24_ch2offset += ch2val;
    }

    ADC_START = 0;

    adc24_ch1offset = adc24_ch1offset / 9;
    adc24_ch2offset = adc24_ch2offset / 9;

    // Configure CH1 for normal operation, PGA gain = 1
    adc24_write_reg(ADC24_REG_CH1SET, 0b000010000);

    // Configure CH2 for normal operation, PGA gain = 1
    adc24_write_reg(ADC24_REG_CH2SET, 0b000010000);
}

void adc24_command(uint8_t cmd) {
    uint16_t temp, i;

    ADC_CSN = 0;

    SPI2BUF = (uint16_t)cmd;
    while (SPI2STATbits.SPIRBF == 0) {}
    temp = SPI2BUF;

    // Delay for 4 clock periods (7.25 µs) before raising CSN
    for (i = 22; i; i--) {}

    ADC_CSN = 1;

    // Delay for 3 clock periods (5.5 µs) to meet minimum CSN high time
    for (i = 16; i; i--) {}
}

void adc24_write_reg(uint8_t reg, uint8_t val) {
    uint16_t temp, i;

    ADC_CSN = 0;

    SPI2BUF = (uint16_t)(ADC24_CMD_WREG | reg);
    while (SPI2STATbits.SPIRBF == 0) {}
    temp = SPI2BUF;

    // Delay for 4 clock periods (7.25 µs) for multibyte instruction decode
    for (i = 22; i; i--) {}

    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    temp = SPI2BUF;

    // Delay for 4 clock periods (7.25 µs) for multibyte instruction decode
    for (i = 22; i; i--) {}

    SPI2BUF = (uint16_t)val;
    while (SPI2STATbits.SPIRBF == 0) {}
    temp = SPI2BUF;

    // Delay for 4 clock periods (7.25 µs) before raising CSN
    for (i = 22; i; i--) {}

    ADC_CSN = 1;

    // Delay for 3 clock periods (5.5 µs) to meet minimum CSN high time
    for (i = 16; i; i--) {}
}

uint8_t adc24_read_reg(uint8_t reg) {
    uint16_t temp, i;

    ADC_CSN = 0;

    SPI2BUF = (uint16_t)(ADC24_CMD_RREG | reg);
    while (SPI2STATbits.SPIRBF == 0) {}
    temp = SPI2BUF;

    // Delay for 4 clock periods (7.25 µs) for multibyte instruction decode
    for (i = 22; i; i--) {}

    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    temp = SPI2BUF;

    // Delay for 4 clock periods (7.25 µs) for multibyte instruction decode
    for (i = 22; i; i--) {}

    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    temp = SPI2BUF;

    // Delay for 4 clock periods (7.25 µs) before raising CSN
    for (i = 22; i; i--) {}

    ADC_CSN = 1;

    // Delay for 3 clock periods (5.5 µs) to meet minimum CSN high time
    for (i = 16; i; i--) {}

    return (uint8_t)temp;
}

void adc24_read_data(int32_t *ch1val, int32_t *ch2val) {
    uint16_t temp, i;
    int32_t val1, val2;

    ADC_CSN = 0;

    // Send the RDATA command
    SPI2BUF = (uint16_t)ADC24_CMD_RDATA;
    while (SPI2STATbits.SPIRBF == 0) {}
    temp = SPI2BUF;

    // Read three bytes of status and discard
    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    temp = SPI2BUF;

    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    temp = SPI2BUF;

    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    temp = SPI2BUF;

    // Read 24-bit CH1 value
    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    val1 = (int32_t)SPI2BUF;

    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    val1 = (val1 << 8) | SPI2BUF;

    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    val1 = (val1 << 8) | SPI2BUF;

    // Read 24-bit CH2 value
    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    val2 = (int32_t)SPI2BUF;

    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    val2 = (val2 << 8) | SPI2BUF;

    SPI2BUF = 0;
    while (SPI2STATbits.SPIRBF == 0) {}
    val2 = (val2 << 8) | SPI2BUF;

    // Delay for 4 clock periods (7.25 µs) before raising CSN
    for (i = 22; i; i--) {}

    ADC_CSN = 1;

    // Delay for 3 clock periods (5.5 µs) to meet minimum CSN high time
    for (i = 16; i; i--) {}

    // Sign extend CH1 value to 32 bits
    if (val1 > 0x7FFFFF)
        val1 |= 0xFF000000;

    // Sign extend CH2 value to 32 bits
    if (val2 > 0x7FFFFF)
        val2 |= 0xFF000000;

    *ch1val = val1;
    *ch2val = val2;
}

void adc24_meas_both(int32_t *ch1val, int32_t *ch2val) {
    int32_t val1, val2;

    ADC_START = 1;

    while (ADC_DRDY == 1) {}
    adc24_read_data(&val1, &val2);

    while (ADC_DRDY == 1) {}
    adc24_read_data(&val1, &val2);

    ADC_START = 0;

    *ch1val = val1 - adc24_ch1offset;
    *ch2val = val2 - adc24_ch2offset;
}

void adc24_meas_both_avg9(int32_t *ch1val, int32_t *ch2val) {
    uint16_t i;
    int32_t val1, val2;

    *ch1val = 0;
    *ch2val = 0;

    ADC_START = 1;

    while (ADC_DRDY == 1) {}
    adc24_read_data(&val1, &val2);

    for (i = 0; i < 9; i++) {
        while (ADC_DRDY == 1) {}
        adc24_read_data(&val1, &val2);

        *ch1val += val1;
        *ch2val += val2;
    }

    ADC_START = 0;

    *ch1val = (*ch1val / 9) - adc24_ch1offset;
    *ch2val = (*ch2val / 9) - adc24_ch2offset;
}

void adc24_meas_both_raw(int32_t *ch1val, int32_t *ch2val) {
    int32_t val1, val2;

    ADC_START = 1;

    while (ADC_DRDY == 1) {}
    adc24_read_data(&val1, &val2);

    while (ADC_DRDY == 1) {}
    adc24_read_data(&val1, &val2);

    ADC_START = 0;

    *ch1val = val1;
    *ch2val = val2;
}

void adc24_set_ch1offset(int32_t val) {
    adc24_ch1offset = val;
}

int32_t adc24_get_ch1offset(void) {
    return adc24_ch1offset;
}

void adc24_set_ch2offset(int32_t val) {
    adc24_ch2offset = val;
}

int32_t adc24_get_ch2offset(void) {
    return adc24_ch2offset;
}

// Functions relating to the BLE module (RN4871)
void init_ble(void) {
    uint8_t *RPOR, *RPINR;
    uint16_t i;

    RPOR = (uint8_t *)&RPOR0;
    RPINR = (uint8_t *)&RPINR0;

    // Configure BLE module pins
    BLE_RST_N_DIR = OUT; BLE_RST_N = 1;
    BLE_RX_DIR = OUT; BLE_RX = 1;
    BLE_TX_DIR = IN;
//    BLE_CTS_DIR = OUT; BLE_CTS = 1;
//    BLE_RTS_DIR = IN;

    // Configure BLE pins to use UART1
    __builtin_write_OSCCONL(OSCCON & 0xBF);
    RPINR[U1RX_RP] = BLE_TX_RP;
    RPOR[BLE_RX_RP] = U1TX_RP;
//    RPINR[U1CTS_RP] = BLE_RTS_RP;
//    RPOR[BLE_CTS_RP] = U1RTS_RP;
    __builtin_write_OSCCONL(OSCCON | 0x40);

    U1MODE = 0x0008;            // configure UART1 for transmission at
    U1BRG = 0x0022;             //   115,200 baud, no parity, 1 stop bit
//    U1MODE = 0x0208;            // configure UART1 for transmission at
//    U1BRG = 0x0022;             //   115,200 baud, no parity, 1 stop bit
//                                //   with hardware flow control

    U1TXbuffer.data = U1TX_buffer;
    U1TXbuffer.length = U1TX_BUFFER_LENGTH;
    U1TXbuffer.head = 0;
    U1TXbuffer.tail = 0;
    U1TXbuffer.count = 0;
    U1TXthreshold = 3 * U1TX_BUFFER_LENGTH / 4;

    U1RXbuffer.data = U1RX_buffer;
    U1RXbuffer.length = U1RX_BUFFER_LENGTH;
    U1RXbuffer.head = 0;
    U1RXbuffer.tail = 0;
    U1RXbuffer.count = 0;

    U1STAbits.UTXISEL1 = 0;     // set UART1 UTXISEL<1:0> = 01, TX interrupt
    U1STAbits.UTXISEL0 = 1;     //   when all transmit operations are done

    IFS0bits.U1TXIF = 0;        // lower UART1 TX interrupt flag
    IEC0bits.U1TXIE = 1;        // enable UART1 TX interrupt

    IFS0bits.U1RXIF = 0;        // lower UART1 RX interrupt flag
    IEC0bits.U1RXIE = 1;        // enable UART1 RX interrupt

    U1MODEbits.UARTEN = 1;      // enable UART1 module
    U1STAbits.UTXEN = 1;        // enable UART1 data transmission

    BLE_RST_N = 0;
    for (i = 1000; i; i--) {}
    BLE_RST_N = 1;
}

void __attribute__((interrupt, auto_psv)) _U1TXInterrupt(void) {
    uint8_t ch;

    IFS0bits.U1TXIF = 0;            // lower UART1 TX interrupt flag

    if (U1TXbuffer.count == 0)      // if nothing left in UART1 TX buffer, 
        U1STAbits.UTXEN = 0;        //   disable data transmission

    while ((U1STAbits.UTXBF == 0) && (U1TXbuffer.count != 0)) {
        disable_interrupts();
        ch = U1TXbuffer.data[U1TXbuffer.head];
        U1TXbuffer.head++;
        if (U1TXbuffer.head == U1TXbuffer.length)
            U1TXbuffer.head = 0;
        U1TXbuffer.count--;
        enable_interrupts();
        U1TXREG = (uint16_t)ch;
    }
}

void __attribute__((interrupt, auto_psv)) _U1RXInterrupt(void) {
    IFS0bits.U1RXIF = 0;            // lower UART1 RX interrupt flag

    while ((U1STAbits.URXDA == 1) && (U1RXbuffer.count != U1RXbuffer.length)) {
        disable_interrupts();
        U1RXbuffer.data[U1RXbuffer.tail] = (uint8_t)U1RXREG;
        U1RXbuffer.tail++;
        if (U1RXbuffer.tail == U1RXbuffer.length)
            U1RXbuffer.tail = 0;
        U1RXbuffer.count++;
        enable_interrupts();
    }
}

uint16_t U1inWaiting(void) {
    return U1RXbuffer.count;
}

void U1flushTxBuffer(void) {
    if (U1STAbits.UTXEN == 0)       // if UART1 transmission is disabled,
        U1STAbits.UTXEN = 1;        //   enable it
}

void U1putc(uint8_t ch) {
    while (U1TXbuffer.count == U1TXbuffer.length) {}    // wait until UART1 TX 
                                                        //   buffer is not full
    disable_interrupts();
    U1TXbuffer.data[U1TXbuffer.tail] = ch;
    U1TXbuffer.tail++;
    if (U1TXbuffer.tail == U1TXbuffer.length)
        U1TXbuffer.tail = 0;
    U1TXbuffer.count++;
    enable_interrupts();

    if (U1TXbuffer.count >= U1TXthreshold)          // if UART1 TX buffer is 
        U1STAbits.UTXEN = 1;                        //   full enough, enable 
                                                    //   data transmission
}

uint8_t U1getc(void) {
    uint8_t ch;

    while (U1RXbuffer.count == 0) {}    // wait until UART1 RX buffer is not empty

    disable_interrupts();
    ch = U1RXbuffer.data[U1RXbuffer.head];
    U1RXbuffer.head++;
    if (U1RXbuffer.head == U1RXbuffer.length)
        U1RXbuffer.head = 0;
    U1RXbuffer.count--;
    enable_interrupts();

    return ch;
}

void U1puts(uint8_t *str) {
    while (*str)
        U1putc(*str++);
    U1flushTxBuffer();
}

void U1gets(uint8_t *str, uint16_t len) {
    if (len == 0)
        return;

    if (len == 1) {
        *str = '\0';
        return;
    }

    while (1) {
        *str = U1getc();
        if ((*str == '\r') || (len == 1))
            break;
        str++;
        len--;
    }
    *str = '\0';
}

void U1gets_term(uint8_t *str, uint16_t len) {
    uint8_t *start;
    uint16_t left;

    if (len == 0)
        return;

    if (len == 1) {
        *str = '\0';
        return;
    }

    U1putc(0x1B);                           // save current cursor position
    U1putc('7');
    U1flushTxBuffer();
    start = str;
    left = len;
    while (1) {
        *str = U1getc();                    // get a character
        if (*str == '\r')                   // if character is return,
            break;                          //   end the loop
        if (*str == 0x1B) {                 // if character is escape,
            U1putc(0x1B);                   //   restore cursor position,
            U1putc('8');
            U1putc(0x1B);                   //   clear to end of line, and
            U1putc('[');
            U1putc('K');
            U1flushTxBuffer();
            str = start;                    //   start over at the beginning
            left = len;
            continue;
        }
        if ((*str == '\b') ||               // if character is backspace
            (*str == 0x7F)) {               //   or delete, 
            if (str > start) {              //   and we are not at the start, 
                U1putc('\b');               //   erase the last character and
                U1putc(' ');
                U1putc('\b');
                U1flushTxBuffer();
                str--;                      //   back up the pointer,
                left++;
            } else {                        //   otherwise
                U1putc('\a');               //   send alert/bell character
                U1flushTxBuffer();
            }
            continue;
        }
        if (left == 1) {                    // if string buffer is full,
            U1putc('\a');                   //   send alert/bell character
            U1flushTxBuffer();
            continue;
        }
        if ((*str >= 32) && (*str < 127)) { // if character is printable,
            U1putc(*str);                   //   echo the received character
            U1flushTxBuffer();
            str++;                          //   and advance the pointer
            left--;
        }
    }
    *str = '\0';                            // terminarte the string with null
    U1putc('\n');                           // send newline and
    U1putc('\r');                           //   carriage return
    U1flushTxBuffer();
}

