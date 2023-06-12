#include "parser.h"
#include "cdc.h"
#include "smu_base.h"

#define CMD_BUFFER_LENGTH   128
#define END_FWD_CHAR        '`'

STATE_HANDLER_T parser_state, parser_last_state, parser_task;

PARSER_PUTC_T parser_putc;
PARSER_PUTS_T parser_puts;

char cdc_cmd_buffer[CMD_BUFFER_LENGTH], ble_cmd_buffer[CMD_BUFFER_LENGTH];
char *cdc_cmd_buffer_pos, *ble_cmd_buffer_pos;
uint16_t cdc_cmd_buffer_left, ble_cmd_buffer_left, end_fwd_char_count;

void parser_disconnected(void);
void parser_connected(void);
void parser_forwarding(void);

void ui_handler(char *args);
void pwr_handler(char *args);
void dac10_handler(char *args);
void dac16_handler(char *args);
void adc16_handler(char *args);
void adc24_handler(char *args);
void digout_handler(char *args);
void ble_handler(char *args);
void flash_handler(char *args);

DISPATCH_ENTRY_T root_table[] = {{ "UI", ui_handler }, 
                                 { "PWR", pwr_handler }, 
                                 { "DAC10", dac10_handler }, 
                                 { "DAC16", dac16_handler }, 
                                 { "ADC16", adc16_handler }, 
                                 { "ADC24", adc24_handler }, 
                                 { "DIGOUT", digout_handler }, 
                                 { "BLE", ble_handler }, 
                                 { "FLASH", flash_handler }};

#define ROOT_TABLE_ENTRIES      sizeof(root_table) / sizeof(DISPATCH_ENTRY_T)

void led1_handler(char *args);
void led1Q_handler(char *args);
void led2_handler(char *args);
void led2Q_handler(char *args);
void led3_handler(char *args);
void led3Q_handler(char *args);
void sw1Q_handler(char *args);

DISPATCH_ENTRY_T ui_table[] = {{ "LED1", led1_handler }, 
                               { "LED1?", led1Q_handler }, 
                               { "LED2", led2_handler }, 
                               { "LED2?", led2Q_handler }, 
                               { "LED3", led3_handler }, 
                               { "LED3?", led3Q_handler }, 
                               { "SW1?", sw1Q_handler }};

#define UI_TABLE_ENTRIES       sizeof(ui_table) / sizeof(DISPATCH_ENTRY_T)

void ena12V_handler(char *args);
void ena12VQ_handler(char *args);

DISPATCH_ENTRY_T pwr_table[] = {{ "ENA12V", ena12V_handler }, 
                                { "ENA12V?", ena12VQ_handler }};

#define PWR_TABLE_ENTRIES       sizeof(pwr_table) / sizeof(DISPATCH_ENTRY_T)

void dac10_dac1_handler(char *args);
void dac10_dac1Q_handler(char *args);
void dac10_dac2_handler(char *args);
void dac10_dac2Q_handler(char *args);
void dac10_diff_handler(char *args);
void dac10_diffQ_handler(char *args);

DISPATCH_ENTRY_T dac10_table[] = {{ "DAC1", dac10_dac1_handler }, 
                                  { "DAC1?", dac10_dac1Q_handler }, 
                                  { "DAC2", dac10_dac2_handler }, 
                                  { "DAC2?", dac10_dac2Q_handler }, 
                                  { "DIFF", dac10_diff_handler }, 
                                  { "DIFF?", dac10_diffQ_handler }};

#define DAC10_TABLE_ENTRIES       sizeof(dac10_table) / sizeof(DISPATCH_ENTRY_T)

void dac16_dac0_handler(char *args);
void dac16_dac0Q_handler(char *args);
void dac16_dac1_handler(char *args);
void dac16_dac1Q_handler(char *args);
void dac16_dac2_handler(char *args);
void dac16_dac2Q_handler(char *args);
void dac16_dac3_handler(char *args);
void dac16_dac3Q_handler(char *args);
void dac16_ch1_handler(char *args);
void dac16_ch1Q_handler(char *args);
void dac16_ch2_handler(char *args);
void dac16_ch2Q_handler(char *args);

DISPATCH_ENTRY_T dac16_table[] = {{ "DAC0", dac16_dac0_handler }, 
                                  { "DAC0?", dac16_dac0Q_handler }, 
                                  { "DAC1", dac16_dac1_handler }, 
                                  { "DAC1?", dac16_dac1Q_handler }, 
                                  { "DAC2", dac16_dac2_handler }, 
                                  { "DAC2?", dac16_dac2Q_handler }, 
                                  { "DAC3", dac16_dac3_handler }, 
                                  { "DAC3?", dac16_dac3Q_handler }, 
                                  { "CH1", dac16_ch1_handler }, 
                                  { "CH1?", dac16_ch1Q_handler }, 
                                  { "CH2", dac16_ch2_handler }, 
                                  { "CH2?", dac16_ch2Q_handler }};

#define DAC16_TABLE_ENTRIES       sizeof(dac16_table) / sizeof(DISPATCH_ENTRY_T)

void adc16_ch1Q_handler(char *args);
void adc16_ch2Q_handler(char *args);
void adc16_ch1avgQ_handler(char *args);
void adc16_ch2avgQ_handler(char *args);
void adc16_ch1rawQ_handler(char *args);
void adc16_ch2rawQ_handler(char *args);
void adc16_calibrate_handler(char *args);
void adc16_offsetQ_handler(char *args);
void adc16_maxvalQ_handler(char *args);

