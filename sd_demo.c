/******************************************************************************
*
*                        Microchip File I/O Library
*
******************************************************************************
* FileName:           main.c
* Dependencies:       sd_spi.h
*                     fileio.h
*                     main.h
*                     rtcc.h
* Processor:          PIC24/dsPIC30/dsPIC33
* Compiler:           XC16
* Company:            Microchip Technology, Inc.
*
* Software License Agreement
*
* The software supplied herewith by Microchip Technology Incorporated
* (the "Company") for its PICmicro(R) Microcontroller is intended and
* supplied to you, the Company's customer, for use solely and
* exclusively on Microchip PICmicro Microcontroller products. The
* software is owned by the Company and/or its supplier, and is
* protected under applicable copyright laws. All rights are reserved.
* Any use in violation of the foregoing restrictions may subject the
* user to criminal sanctions under applicable laws, as well as to
* civil liability for the breach of the terms and conditions of this
* license.
*
* THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
* TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
* IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
* CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
*
********************************************************************/

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

StateInfo sd_state = {DEMO_STATE_NO_MEDIA,DEMO_STATE_NO_MEDIA,0};
volatile bool g_flag_cache_sd = true;
bool g_sd_card_hybernate = false;
const char data_file_name[]=DATA_FILE_NAME;
char g_config_file_name[32]="SDLOG000.CFG";
volatile bool exist_config_file = false;

char data_file_path_name[32+9]="";
volatile int g_sd_data_num=0;
char log_time_stamp[21]="11/01-12:13:40 ";

static FILEIO_OBJECT g_file_obj;


const char *demo_state_string[]={
    "DEMO_STATE_NO_MEDIA",
    "DEMO_STATE_MEDIA_DETECTED",
    "DEMO_STATE_FIND_CONFIG_FILE",
    "DEMO_STATE_READ_CONFIG_FILE",
    "DEMO_STATE_SCAN_DATA_FILE",
    "DEMO_STATE_OPEN_DATA_FILE",
    "DEMO_STATE_CACHE_DATA",
    "DEMO_STATE_WAKEUP_SDCARD",
    "DEMO_STATE_WRITE_DATA_FILE",
    "DEMO_STATE_CLOSE_DATA_FILE",
    "DEMO_STATE_RENEW_DATA_FILE",
    "DEMO_STATE_DONE",
    "DEMO_STATE_RETRY_WAIT",
    "DEMO_STATE_FAILED"    
};

// The gSDDrive structure allows the user to specify which set of driver functions should be used by the
// FILEIO library to interface to the drive.
// This structure must be maintained as long as the user wishes to access the specified drive.
const FILEIO_DRIVE_CONFIG gSdDrive =
{
    (FILEIO_DRIVER_IOInitialize)FILEIO_SD_IOInitialize,                      // Function to initialize the I/O pins used by the driver.
    (FILEIO_DRIVER_MediaDetect)FILEIO_SD_MediaDetect,                       // Function to detect that the media is inserted.
    (FILEIO_DRIVER_MediaInitialize)FILEIO_SD_MediaInitialize,               // Function to initialize the media.
    (FILEIO_DRIVER_MediaDeinitialize)FILEIO_SD_MediaDeinitialize,           // Function to de-initialize the media.
    (FILEIO_DRIVER_SectorRead)FILEIO_SD_SectorRead,                         // Function to read a sector from the media.
    (FILEIO_DRIVER_SectorWrite)FILEIO_SD_SectorWrite,                       // Function to write a sector to the media.
    (FILEIO_DRIVER_WriteProtectStateGet)FILEIO_SD_WriteProtectStateGet,     // Function to determine if the media is write-protected.
};

void SD_SYSTEM_Initialize (void)
{
    SYSTEM_RegUnlock(); // unlock PPS
    RPCONbits.IOLOCK = 0;

    RPOR3bits.RP13R = 3; // RP13=RB13  3:SDO2
    RPOR2bits.RP10R = 4; //RP10=RB15=SCK2OUT
    RPINR11bits.SDI2R = 11 ; //RB7=RP11 (Pin11) 

    RPCONbits.IOLOCK = 1; // lock   PPS
    SYSTEM_RegLock(); 

    TRISBbits.TRISB13 = 0; 
    TRISBbits.TRISB15 = 0;
    CNPDBbits.CNPDB13 = 0;
    CNPDBbits.CNPDB15 = 0;
//    CNPUBbits.CNPUB12 = 1;
//    SPI1CONbits.MCLKSEL = 1; // USE REFCLKO as clock source
    TRISBbits.TRISB5 = 0;
}

