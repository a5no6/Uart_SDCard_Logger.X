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
/* 
 * File:   uart_print.h
 * Author: konomu
 *
 * Created on 2019/04/14, 19:13
 */

#ifndef UART_PRINT_H
#define	UART_PRINT_H

#ifdef	__cplusplus
extern "C" {
#endif

void UART_puts(const char *buf);
void UART_init(unsigned long clk,unsigned short bps);
void UART_TX_BK_TASK(void);
unsigned char is_UART_busy(void);
void UART_flush(void);
int UART_put_uint8(unsigned char d);
int UART_put_int8(signed char d);
int UART_put_HEX8(unsigned char d);
int UART_put_HEX16(unsigned short d);
int UART_put_uint16(unsigned short d);
int UART_put_int16(short d);

#ifdef USE_DEBUG_LONG_TYPE
int UART_put_HEX32(unsigned long d);
int UART_put_uint32(unsigned long d);
int UART_put_int32(long d);
#endif

#ifdef UART_TX_INTERRUPT
void UART_TX_Interrupt_Handler(void);
#endif

#define _PL()	{UART_puts(__FILE__);UART_puts(":");UART_put_uint16(__LINE__);UART_puts("\n");UART_flush();}
//#define _PL()	UART_put_uint16(__LINE__);UART_puts("\n");UART_flush();



#ifdef	__cplusplus
}
#endif

#endif	/* UART_PRINT_H */

