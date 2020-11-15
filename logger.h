/*
 MIT License

Copyright (c) 2020 Konomu Abe

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _EXAMPLE_FILE_NAME_H    /* Guard against multiple inclusion */
#define _EXAMPLE_FILE_NAME_H

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif


   #define DATA_FILE_NAME  "TEST0000.TXT"
//#define DATA_FILE_NUM_POS   4
//#define DEFAUT_BAUDRATE (57600)
#define DEFAUT_BAUDRATE (38400)
//#define DEFAUT_BAUDRATE (9600)
//#define DEFAUT_BAUDRATE (2400)
//#define DEFAUT_BAUDRATE (4800)
//#define DEFAUT_BAUDRATE (2400)
#define TIME_SEC_TO_SLEEP    (1)
#define MAIN_LOOP_FREQ  (100)
#define SEC2CNT(t) ((unsigned short)(t*MAIN_LOOP_FREQ))
#define UART_SD_BUF_SIZE    (512*8)
#define UART_SD_FLUSH_IF_REACH    (512*5)
#define SD_WAKEUP_RETRY_MAX (5)     /* Retry limit of SD SPI mode enter nigotiation. */

#define TIME_SEC_TO_LED_OFF (7)  // TIME for BOTH LED AND DISPLAY to be ON
#define TIME_SEC_TO_DISP_OFF (0)  // TIME for only DISPLAY to be ON

#define REMAINING_TIME_SEC_TO_POWER_OFF_SD  (10)
/*
 inrush current of OSMR TFV10 32G is about 70mA 0.2ms
 * 80mA*0.3ms = 12uJ
 * stanby current is about 0.1mA, so 120ms stanby is even.
 */

/*  */
#define SD_QUE_RATE_FILTER_TC   ((unsigned short)(90))  /* Coefficient A, max 100(=do not change) */
//#define FLIP_SOFT_RX_CAPTURE_TIMING

    
    
 typedef struct {
    unsigned char current;
    unsigned char previous;
    unsigned short count;
} StateInfo;

typedef struct {
    const char *string;
    unsigned char len;
} CompareString;

typedef enum
{
    s_empty,
    s_read,
     s_over
}RX_RECIEVE_STATUS;

typedef struct {
    unsigned vreg :1;
    unsigned reten :1;
    unsigned wakeup_us:10;
    unsigned max_bps:20;
} __attribute__((packed)) _power_saving_info;

typedef enum
{
    s_idle,
    s_sleep_with_leddisp,
    s_sleep_with_disp,
    s_sleep,
    s_deep_sleep,
}POWER_SAVE_STATE;

typedef enum
{
    s_uart,
    s_sd_initializing,
    s_sd_open_data_file,
    s_sd_scan_data_file,
    s_sd_show_config,
    s_sd_unmounting,
    s_sd_error,
    s_sd_no_media,
    s_sd_ready_to_eject,
    s_clear_disp
} DISP_STATUS;

extern POWER_SAVE_STATE ps_state ,ps_prev;
extern StateInfo disp_state;


extern unsigned long rx_rate_byte_per_sec;
extern bool g_possible_sc1602_reset;
extern bool g_sd_unmount_request;
extern volatile unsigned char g_software_rx;
extern unsigned short g_deep_sleep_min;
extern unsigned char g_power_saving_mode ;
extern bool g_sdcard_detected;
extern volatile unsigned short g_count_10ms ,g_last_count_u2rx ;
extern unsigned short g_softrx_first_capture,g_softrx_bit_interval;
extern bool is_woken_by_rtcc;
extern volatile bool g_flag_10ms;




void local_UART2_Receive_ISR(void);


void SDCard_Power(bool on_off);
void logger_main(void);
   
    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