void SD_SYSTEM_Deinitialize (void)
{
    SYSTEM_RegUnlock(); // unlock PPS
    RPCONbits.IOLOCK = 0;

    RPOR3bits.RP13R = 0; 
    RPOR2bits.RP10R = 0; 
    RPINR11bits.SDI2R = 0 ;  

    RPCONbits.IOLOCK = 1; // lock   PPS
    SYSTEM_RegLock(); 

    TRISBbits.TRISB13 = 1; 
    TRISBbits.TRISB15 = 1;
    CNPDBbits.CNPDB13 = 1;
    CNPDBbits.CNPDB15 = 1;
    TRISBbits.TRISB5 = 1;
//    CNPDBbits.CNPDB5 = 1;
//    CNPUBbits.CNPUB5 = 1;

//    CNPUBbits.CNPUB12 = 1;
//    SPI1CONbits.MCLKSEL = 1; // USE REFCLKO as clock source
}


void USER_SdSpiConfigurePins (void)
{
    // Deassert the chip select pin
//    LATBbits.LATB1 = 1;
    // Configure CS pin as an output
//    ANSELBbits.ANSB5 = 0;
    TRISBbits.TRISB5 = 0;
    // Configure CD pin as an input
//    TRISFbits.TRISF0 = 1;
    // Configure WP pin as an input
//    TRISFbits.TRISF1 = 1;
}

inline void USER_SdSpiSetCs(uint8_t a)
{
    LATBbits.LATB5 = a;
}

//inline bool USER_SdSpiGetCd(void)
bool USER_SdSpiGetCd(void)
{
//    TRISBbits.TRISB12 = 1;
//    CNPUBSET = _CNPUB_CNPUB12_MASK;
    if(PORTBbits.RB12){
        g_sdcard_detected = false;
    }else{
//       CNPUBCLR = _CNPUB_CNPUB12_MASK;       
       g_sdcard_detected = true;
    }
    return(g_sdcard_detected);
//   return (!PORTBbits.RB12) ? true : false;
//    return true;
}

inline bool USER_SdSpiGetWp(void)
{
//    return (PORTFbits.RF1) ? true : false;
    return false;
}

// The sdCardMediaParameters structure defines user-implemented functions needed by the SD-SPI fileio driver.
// The driver will call these when necessary.  For the SD-SPI driver, the user must provide
// parameters/functions to define which SPI module to use, Set/Clear the chip select pin,
// get the status of the card detect and write protect pins, and configure the CS, CD, and WP
// pins as inputs/outputs (as appropriate).
// For this demo, these functions are implemented in system.c, since the functionality will change
// depending on which demo board/microcontroller you're using.
// This structure must be maintained as long as the user wishes to access the specified drive.
FILEIO_SD_DRIVE_CONFIG sdCardMediaParameters =
{
    2,                                  // Use SPI module 2
    USER_SdSpiSetCs,                    // User-specified function to set/clear the Chip Select pin.
    USER_SdSpiGetCd,                    // User-specified function to get the status of the Card Detect pin.
    USER_SdSpiGetWp,                    // User-specified function to get the status of the Write Protect pin.
    USER_SdSpiConfigurePins             // User-specified function to configure the pins' TRIS bits.
};

void GetTimestamp (FILEIO_TIMESTAMP * timeStamp)
{
 //   BSP_RTCC_DATETIME dateTime;
     struct tm dateTime;

  //  dateTime.bcdFormat = false;

    RTCC_TimeGet(&dateTime);

    timeStamp->timeMs = 0;
    timeStamp->time.bitfield.hours = dateTime.tm_hour;
    timeStamp->time.bitfield.minutes = dateTime.tm_min;
    timeStamp->time.bitfield.secondsDiv2 = dateTime.tm_sec / 2;

    timeStamp->date.bitfield.day = dateTime.tm_mday;
    timeStamp->date.bitfield.month = dateTime.tm_mon;
    // Years in the RTCC module go from 2000 to 2099.  Years in the FAT file system go from 1980-2108.
    timeStamp->date.bitfield.year = dateTime.tm_year + 20;;
}



