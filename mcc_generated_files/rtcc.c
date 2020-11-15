
/**
  RTCC Generated Driver API Header File

  @Company:
    Microchip Technology Inc.

  @File Name:
    rtcc.c

  @Summary:
    This is the generated header file for the RTCC driver using PIC24 / dsPIC33 / PIC32MM MCUs

  @Description:
    This header file provides APIs for driver for RTCC.
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
 Section: Included Files
*/

#include "rtcc.h"
#include "system.h"

/**
  Section: Static function
*/


static bool rtccTimeInitialized;
static bool RTCCTimeInitialized(void);
static uint8_t ConvertHexToBCD(uint8_t hexvalue);
static uint8_t ConvertBCDToHex(uint8_t bcdvalue);

/**
   Section: Driver Interface Function Definitions
*/

void RTCC_Initialize(void)
{
  
   SYSTEM_RegUnlock();
   RTCCON1CLR = (1 << _RTCCON1_WRLOCK_POSITION);
   RTCCON1CLR = (1 << _RTCCON1_ON_POSITION);
     
   if(!RTCCTimeInitialized())
   {
       // set 2019-09-28 20-42-10
       RTCDATE = 0x19092806; // Year/Month/Date/Wday
       RTCTIME = 0x20421000; //  hours/minutes/seconds
       
   }
       // set 2019-09-28 20-42-10        
       ALMDATE = 0x92806; // Month/Date/Wday
       ALMTIME = 0x20421000; // hours/minutes/seconds
       // ON enabled; OUTSEL Alarm Event; WRLOCK disabled; AMASK Every Second; ALMRPT 1; RTCOE disabled; CHIME enabled; ALRMEN enabled; 
       RTCCON1 = 0xC1018000;
       // ON enabled; OUTSEL Alarm Event; WRLOCK disabled; AMASK Every Week; ALMRPT 128; RTCOE disabled; CHIME enabled; ALRMEN enabled; 
//       RTCCON1 = 0xC7808000;
//       RTCCON1bits.ALMRPT = 0;
   
       // DIV 15999; CLKSEL LPRC; FDIV 0; 
       RTCCON2 = 0x3E7F0001;       
       
   // Enable RTCC 
   RTCCON1SET = (1 << _RTCCON1_ON_POSITION);
   RTCCON1SET = (1 << _RTCCON1_WRLOCK_POSITION);
   SYSTEM_RegLock();

   IEC0bits.RTCCIE = 1;
}

bool RTCC_TimeGet(struct tm *currentTime)
{
    uint32_t register_value;
    if(RTCSTATbits.SYNC){
        return false;
    }
     
    register_value = RTCDATE;
    currentTime->tm_year = ConvertBCDToHex((register_value & 0xFF000000) >> 24);
    currentTime->tm_mon = ConvertBCDToHex((register_value & 0xFF0000) >> 16);
    currentTime->tm_mday = ConvertBCDToHex((register_value & 0xFF00) >> 8);
    currentTime->tm_wday = ConvertBCDToHex(register_value & 0xF);
    
    register_value = RTCTIME;
    currentTime->tm_hour = ConvertBCDToHex((register_value & 0xFF000000) >> 24);
    currentTime->tm_min = ConvertBCDToHex((register_value & 0xFF0000) >> 16);
    currentTime->tm_sec = ConvertBCDToHex((register_value & 0xFF00) >> 8);

    return true;
}

void RTCC_TimeSet(struct tm *initialTime)
{
   SYSTEM_RegUnlock();
   RTCCON1CLR = (1 << _RTCCON1_WRLOCK_POSITION);
   RTCCON1CLR = (1 << _RTCCON1_ON_POSITION);
  
     IFS0CLR= 1 << _IFS0_RTCCIF_POSITION;
     IEC0bits.RTCCIE = 0;

   // set RTCC initial time
   RTCDATE = (ConvertHexToBCD(initialTime->tm_year) << 24) | (ConvertHexToBCD(initialTime->tm_mon) << 16 ) | (ConvertHexToBCD(initialTime->tm_mday) << 8) | ConvertHexToBCD(initialTime->tm_wday) ; // YEAR/MONTH-1/DAY-1/WEEKDAY
   RTCTIME = (ConvertHexToBCD(initialTime->tm_hour) << 24) | (ConvertHexToBCD(initialTime->tm_min) << 16 ) | (ConvertHexToBCD(initialTime->tm_sec) << 8) ; // HOURS/MINUTES/SECOND
          
   // Enable RTCC 
   RTCCON1SET = (1 << _RTCCON1_ON_POSITION);
    
    // SET WRLOCK
   RTCCON1SET = (1 << _RTCCON1_WRLOCK_POSITION);
   SYSTEM_RegLock();

   //Enable RTCC interrupt
   IEC0bits.RTCCIE = 1;
}