DISPATCH_ENTRY_T adc16_table[] = {{ "CH1?", adc16_ch1Q_handler }, 
                                  { "CH2?", adc16_ch2Q_handler }, 
                                  { "CH1AVG?", adc16_ch1avgQ_handler }, 
                                  { "CH2AVG?", adc16_ch2avgQ_handler }, 
                                  { "CH1RAW?", adc16_ch1rawQ_handler }, 
                                  { "CH2RAW?", adc16_ch2rawQ_handler }, 
                                  { "CALIBRATE", adc16_calibrate_handler }, 
                                  { "OFFSET?", adc16_offsetQ_handler }, 
                                  { "MAXVAL?", adc16_maxvalQ_handler }};

#define ADC16_TABLE_ENTRIES       sizeof(adc16_table) / sizeof(DISPATCH_ENTRY_T)

void adc24_ch1Q_handler(char *args);
void adc24_ch2Q_handler(char *args);
void adc24_ch1avgQ_handler(char *args);
void adc24_ch2avgQ_handler(char *args);
void adc24_ch1rawQ_handler(char *args);
void adc24_ch2rawQ_handler(char *args);
void adc24_ch1offset_handler(char *args);
void adc24_ch1offsetQ_handler(char *args);
void adc24_ch2offset_handler(char *args);
void adc24_ch2offsetQ_handler(char *args);
void adc24_bothQ_handler(char *args);
void adc24_bothavgQ_handler(char *args);
void adc24_bothrawQ_handler(char *args);
void adc24_calibrate_handler(char *args);
void adc24_reg_handler(char *args);
void adc24_regQ_handler(char *args);

DISPATCH_ENTRY_T adc24_table[] = {{ "CH1?", adc24_ch1Q_handler }, 
                                  { "CH2?", adc24_ch2Q_handler }, 
                                  { "CH1AVG?", adc24_ch1avgQ_handler }, 
                                  { "CH2AVG?", adc24_ch2avgQ_handler }, 
                                  { "CH1RAW?", adc24_ch1rawQ_handler }, 
                                  { "CH2RAW?", adc24_ch2rawQ_handler }, 
                                  { "CH1OFFSET", adc24_ch1offset_handler }, 
                                  { "CH1OFFSET?", adc24_ch1offsetQ_handler }, 
                                  { "CH2OFFSET", adc24_ch2offset_handler }, 
                                  { "CH2OFFSET?", adc24_ch2offsetQ_handler }, 
                                  { "BOTH?", adc24_bothQ_handler }, 
                                  { "BOTHAVG?", adc24_bothavgQ_handler }, 
                                  { "BOTHRAW?", adc24_bothrawQ_handler }, 
                                  { "CALIBRATE", adc24_calibrate_handler }, 
                                  { "REG", adc24_reg_handler }, 
                                  { "REG?", adc24_regQ_handler }};

#define ADC24_TABLE_ENTRIES       sizeof(adc24_table) / sizeof(DISPATCH_ENTRY_T)

void portd_handler(char *args);
void portdQ_handler(char *args);
void rd0_handler(char *args);
void rd0Q_handler(char *args);
void rd1_handler(char *args);
void rd1Q_handler(char *args);
void rd2_handler(char *args);
void rd2Q_handler(char *args);
void rd3_handler(char *args);
void rd3Q_handler(char *args);
void rd4_handler(char *args);
void rd4Q_handler(char *args);
void rd5_handler(char *args);
void rd5Q_handler(char *args);
void rd6_handler(char *args);
void rd6Q_handler(char *args);
void porte_handler(char *args);
void porteQ_handler(char *args);
void re0_handler(char *args);
void re0Q_handler(char *args);
void re1_handler(char *args);
void re1Q_handler(char *args);
void re2_handler(char *args);
void re2Q_handler(char *args);
void re3_handler(char *args);
void re3Q_handler(char *args);
void re4_handler(char *args);
void re4Q_handler(char *args);
void re5_handler(char *args);
void re5Q_handler(char *args);
void re6_handler(char *args);
void re6Q_handler(char *args);

DISPATCH_ENTRY_T digout_table[] = {{ "PORTD", portd_handler }, 
                                   { "PORTD?", portdQ_handler }, 
                                   { "RD0", rd0_handler }, 
                                   { "RD0?", rd0Q_handler }, 
                                   { "RD1", rd1_handler }, 
                                   { "RD1?", rd1Q_handler }, 
                                   { "RD2", rd2_handler }, 
                                   { "RD2?", rd2Q_handler }, 
                                   { "RD3", rd3_handler }, 
                                   { "RD3?", rd3Q_handler }, 
                                   { "RD4", rd4_handler }, 
                                   { "RD4?", rd4Q_handler }, 
                                   { "RD5", rd5_handler }, 
                                   { "RD5?", rd5Q_handler }, 
                                   { "RD6", rd6_handler }, 
                                   { "RD6?", rd6Q_handler }, 
                                   { "PORTE", porte_handler }, 
                                   { "PORTE?", porteQ_handler }, 
                                   { "RE0", re0_handler }, 
                                   { "RE0?", re0Q_handler }, 
                                   { "RE1", re1_handler }, 
                                   { "RE1?", re1Q_handler }, 
                                   { "RE2", re2_handler }, 
                                   { "RE2?", re2Q_handler }, 
                                   { "RE3", re3_handler }, 
                                   { "RE3?", re3Q_handler }, 
                                   { "RE4", re4_handler }, 
                                   { "RE4?", re4Q_handler }, 
                                   { "RE5", re5_handler }, 
                                   { "RE5?", re5Q_handler }, 
                                   { "RE6", re6_handler }, 
                                   { "RE6?", re6Q_handler }};

#define DIGOUT_TABLE_ENTRIES    sizeof(digout_table) / sizeof(DISPATCH_ENTRY_T)

void ble_reset_handler(char *args);
void ble_resetQ_handler(char *args);
void ble_forward_handler(char *args);

DISPATCH_ENTRY_T ble_table[] = {{ "RESET", ble_reset_handler }, 
                                { "RESET?", ble_resetQ_handler }, 
                                { "FORWARD", ble_forward_handler }};

