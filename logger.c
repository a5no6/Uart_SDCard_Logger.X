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

#include <proc/p32mm0064gpl028.h>
//#include "mcc_generated_files/system.h"
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/rtcc.h"
#include "mcc_generated_files/interrupt_manager.h"
#include "mcc_generated_files/clock.h"

#include <string.h>

#include "fileio.h"
#include "logger.h"

#include "sd_demo.h"
#include "sc1602.h"

#define _PL()

unsigned long rx_rate_byte_per_sec = 10000;

volatile bool g_flag_led = false;
volatile __PORTBbits_t filtered_RB={0,0,0,1},filtered_RB_pre={0,0,0,1};
volatile unsigned short sw_push_count = 0;
volatile unsigned short g_count_10ms = 0,g_last_count_u2rx = 0;
volatile unsigned char g_software_rx = 0;
bool g_sd_unmount_request = false;
bool g_possible_sc1602_reset = false;

unsigned int g_uart2_bps=DEFAUT_BAUDRATE;
unsigned short g_softrx_first_capture,g_softrx_bit_interval;
volatile unsigned short g_sd_que_count = 0; 
unsigned short g_deep_sleep_min = 10;
bool g_sdcard_detected = false;
bool is_woken_by_rtcc;
volatile bool g_flag_10ms = false;

const char* rx_recieve_status_strings[]={
    "s_empty",
    "s_read",
    "s_over"
};

const char *power_state_strings[]=
{
    "s_idle",
    "s_sleep_with_leddisp",
    "s_sleep_with_disp",
    "s_sleep",
    "s_deep_sleep",
};


#define NUM_COMMAND (4)
#define MAX_OPTION  (2)

const CompareString Cmd_bps = {"bps",3},Cmd_folder = {"folder",6},Cmd_date = {"date",4},Cmd_sleep = {"sleep",5};
const CompareString *Available_commands[NUM_COMMAND] = {&Cmd_bps,&Cmd_folder,&Cmd_date,&Cmd_sleep};
unsigned char MatchLength[NUM_COMMAND] = {0,0,0,0};
bool Command_Valid[NUM_COMMAND] = {true,true,true,true};
static unsigned long cmd_option[MAX_OPTION] = {0,0};
volatile static char uart2_vram[33]="";


typedef struct{
    char buf[UART_SD_BUF_SIZE+1];
    unsigned short head;
    unsigned short tail;
} RX2SDBUFFER;
RX2SDBUFFER rx2sdbuf;

const _power_saving_info power_saving_setting[4] = {
    {0,1,163,4800}, // 0.5uA typ163us observed is 139us
//    {1,1,74,19200}, // 2.7uA
    {1,1,32,19200}, // 2.7uA
    {0,0,28,57600}, // 6uA
    {1,0,10,9600*30}};//141uA
unsigned char g_power_saving_mode = 0;


void calc_rx_rate_task(void)
{
    static unsigned short prev = 0,delta;
    delta = (g_sd_que_count-prev);  /* max 11.5kbps/10 *10ms = 12 Bytes*/ 
    
    prev = g_sd_que_count;
    rx_rate_byte_per_sec *= SD_QUE_RATE_FILTER_TC;
    rx_rate_byte_per_sec /= 100;
    rx_rate_byte_per_sec += (delta*(100-SD_QUE_RATE_FILTER_TC));
}

unsigned short UART_SD_stored(void)
{
    unsigned short size,tail;
    tail = rx2sdbuf.tail;
    
    if (tail < rx2sdbuf.head )
    {
        size = ( UART_SD_BUF_SIZE - (rx2sdbuf.head-tail));
    }
    else
    {
        size = (tail - rx2sdbuf.head);
    }
    
    if(size > UART_SD_BUF_SIZE)
    {
        return UART_SD_BUF_SIZE;
    }
    return(size);
}

void UART_SD_que(char c)
{
    static bool is_buf_full = false,
                prev_buf_full = false;

    rx2sdbuf.buf[rx2sdbuf.tail] = c;
    g_sd_que_count++;

    // Will the increment not result in a wrap and not result in a pure collision?
    // This is most often condition so check first
    if ( ( rx2sdbuf.tail    != ( UART_SD_BUF_SIZE-1)) &&
         ((rx2sdbuf.tail+1) != rx2sdbuf.head) )
    {
        rx2sdbuf.tail++;
    } 
    else if ( (rx2sdbuf.tail == ( UART_SD_BUF_SIZE-1)) &&
              (rx2sdbuf.head !=  0) )
    {
        // Pure wrap no collision
        rx2sdbuf.tail = 0;
    } 
    else // must be collision
    {
//        IEC1CLR = _IEC1_U2RXIE_MASK;  /* Should not stop, to continue disp after SD eject */
        is_buf_full = true;
    }
    if(!prev_buf_full&&is_buf_full)
        UART_puts("que buf full.\n");
    else if(prev_buf_full&&!is_buf_full)
        UART_puts("que buf recovered.\n");
    
    prev_buf_full = is_buf_full;
}