void SDCard_task(void)
{
    static int fnum = 0,pos_num = 0;
    int i,j,c;
    static unsigned short total_written = 0;
    static unsigned long file_num_high ,file_num_low,file_num_step;
    static unsigned short sd_wakeup_retry = 0;
    
    sd_state.previous = sd_state.current;
    switch (sd_state.current)
    {
        case DEMO_STATE_NO_MEDIA:
_PL();
            if(g_sd_unmount_request){ // clear unmount request in case there had been while SD card is abscent
                g_sd_unmount_request = false;
            }
            // Loop on this function until the SD Card is detected.
            if (FILEIO_MediaDetect(&gSdDrive, &sdCardMediaParameters) == true)
            {
                UART_puts("SD Media detected.\n");
                UART_flush();
                g_possible_sc1602_reset = true;
                sd_state.current = DEMO_STATE_MEDIA_DETECTED;
                file_num_high=1024;
                file_num_low=0;
                file_num_step = file_num_high/2;
                fnum = 0;
                pos_num = 0;
                total_written = 0;
                g_sd_data_num=0;
                SD_SYSTEM_Initialize();
            }
_PL();
//while(1);
            break;

        case DEMO_STATE_MEDIA_DETECTED:
_PL();
#if defined (__XC8__)
            // When initializing an SD Card on PIC18, the the SPI clock must be between 100 and 400 kHz.
            // The largest prescale divider in most PIC18 parts is 64x, which means the clock frequency must
            // be between 400 kHz (for a 4x divider) and 25.6 MHz (for a 64x divider).  In this demo, we are
            // achieving this by disabling the PIC's PLL during the drive mount operation (note that the
            // SYS_CLK_FrequencySystemGet() function must accurate reflect this change).  You could also map the "slow"
            // SPI functions used by the sd-spi driver to an SPI implementation that will run between
            // 100 kHz and 400 kHz at Fosc values greater than 25.6 MHz.  The macros to remap are located in
            // fileio_config.h.
            OSCTUNEbits.PLLEN = 0;
#endif
            // Try to mount the drive we've defined in the gSdDrive structure.
            // If mounted successfully, this drive will use the drive Id 'A'
            // Since this is the first drive we're mounting in this application, this
            // drive's root directory will also become the current working directory
            // for our library.
            SDCard_Power(1);
            
            if (FILEIO_DriveMount('A', &gSdDrive, &sdCardMediaParameters) == FILEIO_ERROR_NONE)
            {
                sd_state.current = DEMO_STATE_FIND_CONFIG_FILE;
//                strcpy(g_config_file_name,config_file_name);
            }
            else
            {
                sd_state.current = DEMO_STATE_FAILED;
            }
#if defined (__XC8__)
            OSCTUNEbits.PLLEN = 1;
#endif
            break;
        case DEMO_STATE_RETRY_WAIT:
            if(sd_state.count>20000)
                sd_state.current = DEMO_STATE_NO_MEDIA;
            break;
        case DEMO_STATE_FIND_CONFIG_FILE:
            // Open SDLOG000.CFG
            fnum = file_num_low + file_num_step;
//            while(fnum<100){
            i2a_digi('0',3,g_config_file_name+5,fnum);
            UART_puts("Open g_file ");
            UART_puts(g_config_file_name);
            UART_puts("\n");
            UART_flush();

            if (FILEIO_Open (&g_file_obj, g_config_file_name, FILEIO_OPEN_READ ) == FILEIO_RESULT_FAILURE)
            {
                file_num_high = fnum;
                UART_puts("file not opened.\n");
                UART_flush();
            }else{
                file_num_low = fnum;
                if (FILEIO_Close (&g_file_obj) != FILEIO_RESULT_SUCCESS){
                    UART_puts("file close failed.\n");
                    UART_puts(g_config_file_name);
                    UART_puts("\n");
                    UART_flush();
                }
            }
            file_num_step = file_num_step/2;
            if(file_num_step==0){
                sd_state.current = DEMO_STATE_READ_CONFIG_FILE;
            }
            if(fnum==0){
                /* If there is no config file, skip with default configuration. */
                sd_state.current = DEMO_STATE_OPEN_DATA_FILE; 
                break;
            }
            break;
        case DEMO_STATE_READ_CONFIG_FILE:

//            fnum--;
            i2a_digi('0',3,g_config_file_name+5,file_num_low);
            UART_puts("Read from ");
            UART_puts(g_config_file_name);
            UART_puts("\n");
            UART_flush();

            if (FILEIO_Open (&g_file_obj, g_config_file_name, FILEIO_OPEN_READ ) == FILEIO_RESULT_FAILURE)
            {
                UART_puts("file not opened.\n");
                UART_flush();
//                sd_state.current = DEMO_STATE_SCAN_DATA_FILE;
                exist_config_file = false;
//                break;
            }else{
                exist_config_file = true;

                while((c = FILEIO_GetChar (&g_file_obj))!=FILEIO_RESULT_FAILURE )
                {
                    read_opt(c);
                }

                if (FILEIO_Close (&g_file_obj) != FILEIO_RESULT_SUCCESS){
                    UART_puts("file close failed.\n");
                    UART_puts(g_config_file_name);
                    UART_puts("\n");
                    UART_flush();
                }
            }
            sd_state.current = DEMO_STATE_SCAN_DATA_FILE;
            file_num_high=8192;
            file_num_low=0;
            file_num_step = file_num_high/2;
            for(i=0;i<32;i++)
                if(data_file_path_name[i]==0)
                    break;
_PL();
            for(j=0;data_file_name[j]!=0;j++,i++){
                data_file_path_name[i]=data_file_name[j];
                if(pos_num==0 && data_file_path_name[i]=='0')
                    pos_num = i;
            }
            UART_puts(data_file_path_name);
            UART_puts("\n");
            UART_flush();
_PL();
            UART_put_int32(pos_num);
            UART_puts("\n");
            UART_flush();
            data_file_path_name[i++] = 0;
            break;

        case DEMO_STATE_SCAN_DATA_FILE:
            g_sd_data_num = file_num_low + file_num_step;
//            while(g_sd_data_num<1000){
            i2a_digi('0',4,data_file_path_name+pos_num,g_sd_data_num);
            UART_puts("Open file ");
            UART_puts(data_file_path_name);
            UART_puts("\n");
            UART_flush();
_PL();

            if (FILEIO_Open (&g_file_obj, data_file_path_name, FILEIO_OPEN_READ ) == FILEIO_RESULT_FAILURE)
            {
                file_num_high = g_sd_data_num;
                UART_puts("file not opened.\n");
                UART_flush();
//                    break;
            }else{
                file_num_low = g_sd_data_num;
            }                  
//                g_sd_data_num++;
#ifdef SD_WRITELESS_CHECK
break; /// For test , open for read
#endif
            if (FILEIO_Close (&g_file_obj) != FILEIO_RESULT_SUCCESS){
                UART_puts("file close failed.\n");
                UART_puts(g_config_file_name);
                UART_puts("\n");
                UART_flush();
            }
_PL();
            file_num_step = file_num_step/2;
            if(file_num_step==0){
                sd_state.current = DEMO_STATE_OPEN_DATA_FILE;
                g_sd_data_num = file_num_high;
            }
            if(g_sd_data_num>=8190){
                sd_state.current = DEMO_STATE_FAILED;
                break;
            }
            break;
        case DEMO_STATE_OPEN_DATA_FILE:

#ifdef SD_WRITELESS_CHECK
            if (0)// for test , do not open for write.
#else
            i2a_digi('0',4,data_file_path_name+pos_num,g_sd_data_num);
            UART_puts("Open for write: ");
            UART_puts(data_file_path_name);
            UART_puts("\n");
            UART_flush();
//            if (FILEIO_Open (&g_file_obj, data_file_path_name, FILEIO_OPEN_WRITE | FILEIO_OPEN_APPEND | FILEIO_OPEN_CREATE) == FILEIO_RESULT_FAILURE)
            if (FILEIO_Open (&g_file_obj, data_file_path_name, FILEIO_OPEN_WRITE | FILEIO_OPEN_CREATE) == FILEIO_RESULT_FAILURE)
#endif
            {
                UART_puts("file not opened.\n");
                UART_flush();
                sd_state.current = DEMO_STATE_FAILED;
                break;
            }
            sd_state.current = DEMO_STATE_WRITE_DATA_FILE;
            break;

        case DEMO_STATE_CACHE_DATA:
            if(UART_SD_stored()>UART_SD_FLUSH_IF_REACH || g_sd_unmount_request){
                if(g_sd_card_hybernate){
                    sd_state.current = DEMO_STATE_WAKEUP_SDCARD;
                    UART_puts("SDCard Power On.\n");
                    SDCard_Power(1);
                }else{
                    sd_state.current = DEMO_STATE_WRITE_DATA_FILE;                    
                }
            }
            if(0&&rx_rate_byte_per_sec>0){
                UART_put_uint32(rx_rate_byte_per_sec);
                UART_puts("\n");
            }
            if(!g_sd_card_hybernate &&
                    (rx_rate_byte_per_sec==0||((512*3)/rx_rate_byte_per_sec)>REMAINING_TIME_SEC_TO_POWER_OFF_SD)){  // no write more than 20s
                g_sd_card_hybernate = true;
                SDCard_Power(0);
                UART_puts("SDCard Power Off.\n");
            }
            break;
        case DEMO_STATE_WAKEUP_SDCARD:
            FILEIO_SD_MediaInitialize(&sdCardMediaParameters);
            if(mediaInformation.errorCode == MEDIA_NO_ERROR){
                g_sd_card_hybernate = false;
                sd_state.current = DEMO_STATE_WRITE_DATA_FILE;
                sd_wakeup_retry = 0;
            }else{
                sd_wakeup_retry++;
                SDCard_Power(0);                                
                if(sd_wakeup_retry<SD_WAKEUP_RETRY_MAX){
                    sd_state.current = DEMO_STATE_CACHE_DATA;
                }else{
                    sd_state.current = DEMO_STATE_FAILED;                
                }
                UART_puts("SD wake up retry ");
                UART_put_uint16(sd_wakeup_retry);
                UART_puts("\n");
            }
            break;

        case DEMO_STATE_WRITE_DATA_FILE:
            while(UART_SD_stored()>0){
                char c = UART_SD_READ();
#ifdef SD_WRITELESS_CHECK   
                char buf[2]={c,0};
                if(0) 
#else
                if (FILEIO_Write (&c, 1, 1, &g_file_obj) != 1)
#endif
                {
                    sd_state.current = DEMO_STATE_FAILED;
                    UART_puts("file write failed.\n");
//                    UART_flush();                    
                    break;
                }
                total_written++;
            }
            if(!IEC1bits.U2RXIE){  /* Do I need this? */
                IEC1SET = _IEC1_U2RXIE_MASK;
                UART_puts("U2RXIE enabled\n");
            }
            if(g_sd_unmount_request){ //
                sd_state.current = DEMO_STATE_CLOSE_DATA_FILE;
                g_sd_unmount_request = false;
            }else{
                sd_state.current = DEMO_STATE_CACHE_DATA;
                g_sd_card_hybernate = false;
    ////// add sd power off code here.
            }
            break;

        case DEMO_STATE_RENEW_DATA_FILE:
            // Close the file to save the data
            UART_puts("Close file for new file.\n");
            UART_flush();
            if (FILEIO_Close (&g_file_obj) != FILEIO_RESULT_SUCCESS)
            {
                sd_state.current = DEMO_STATE_FAILED;
                break;
            }else{
                UART_puts("file closed.\n");
                UART_flush();                    
            }
            sd_state.current = DEMO_STATE_OPEN_DATA_FILE;
            break;
        case DEMO_STATE_CLOSE_DATA_FILE:
            // Close the file to save the data
            UART_puts("Close file\n");
            UART_flush();
#ifndef SD_WRITELESS_CHECK   
            if (FILEIO_Close (&g_file_obj) != FILEIO_RESULT_SUCCESS)
            {
                sd_state.current = DEMO_STATE_FAILED;
                break;
            }else{
                UART_puts("file closed.\n");
                UART_flush();                    
            }
#endif 
            // We're done with this drive.
            // Unmount it.
            UART_puts("Unmount Drive.\n");
            UART_flush();
            SDCard_Power(0);
            FILEIO_DriveUnmount ('A');
            sd_state.current = DEMO_STATE_DONE;
//            CNPUBSET = _CNPUB_CNPUB12_MASK; // Card detect //done by sdcard_detect_power_task())
            UART_puts("SD Done.\n");
            UART_flush();
            exist_config_file = false;
            break;
        case DEMO_STATE_DONE:
            // Now that we've written all of the data we need to write in the application, wait for the user
            // to remove the card
            if (FILEIO_MediaDetect(&gSdDrive, &sdCardMediaParameters) == false)
            {
                sd_state.current = DEMO_STATE_NO_MEDIA;                   
                data_file_path_name[0]=0;                
                SD_SYSTEM_Deinitialize();
            }
            break;
        case DEMO_STATE_FAILED:
            // An operation has failed.  Try to unmount the drive.  This will also try to
            // close all open files that use this drive (it will at least deallocate them).
            if(sd_state.count==0){
                FILEIO_DriveUnmount ('A');
            }
            // Return to the media-detect state
            if (FILEIO_MediaDetect(&gSdDrive, &sdCardMediaParameters) == false){
                sd_state.current = DEMO_STATE_NO_MEDIA;                   
            }
            break;
    }
    if(sd_state.current!=sd_state.previous){
        UART_puts(demo_state_string[sd_state.previous]);
        UART_puts("->");
        UART_puts(demo_state_string[sd_state.current]);
        UART_puts("\n");
        sd_state.count = 0;
        if(sd_state.current==DEMO_STATE_FAILED||sd_state.current==DEMO_STATE_DONE){
            g_flag_cache_sd = false;                        
        }else{
            g_flag_cache_sd = true;            
        }
    }else{
        if(sd_state.count<0xffff){
            sd_state.count++;
        }
    }
}