bool RTCC_BCDTimeGet(bcdTime_t *currentTime)
{
    uint32_t register_value;
    if(RTCSTATbits.SYNC){
        return false;
    }
    
    register_value = RTCDATE;
    currentTime->tm_year = (register_value & 0xFF000000) >> 24;
    currentTime->tm_mon = (register_value & 0xFF0000) >> 16;
    currentTime->tm_mday = (register_value & 0xFF00) >> 8;
    currentTime->tm_wday = (register_value & 0xF);
    
    register_value = RTCTIME;
    currentTime->tm_hour = (register_value & 0xFF000000) >> 24;
    currentTime->tm_min = (register_value & 0xFF0000) >> 16;
    currentTime->tm_sec = (register_value & 0xFF00) >> 8;
    
    return true;
}

void RTCC_BCDTimeSet(bcdTime_t *initialTime)
{
   SYSTEM_RegUnlock();
   RTCCON1CLR = (1 << _RTCCON1_WRLOCK_POSITION);
   RTCCON1CLR = (1 << _RTCCON1_ON_POSITION);
   IFS0CLR= 1 << _IFS0_RTCCIF_POSITION;
   IEC0bits.RTCCIE = 0;

   // set RTCC initial time
   RTCDATE = (initialTime->tm_year << 24) | (initialTime->tm_mon << 16 ) | (initialTime->tm_mday << 8) | (initialTime->tm_wday) ; // YEAR/MONTH-1/DAY-1/WEEKDAY
   RTCTIME = (initialTime->tm_hour << 24) | (initialTime->tm_min << 16 ) | (initialTime->tm_sec << 8) ; // HOURS/MINUTES/SECOND
              
   // Enable RTCC  
   RTCCON1SET = (1 << _RTCCON1_ON_POSITION);
   
   // SET WRLOCK 
   RTCCON1SET = (1 << _RTCCON1_WRLOCK_POSITION);
   SYSTEM_RegLock();

   //Enable RTCC interrupt
   IEC0bits.RTCCIE = 1;
}

/**
 This function implements RTCC_TimeReset.This function is used to
 used by application to reset the RTCC value and reinitialize RTCC value.
*/
void RTCC_TimeReset(bool reset)
{
    rtccTimeInitialized = reset;
}

static bool RTCCTimeInitialized(void)
{
    return(rtccTimeInitialized);
}

static uint8_t ConvertHexToBCD(uint8_t hexvalue)
{
    uint8_t bcdvalue;
    bcdvalue = (hexvalue / 10) << 4;
    bcdvalue = bcdvalue | (hexvalue % 10);
    return (bcdvalue);
}

static uint8_t ConvertBCDToHex(uint8_t bcdvalue)
{
    uint8_t hexvalue;
    hexvalue = (((bcdvalue & 0xF0) >> 4)* 10) + (bcdvalue & 0x0F);
    return hexvalue;
}

void __attribute__ ((weak)) RTCC_CallBack(void)
{
    // Add your custom callback code here
}

/* Function:
  void __attribute__ ((vector(_RTCC_VECTOR), interrupt(IPL2SOFT))) _ISR_RTCC(void) 
  Summary:
    Interrupt Service Routine for the RTCC Peripheral

  Description:
    This is the interrupt service routine for the RTCC peripheral. Add in code if 
    required in the ISR. 
*/
void __attribute__ ((vector(_RTCC_VECTOR), interrupt(IPL2SOFT))) _ISR_RTCC(void) 
{
	// RTCC callback function 
	RTCC_CallBack();
	
    /* TODO : Add interrupt handling code */
      IFS0CLR= 1 << _IFS0_RTCCIF_POSITION;
}
/**
 End of File
*/