#define BLE_TABLE_ENTRIES       sizeof(ble_table) / sizeof(DISPATCH_ENTRY_T)

void flash_erase_handler(char *args);
void flash_read_handler(char *args);
void flash_write_handler(char *args);

DISPATCH_ENTRY_T flash_table[] = {{ "ERASE", flash_erase_handler },
                                  { "READ", flash_read_handler },
                                  { "WRITE", flash_write_handler }};

#define FLASH_TABLE_ENTRIES     sizeof(flash_table) / sizeof(DISPATCH_ENTRY_T)

int16_t str2hex(char *str, uint16_t *num) {
    if (!str)
        return -1;

    while ((*str == ' ') || (*str == '\t'))
        str++;

    *num = 0;
    while (*str) {
        if ((*str >= '0') && (*str <= '9'))
            *num = (*num << 4) + (*str - '0');
        else if ((*str >= 'a') && (*str <= 'f'))
            *num = (*num << 4) + 10 + (*str - 'a');
        else if ((*str >= 'A') && (*str <= 'F'))
            *num = (*num << 4) + 10 + (*str - 'A');
        else
            return -1;
        str++;
    }
    return 0;
}

int16_t str2num(char *str, uint16_t *num) {
    if (!str)
        return -1;

    while ((*str == ' ') || (*str == '\t'))
        str++;

    *num = 0;
    while (*str) {
        if ((*str >= '0') && (*str <= '9'))
            *num = *num * 10 + (*str - '0');
        else
            return -1;
        str++;
    }
    return 0;
}

void hex2str(uint16_t num, char *str) {
    uint16_t digit, i;

    for (i = 0; i < 4; i++) {
        digit = num >> 12;
        if (digit < 10)
            *str = '0' + (uint8_t)digit;
        else
            *str = 'A' + (uint8_t)digit - 10;
        str++;
        num = (num & 0x0FFF) << 4;
    }
    *str = '\0';
}

void hex2str_alt(uint16_t num, char *str) {
    uint16_t digit, i, hit_nonzero_digit = FALSE;

    for (i = 0; i < 4; i++) {
        digit = num >> 12;
        if (digit)
            hit_nonzero_digit = TRUE;
        if ((hit_nonzero_digit) || (i == 3)) {
            if (digit < 10)
                *str = '0' + (uint8_t)digit;
            else
                *str = 'A' + (uint8_t)digit - 10;
            str++;
        }
        num = (num & 0x0FFF) << 4;
    }
    *str = '\0';
}

int16_t str_cmp(char *str1, char *str2) {
    while ((*str1) && (*str1 == *str2)) {
        str1++;
        str2++;
    }

    if (*str1 == *str2)
        return 0;
    else if (*str1 < *str2)
        return -1;
    else
        return 1;
}

int16_t str_ncmp(char *str1, char *str2, uint16_t n) {
    if (n == 0)
        return 0;

    while ((*str1) && (*str2) && (*str1 == *str2) && (--n)) {
        str1++;
        str2++;
    }

    if (*str1 == *str2)
        return 0;
    else if (*str1 < *str2)
        return -1;
    else
        return 1;
}

char *str_tok_r(char *str, char *delim, char **save_str) {
    char *spos, *dpos, *token_start;

    if (!(str) && !(*save_str)) 
        return (char *)NULL;

    // Find the first non-delimiter character in the string
    for (spos = (str) ? str : *save_str; *spos; spos++) {
        for (dpos = delim; *dpos; dpos++) {
            if (*spos == *dpos)
                break;
        }
        if (*dpos == '\0')
            break;
    }
    if (*spos)
        token_start = spos;
    else {
        *save_str = (char *)NULL;
        return (char *)NULL;
    }

    // Find the first delimiter character in the string
    for (; *spos; spos++) {
        for (dpos = delim; *dpos; dpos++) {
            if (*spos == *dpos)
                break;
        }
        if (*spos == *dpos)
            break;
    }
    if (*spos) {
        *spos = '\0';
        *save_str = spos + 1;
    } else {
        *save_str = (char *)NULL;
    }

    return token_start;
}

// UI commands
void ui_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < UI_TABLE_ENTRIES; i++) {
            if (str_cmp(command, ui_table[i].command) == 0) {
                ui_table[i].handler(remainder);
                break;
            }
        }
    }
}

void led1_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            LED1 = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            LED1 = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            LED1 = !LED1;
        } else if (str2hex(token, &val) == 0) {
            LED1 = (val) ? 1 : 0;
        }
    }
}

void led1Q_handler(char *args) {
    if (LED1 == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void led2_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            LED2 = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            LED2 = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            LED2 = !LED2;
        } else if (str2hex(token, &val) == 0) {
            LED2 = (val) ? 1 : 0;
        }
    }
}

void led2Q_handler(char *args) {
    if (LED2 == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void led3_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            LED3 = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            LED3 = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            LED3 = !LED3;
        } else if (str2hex(token, &val) == 0) {
            LED3 = (val) ? 1 : 0;
        }
    }
}

void led3Q_handler(char *args) {
    if (LED3 == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void sw1Q_handler(char *args) {
    parser_putc((SW1) ? '1' : '0');
    parser_puts("\r\n");
}

// PWR commands
void pwr_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < PWR_TABLE_ENTRIES; i++) {
            if (str_cmp(command, pwr_table[i].command) == 0) {
                pwr_table[i].handler(remainder);
                break;
            }
        }
    }
}

void ena12V_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            ENA12V = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            ENA12V = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            ENA12V = !ENA12V;
        } else if (str2hex(token, &val) == 0) {
            ENA12V = (val) ? 1 : 0;
        }
    }
}

