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

//  SC1602(HD44780U) 4bit mode  

#include <proc/p32mm0064gpl028.h>
#include "mcc_generated_files/clock.h"
#include "mcc_generated_files/system.h"
#include "mcc_generated_files/coretimer.h"
#include "sc1602.h"
#include <stdbool.h>

typedef struct {
    unsigned char current;
    unsigned char previous;
    unsigned short count;
} StateInfo;

_sc1602_data_que sc1602_que_data[3+16+1+16+1+12];
bool sc1602_que_start = false;
bool g_sc1602_initialized = false;

extern unsigned int __Count_Per_microsec ;
extern bool g_possible_sc1602_reset;

#define COUNT_PER_US    (80000000/4/1000000)    /* TMR clock/prescalor/1MHz */

void __delay_us(int s)
{
    uint32_t c,d;
#ifdef USE_CORETIMER
    c = CORETIMER_CountGet();
    d = __Count_Per_microsec*s; /* seems coretimer is cpu_freq/2*/
    while((uint32_t)(CORETIMER_CountGet()-c)<d);
#endif
    c = CCP2TMRbits.TMRL;
    d = COUNT_PER_US*s; //8MHz
    while((uint16_t)((CCP2TMRbits.TMRL-c)&CCP2PRbits.PRL)<d);
}

void __delay_ms(int s)
{
    int i;
    for(i=0;i<s;i++)
        __delay_us(1000);
}

void sc1602_strobe(void)
{
    LCD_E_SET;
    __delay_us(1); /* >450ns */
    LCD_E_CLR;
}

static void lcd_port_hi_out(unsigned char d)
{
    if(d&0x01){
        LCD_PORT_DB4_SET;
    }else{
        LCD_PORT_DB4_CLR;        
    }
    if(d&0x02){
        LCD_PORT_DB5_SET;
    }else{
        LCD_PORT_DB5_CLR;        
    }
    if(d&0x04){
        LCD_PORT_DB6_SET;
    }else{
        LCD_PORT_DB6_CLR;        
    }
    if(d&0x08){
        LCD_PORT_DB7_SET;
    }else{
        LCD_PORT_DB7_CLR;        
    }
}
/* typical sequence, but not used internally
void putc_lcd4(char c, char rs){
    __delay_us(52); //  >40us 
    if(rs) 
        LCD_RS_SET;
    else
        LCD_RS_CLR;
    lcd_port_hi_out((c>>4) & 0x000f);
    sc1602_strobe();
    lcd_port_hi_out(c & 0x000f);
    sc1602_strobe();
}
*/

void sc1602_power(bool pow)
{
    if(pow){
        UART_puts("sc1602 on\n");
        CNPDACLR = _CNPDA_CNPDA0_MASK | _CNPDA_CNPDA1_MASK | _CNPDA_CNPDA2_MASK | _CNPDA_CNPDA3_MASK;
        CNPDBCLR = _CNPDB_CNPDB8_MASK | _CNPDB_CNPDB9_MASK | _CNPDB_CNPDB2_MASK;

        TRISACLR = _TRISA_TRISA0_MASK | _TRISA_TRISA1_MASK | _TRISA_TRISA2_MASK | _TRISA_TRISA3_MASK;
        TRISBCLR = _TRISB_TRISB8_MASK | _TRISB_TRISB9_MASK ;
        TRISBCLR = _TRISB_TRISB2_MASK;
    }else{
        UART_puts("sc1602 off\n");
        TRISASET = _TRISA_TRISA0_MASK | _TRISA_TRISA1_MASK | _TRISA_TRISA2_MASK | _TRISA_TRISA3_MASK;
        TRISBSET = _TRISB_TRISB8_MASK | _TRISB_TRISB9_MASK ;
        TRISBSET = _TRISB_TRISB2_MASK;
        CNPDASET = _CNPDA_CNPDA0_MASK | _CNPDA_CNPDA1_MASK | _CNPDA_CNPDA2_MASK | _CNPDA_CNPDA3_MASK;
        CNPDBSET = _CNPDB_CNPDB8_MASK | _CNPDB_CNPDB9_MASK | _CNPDB_CNPDB2_MASK;
    }
}
typedef enum {
    SC1602_LCD_CMD,
    SC1602_LCD_DATA,
    SC1602_LCD_WAIT,
    SC1602_LCD_END,
    SC1602_LCD_INIT1,
    SC1602_LCD_INIT2,
    SC1602_LCD_INIT3,
} SC1602_BK_STATE;