uint8_t UART_SD_READ( void)
{
    uint8_t data = 0;

    if (rx2sdbuf.head == rx2sdbuf.tail )
        return(0);
    
    data = rx2sdbuf.buf[rx2sdbuf.head];

    rx2sdbuf.head++;

    if (rx2sdbuf.head ==  UART_SD_BUF_SIZE)
    {
        rx2sdbuf.head = 0;
    }
    return data;
}

void set_software_rx_param(void)
{
    for(g_power_saving_mode=0;g_power_saving_mode<4;g_power_saving_mode++){
        if(g_uart2_bps<=power_saving_setting[g_power_saving_mode].max_bps)
            break;
    }
    
    long temp = CLOCK_SystemFrequencyGet()/2/g_uart2_bps;
    g_softrx_bit_interval = temp;
    temp = ((3*1000000/2/g_uart2_bps)-power_saving_setting[g_power_saving_mode].wakeup_us);
    if(temp>0){
        g_softrx_first_capture = temp;
        temp *= (CLOCK_SystemFrequencyGet()/2/1000000);
        g_softrx_first_capture = temp;
    }else{
        g_softrx_first_capture = 0;
        g_softrx_bit_interval*=100;
        g_softrx_bit_interval/=102;
    }
    U2MODECLR = _U2MODE_ON_MASK;   // enabling UART ON bit
    U2BRG = (((8000000/4) / g_uart2_bps) - 1);
    U2MODESET = _U2MODE_ON_MASK;   // enabling UART ON bit
}

void match_command(char c, int loc)
{
    int i;
	for(i=0;i<NUM_COMMAND;i++){
		if(loc<Available_commands[i]->len && c==Available_commands[i]->string[loc]){
				MatchLength[i]++;
		}else{
            Command_Valid[i]=false;
        }
	}
}

char *i2a_digi(char zero,unsigned char digi,char *dst,unsigned long n)
{
    char chartbl[11] = "01234567890";
    unsigned long m=1,i,temp,j=0;
    for(i=1;i<digi;i++) 
        m*=10;
    for(i=0;i<digi;i++) {
        temp = n/m;
        while(temp>10){
            temp-=10;
        }
        if(temp==0){
            if(zero!=0){
                dst[j++]=zero;
            }
        }else{    
            dst[j++]=chartbl[temp];
        }
        m/=10;
    }
    return(dst+j);
}

int read_cmd_int(const char* buf)
{
    int j=0,v = 0;
    char c;
    
    const char num_char[10] = "0123456789";
    
    while((c=buf[j++])!=0){
        int i = 0;
        while(i<10){
            if(c==num_char[i])
                break;
            i++;
        }
        if(i<10){
            v *= 10;
            v += i;
        }
    }
    return(v);
}

unsigned short read_cmd_bcd(const char* buf)
{
    int j=0;
    unsigned short v=0;
    char c;
    
    const char num_char[10] = "0123456789";
    
    UART_puts(buf);
    UART_flush();
    while((c=buf[j++])!=0){
        int i = 0;
        while(i<10){
            if(c==num_char[i])
                break;
            i++;
        }
        if(i<10){
            v = (v<<4);
            v += i;
        }
    }
    return(v);
}

static struct tm prev_set_datetime;