void ena12VQ_handler(char *args) {
    if (ENA12V == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

// DAC10 commands
void dac10_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < DAC10_TABLE_ENTRIES; i++) {
            if (str_cmp(command, dac10_table[i].command) == 0) {
                dac10_table[i].handler(remainder);
                break;
            }
        }
    }
}

void dac10_dac1_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ", ", &remainder);
    if (token && (str2hex(token, &val) == 0)) {
        DAC1DAT = val & 0x3FF;
    }
}

void dac10_dac1Q_handler(char *args) {
    char str[5];

    hex2str_alt(DAC1DAT, str);
    parser_puts(str);
    parser_puts("\r\n");
}

void dac10_dac2_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ", ", &remainder);
    if (token && (str2hex(token, &val) == 0)) {
        DAC2DAT = val & 0x3FF;
    }
}

void dac10_dac2Q_handler(char *args) {
    char str[5];

    hex2str_alt(DAC2DAT, str);
    parser_puts(str);
    parser_puts("\r\n");
}

void dac10_diff_handler(char *args) {
    char *token, *remainder;
    uint16_t val, dac1, dac2;

    remainder = (char *)NULL;
    token = str_tok_r(args, ", ", &remainder);
    if (token && (str2hex(token, &val) == 0)) {
        dac1 = ((0x400 + (int16_t)val) >> 1) & 0x3FF;
        dac2 = ((0x400 - (int16_t)val) >> 1) & 0x3FF;
        DAC1DAT = dac1;
        DAC2DAT = dac2;
    }
}

void dac10_diffQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)((int16_t)DAC1DAT - (int16_t)DAC2DAT), str);
    parser_puts(str);
    parser_puts("\r\n");
}

// DAC16 commands
void dac16_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < DAC16_TABLE_ENTRIES; i++) {
            if (str_cmp(command, dac16_table[i].command) == 0) {
                dac16_table[i].handler(remainder);
                break;
            }
        }
    }
}

void dac16_dac0_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ", ", &remainder);
    if (token && (str2hex(token, &val) == 0)) {
        dac16_set_dac0(val);
    }
}

