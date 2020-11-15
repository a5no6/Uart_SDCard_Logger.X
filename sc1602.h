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

/*  SC1602(HD44780U) 4bit mode  background access  */ 


#ifndef _SC1602_H    /* Guard against multiple inclusion */
#define _SC1602_H
#ifdef __cplusplus
extern "C" {
#endif

/* Port setting SC1602 DB7-4 , RS,E*/
#define LCD_PORT_DB4_SET    LATASET = _LATA_LATA0_MASK
#define LCD_PORT_DB4_CLR    LATACLR = _LATA_LATA0_MASK
#define LCD_PORT_DB5_SET    LATASET = _LATA_LATA1_MASK
#define LCD_PORT_DB5_CLR    LATACLR = _LATA_LATA1_MASK
#define LCD_PORT_DB6_SET    LATASET = _LATA_LATA2_MASK
#define LCD_PORT_DB6_CLR    LATACLR = _LATA_LATA2_MASK
#define LCD_PORT_DB7_SET    LATASET = _LATA_LATA3_MASK
#define LCD_PORT_DB7_CLR    LATACLR = _LATA_LATA3_MASK

#define LCD_RS_SET    LATBSET = _LATB_LATB8_MASK
#define LCD_RS_CLR    LATBCLR = _LATB_LATB8_MASK
#define LCD_E_SET    LATBSET = _LATB_LATB9_MASK
#define LCD_E_CLR    LATBCLR = _LATB_LATB9_MASK
    
#define LCD_CMD 0
#define LCD_DATA 1

    
 void sc1602_set_que(const char *upper_disp_string , const char *lower_disp_string );
void sc1602_que_handler(void);

typedef struct {
    unsigned char cmd; // 0:cmd,1:data,2:skip,3:end
    unsigned char data;
} _sc1602_data_que ;

extern _sc1602_data_que sc1602_que_data[3+16+1+16+1+12];
extern bool sc1602_que_start;
extern bool g_sc1602_initialized;
void __delay_us(int s);
void __delay_ms(int s);
void sc1602_strobe(void);

#define SC1602_BK_INTERVAL_MS   (0x4000/(8000/4))/* CCP2PR/Clock_KHz/prescalor */

    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _SC1602_H */

/* *****************************************************************************
 End of File
 */
