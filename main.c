/**
  Generated main.c file from MPLAB Code Configurator

  @Company
    Microchip Technology Inc.

  @File Name
    main.c

  @Summary
    This is the generated main.c using PIC24 / dsPIC33 / PIC32MM MCUs.

  @Description
    This source file provides main entry point for system initialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.125
        Device            :  PIC32MM0064GPL028
    The generated drivers are tested against the following:
        Compiler          :  XC16 v1.36B
        MPLAB 	          :  MPLAB X v5.20
*/

/*
    (c) 2016 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/
/*  Known bugs:
 * 
 *  1. After SC1602 is turned off, it does not work properly after 2nd turn on.(Only use with SD,more clock 1MHz)
 *     Do not enable sc1602_power, if you want use this program right now.
 *     (DO_NOT_TURN_OFF_SC1602)
 *  2. SSC2 Timer interrupt disabled somewhere, so re-enable interrupt in main 
 *     (refer FIXED_CCT2IE_DISABLED_BUG)
 *  3. sleep current is 0.17mA,they shoud be uA order.
 *     (refer TRY_TO_USE_PMD)
 *  4. Without SD card and after sleep, new UART reception causes system reset.
 *  
 */
#undef TRY_TO_USE_PMD
#undef FIXED_CCT2IE_DISABLED_BUG
#undef DO_NOT_TURN_OFF_SC1602
#undef USE_CLOCK_SWITCH /* It is better to get idle than lower the clock. */
//#define WORKAROUND_WAKEUP_RESET   /* The cause of reset seems to be PUB12. After getting right SD sokcet, design proper logic. */

/*
 * It seems, with about 10M Bytes file write, FAT sector(frequently used) will be changed. 
 */

/**
  Section: Included Files
*/
#include <proc/p32mm0064gpl028.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdbool.h>
#include "mcc_generated_files/system.h"
#include "sc1602.h"
#include "fileio.h"
#include <string.h>
#include "mcc_generated_files/rtcc.h"
#include "mcc_generated_files/interrupt_manager.h"
//#include "mcc_generated_files/watchdog.h"
//#include "system.h"
#include "logger.h"
#include "sd_demo.h"
#include "sd_spi.h"
#include "uart_print.h"
#undef _PL
#define _PL()

#define USE_SHADOW_REGISTER
// In uart2.c use
//void __attribute__ ((vector(_UART2_RX_VECTOR), interrupt(IPL7SRS))) _UART2_RX( void )
// else use
//void __attribute__ ((vector(_UART2_RX_VECTOR), interrupt(IPL7SOFT))) _UART2_RX( void )

#define DISPTASK_USE_UART_LOG

//#define SD_WRITELESS_CHECK    1


//#define _NI(x) /* Disable UART_put related function when in interrupt */
#define _NI(x)  x

void RTCC_CallBack(void)
{
    struct tm ntime;
    is_woken_by_rtcc = true;
    if(RTCC_TimeGet(&ntime)){
        i2a_digi('0',2,log_time_stamp+0,ntime.tm_mon);
        log_time_stamp[2]='/';
        i2a_digi('0',2,log_time_stamp+3,ntime.tm_mday);
        log_time_stamp[5]='-';
        i2a_digi('0',2,log_time_stamp+6,ntime.tm_hour);
        log_time_stamp[8]=':';
        i2a_digi('0',2,log_time_stamp+9,ntime.tm_min);
        log_time_stamp[11]=':';
        i2a_digi('0',2,log_time_stamp+12,ntime.tm_sec);                
        log_time_stamp[14]=' ';
        log_time_stamp[15]=0;
    }
}

void UART2_ChangeISRHandler(void)
{
    U2MODECLR = _U2MODE_ON_MASK;   // enabling UART ON bit
    IEC1bits.U2RXIE = 0;

    UART2_SetRxInterruptHandler(local_UART2_Receive_ISR);

    IEC1bits.U2RXIE = 1;
    
    //Make sure to set LAT bit corresponding to TxPin as high before UART initialization
    U2MODESET = _U2MODE_ON_MASK;   // enabling UART ON bit
}

void  TMR1_Local_InterruptHandler(void)
{ 
        if(!g_flag_10ms)
            g_flag_10ms=true;
        g_count_10ms++;
        SW_task();
        disp_task();
//        LED_task();
        calc_rx_rate_task();
}

unsigned int __Count_Per_microsec = CLOCK_InstructionFrequencyGet()/2;