void dac16_dac0Q_handler(char *args) {
    char str[5];

    hex2str_alt(dac16_get_dac0(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void dac16_dac1_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ", ", &remainder);
    if (token && (str2hex(token, &val) == 0)) {
        dac16_set_dac1(val);
    }
}

void dac16_dac1Q_handler(char *args) {
    char str[5];

    hex2str_alt(dac16_get_dac1(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void dac16_dac2_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ", ", &remainder);
    if (token && (str2hex(token, &val) == 0)) {
        dac16_set_dac2(val);
    }
}

void dac16_dac2Q_handler(char *args) {
    char str[5];

    hex2str_alt(dac16_get_dac2(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void dac16_dac3_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ", ", &remainder);
    if (token && (str2hex(token, &val) == 0)) {
        dac16_set_dac3(val);
    }
}

void dac16_dac3Q_handler(char *args) {
    char str[5];

    hex2str_alt(dac16_get_dac3(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void dac16_ch1_handler(char *args) {
    uint16_t pos, neg;
    char *arg1, *arg2;

    arg2 = (char *)NULL;
    arg1 = str_tok_r(args, ", ", &arg2);
    if (arg1 && arg2) {
        if ((str2hex(arg1, &pos) == 0) && (str2hex(arg2, &neg) == 0)) {
            dac16_set_ch1(pos, neg);
        }
    }
}

void dac16_ch1Q_handler(char *args) {
    char str[5];

    hex2str_alt(dac16_get_dac1(), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt(dac16_get_dac0(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void dac16_ch2_handler(char *args) {
    uint16_t pos, neg;
    char *arg1, *arg2;

    arg2 = (char *)NULL;
    arg1 = str_tok_r(args, ", ", &arg2);
    if (arg1 && arg2) {
        if ((str2hex(arg1, &pos) == 0) && (str2hex(arg2, &neg) == 0)) {
            dac16_set_ch2(pos, neg);
        }
    }
}

void dac16_ch2Q_handler(char *args) {
    char str[5];

    hex2str_alt(dac16_get_dac3(), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt(dac16_get_dac2(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

// ADC16 commands
void adc16_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < ADC16_TABLE_ENTRIES; i++) {
            if (str_cmp(command, adc16_table[i].command) == 0) {
                adc16_table[i].handler(remainder);
                break;
            }
        }
    }
}

void adc16_ch1Q_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)adc16_meas_ch1(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc16_ch2Q_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)adc16_meas_ch2(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc16_ch1avgQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)adc16_meas_ch1_avg(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc16_ch2avgQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)adc16_meas_ch2_avg(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc16_ch1rawQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)adc16_meas_ch1_raw(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc16_ch2rawQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)adc16_meas_ch2_raw(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc16_calibrate_handler(char *args) {
    adc16_calibrate();
}

void adc16_offsetQ_handler(char *args) {
    char str[5];

    hex2str_alt((uint16_t)adc16_get_offset(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc16_maxvalQ_handler(char *args) {
    char str[5];

    hex2str_alt(adc16_get_max_val(), str);
    parser_puts(str);
    parser_puts("\r\n");
}

// ADC24 commands
void adc24_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < ADC24_TABLE_ENTRIES; i++) {
            if (str_cmp(command, adc24_table[i].command) == 0) {
                adc24_table[i].handler(remainder);
                break;
            }
        }
    }
}

void adc24_ch1Q_handler(char *args) {
    int32_t val1, val2;
    char str[5];

    adc24_meas_both(&val1, &val2);
    hex2str_alt((uint16_t)(val1 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val1 >> 16), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc24_ch2Q_handler(char *args) {
    int32_t val1, val2;
    char str[5];

    adc24_meas_both(&val1, &val2);
    hex2str_alt((uint16_t)(val2 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val2 >> 16), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc24_ch1avgQ_handler(char *args) {
    int32_t val1, val2;
    char str[5];

    adc24_meas_both_avg(&val1, &val2);
    hex2str_alt((uint16_t)(val1 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val1 >> 16), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc24_ch2avgQ_handler(char *args) {
    int32_t val1, val2;
    char str[5];

    adc24_meas_both_avg(&val1, &val2);
    hex2str_alt((uint16_t)(val2 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val2 >> 16), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc24_ch1rawQ_handler(char *args) {
    int32_t val1, val2;
    char str[5];

    adc24_meas_both_raw(&val1, &val2);
    hex2str_alt((uint16_t)(val1 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val1 >> 16), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc24_ch2rawQ_handler(char *args) {
    int32_t val1, val2;
    char str[5];

    adc24_meas_both_raw(&val1, &val2);
    hex2str_alt((uint16_t)(val2 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val2 >> 16), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc24_ch1offset_handler(char *args) {
    uint16_t val1, val2;
    char *arg1, *arg2;

    arg2 = (char *)NULL;
    arg1 = str_tok_r(args, ", ", &arg2);
    if (arg1 && arg2) {
        if ((str2hex(arg1, &val1) == 0) && (str2hex(arg2, &val2) == 0)) {
            adc24_set_ch1offset((int32_t)(((uint32_t)(val2) << 16) | (uint32_t)(val1)));
        }
    }
}

void adc24_ch1offsetQ_handler(char *args) {
    int32_t val;
    char str[5];

    val = adc24_get_ch1offset();
    hex2str_alt((uint16_t)(val & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val >> 16), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc24_ch2offset_handler(char *args) {
    uint16_t val1, val2;
    char *arg1, *arg2;

    arg2 = (char *)NULL;
    arg1 = str_tok_r(args, ", ", &arg2);
    if (arg1 && arg2) {
        if ((str2hex(arg1, &val1) == 0) && (str2hex(arg2, &val2) == 0)) {
            adc24_set_ch2offset((int32_t)(((uint32_t)(val2) << 16) | (uint32_t)(val1)));
        }
    }
}

void adc24_ch2offsetQ_handler(char *args) {
    int32_t val;
    char str[5];

    val = adc24_get_ch2offset();
    hex2str_alt((uint16_t)(val & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val >> 16), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc24_bothQ_handler(char *args) {
    int32_t val1, val2;
    char str[5];

    adc24_meas_both(&val1, &val2);
    hex2str_alt((uint16_t)(val1 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val1 >> 16), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)(val2 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val2 >> 16), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc24_bothavgQ_handler(char *args) {
    int32_t val1, val2;
    char str[5];

    adc24_meas_both_avg(&val1, &val2);
    hex2str_alt((uint16_t)(val1 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val1 >> 16), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)(val2 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val2 >> 16), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc24_bothrawQ_handler(char *args) {
    int32_t val1, val2;
    char str[5];

    adc24_meas_both_raw(&val1, &val2);
    hex2str_alt((uint16_t)(val1 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val1 >> 16), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)(val2 & 0xFFFF), str);
    parser_puts(str);
    parser_putc(',');
    hex2str_alt((uint16_t)((uint32_t)val2 >> 16), str);
    parser_puts(str);
    parser_puts("\r\n");
}

void adc24_calibrate_handler(char *args) {
    adc24_calibrate();
}

void adc24_reg_handler(char *args) {
    uint16_t reg, val;
    char *arg1, *arg2;

    arg2 = (char *)NULL;
    arg1 = str_tok_r(args, ", ", &arg2);
    if (arg1 && arg2) {
        if ((str2hex(arg1, &reg) == 0) && (str2hex(arg2, &val) == 0)) {
            adc24_write_reg((uint8_t)reg, (uint8_t)val);
        }
    }
}

void adc24_regQ_handler(char *args) {
    char *token, *remainder;
    uint16_t reg;
    char str[5];

    remainder = (char *)NULL;
    token = str_tok_r(args, ", ", &remainder);
    if (token && (str2hex(token, &reg) == 0)) {
        hex2str_alt((uint16_t)adc24_read_reg((uint8_t)reg), str);
        parser_puts(str);
        parser_puts("\r\n");
    }
}

// DIGOUT commands
void digout_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < DIGOUT_TABLE_ENTRIES; i++) {
            if (str_cmp(command, digout_table[i].command) == 0) {
                digout_table[i].handler(remainder);
                break;
            }
        }
    }
}

void portd_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ", ", &remainder);
    if (token && (str2hex(token, &val) == 0)) {
        LATD = (LATD & 0xFF80) | (val & 0x7F);
    }
}

void portdQ_handler(char *args) {
    char str[5];

    hex2str_alt(PORTD & 0x7F, str);
    parser_puts(str);
    parser_puts("\r\n");
}

void rd0_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RD0_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RD0_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RD0_ = !RD0_;
        } else if (str2hex(token, &val) == 0) {
            RD0_ = (val) ? 1 : 0;
        }
    }
}

void rd0Q_handler(char *args) {
    if (RD0_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void rd1_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RD1_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RD1_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RD1_ = !RD1_;
        } else if (str2hex(token, &val) == 0) {
            RD1_ = (val) ? 1 : 0;
        }
    }
}

void rd1Q_handler(char *args) {
    if (RD1_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void rd2_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RD2_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RD2_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RD2_ = !RD2_;
        } else if (str2hex(token, &val) == 0) {
            RD2_ = (val) ? 1 : 0;
        }
    }
}

void rd2Q_handler(char *args) {
    if (RD2_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void rd3_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RD3_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RD3_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RD3_ = !RD3_;
        } else if (str2hex(token, &val) == 0) {
            RD3_ = (val) ? 1 : 0;
        }
    }
}

void rd3Q_handler(char *args) {
    if (RD3_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void rd4_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RD4_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RD4_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RD4_ = !RD4_;
        } else if (str2hex(token, &val) == 0) {
            RD4_ = (val) ? 1 : 0;
        }
    }
}

void rd4Q_handler(char *args) {
    if (RD4_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void rd5_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RD5_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RD5_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RD5_ = !RD5_;
        } else if (str2hex(token, &val) == 0) {
            RD5_ = (val) ? 1 : 0;
        }
    }
}

void rd5Q_handler(char *args) {
    if (RD5_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void rd6_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RD6_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RD6_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RD6_ = !RD6_;
        } else if (str2hex(token, &val) == 0) {
            RD6_ = (val) ? 1 : 0;
        }
    }
}

void rd6Q_handler(char *args) {
    if (RD6_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void porte_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ", ", &remainder);
    if (token && (str2hex(token, &val) == 0)) {
        LATE = (LATE & 0xFF80) | (val & 0x7F);
    }
}

void porteQ_handler(char *args) {
    char str[5];

    hex2str_alt(LATE & 0x7F, str);
    parser_puts(str);
    parser_puts("\r\n");
}

void re0_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RE0_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RE0_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RE0_ = !RE0_;
        } else if (str2hex(token, &val) == 0) {
            RE0_ = (val) ? 1 : 0;
        }
    }
}

void re0Q_handler(char *args) {
    if (RE0_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void re1_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RE1_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RE1_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RE1_ = !RE1_;
        } else if (str2hex(token, &val) == 0) {
            RE1_ = (val) ? 1 : 0;
        }
    }
}

void re1Q_handler(char *args) {
    if (RE1_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void re2_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RE2_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RE2_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RE2_ = !RE2_;
        } else if (str2hex(token, &val) == 0) {
            RE2_ = (val) ? 1 : 0;
        }
    }
}

void re2Q_handler(char *args) {
    if (RE2_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void re3_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RE3_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RE3_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RE3_ = !RE3_;
        } else if (str2hex(token, &val) == 0) {
            RE3_ = (val) ? 1 : 0;
        }
    }
}

void re3Q_handler(char *args) {
    if (RE3_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void re4_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RE4_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RE4_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RE4_ = !RE4_;
        } else if (str2hex(token, &val) == 0) {
            RE4_ = (val) ? 1 : 0;
        }
    }
}

void re4Q_handler(char *args) {
    if (RE4_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void re5_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RE5_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RE5_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RE5_ = !RE5_;
        } else if (str2hex(token, &val) == 0) {
            RE5_ = (val) ? 1 : 0;
        }
    }
}

void re5Q_handler(char *args) {
    if (RE5_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void re6_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            RE6_ = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            RE6_ = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            RE6_ = !RE6_;
        } else if (str2hex(token, &val) == 0) {
            RE6_ = (val) ? 1 : 0;
        }
    }
}

void re6Q_handler(char *args) {
    if (RE6_ == ON)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

// BLE commands
void ble_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < BLE_TABLE_ENTRIES; i++) {
            if (str_cmp(command, ble_table[i].command) == 0) {
                ble_table[i].handler(remainder);
                break;
            }
        }
    }
}

void ble_reset_handler(char *args) {
    char *token, *remainder;
    uint16_t val;

    remainder = (char *)NULL;
    token = str_tok_r(args, ":, ", &remainder);
    if (token) {
        if (str_cmp(token, "ON") == 0) {
            BLE_RST_N = ON;
        } else if (str_cmp(token, "OFF") == 0) {
            BLE_RST_N = OFF;
        } else if (str_cmp(token, "TOGGLE") == 0) {
            BLE_RST_N = !BLE_RST_N;
        } else if (str2hex(token, &val) == 0) {
            BLE_RST_N = (val) ? 1 : 0;
        }
    }
}

void ble_resetQ_handler(char *args) {
    if (BLE_RST_N == 1)
        parser_puts("1\r\n");
    else
        parser_puts("0\r\n");
}

void ble_forward_handler(char *args) {
    if (parser_state == parser_disconnected) {
        parser_state = parser_forwarding;
    }
}

// FLASH commands
void flash_handler(char *args) {
    uint16_t i;
    char *command, *remainder;

    remainder = (char *)NULL;
    command = str_tok_r(args, ":, ", &remainder);
    if (command) {
        for (i = 0; i < FLASH_TABLE_ENTRIES; i++) {
            if (str_cmp(command, flash_table[i].command) == 0) {
                flash_table[i].handler(remainder);
                break;
            }
        }
    }
}

void flash_erase_handler(char *args) {
    uint16_t val1, val2;
    char *arg1, *arg2;

    arg2 = (char *)NULL;
    arg1 = str_tok_r(args, ", ", &arg2);
    if (arg1 && arg2) {
        if ((str2hex(arg1, &val1) == 0) && (str2hex(arg2, &val2) == 0)) {
            NVMCON = 0x4042;                // set up NVMCON to erase a page of program memory
            __asm__("push _TBLPAG");
            TBLPAG = val1;
            __builtin_tblwtl(val2, 0x0000);
            __asm__("disi #16");            // disable interrupts for 16 cycles
            __builtin_write_NVM();          // issue the unlock sequence and perform the write
            while (NVMCONbits.WR == 1) {}   // wait until the write is complete
            NVMCONbits.WREN = 0;            // disable further writes to program memory
            __asm__("pop _TBLPAG");
        }
    }
}

void flash_read_handler(char *args) {
    uint16_t val1, val2, val3;
    char *arg, *remainder;
    char str[5];
    WORD temp;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val1) != 0)
        return;
    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &val2) != 0)
        return;
    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &val3) != 0)
        return;

    __asm__("push _TBLPAG");
    TBLPAG = val1;
    for (val1 = 0; val1 < val3; val2 += 2) {
        temp.w = __builtin_tblrdl(val2);
        hex2str_alt(temp.b[0], str);
        parser_puts(str);
        parser_putc(',');
        hex2str_alt(temp.b[1], str);
        parser_puts(str);
        parser_putc(',');
        temp.w = __builtin_tblrdh(val2);
        hex2str_alt(temp.b[0], str);
        parser_puts(str);
        parser_putc(',');
        hex2str_alt(temp.b[1], str);
        parser_puts(str);
        val1 += 4;
        if (val1 < val3)
            parser_putc(',');
        else
            parser_puts("\r\n");
    }
    __asm__("pop _TBLPAG");
}