int exec_command(int len,const char *option)
{
 _PL();
	int i,j,k=0;
    bool update;
    struct tm datetime;
    char buf[5];
	for(i=0;i<NUM_COMMAND;i++){
		if(MatchLength[i]==len && Available_commands[i]->len==len)
			break;
	}
_PL();

	switch(i){
		case 0:  // read length
            UART_puts("bps\n");
            g_uart2_bps = read_cmd_int(option);
            UART_put_int32(g_uart2_bps);
            UART_puts("\n");
            if(900<g_uart2_bps&&g_uart2_bps<9600*16){
                UART_puts("uart baud rate changed.\n");
                set_software_rx_param();
            }
            UART_puts("\n");
            UART_flush();
			break;
		case 1: // set 
            UART_puts("folder\n");
            UART_puts(option);
            UART_puts("\n");
            for(k=0;(k<32+5)&&(option[k]!=0);k++){
                data_file_path_name[k]=option[k];
            }
            if(k>0 && data_file_path_name[k-1]!='/'){
                data_file_path_name[k++]='/';
            }
            data_file_path_name[k]=0;
            UART_puts(data_file_path_name);
            UART_puts("\n");
            UART_flush();
			break;
		case 2: // date 2019/01/23(4)12:34:56
            for(j=0;j<4;j++) buf[j] = option[j+k];buf[j] = 0;            
            datetime.tm_year = (read_cmd_int(buf)-2000);
            k+=5;
            for(j=0;j<2;j++) buf[j] = option[j+k];buf[j] = 0;            
            datetime.tm_mon = read_cmd_int(buf);
            k+=3;
            for(j=0;j<2;j++) buf[j] = option[j+k];buf[j] = 0;            
            datetime.tm_mday = read_cmd_int(buf);
            k+=3;
            for(j=0;j<1;j++) buf[j] = option[j+k];buf[j] = 0;            
            datetime.tm_wday = read_cmd_int(buf);
            k+=2;
            for(j=0;j<2;j++) buf[j] = option[j+k];buf[j] = 0;            
            datetime.tm_hour = read_cmd_int(buf);
            k+=3;
            for(j=0;j<2;j++) buf[j] = option[j+k];buf[j] = 0;            
            datetime.tm_min = read_cmd_int(buf);
            k+=3;
            for(j=0;j<2;j++) buf[j] = option[j+k];buf[j] = 0;            
            datetime.tm_sec = read_cmd_int(buf);

            if(memcmp(&prev_set_datetime,&datetime,sizeof(struct tm))!=0){
                RTCC_TimeSet(&datetime);
                memcpy(&prev_set_datetime,&datetime,sizeof(struct tm));                        
                UART_puts("rtcc changed.\n");                
            }
            
			break;
		case 3:  // read length
            UART_puts("sleep\n");
            g_deep_sleep_min = read_cmd_int(option);
            UART_put_int32(g_deep_sleep_min);
            UART_puts("\n");
            if(g_deep_sleep_min < 7*24*60){
                UART_puts("time to deep sleep changed.\n");
            }
            UART_puts("\n");
            UART_flush();
			break;
		default:
			break;
	}
    for(j=0;j<NUM_COMMAND;j++){
            MatchLength[j]=0;
            Command_Valid[j]=true;
    }
    if(i<NUM_COMMAND)
        return(i);
    else
        return(-1);
}

#ifdef USE_MULTIPLE_READ_INPUT_FILTER
#define INPUT_FILTER_THRESH (5)
typedef struct {
    unsigned prev :1;
    unsigned output :1;
    unsigned count:6;
} __attribute__((packed)) _io_input_filter;

void input_filter(_io_input_filter *f,unsigned char input)
{
    if(input==f->prev){
        if(f->count<INPUT_FILTER_THRESH){
            f->count++;
        }
        if(f->count==INPUT_FILTER_THRESH){
            f->count++;
            f->output = f->prev;
        }
    }else{
        f->count = 0;
        f->prev = input;
    }
}
#endif
void SW_task(void)
{
#ifdef USE_MULTIPLE_READ_INPUT_FILTER
    static _io_input_filter filtered_rb4;
    input_filter(&filtered_rb4,PORTBbits.RB4);
    filtered_RB.RB4 = filtered_rb4.output;
#else
    filtered_RB_pre = filtered_RB;
    filtered_RB = PORTBbits;
#endif
    if(filtered_RB.RB4==0){
        if(sw_push_count<0xffff)
            sw_push_count++;
    }else{
        sw_push_count = 0;        
    }
    if(sw_push_count==SEC2CNT(5)){
        if(!g_sd_unmount_request)
            g_sd_unmount_request = true;
    }
}

typedef enum
{
    s_black,
    s_led_0_5,
    s_led_1,
    s_led_1_5,
    s_led_2,
    s_led_50,
    s_sc1602_off
}LED_STATUS;

const char *led_state_strings[]=
{
    "s_black",
    "s_led_0_5",
    "s_led_1",
    "s_led_1_5",
    "s_led_2",
    "s_led_50",
    "s_sc1602_off"
};

volatile StateInfo g_led_stat = {s_led_1,s_led_1,0};

