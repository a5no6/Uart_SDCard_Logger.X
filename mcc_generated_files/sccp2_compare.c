/**
  SCCP2 Generated Driver File 

  @Company
    Microchip Technology Inc.

  @File Name
    sccp2.c

  @Summary
    This is the generated driver implementation file for the SCCP2 driver using PIC24 / dsPIC33 / PIC32MM MCUs

  @Description
    This header file provides implementations for driver APIs for SCCP2. 
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
  Section: Included Files
*/

#include "sccp2_compare.h"

/** OC Mode.

  @Summary
    Defines the OC Mode.

  @Description
    This data type defines the OC Mode of operation.

*/

static uint16_t         gSCCP2Mode;

/**
  Section: Driver Interface
*/


void SCCP2_COMPARE_Initialize (void)
{
    // ON enabled; MOD Dual Edge Compare, Buffered(PWM); ALTSYNC disabled; SIDL disabled; OPS Each Time Base Period Match; CCPSLP disabled; TMRSYNC disabled; RTRGEN disabled; CCSEL disabled; ONESHOT disabled; TRIGEN disabled; T32 16 Bit; SYNC None; OPSSRC Timer Interrupt Event; TMRPS 1:4; CLKSEL REFO; 
    CCP2CON1 = (0x8145 & 0xFFFF7FFF); //Disabling CCPON bit
    //ASDGM disabled; ICGSM Level-Sensitive mode; ICS ICM2; SSDG disabled; AUXOUT OC Signal; ASDG 0; PWMRSEN disabled; OCAEN enabled; OENSYNC enabled; 
    CCP2CON2 = 0x81180000;
    //OETRIG disabled; OSCNT None; POLACE disabled; PSSACE Tri-state; 
    CCP2CON3 = 0x00;
    //SCEVT disabled; TRSET disabled; ICOV disabled; ASEVT disabled; ICGARM disabled; RBWIP disabled; TRCLR disabled; RAWIP disabled; TMRHWIP disabled; TMRLWIP disabled; PRLWIP disabled; 
    CCP2STAT = 0x00;
    //TMRL 0; TMRH 0; 
    CCP2TMR = 0x00;
    //PRH 0; PRL 16383; 
    CCP2PR = 0x3FFF;
    //CMPA 0; 
    CCP2RA = 0x00;
    //CMPB 8191; 
//    CCP2RB = 0x1FFF;
    CCP2RB = 0x0;
    //BUFL 0; BUFH 0; 
    CCP2BUF = 0x00;

    CCP2CON1bits.ON = 0x1; //Enabling CCP

    gSCCP2Mode = CCP2CON1bits.MOD;
    // Clearing IF flag before enabling the interrupt.
    IFS0CLR= 1 << _IFS0_CCP2IF_POSITION;
    // Enabling SCCP2 interrupt.
    IEC0bits.CCP2IE = 1;

    // Clearing IF flag before enabling the interrupt.
    IFS1CLR= 1 << _IFS1_CCT2IF_POSITION;
    // Enabling SCCP2 interrupt.
    IEC1bits.CCT2IE = 1;
}

void __attribute__ ((weak)) SCCP2_COMPARE_CallBack(void)
{
    // Add your custom callback code here
}

void __attribute__ ( ( vector ( _CCP2_VECTOR ), interrupt ( IPL3SOFT ))) CCP2_ISR (void)
{
    if(IFS0bits.CCP2IF)
    {
		// SCCP2 COMPARE callback function 
		SCCP2_COMPARE_CallBack();
		
        IFS0CLR= 1 << _IFS0_CCP2IF_POSITION;
    }
}

void __attribute__ ((weak)) SCCP2_COMPARE_TimerCallBack(void)
{
    // Add your custom callback code here
}

void __attribute__ ( ( vector ( _CCT2_VECTOR ), interrupt ( IPL3SOFT ))) CCT2_ISR (void)
{
    if(IFS1bits.CCT2IF)
    {
		// SCCP2 COMPARE Timer callback function 
		SCCP2_COMPARE_TimerCallBack();
	
        IFS1CLR= 1 << _IFS1_CCT2IF_POSITION;
    }
}

void SCCP2_COMPARE_Start( void )
{
    /* Start the Timer */
    CCP2CON1SET = (1 << _CCP2CON1_ON_POSITION);
}

void SCCP2_COMPARE_Stop( void )
{
    /* Start the Timer */
    CCP2CON1CLR = (1 << _CCP2CON1_ON_POSITION);
}

void SCCP2_COMPARE_SingleCompare16ValueSet( uint16_t value )
{   
    CCP2RA = value;
}


void SCCP2_COMPARE_DualCompareValueSet( uint16_t priVal, uint16_t secVal )
{

    CCP2RA = priVal;

    CCP2RB = secVal;
}

void SCCP2_COMPARE_DualEdgeBufferedConfig( uint16_t priVal, uint16_t secVal )
{

    CCP2RA = priVal;

    CCP2RB = secVal;
}

void SCCP2_COMPARE_CenterAlignedPWMConfig( uint16_t priVal, uint16_t secVal )
{

    CCP2RA = priVal;

    CCP2RB = secVal;
}

void SCCP2_COMPARE_EdgeAlignedPWMConfig( uint16_t priVal, uint16_t secVal )
{

    CCP2RA = priVal;

    CCP2RB = secVal;
}

void SCCP2_COMPARE_VariableFrequencyPulseConfig( uint16_t priVal )
{
    CCP2RA = priVal;
}

bool SCCP2_COMPARE_IsCompareCycleComplete( void )
{
    return(IFS0bits.CCP2IF);
}

bool SCCP2_COMPARE_TriggerStatusGet( void )
{
    return( CCP2STATbits.CCPTRIG );
    
}

void SCCP2_COMPARE_TriggerStatusSet( void )
{
    CCP2STATSET = (1 << _CCP2STAT_TRSET_POSITION);
}

void SCCP2_COMPARE_TriggerStatusClear( void )
{
    /* Clears the trigger status */
    CCP2STATCLR = (1 << _CCP2STAT_TRCLR_POSITION);
}

bool SCCP2_COMPARE_SingleCompareStatusGet( void )
{
    return( CCP2STATbits.SCEVT );
}

void SCCP2_COMPARE_SingleCompareStatusClear( void )
{
    /* Clears the trigger status */
    CCP2STATCLR = (1 << _CCP2STAT_SCEVT_POSITION);
    
}
/**
 End of File
*/
