/**
  PIN MANAGER Generated Driver File

  @Company:
    Microchip Technology Inc.

  @File Name:
    pin_manager.c

  @Summary:
    This is the generated manager file for the PIC24 / dsPIC33 / PIC32MM MCUs device.  This manager
    configures the pins direction, initial state, analog setting.
    The peripheral pin select, PPS, configuration is also handled by this manager.

  @Description:
    This source file provides implementations for PIN MANAGER.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.125
        Device            :  PIC32MM0064GPL028
    The generated drivers are tested against the following:
        Compiler          :  XC32 v2.20
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


/**
    Section: Includes
*/

#include <xc.h>
#include "pin_manager.h"
#include "system.h"

/**
 Section: Driver Interface Function Definitions
*/
void PIN_MANAGER_Initialize (void)
{
    /****************************************************************************
     * Setting the Output Latch SFR(s)
     ***************************************************************************/
    LATA = 0x0000;
    LATB = 0x4004;
    LATBSET = _LATB_LATB3_MASK;
    LATC = 0x0000;

    /****************************************************************************
     * Setting the GPIO Direction SFR(s)
     ***************************************************************************/
    TRISA = 0x0010;
//    TRISB = 0xB4BB;
//    TRISB = 0xB4BF;
    TRISB = 0xB4B7;
    TRISBCLR = _TRISB_TRISB3_MASK;
    TRISC = 0x0200;

    /****************************************************************************
     * Setting the Weak Pull Up and Weak Pull Down SFR(s)
     ***************************************************************************/
    CNPDA = 0x0000;
    CNPDB = 0x0000;
    CNPDC = 0x0000;
    CNPUA = 0x0010;
//    CNPUB = 0x0010;
    CNPUB = 0x0000;
    CNPUC = 0x0000;

    /****************************************************************************
     * Setting the Open Drain SFR(s)
     ***************************************************************************/
    ODCA = 0x0000;
    ODCB = 0x0000;
    ODCC = 0x0000;

    /****************************************************************************
     * Setting the Analog/Digital Configuration SFR(s)
     ***************************************************************************/
    ANSELA = 0x0000;
//    ANSELB = 0xB008;
    ANSELB = 0x0000;

    /****************************************************************************
     * Set the PPS
     ***************************************************************************/
    SYSTEM_RegUnlock(); // unlock PPS
    RPCONbits.IOLOCK = 0;

//    RPOR4bits.RP18R = 0x0006;    //RB11->SCCP2:OCM2
    RPINR9bits.U2RXR = 0x0011;    //RB10->UART2:U2RX

    RPCONbits.IOLOCK = 1; // lock   PPS
    SYSTEM_RegLock(); 

    
    /****************************************************************************
     * Interrupt On Change: negative
     ***************************************************************************/
    CNEN1Bbits.CNIE1B10 = 1;    //Pin : RB10
    CNEN1Bbits.CNIE1B4 = 1;    //Pin : RB4
    CNEN1Bbits.CNIE1B12 = 1;    //Pin : RB12 Card detect

    /****************************************************************************
     * Interrupt On Change: positive
     ***************************************************************************/
    CNEN0Bbits.CNIE0B12 = 1;    //Pin : RB12 Card detect
    
    /****************************************************************************
     * Interrupt On Change: flag
     ***************************************************************************/
    CNFBbits.CNFB10 = 0;    //Pin : RB10
    CNFBbits.CNFB4 = 0;    //Pin : RB4
    CNFBbits.CNFB12 = 0;    //Pin : 
    
    /****************************************************************************
     * Interrupt On Change: config
     ***************************************************************************/
    CNCONBbits.CNSTYLE = 1;    //Config for PORTB
    CNCONBbits.ON = 1;    //Config for PORTB

    /****************************************************************************
     * Interrupt On Change: Interrupt Enable
     ***************************************************************************/
    IFS0CLR= 1 << _IFS0_CNBIF_POSITION; //Clear CNBI interrupt flag
    IEC0bits.CNBIE = 1; //Enable CNBI interrupt
}

/* Interrupt service routine for the CNBI interrupt. */
void __attribute__ ((vector(_CHANGE_NOTICE_B_VECTOR), interrupt(IPL6SOFT))) _CHANGE_NOTICE_B( void )
{
//    IEC0CLR = _IEC0_CNBIE_MASK; //Clear CNBI interrupt flag for chattering
    if(IFS0bits.CNBIF == 1)
    {
        // Clear the flag
        IFS0CLR= 1 << _IFS0_CNBIF_POSITION; // Clear IFS0bits.CNBIF
        if(CNFBbits.CNFB4 == 1)
        {
//            CNFBCLR = 0x10;  //Clear CNFBbits.CNFB4
            // Add handler code here for Pin - RB4
        }
        if(CNFBbits.CNFB10 == 1)
        {
//            CNFBCLR = 0x400;  //Clear CNFBbits.CNFB10
            // Add handler code here for Pin - RB10
        }
        if(CNFBbits.CNFB12 == 1)
        {
//            CNFBCLR = 0x1000;  //Clear CNFBbits.CNFB12
            // Add handler code here for Pin - RB12
        }
    }
}