void LED_task(void)
{
    g_led_stat.previous=g_led_stat.current;
    switch(g_led_stat.current){
        case s_black:
            CCP2RB = 0x0000;
            if(g_led_stat.count>SEC2CNT(5)){
//                LATBCLR = _LATB_LATB2_MASK;
#ifndef DO_NOT_TURN_OFF_SC1602
                sc1602_power(0);
#endif
                g_sc1602_initialized = false;
                g_led_stat.current = s_sc1602_off;
            }
            break;
        case s_led_1:
//            CCP2RB = (CCP2PR/100);
            CCP2RB = (CCP2PR/50);
            if(g_led_stat.count>SEC2CNT(5)){
                g_led_stat.current = s_black;
            }
            break;
        case s_led_50:
            g_led_stat.current = s_led_1;
            break;
//            CCP2RB = (CCP2PR/2);
            CCP2RB = (CCP2PR/5);
            if(g_led_stat.count>SEC2CNT(1)){
                g_led_stat.current = s_led_1;
            }
            break;
        default:
            break;
    }
    if(g_flag_led){
        g_led_stat.current = s_led_50;
        g_flag_led = false;
        g_led_stat.count=0;
    } if(filtered_RB.RB4==0){
        g_led_stat.current = s_led_50;
    }else if(filtered_RB_pre.RB4==0 && filtered_RB.RB4==1){
        g_led_stat.current = s_led_1;        
    }
    if(g_led_stat.previous!=g_led_stat.current){
        g_led_stat.count=0;
    }else{
        if(g_led_stat.count<0xffff)
            g_led_stat.count++;
    }
/*    _NI(if(g_led_stat.current!=g_led_stat.previous){
        UART_puts(led_state_strings[g_led_stat.previous]);
        UART_puts("->");
        UART_puts(led_state_strings[g_led_stat.current]);
        UART_puts("\n");
//        UART_flush();
    })*/
}

void SDCard_Power(bool on_off)
{
    unsigned int coretimer;
    if(on_off){ /* POWER ON */
        coretimer = CORETIMER_CountGet();
        INTERRUPT_GlobalDisable();
        LATBCLR = _LATB_LATB3_MASK;
        while((CORETIMER_CountGet()-coretimer)<10*1000);
        INTERRUPT_GlobalEnable();
        SD_SYSTEM_Initialize();
        g_possible_sc1602_reset = true;
    }else{ /* POWER OFF */
        LATBSET = _LATB_LATB3_MASK;
        SD_SYSTEM_Deinitialize();
        g_sd_card_hybernate = true;
    }
}

void read_opt(char c)
{   
    static unsigned char cmd_len = 0,main_state = 0,len=0;
    static unsigned char cmd_buf[32],cmd_buf_i=0;
    switch(c)
    {
        case ':':
            if(main_state==0){
                cmd_len = len;
                main_state++;
            }else{
                if(cmd_buf_i<32-1)                                
                    cmd_buf[cmd_buf_i++] = c;                    
            }
            break;
        case 0x0A:
        case 0x0D:
            if(main_state==0)
                cmd_len = len;
            cmd_buf[cmd_buf_i++] = 0;
            exec_command(cmd_len,cmd_buf);
            cmd_len = 0;
            len = 0;
            main_state = 0;
            cmd_buf_i = 0;
            break;

        default:
            switch(main_state)
            {
                case 0:
                    match_command(c,len);
                    break;
                default:
                    if(cmd_buf_i<32-1)                                
                        cmd_buf[cmd_buf_i++] = c;
                    break;
            }
            len++;
            break;
    }
}

const char* disp_task_string[]={
    "s_uart",
    "s_sd_initializing",
    "s_sd_open_data_file",
    "s_sd_scan_data_file",
    "s_sd_show_config",
    "s_sd_unmounting",
    "s_sd_error",
    "s_sd_no_media",
    "s_sd_ready_to_eject",
    "s_clear_disp"
};
        
void disp_task(void);

static StateInfo g_uart2_state = {s_empty,s_empty,0};
void local_UART2_Receive_ISR(void)
{
    static unsigned char i=0,j;
    static char shadow_vram[33];
    while(U2STAbits.URXDA == 1 || g_software_rx)
    {
        char c;
        g_uart2_state.previous = g_uart2_state.current;
        if(g_software_rx){
            c = g_software_rx;
            g_software_rx = 0;
        }else{
            c = U2RXREG;            
        }
        switch(g_uart2_state.current){
            case s_empty:
                if(c==0x0d || c==0x0a)
                    break;
                i=0;
                if(g_flag_cache_sd){
                    for(j=0;log_time_stamp[j];j++){
                        UART_SD_que(log_time_stamp[j]);
                    }
                    UART_SD_que(c);                    
                }
                shadow_vram[i++]=c;
                g_uart2_state.current = s_read;
                break;
            case s_read:
                if(c==0x0d || c==0x0a){
                    shadow_vram[i]=0;
                    if(uart2_vram[0]==0){
                        for(j=0;j<32&&shadow_vram[j];j++){
                            uart2_vram[j] = shadow_vram[j];                        
                        }
                        uart2_vram[j] = 0;
                    }else{
                    }
                    g_uart2_state.current=s_empty;
                }else{
                    if(i<32){
                        shadow_vram[i++]=c;
                    }
                }
                if(g_flag_cache_sd){
                    UART_SD_que(c);
                }
                break;       
        }
/*        if(g_uart2_state.current!=g_uart2_state.previous){
            UART_puts(rx_recieve_status_strings[g_uart2_state.previous]);
            UART_puts("->");
            UART_puts(rx_recieve_status_strings[g_uart2_state.current]);
            UART_puts("\n");
            g_uart2_state.count = 0;
        }else{
            if(g_uart2_state.count<0xffff){
                g_uart2_state.count++;
            }
        }*/
    }
    g_last_count_u2rx = g_count_10ms;
    IFS1CLR= 1 << _IFS1_U2RXIF_POSITION;
}