void sc1602_que_handler(void)
{
    static StateInfo sc1602_que_state={0,0,0};
    char rs;
    if(sc1602_que_start==0)
        return;
    sc1602_que_state.previous = sc1602_que_state.current;
    switch(sc1602_que_data[sc1602_que_state.current].cmd){
        case SC1602_LCD_CMD:
        case SC1602_LCD_DATA:
            rs = sc1602_que_data[sc1602_que_state.current].cmd;
            if(rs)
                LCD_RS_SET;
            else
                LCD_RS_CLR;
            lcd_port_hi_out(((sc1602_que_data[sc1602_que_state.current].data)>>4) & 0x000f);
            sc1602_strobe();
            lcd_port_hi_out((sc1602_que_data[sc1602_que_state.current].data)&0x000f);
            sc1602_strobe();
            sc1602_que_state.current++;
            break;
        case SC1602_LCD_WAIT: //wait n cycles
            if(sc1602_que_data[sc1602_que_state.current].data==0){
                sc1602_que_state.current++;
            }else{
                sc1602_que_data[sc1602_que_state.current].data--;
            }
            break;
        case SC1602_LCD_END: // end
            sc1602_que_state.current = 0;
            sc1602_que_state.count = 0;
            sc1602_que_start = 0;
            break;
        case SC1602_LCD_INIT1: //
            lcd_port_hi_out(0x03);
            sc1602_strobe();
            sc1602_que_state.current++;
            break;
        case SC1602_LCD_INIT2: //
//            __delay_ms(5); /* > 4.1ms */
            sc1602_strobe();
            sc1602_que_state.current++;
            break;
        case SC1602_LCD_INIT3: //
//            __delay_us(128); /* >100us */
            sc1602_strobe();

            /* set 4bit mode */
            lcd_port_hi_out(0x02);
            sc1602_strobe();
            sc1602_que_state.current++;
            break;
        default :
            sc1602_que_state.current = 0;
            sc1602_que_state.count = 0;
            sc1602_que_start = 0;
            break;
    }
    if(sc1602_que_state.previous != sc1602_que_state.current){
        if(sc1602_que_state.count<0xffff)        
            sc1602_que_state.count++;        
    }
}

void sc1602_set_que(const char *upper_disp,const char *lower_disp)
{
    int i=0,j;
    if(g_possible_sc1602_reset){
        sc1602_power(0);
        g_sc1602_initialized = false;
        g_possible_sc1602_reset = false;
    }
    if(!g_sc1602_initialized){
        sc1602_power(1);
        sc1602_que_data[i].cmd = SC1602_LCD_WAIT;
        sc1602_que_data[i].data = 30/(SC1602_BK_INTERVAL_MS);
        i++;        
        sc1602_que_data[i].cmd = SC1602_LCD_INIT1;
        sc1602_que_data[i].data = 0;
        i++;        
        sc1602_que_data[i].cmd = SC1602_LCD_WAIT;
        sc1602_que_data[i].data = 4.1/(SC1602_BK_INTERVAL_MS);
        i++;        
        sc1602_que_data[i].cmd = SC1602_LCD_INIT2;
        sc1602_que_data[i].data = 0;
        i++;        
        sc1602_que_data[i].cmd = SC1602_LCD_INIT3;
        sc1602_que_data[i].data = 0;
        i++;        

        sc1602_que_data[i].cmd = SC1602_LCD_CMD;   
        sc1602_que_data[i].data = 0x28;   /* display mode  2 lines 0b0010NFxx N:display lines,Font*/
        i++;
        sc1602_que_data[i].cmd = SC1602_LCD_CMD;  
        sc1602_que_data[i].data = 0x0c;     /* display on */
        i++;
        sc1602_que_data[i].cmd = SC1602_LCD_CMD;  
        sc1602_que_data[i].data = 0x01;    /* Clear Display */
        i++;
//        sc1602_que_data[i].cmd = SC1602_LCD_WAIT;
//        sc1602_que_data[i].data = 2/(SC1602_BK_INTERVAL_MS);
//        i++;        
        sc1602_que_data[i].cmd = SC1602_LCD_CMD;  
        sc1602_que_data[i].data = 0x06;  /* entry mode 0b000001(I/D)S, I/D=1:incremental,Shift */
        i++;
        g_sc1602_initialized = true;
    }
    sc1602_que_data[i].cmd = SC1602_LCD_WAIT;
    sc1602_que_data[i].data = 0x01;
    i++;
    sc1602_que_data[i].cmd = SC1602_LCD_CMD;  
    sc1602_que_data[i].data = ((0)&0x7F)|0x80;  // Set Position to 0
    i++;
    for(j=0;upper_disp[j]&&j<16;j++){
        sc1602_que_data[i].cmd = SC1602_LCD_DATA;  
        sc1602_que_data[i].data = upper_disp[j];  
        i++;        
    }
    for(;j<16;j++){
        sc1602_que_data[i].cmd = SC1602_LCD_DATA;  
        sc1602_que_data[i].data = ' '  ;
        i++;        
    }
    sc1602_que_data[i].cmd = SC1602_LCD_CMD;  
    sc1602_que_data[i].data = ((0x40)&0x7F)|0x80;    // Set position to 0x40
    i++;
    for(j=0;lower_disp[j]&&j<16;j++){
        sc1602_que_data[i].cmd = SC1602_LCD_DATA;  
        sc1602_que_data[i].data = lower_disp[j];  
        i++;        
    }
    for(;j<16;j++){
        sc1602_que_data[i].cmd = SC1602_LCD_DATA;  
        sc1602_que_data[i].data = ' ' ; 
        i++;        
    }
    sc1602_que_data[i].cmd = SC1602_LCD_END;  
    sc1602_que_data[i].data = 0;  
    i++;
    sc1602_que_start = true;
    IEC1SET = _IEC1_CCT2IE_MASK;
}


