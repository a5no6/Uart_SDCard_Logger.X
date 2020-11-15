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


// Declare a state machine for our device
typedef enum
{
    DEMO_STATE_NO_MEDIA = 0,
    DEMO_STATE_MEDIA_DETECTED,
    DEMO_STATE_FIND_CONFIG_FILE,
    DEMO_STATE_READ_CONFIG_FILE,
    DEMO_STATE_SCAN_DATA_FILE,
    DEMO_STATE_OPEN_DATA_FILE,
    DEMO_STATE_CACHE_DATA,
    DEMO_STATE_WAKEUP_SDCARD,
    DEMO_STATE_WRITE_DATA_FILE,
    DEMO_STATE_CLOSE_DATA_FILE,
    DEMO_STATE_RENEW_DATA_FILE,
    DEMO_STATE_DONE,
    DEMO_STATE_RETRY_WAIT,
    DEMO_STATE_FAILED
} DEMO_STATE;

extern StateInfo sd_state;

void GetTimestamp (FILEIO_TIMESTAMP * timeStamp);

extern volatile bool g_flag_cache_sd ;
extern FILEIO_MEDIA_INFORMATION mediaInformation;
extern bool g_sd_card_hybernate ;

extern const char data_file_name[];
extern char g_config_file_name[32];
extern volatile bool exist_config_file ;

extern char data_file_path_name[32+9];
extern volatile int g_sd_data_num;
extern char log_time_stamp[21];