StateInfo disp_state = {s_sd_initializing,s_sd_initializing,0};
void disp_task(void)
{    
    char temp[21];
    char *ptr;
    unsigned char i,j;;
    static char uart2_shadow1[16];
    static char uart2_shadow2[16];
    static bool have_que = false;
    
    disp_state.previous = disp_state.current;

    switch(disp_state.current){
        case s_sd_show_config:
            if(disp_state.count==0){
#ifdef DISPTASK_USE_UART_LOG
                _NI(UART_puts(log_time_stamp));
#endif
                ptr = i2a_digi(0,6,temp,g_uart2_bps);
                *ptr++ = 'B';
                *ptr++ = 'P';
                *ptr++ = 'S';
                i = 0;
                *ptr=0;
#ifdef DISPTASK_USE_UART_LOG
                _NI(UART_puts(temp);
                UART_puts("\n");
//                UART_flush();
                )
#endif
                have_que = true;
                g_flag_led = true;
            }
            if(have_que && !sc1602_que_start ){
                if(exist_config_file)
                    sc1602_set_que(g_config_file_name,temp);
                else
                    sc1602_set_que("NO SDLOG???.CFG",temp);
                have_que = false;
            }
            if(disp_state.count>SEC2CNT(4)){
                disp_state.current = s_sd_open_data_file;
            }
           break;
        case s_sd_open_data_file:
            if(disp_state.count%SEC2CNT(1)==0){
                have_que = true;
            }
            if(disp_state.count==0){
                g_flag_led = true;
            }
            if(have_que && !sc1602_que_start){
                sc1602_set_que(log_time_stamp,data_file_path_name);
                have_que = false;
            }
            break;

        case s_uart:
            if(uart2_vram[0]){
                for(i=0;i<5;i++){
                    uart2_shadow1[i] = log_time_stamp[i+6];
                }
                uart2_shadow1[i++]=' ';
                j= 0;
                for(;i<16&&uart2_vram[j];i++,j++){
                    uart2_shadow1[i] = uart2_vram[j];
                }
                for(;i<16;i++){
                    uart2_shadow1[i] = ' ';
                }
                for(i=0;i<16&&uart2_vram[j];i++,j++){
                    uart2_shadow2[i] = uart2_vram[j];
                }
                for(;i<16;i++){
                    uart2_shadow2[i] = ' ';
                }
                have_que = true;
                g_flag_led = true;
                uart2_vram[0]=0;
            }else{                 
                if(filtered_RB_pre.RB4==1 && filtered_RB.RB4==0){
                    have_que = true;
                    g_flag_led = true;
                }
                if(U2STAbits.OERR == 1){
                    U2STACLR = _U2STA_OERR_MASK;
                    UART_puts("rx2oerr\n");
                }
                if(U2STAbits.FERR == 1){
                    U2STACLR = _U2STA_FERR_MASK;
                    UART_puts("rx2ferr\n");
                }
                if(U2STAbits.PERR == 1){
                    U2STACLR = _U2STA_PERR_MASK;
                    UART_puts("rx2perr\n");
                }
            }
            if(have_que && !sc1602_que_start){
                sc1602_set_que(uart2_shadow1,uart2_shadow2);
                have_que = false;
            }
            break;
        case s_sd_ready_to_eject:
            if(disp_state.count==0){
                have_que = true;
            }else{
                if(disp_state.count>SEC2CNT(4)){
                    disp_state.current = s_uart;
                }
            }
            if(have_que && !sc1602_que_start){
                sc1602_set_que("Unmounted.","Ready to eject");
#ifdef DISPTASK_USE_UART_LOG
                _NI(UART_puts("Ready to eject  \n");
                )
#endif
                g_flag_led = true;
                have_que = false;
            }
            break;
        case s_sd_error:
            if(disp_state.count==0){
                have_que = true;
            }else{
                if(disp_state.count>SEC2CNT(4.1)){
//                    disp_state.current = s_uart;
                    disp_state.current = s_sd_show_config;
                }
            }
            if(have_que && !sc1602_que_start ){
                sc1602_set_que("SD Card Error."," ");
#ifdef DISPTASK_USE_UART_LOG
                _NI(UART_puts("SD Card accessd error.\n");
//                UART_flush();
                )
#endif
                have_que = false;
                g_flag_led = true;
            }
            break;

        case s_sd_no_media:
            if(disp_state.count==0){
                have_que = true;
            }else{
                if(disp_state.count>SEC2CNT(4.1)){
                    disp_state.current = s_sd_show_config;
                }
            }
            if(have_que && !sc1602_que_start ){
                sc1602_set_que("No SD Card"," ");
#ifdef DISPTASK_USE_UART_LOG
                _NI(UART_puts("SD Card not found.\n");
                )
#endif
                have_que = false;
                g_flag_led = true;
            }
            break;

        case s_sd_initializing:
            break;
            if(sd_state.count%SEC2CNT(0.5)==0 && !sc1602_que_start){
                sc1602_set_que("Read config file",g_config_file_name);
                g_flag_led = true;
            }
            break;

        case s_sd_scan_data_file:
            have_que = true;
            if(have_que && !sc1602_que_start){
                sc1602_set_que("Scanning... ",data_file_path_name);
                g_flag_led = true;
                have_que = false;
            }
            break;
        case s_clear_disp:            
            if(uart2_vram[0]==0 ){
                if(!sc1602_que_start){
                    for(i=0;i<16;i++){
                        uart2_shadow1[i]=' ';
                        uart2_shadow2[i]=' ';
                    }
                    sc1602_set_que(uart2_shadow1,uart2_shadow2);
                    disp_state.current = s_uart;
                }
            }else{
                disp_state.current = s_uart;
            }
            break;
        default :
            break;
    }
    if(sd_state.current==DEMO_STATE_DONE && sd_state.count==0){
        disp_state.current = s_sd_ready_to_eject;        
    }else if(sd_state.current==DEMO_STATE_SCAN_DATA_FILE && sd_state.count==0){
        disp_state.current = s_sd_show_config;        
    }else if(sd_state.current==DEMO_STATE_NO_MEDIA && sd_state.count==0){
        disp_state.current = s_sd_no_media;                
    }else if(sd_state.current==DEMO_STATE_FAILED && sd_state.count==0){
        disp_state.current = s_sd_error;                
//    }else if(disp_state.current!=s_uart && disp_state.count>SEC2CNT(6) && (uart2_vram[0])){
    }else if(disp_state.current!=s_uart && disp_state.count>SEC2CNT(6) ){
        disp_state.current = s_clear_disp;
    }
    
    if(disp_state.current != disp_state.previous)
        disp_state.count = 0;
    else{
        if(disp_state.count<0xffff){
            disp_state.count++;
        }
    }
#ifdef DISPTASK_USE_UART_LOG
    _NI(if(disp_state.current!=disp_state.previous){
        UART_puts(disp_task_string[disp_state.previous]);
        UART_puts("->");
        UART_puts(disp_task_string[disp_state.current]);
        UART_puts("\n");
    })
#endif    
}