#ifdef USE_CLOCK_SWITCH
void CLOCK_switch(unsigned long clock_hz)
{
    UART_puts("clock change to ");
    UART_put_int32(clock_hz);
    UART_puts("hz\n");
    switch(clock_hz){
        case 24000000:
            asm("di");
            SYSTEM_RegUnlock();
            asm("ei");
            // TUN Center frequency; 
            OSCTUN = 0x00;
            // PLLODIV 1:1; PLLMULT 3x; PLLICLK FRC; 
            SPLLCON = 0x10080;
            // SBOREN disabled; VREGS disabled; RETEN disabled; 
            PWRCON = 0x00;
            //CF No Clock Failure has been detected;; FRCDIV FRC/1; SLPEN Device will enter Idle mode when a WAIT instruction is issued; NOSC SPLL; SOSCEN disabled; CLKLOCK Clock and PLL selections are not locked and may be modified; OSWEN Switch is Complete; 
            OSCCON = (0x100);
            asm("di");
            SYSTEM_RegLock();
            asm("ei");
            // ON enabled; DIVSWEN disabled; RSLP disabled; ROSEL FRC; OE disabled; SIDL disabled; RODIV 0; 
            REFO1CON = 0x8003;
            while(!REFO1CONbits.ACTIVE & REFO1CONbits.ON);
            // ROTRIM 0; 
            REFO1TRIM = 0x00;
            __Count_Per_microsec = 12;
            break;
        default:
            asm("di");
            SYSTEM_RegUnlock();
            asm("ei");
            // TUN Center frequency; 
            OSCTUN = 0x00;
            // PLLODIV 1:1; PLLMULT 3x; PLLICLK FRC; 
            SPLLCON = 0x10080;
            // SBOREN disabled; VREGS disabled; RETEN disabled; 
            PWRCON = 0x00;
            //CF No Clock Failure has been detected;; FRCDIV FRC/1; SLPEN Device will enter Idle mode when a WAIT instruction is issued; NOSC FRCDIV; SOSCEN disabled; CLKLOCK Clock and PLL selections are not locked and may be modified; OSWEN Switch is Complete; 
            OSCCON = (0x00);
            switch(clock_hz){
                case 4000000:
                    OSCCONbits.FRCDIV = 1;
                    break;
                case 2000000:
                    OSCCONbits.FRCDIV = 2;
                    break;
                case 1000000:
                    OSCCONbits.FRCDIV = 3;
                    break;
                case 8000000:
                default:
                    OSCCONbits.FRCDIV = 0;
                    clock_hz = 8000000;
                    break;
            }
            SYSTEM_RegLock();
            // ON enabled; DIVSWEN disabled; RSLP disabled; ROSEL FRC; OE disabled; SIDL disabled; RODIV 0; 
            REFO1CON = 0x8003;
            while(!REFO1CONbits.ACTIVE & REFO1CONbits.ON);
            // ROTRIM 0; 
            REFO1TRIM = 0x00;
            __Count_Per_microsec = clock_hz/2000000;
            break;
    }
}
#endif

void SCCP2_COMPARE_TimerCallBack(void)
{
    sc1602_que_handler();
    if(sc1602_que_start==0){
        IEC1CLR = _IEC1_CCT2IE_MASK;
    }    
}

uint8_t ConvertHexToBCD(uint8_t hexvalue)
{
    uint8_t bcdvalue;
    bcdvalue = (hexvalue / 10) << 4;
    bcdvalue = bcdvalue | (hexvalue % 10);
    return (bcdvalue);
}

int main(void)
{
    
    // initialize the device
    TMR1_SetInterruptHandler(TMR1_Local_InterruptHandler);
#ifdef USE_SHADOW_REGISTER
    PRISSbits.PRI7SS = 1;
#endif
    SYSTEM_Initialize();
    SD_SYSTEM_Deinitialize();
    UART2_ChangeISRHandler();
    IEC0bits.CCP2IE = 0;   
    IEC1bits.CCT2IE = 0;
    IEC1CLR = _IEC1_U2EIE_MASK;  //Polling
    IEC0CLR = _IEC0_CNBIE_MASK; // Disable All CNB 

    CNPUAbits.CNPUA4 = 1;

    CNPUBbits.CNPUB0 = 1;
    CNPUBbits.CNPUB1 = 1;
    CNPUBbits.CNPUB6 = 1;
    
    LATBbits.LATB14 = 1;
    TRISBbits.TRISB14 = 0;
    
    CNPUBSET = _CNPUB_CNPUB12_MASK;
    CNEN1Bbits.CNIE1B12 = 0;    //Pin : RB12 Card detect

#ifdef FLIP_SOFT_RX_CAPTURE_TIMING
    TRISAbits.TRISA4 = 0;
#endif
    SDCard_Power(0);

    UART_puts("Hello.\n");
    UART_flush();
    UART_puts("IPL ");UART_put_uint32(_CP0_GET_VIEW_IPL());UART_puts("\n");
    // Register the GetTimestamp function as the timestamp source for the library.
    FILEIO_RegisterTimestampGet (GetTimestamp);
    UART_puts("Hello uart2.\n");
//    sc1602_set_que("Hello.","Please wait.");
    set_software_rx_param();
    UART_puts("g_softrx_first_capture=");
    UART_put_uint16(g_softrx_first_capture);
    UART_puts("\n");
    UART_puts("g_softrx_bit_interval=");
    UART_put_uint16(g_softrx_bit_interval);
    UART_puts("\n");
    
//    PRISSbits.PRI7SS
    UART_puts("PRISS = ");
    UART_put_HEX32(PRISS);
    UART_puts("\n");
    
#ifdef TRY_TO_USE_PMD
    asm("di");
    SYSTEM_RegUnlock();
    asm("ei");
    PMDCONCLR = _PMDCON_PMDLOCK_MASK;
    PMD1 = 0xffffffff;
    PMD2 = 0xffffffff;
    PMD3SET = _PMD3_CCP1MD_MASK; 
    PMD3SET = _PMD3_CCP3MD_MASK; 
    PMD5SET = _PMD5_SPI1MD_MASK; 
    PMD7 = 0xffffffff;
    PMDCONSET = _PMDCON_PMDLOCK_MASK;
    asm("di");
    SYSTEM_RegLock();
    asm("ei");
#endif    
    FILEIO_Initialize();
    while(1){
        logger_main();
    }
    return 1; 
}

/**
 End of File
*/