void flash_write_handler(char *args) {
    uint16_t val1, val2, i;
    char *arg, *remainder;
    WORD temp;

    remainder = (char *)NULL;
    arg = str_tok_r(args, ", ", &remainder);
    if (str2hex(arg, &val1) != 0)
        return;

    arg = str_tok_r((char *)NULL, ", ", &remainder);
    if (str2hex(arg, &val2) != 0)
        return;

    NVMCON = 0x4001;                // set up NVMCON to program a row of program memory
    __asm__("push _TBLPAG");        // save the value of TBLPAG
    TBLPAG = val1;
    val1 = val2 & 0xFFF8;
    for (i = 0; i < 128; i += 2) {
        __builtin_tblwtl(val1 + i, 0xFFFF);
        __builtin_tblwth(val1 + i + 1, 0x00FF);
    }

    for (;; val2 += 2) {
        arg = str_tok_r((char *)NULL, ", ", &remainder);
        if (!arg) 
            break;
        temp.b[0] = (str2hex(arg, &val1) == 0) ? (uint8_t)val1 : 0xFF;

        arg = str_tok_r((char *)NULL, ", ", &remainder);
        if (!arg) 
            break;
        temp.b[1] = (str2hex(arg, &val1) == 0) ? (uint8_t)val1 : 0xFF;

        __builtin_tblwtl(val2, temp.w);

        arg = str_tok_r((char *)NULL, ", ", &remainder);
        if (!arg) 
            break;
        temp.b[0] = (str2hex(arg, &val1) == 0) ? (uint8_t)val1 : 0xFF;

        arg = str_tok_r((char *)NULL, ", ", &remainder);
        if (!arg) 
            break;
        temp.b[1] = (str2hex(arg, &val1) == 0) ? (uint8_t)val1 : 0x00;

        __builtin_tblwth(val2, temp.w);
    }

    __asm__("disi #16");            // disable interrupts for 16 cycles
    __builtin_write_NVM();          // issue the unlock sequence and perform the write
    while (NVMCONbits.WR == 1) {}   // wait until the write is done
    NVMCONbits.WREN = 0;            // disable further writes to program memory
    __asm__("pop _TBLPAG");         // restore original value to TBLPAG
}