void set_alarm(unsigned short minute,unsigned short sec)
{
   IEC0bits.RTCCIE = 0;
   
//   int wake_mod = minute%60;
//       minute /= 60;

    unsigned int count = minute*60+sec;
    unsigned int wake_mod = count%60;
    count /= 60;
   
    bcdTime_t currentTime;
    RTCC_TimeGet(&currentTime);

    currentTime.tm_sec += wake_mod;
    if(currentTime.tm_sec  >= 60){
       currentTime.tm_sec -= 60 ;
       currentTime.tm_min++;
    }

   wake_mod = count%60;
   count /= 60;
   currentTime.tm_min += wake_mod;
   if(currentTime.tm_min  >= 60){
       currentTime.tm_min -= 60 ;
       currentTime.tm_hour++;
   }
   wake_mod = count%24;
   count /= 24;
   currentTime.tm_hour += wake_mod;
   if(currentTime.tm_hour  >= 24){
       currentTime.tm_hour -= 64 ;
       currentTime.tm_wday++;
   }
   currentTime.tm_wday += count;
   if(currentTime.tm_wday  >= 7){
       currentTime.tm_wday -= 7 ;
   }
   ALMDATE = (ConvertHexToBCD(currentTime.tm_mon) << 16 ) | (ConvertHexToBCD(currentTime.tm_mday) << 8) | ConvertHexToBCD(currentTime.tm_wday) ; // YEAR/MONTH-1/DAY-1/WEEKDAY
   ALMTIME = (ConvertHexToBCD(currentTime.tm_hour) << 24) | (ConvertHexToBCD(currentTime.tm_min) << 16 ) | (ConvertHexToBCD(currentTime.tm_sec) << 8) ; // HOURS/MINUTES/SECOND
   
   IFS0CLR = _IFS0_RTCCIF_MASK;
   IEC0bits.RTCCIE = 1;   
}

