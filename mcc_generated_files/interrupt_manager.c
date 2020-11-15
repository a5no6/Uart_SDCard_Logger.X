/**
  System Interrupts Generated Driver File 

  @Company:
    Microchip Technology Inc.

  @File Name:
    interrupt_manager.h

  @Summary:
    This is the generated driver implementation file for setting up the
    interrupts using PIC24 / dsPIC33 / PIC32MM MCUs

  @Description:
    This source file provides implementations for PIC24 / dsPIC33 / PIC32MM MCUs interrupts.
    Generation Information : 
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.125
        Device            :  PIC32MM0064GPL028
    The generated drivers are tested against the following:
        Compiler          :  XC32 v2.20
        MPLAB             :  MPLAB X v5.20
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

/**
    Section: Includes
*/
#include <xc.h>

/**
    void INTERRUPT_Initialize (void)
*/
void INTERRUPT_Initialize (void)
{
    // Enable Multi Vector Configuration
    INTCONbits.MVEC = 1;
    
    //    RTCI: Real Time Clock Alarm
    //    Priority: 2
        IPC3bits.RTCCIP = 2;
    //    Sub Priority: 0
        IPC3bits.RTCCIS = 0;
    //    CCPI: CCP 2 Input Capture or Output Compare
    //    Priority: 3
        IPC7bits.CCP2IP = 3;
    //    Sub Priority: 0
        IPC7bits.CCP2IS = 0;
    //    CCTI: CCP 2 Timer
    //    Priority: 3
        IPC8bits.CCT2IP = 3;
    //    Sub Priority: 0
        IPC8bits.CCT2IS = 0;
    //    UERI: UART 2 Error
    //    Priority: 7
        IPC10bits.U2EIP = 7;
    //    Sub Priority: 0
        IPC10bits.U2EIS = 0;
    //    UTXI: UART 2 Transmission
    //    Priority: 7
        IPC10bits.U2TXIP = 7;
    //    Sub Priority: 0
        IPC10bits.U2TXIS = 0;
    //    URXI: UART 2 Reception
    //    Priority: 7
        IPC10bits.U2RXIP = 7;
    //    Sub Priority: 0
        IPC10bits.U2RXIS = 0;
    //    CNBI: PORT B Change Notification
    //    Priority: 6
        IPC2bits.CNBIP = 6;
    //    Sub Priority: 0
        IPC2bits.CNBIS = 0;
    //    UERI: UART 1 Error
    //    Priority: 2
        IPC6bits.U1EIP = 2;
    //    Sub Priority: 0
        IPC6bits.U1EIS = 0;
    //    UTXI: UART 1 Transmission
    //    Priority: 2
        IPC6bits.U1TXIP = 2;
    //    Sub Priority: 0
        IPC6bits.U1TXIS = 0;
    //    URXI: UART 1 Reception
    //    Priority: 2
        IPC5bits.U1RXIP = 2;
    //    Sub Priority: 0
        IPC5bits.U1RXIS = 0;
    //    TI: Timer 1
    //    Priority: 4
        IPC2bits.T1IP = 4;
    //    Sub Priority: 0
        IPC2bits.T1IS = 0;
}