// Parser public methods
void init_parser(void) {
    cdc_cmd_buffer_pos = cdc_cmd_buffer;
    cdc_cmd_buffer_left = CMD_BUFFER_LENGTH;

    ble_cmd_buffer_pos = ble_cmd_buffer;
    ble_cmd_buffer_left = CMD_BUFFER_LENGTH;

    parser_state = parser_disconnected;
    parser_last_state = (STATE_HANDLER_T)NULL;
    parser_task = (STATE_HANDLER_T)NULL;

    parser_putc = cdc_putc;
    parser_puts = cdc_puts;
}

void parser_disconnected(void) {
    uint8_t ch;
    uint16_t i;
    char *command, *remainder;

    if (parser_state != parser_last_state) {
        parser_last_state = parser_state;

        ble_cmd_buffer_pos = ble_cmd_buffer;
        ble_cmd_buffer_left = CMD_BUFFER_LENGTH;
    }

    if (parser_task)
        parser_task();

    if (ble_in_waiting() > 0) {
        ch = ble_getc();
        if (ble_cmd_buffer_left == 1) {
            ble_cmd_buffer_pos = ble_cmd_buffer;
            ble_cmd_buffer_left = CMD_BUFFER_LENGTH;

            *ble_cmd_buffer_pos++ = ch;
            ble_cmd_buffer_left--;
        } else if (ch == '%') {
            if ((ble_cmd_buffer[0] == '%') && (ble_cmd_buffer_left < CMD_BUFFER_LENGTH)) {
                *ble_cmd_buffer_pos++ = ch;
                *ble_cmd_buffer_pos = '\0';

                if (str_cmp(ble_cmd_buffer, "%STREAM_OPEN%") == 0)
                    parser_state = parser_connected;

                ble_cmd_buffer_pos = ble_cmd_buffer;
                ble_cmd_buffer_left = CMD_BUFFER_LENGTH;
            } else {
                ble_cmd_buffer_pos = ble_cmd_buffer;
                ble_cmd_buffer_left = CMD_BUFFER_LENGTH;

                *ble_cmd_buffer_pos++ = ch;
                ble_cmd_buffer_left--;
            }
        } else {
            *ble_cmd_buffer_pos++ = ch;
            ble_cmd_buffer_left--;
        }
    }

    if (cdc_in_waiting() > 0) {
        ch = cdc_getc();
        if (cdc_cmd_buffer_left == 1) {
            cdc_cmd_buffer_pos = cdc_cmd_buffer;
            cdc_cmd_buffer_left = CMD_BUFFER_LENGTH;

            *cdc_cmd_buffer_pos++ = ch;
            cdc_cmd_buffer_left--;
        } else if (ch == '\r') {
            *cdc_cmd_buffer_pos = '\0';

//            cdc_putc('[');
//            cdc_puts(cdc_cmd_buffer);
//            cdc_puts("]\r\n");

            parser_putc = cdc_putc;
            parser_puts = cdc_puts;

            remainder = (char *)NULL;
            command = str_tok_r(cdc_cmd_buffer, ":, ", &remainder);
            if (command) {
                for (i = 0; i < ROOT_TABLE_ENTRIES; i++) {
                    if (str_cmp(command, root_table[i].command) == 0) {
                        root_table[i].handler(remainder);
                        break;
                    }
                }
            }

            cdc_cmd_buffer_pos = cdc_cmd_buffer;
            cdc_cmd_buffer_left = CMD_BUFFER_LENGTH;
        } else {
            *cdc_cmd_buffer_pos++ = ch;
            cdc_cmd_buffer_left--;
        }
    }

    if (parser_state != parser_last_state) {
        parser_task = (STATE_HANDLER_T)NULL;
    }
}