void set_sleep_power(unsigned char mode)
{
    if(power_saving_setting[mode].vreg){
        PWRCONSET = _PWRCON_VREGS_MASK; 
    }else{
        PWRCONCLR = _PWRCON_VREGS_MASK; 
    }
    if(power_saving_setting[mode].reten){
        PWRCONSET = _PWRCON_RETEN_MASK;        
    }else{
        PWRCONCLR = _PWRCON_RETEN_MASK; 
    }
}

POWER_SAVE_STATE ps_state = s_idle,ps_prev=s_idle;

void sd_card_detect_power_task(void)
{
    if(sd_state.current ==DEMO_STATE_DONE){
        if(ps_state==s_sleep||ps_state==s_deep_sleep){
            CNPUBCLR = _CNPUB_CNPUB12_MASK;
        }else{
            CNPUBSET = _CNPUB_CNPUB12_MASK;            
        }
    }else{
        if(g_sdcard_detected){
            CNPUBCLR = _CNPUB_CNPUB12_MASK;                        
        }else{
            CNPUBSET = _CNPUB_CNPUB12_MASK;                        
        }
    }
}

void logger_main(void)
{
        unsigned long coretimer ;
        unsigned long next;
        int i;

        sd_card_detect_power_task();
        if(ps_state==s_idle){
            if((g_count_10ms-g_last_count_u2rx >((unsigned short)(100*TIME_SEC_TO_SLEEP)))
//                    &&(g_led_stat.current== s_sc1602_off)
                    &&(disp_state.current== s_uart)
                    &&(!sc1602_que_start)
                    &&(PORTBbits.RB4==1)
                    && !is_UART_busy() ){
                ps_state = s_sleep_with_leddisp;
            }        
            IEC1CLR = _IEC1_U2TXIE_MASK;  
            IEC1CLR = _IEC1_U2EIE_MASK;  
            IEC0CLR = _IEC0_U1TXIE_MASK; 
            IEC0CLR = _IEC0_U1RXIE_MASK;  
            IEC0CLR = _IEC0_U1EIE_MASK;  
            asm volatile ("wait");            
            IEC1SET = _IEC1_U2TXIE_MASK; 
            IEC1SET = _IEC1_U2EIE_MASK; 
            IEC0SET = _IEC0_U1TXIE_MASK; 
            IEC0SET = _IEC0_U1RXIE_MASK; 
            IEC0SET = _IEC0_U1EIE_MASK; 

            if(g_flag_10ms){
                SDCard_task();
                USER_SdSpiGetCd();
                g_flag_10ms = false;
    #ifndef FIXED_CCT2IE_DISABLED_BUG
                if(sc1602_que_start&&!IEC1bits.CCT2IE){
                    IEC1SET = _IEC1_CCT2IE_MASK;
                }    
    #endif
            }
        }else{  
            UART_puts(power_state_strings[ps_state]);
            UART_puts("\n");
            UART_flush();
//            while(!UART1_IsTxDone());
            
            next = g_softrx_first_capture;  
            asm("di");
            SYSTEM_RegUnlock();
            asm("ei");
#ifdef WORKAROUND_WAKEUP_RESET
            if(g_flag_cache_sd)
#endif
            OSCCONSET = _OSCCON_SLPEN_MASK;
            T1CONCLR = _T1CON_TON_MASK;
            IEC0CLR = _IEC0_RTCCIE_MASK;
            REFO1CONCLR = _REFO1CON_ON_MASK;
            CCP2CON1CLR = _CCP2CON1_ON_MASK;

            RPCONbits.IOLOCK = 0;
            RPINR9bits.U2RXR = 0x0000;    // no port to UART2:U2RX
            RPCONbits.IOLOCK = 1; // lock   PPS
            RTCCON1bits.ALRMEN = 0;
            RTCCON1bits.AMASK = 0b0111; //Every week
            switch(ps_state){
                case s_sleep_with_leddisp:
                    set_sleep_power(g_power_saving_mode);
                    set_alarm(0,TIME_SEC_TO_LED_OFF);
                    RTCCON1bits.ALRMEN = 1;
                    IEC0SET = _IEC0_RTCCIE_MASK;
                    if(TIME_SEC_TO_DISP_OFF>0){
                        ps_state = s_sleep_with_disp;                
                    }else{
                        ps_state = s_sleep;                                        
                    }
                    break;
                case s_sleep_with_disp:
                    set_sleep_power(g_power_saving_mode);
                    ps_state = s_sleep;
                    set_alarm(0,TIME_SEC_TO_DISP_OFF);
                    RTCCON1bits.ALRMEN = 1;
                    IEC0SET = _IEC0_RTCCIE_MASK;
                    break;
                case s_sleep:
                    sc1602_power(0);
                    g_sc1602_initialized = false;
                    set_sleep_power(g_power_saving_mode);
                    ps_state = s_deep_sleep;
                    set_alarm(g_deep_sleep_min,0);
                    RTCCON1bits.ALRMEN = 1;
                    IEC0SET = _IEC0_RTCCIE_MASK;
                    break;
                case s_deep_sleep:
                    set_sleep_power(0);
                    break;
            }
            
            asm("di");
            SYSTEM_RegLock();
            asm("ei");
            g_software_rx = 0;

            is_woken_by_rtcc = false;

            UART_flush();
            
            CNFBCLR = 0xFFFF;  //Clear CNFB
            CNEN1Bbits.CNIE1B12 = 1;    //Pin : RB12 Card detect
            IFS0CLR= _IFS0_CNBIF_MASK; // Clear IFS0bits.CNBIF
            IEC0SET = _IEC0_CNBIE_MASK;

            asm volatile ("wait");

            coretimer = CORETIMER_CountGet();
#ifdef FLIP_SOFT_RX_CAPTURE_TIMING
            LATAINV = _LATA_LATA4_MASK;
#endif

            IEC0CLR = _IEC0_CNBIE_MASK;
            //  asm("di");
            if(next==0){
                if(PORTBbits.RB10){
                    g_software_rx+=0b10000000;                
                }
            }else{
                while(CORETIMER_CountGet()-coretimer < next);
                if(PORTBbits.RB10)
                    g_software_rx+=0b10000000;
            }
#ifdef FLIP_SOFT_RX_CAPTURE_TIMING
            LATAINV = _LATA_LATA4_MASK;
#endif
            for(i=1;i<8;i++){
                next+=g_softrx_bit_interval;
                g_software_rx = (g_software_rx>>1);
                while(CORETIMER_CountGet()-coretimer < next);
                if(PORTBbits.RB10)
                    g_software_rx+=0b10000000;
#ifdef FLIP_SOFT_RX_CAPTURE_TIMING
                LATAINV = _LATA_LATA4_MASK;
#endif
            }
            //asm("ei");
            if(!CNFBbits.CNFB10){ // discard data if not woken by RX2
                g_software_rx = 0;
            }
            asm("di");
            SYSTEM_RegUnlock();
            asm("ei");
            
            CNFBCLR = 0xFFFF;  //Clear CNFB
            while(!PORTBbits.RB10);

            if(!is_woken_by_rtcc){ // wake and start the job
                REFO1CONSET = _REFO1CON_ON_MASK;
                OSCCONCLR = _OSCCON_SLPEN_MASK;
                while(!REFO1CONbits.ACTIVE & REFO1CONbits.ON);

                RPCONbits.IOLOCK = 0;
                RPINR9bits.U2RXR = 0x0011;    //RB10->UART2:U2RX
                RPCONbits.IOLOCK = 1; // lock   PPS
                asm("di");
                SYSTEM_RegLock();
                asm("ei");

                TMR1_Initialize();
                T1CONSET = _T1CON_TON_MASK;
                IEC0SET = _IEC0_RTCCIE_MASK;
                CCP2CON1SET = _CCP2CON1_ON_MASK;
//                CNPUBbits.CNPUB12 = 1;  let sdcard_detect_power_task do this job.
                CNEN1Bbits.CNIE1B12 = 0;    //Pin : RB12 Card detect

                RTCCON1bits.ALRMEN = 0;
                RTCCON1bits.AMASK = 0b0001; //Every second
                RTCCON1bits.ALRMEN = 1;

                ps_state = s_idle;                
                g_last_count_u2rx = g_count_10ms;
            }
        } 
        switch(ps_state){
            case s_idle:
            case s_sleep_with_leddisp:
                LATBbits.LATB11 = 1;
                break;
            default:
                LATBbits.LATB11 = 0;
                break;               
        }
        ps_prev = ps_state;
 }



/*
                         Main application
 */