void parser_connected(void) {
    uint8_t ch;
    uint16_t i;
    char *command, *remainder;

    if (parser_state != parser_last_state) {
        parser_last_state = parser_state;

        LED1 = ON;

        ble_cmd_buffer_pos = ble_cmd_buffer;
        ble_cmd_buffer_left = CMD_BUFFER_LENGTH;
    }

    if (parser_task)
        parser_task();

    if (ble_in_waiting() > 0) {
        ch = ble_getc();
        if (ble_cmd_buffer_left == 1) {
            ble_cmd_buffer_pos = ble_cmd_buffer;
            ble_cmd_buffer_left = CMD_BUFFER_LENGTH;

            *ble_cmd_buffer_pos++ = ch;
            ble_cmd_buffer_left--;
        } else if (ch == '%') {
            if ((ble_cmd_buffer[0] == '%') && (ble_cmd_buffer_left < CMD_BUFFER_LENGTH)) {
                *ble_cmd_buffer_pos++ = ch;
                *ble_cmd_buffer_pos = '\0';

                if (str_cmp(ble_cmd_buffer, "%DISCONNECT%") == 0)
                    parser_state = parser_disconnected;

                ble_cmd_buffer_pos = ble_cmd_buffer;
                ble_cmd_buffer_left = CMD_BUFFER_LENGTH;
            } else {
                ble_cmd_buffer_pos = ble_cmd_buffer;
                ble_cmd_buffer_left = CMD_BUFFER_LENGTH;

                *ble_cmd_buffer_pos++ = ch;
                ble_cmd_buffer_left--;
            }
        } else if (ch == '\r') {
            *ble_cmd_buffer_pos = '\0';

//            ble_putc('[');
//            ble_puts(ble_cmd_buffer);
//            ble_puts("]\n\r");

            parser_putc = ble_putc;
            parser_puts = ble_puts;

            remainder = (char *)NULL;
            command = str_tok_r(ble_cmd_buffer, ":, ", &remainder);
            if (command) {
                for (i = 0; i < ROOT_TABLE_ENTRIES; i++) {
                    if (str_cmp(command, root_table[i].command) == 0) {
                        root_table[i].handler(remainder);
                        break;
                    }
                }
            }

            ble_cmd_buffer_pos = ble_cmd_buffer;
            ble_cmd_buffer_left = CMD_BUFFER_LENGTH;
        } else {
            *ble_cmd_buffer_pos++ = ch;
            ble_cmd_buffer_left--;
        }
    }

    if (cdc_in_waiting() > 0) {
        ch = cdc_getc();
        if (cdc_cmd_buffer_left == 1) {
            cdc_cmd_buffer_pos = cdc_cmd_buffer;
            cdc_cmd_buffer_left = CMD_BUFFER_LENGTH;

            *cdc_cmd_buffer_pos++ = ch;
            cdc_cmd_buffer_left--;
        } else if (ch == '\r') {
            *cdc_cmd_buffer_pos = '\0';

//            cdc_putc('[');
//            cdc_puts(cdc_cmd_buffer);
//            cdc_puts("]\r\n");

            parser_putc = cdc_putc;
            parser_puts = cdc_puts;

            remainder = (char *)NULL;
            command = str_tok_r(cdc_cmd_buffer, ":, ", &remainder);
            if (command) {
                for (i = 0; i < ROOT_TABLE_ENTRIES; i++) {
                    if (str_cmp(command, root_table[i].command) == 0) {
                        root_table[i].handler(remainder);
                        break;
                    }
                }
            }

            cdc_cmd_buffer_pos = cdc_cmd_buffer;
            cdc_cmd_buffer_left = CMD_BUFFER_LENGTH;
        } else {
            *cdc_cmd_buffer_pos++ = ch;
            cdc_cmd_buffer_left--;
        }
    }

    if (parser_state != parser_last_state) {
        parser_task = (STATE_HANDLER_T)NULL;
        LED1 = OFF;
    }
}

void parser_forwarding(void) {
    uint8_t ch;
    uint16_t i;

    if (parser_state != parser_last_state) {
        parser_last_state = parser_state;
        end_fwd_char_count = 0;
        LED2 = ON;
    }

    if (parser_task)
        parser_task();

    if (cdc_in_waiting() > 0) {
        ch = cdc_getc();
        if (ch == END_FWD_CHAR) {
            end_fwd_char_count++;
            if (end_fwd_char_count == 3) {
                parser_state = parser_disconnected;
            }
        } else if ((ch != END_FWD_CHAR) && (end_fwd_char_count > 0)) {
            for (i = 0; i < end_fwd_char_count; i++)
                U1putc(END_FWD_CHAR);
            U1putc(ch);
            U1flushTxBuffer();
            end_fwd_char_count = 0;
        } else {
            U1putc(ch);
            U1flushTxBuffer();
        }
    }

    if (U1inWaiting() > 0) {
        cdc_putc(U1getc());
    }

    if (parser_state != parser_last_state) {
        parser_task = (STATE_HANDLER_T)NULL;
        LED2 = OFF;
    }
}
