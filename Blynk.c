// -------------------------------------------------------------------
// File name: Blynk.c
// Description: This code is used to bridge the TM4C123 board and the Blynk Application
//              via the ESP8266 WiFi board
// Author: Mark McDermott and Andrew Lynch (Arduino source)
// Converted to EE445L style Jonathan Valvano
// Orig gen date: May 21, 2018
// Last update: Sept 20, 2018
//
// Download latest Blynk library here:
//   https://github.com/blynkkk/blynk-library/releases/latest
//
//  Blynk is a platform with iOS and Android apps to control
//  Arduino, Raspberry Pi and the likes over the Internet.
//  You can easily build graphic interfaces for all your
//  projects by simply dragging and dropping widgets.
//
//   Downloads, docs, tutorials: http://www.blynk.cc
//   Sketch generator:           http://examples.blynk.cc
//   Blynk community:            http://community.blynk.cc
//
//------------------------------------------------------------------------------

// TM4C123       ESP8266-ESP01 (2 by 4 header)
// PE5 (U5TX) to Pin 1 (Rx)
// PE4 (U5RX) to Pin 5 (TX)
// PE3 output debugging
// PE2 nc
// PE1 output    Pin 7 Reset
// PE0 input     Pin 3 Rdy IO2
//               Pin 2 IO0, 10k pullup to 3.3V  
//               Pin 8 Vcc, 3.3V (separate supply from LaunchPad 
// Gnd           Pin 4 Gnd  
// Place a 4.7uF tantalum and 0.1 ceramic next to ESP8266 3.3V power pin
// Use LM2937-3.3 and two 4.7 uF capacitors to convert USB +5V to 3.3V for the ESP8266
// http://www.ti.com/lit/ds/symlink/lm2937-3.3.pdf
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "tm4c123gh6pm.h"
#include "ST7735.h"
#include "PLL.h"
#include "Timer2.h"
#include "Timer3.h"
#include "UART.h"
#include "PortF.h"
#include "esp8266.h"
//#include "Lab3.h"
#include <stdbool.h>
#include "Timer1.h"
#include "clockface.h"
#include "SoundDriver.h"

void EnableInterrupts(void);    // Defined in startup.s
void DisableInterrupts(void);   // Defined in startup.s
void WaitForInterrupt(void);    // Defined in startup.s

uint32_t LED;      // VP1
uint32_t LastF;    // VP74
// These 6 variables contain the most recent Blynk to TM4C123 message
// Blynk to TM4C123 uses VP0 to VP15
char serial_buf[64];
char Pin_Number[2]   = "99";       // Initialize to invalid pin number
char Pin_Integer[8]  = "0000";     //
char Pin_Float[8]    = "0.0000";   //
uint32_t pin_num; 
uint32_t pin_int;
 
extern long StartCritical (void);    // previous I bit, disable interrupts
extern void EndCritical(long sr);    // restore I bit to previous value
extern void EnableInterrupts(void);
#define PE0                     (*((volatile uint32_t *)0x40024004))
#define PF3                     (*((volatile uint32_t *)0x40025020))
#define PF2                     (*((volatile uint32_t *)0x40025010))
#define PF1                     (*((volatile uint32_t *)0x40025008))

//the actual time
uint32_t recordedSeconds;
//alarm time
uint32_t alarmSeconds;

//the screen showing on the lcd
enum DisplayState {
    ClockView,
    ClockSet,
    AlarmSet,
};

enum DisplayState displayState;
enum DisplayState prevDisplayState;

//true if sound is playing
bool alarmShouldPlay = false;
//true if sound is triggered when reach alarm time
bool isAlarmOn = false;
bool prevAlarmOn = false;
//holds hours during clock/alarm set
uint32_t setTimeHour = 12;
uint32_t prevSetTimeHour = 0;
//holds minutes during clock/alarm set
uint32_t setTimeMinute = 0;
uint32_t prevSetTimeMinute = 0;

//which state to send to blynk
uint16_t sendblynk; //0:hour 1:minute 2:second
//which state to receive from blynk
uint16_t receiveblynk; 

//timer callback
void UpdateSeconds(void) {
    recordedSeconds++;
    recordedSeconds %= 43200;
    if (alarmSeconds == recordedSeconds) {
        alarmShouldPlay = true;
    }
}

//4 buttons:
//displaymode: cycles through display states when pressed, if not pressed for 3 seconds of no buttons pressed displayState returns to ClockView
//alarm silence/on/off: if alarm sounding turns off otherwise enables/disables alarm
//hour set: cycles hour in clockset and alarmset modes
//min set: cycles minute in clockset and alarmset modes

//callback for mode button
void onCycleDisplayState(void) {
    if (displayState == AlarmSet) displayState = ClockView;
    else displayState += 1;
}

//callback for alarm silence/on/off button
void onAlarmSilence(void) {
    if (alarmShouldPlay) {
        alarmShouldPlay = false;
    } else if (displayState == ClockView) {
        isAlarmOn = !isAlarmOn;
    }
}

//callback for hour set button
void onCycleHourSetTime(void) {
    if (displayState == ClockSet || displayState == AlarmSet) {
        if (setTimeHour == 12) {
            setTimeHour = 1;
        } else {
            setTimeHour++;
        }
    }
}

//callback for minute set button
void onCycleMinuteSetTime(void) {
    if (displayState == ClockSet || displayState == AlarmSet) {
        if (setTimeMinute == 60) {
            setTimeMinute = 0;
        } else {
            setTimeMinute++;
        }
    }
}

//callback for timer to return to clockview after being in timeset or alarmset
void onReturnToClockView() {
    if (displayState == ClockSet) {
        recordedSeconds = setTimeHour * 3600 + setTimeMinute * 60;
    } else if (displayState == AlarmSet) {
        alarmSeconds = setTimeHour * 3600 + setTimeMinute * 60;
    }
}


 
// ----------------------------------- TM4C_to_Blynk ------------------------------
// Send data to the Blynk App
// It uses Virtual Pin numbers between 70 and 99
// so that the ESP8266 knows to forward the data to the Blynk App
void TM4C_to_Blynk(uint32_t pin,uint32_t value){
  if((pin < 70)||(pin > 99)){
    return; // ignore illegal requests
  }
// your account will be temporarily halted if you send too much data
  ESP8266_OutUDec(pin);       // Send the Virtual Pin #
  ESP8266_OutChar(',');
  ESP8266_OutUDec(value);      // Send the current value
  ESP8266_OutChar(',');
  ESP8266_OutString("0.0\n");  // Null value not used in this example
}
 
 
// -------------------------   Blynk_to_TM4C  -----------------------------------
// This routine receives the Blynk Virtual Pin data via the ESP8266 and parses the
// data and feeds the commands to the TM4C.
void Blynk_to_TM4C(void){int j; char data;
// Check to see if a there is data in the RXD buffer
  if(ESP8266_GetMessage(serial_buf)){  // returns false if no message
    // Read the data from the UART5
#ifdef DEBUG1
    j = 0;
    do{
      data = serial_buf[j];
      UART_OutChar(data);        // Debug only
      j++;
    }while(data != '\n');
    UART_OutChar('\r');        
#endif
           
// Rip the 3 fields out of the CSV data. The sequence of data from the 8266 is:
// Pin #, Integer Value, Float Value.
    strcpy(Pin_Number, strtok(serial_buf, ","));
    strcpy(Pin_Integer, strtok(NULL, ","));       // Integer value that is determined by the Blynk App
    strcpy(Pin_Float, strtok(NULL, ","));         // Not used
    pin_num = atoi(Pin_Number);     // Need to convert ASCII to integer
    pin_int = atoi(Pin_Integer);  
  // ---------------------------- VP #1 ----------------------------------------
  // This VP is the LED select button
    if(pin_num == 0x01)  {  
      LED = pin_int;
      PortF_Output(LED<<2); // Blue LED
			onCycleDisplayState();
#ifdef DEBUG3
      Output_Color(ST7735_CYAN);
      ST7735_OutString("Rcv VP1 data=");
      ST7735_OutUDec(LED);
      ST7735_OutChar('\n');
#endif
    }  
    else if(pin_num == 0x02)  {  
       // UP
#ifdef DEBUG4
      Output_Color(ST7735_CYAN);
      ST7735_OutString("Rcv VP2 data=");
      ST7735_OutUDec(LED);
      ST7735_OutChar('\n');
#endif
    } 
		else if(pin_num == 0x03)  {  
       // DOWN
#ifdef DEBUG5
      Output_Color(ST7735_CYAN);
      ST7735_OutString("Rcv VP3 data=");
      ST7735_OutUDec(LED);
      ST7735_OutChar('\n');
#endif
    } 
		else if(pin_num == 0x04)  {  
       // LEFT
#ifdef DEBUG6
      Output_Color(ST7735_CYAN);
      ST7735_OutString("Rcv VP4 data=");
      ST7735_OutUDec(LED);
      ST7735_OutChar('\n');
#endif
    } 
		else if(pin_num == 0x05)  {  
       // RIGHT
#ifdef DEBUG7
      Output_Color(ST7735_CYAN);
      ST7735_OutString("Rcv VP5 data=");
      ST7735_OutUDec(LED);
      ST7735_OutChar('\n');
#endif
    } 
    else if(pin_num == 0x07)  {  
      void onAlarmSilence(void); // Virtual Button2
#ifdef DEBUG8
      Output_Color(ST7735_CYAN);
      ST7735_OutString("Rcv VP7 data=");
      ST7735_OutUDec(LED);
      ST7735_OutChar('\n');
#endif
    } 
    else if(pin_num == 0x08)  {  
       void onCycleHourSetTime(void);// Virtual Button3
#ifdef DEBUG9
      Output_Color(ST7735_CYAN);
      ST7735_OutString("Rcv VP8 data=");
      ST7735_OutUDec(LED);
      ST7735_OutChar('\n');
#endif
    } 		
		else if(pin_num == 0x09)  {  
       void onCycleMinuteSetTime(void);// Virtual Button4
#ifdef DEBUG10
      Output_Color(ST7735_CYAN);
      ST7735_OutString("Rcv VP9 data=");
      ST7735_OutUDec(LED);
      ST7735_OutChar('\n');
#endif
    } // Parse incoming data        
#ifdef DEBUG1
    UART_OutString(" Pin_Number = ");
    UART_OutString(Pin_Number);
    UART_OutString("   Pin_Integer = ");
    UART_OutString(Pin_Integer);
    UART_OutString("   Pin_Float = ");
    UART_OutString(Pin_Float);
    UART_OutString("\n\r");
#endif
  }  
}

void SendInformation(void){
	static uint16_t minuteOrhour = 0;
/*  uint32_t thisF;
  thisF = PortF_Input();
// your account will be temporarily halted if you send too much data
  if(thisF != LastF){
    TM4C_to_Blynk(74, thisF);  // VP74*/
	if(minuteOrhour == 0){
		minuteOrhour = 1;
		sendblynk =0;
	}
	else if(minuteOrhour == 1){
		minuteOrhour = 2;
		sendblynk = 1;
	}
	else if(minuteOrhour == 2){
		minuteOrhour = 0;
		sendblynk = 2;
	}
#ifdef DEBUG3
    Output_Color(ST7735_WHITE);
    ST7735_OutString("Send 74 data=");
    //ST7735_OutUDec(thisF);
    ST7735_OutChar('\n');
#endif
	  //LastF = thisF;
}

  
int main(void){       
  PLL_Init(Bus80MHz);   // Bus clock at 80 MHz
  DisableInterrupts();  // Disable interrupts until finished with inits
  PortF_Init();
  LastF = PortF_Input();
/*#ifdef DEBUG3
  Output_Init();        // initialize ST7735
  ST7735_OutString("EE445L Lab 4D\nBlynk example\n");
#endif
#ifdef DEBUG1
  UART_Init(5);         // Enable Debug Serial Port
  UART_OutString("\n\rEE445L Lab 4D\n\rBlynk example");
#endif
  ESP8266_Init();       // Enable ESP8266 Serial Port
  ESP8266_Reset();      // Reset the WiFi module
  ESP8266_SetupWiFi();  // Setup communications to Blynk Server  
  Timer2_Init(&Blynk_to_TM4C,800000); */
  // check for receive data from Blynk App every 10ms

  Timer3_Init(&SendInformation,40000000); 
  // Send data back to Blynk App every 1/2 second
 // EnableInterrupts();
 
  Clockface_Init();
	Sound_Init();

	//SwitchPE_Init(&onAlarmSilence, &onCycleHourSetTime, &onCycleMinuteSetTime);
	//SwitchPF4_Init(&onCycleDisplayState);

	Timer1_Init(80000000, &UpdateSeconds);
	Clockface_Draw();
  Clockface_setAlarmOn(false);
	EnableInterrupts();
    while (1) {
				uint16_t hour = recordedSeconds / 3600;
        uint16_t minute = (recordedSeconds - hour*3600) / 60;
        //uint16_t second = recordedSeconds - hour*3600 - minute*60;
			  if(sendblynk == 0){
					TM4C_to_Blynk(74, hour);
				}
				else if(sendblynk == 1){
					TM4C_to_Blynk(75, minute);
				}
        //cache things that may change
        const uint32_t seconds = recordedSeconds;
        const enum DisplayState currentState = displayState;

        if (currentState == ClockView) {
            long it = StartCritical();
            const bool alarmOn = isAlarmOn;
            const bool playAlarm = alarmShouldPlay;
            EndCritical(it);
            if (prevDisplayState != ClockView) {
                Clockface_Draw();
                Clockface_setAlarmOn(alarmOn);
            }

            if (alarmOn != prevAlarmOn) {
                prevAlarmOn = alarmOn;
                Clockface_setAlarmOn(alarmOn);
            }

            if (alarmOn && playAlarm) {
                Sound_Play();
            } else {
                Sound_Stop();
            }

            Clockface_setDigitalTime(seconds);
            Clockface_setGraphicalTime(seconds);
        } else if (currentState == ClockSet) {
            long it = StartCritical();
            uint32_t setHour = setTimeHour;
            uint32_t setMin = setTimeMinute;
            EndCritical(it);
            if (prevDisplayState != ClockSet) {
                Clockface_DrawClockSet();
                setHour = 12;
                setMin = 0;
                Clockface_setClockSetTime(setHour,setMin);
            }
            if (setHour != prevSetTimeHour || setMin != prevSetTimeMinute) {
                prevSetTimeHour = setHour;
                prevSetTimeMinute = setMin;
                Clockface_setClockSetTime(setHour, setMin);
            }
        } else if (currentState == AlarmSet) {
            long it = StartCritical();
            uint32_t setHour = setTimeHour;
            uint32_t setMin = setTimeMinute;
            EndCritical(it);
            if (prevDisplayState != AlarmSet) {
                Clockface_DrawAlarmSet();
                setHour = 12;
                setMin = 0;
                Clockface_setClockSetTime(setHour,setMin);
            }
            if (setHour != prevSetTimeHour || setMin != prevSetTimeMinute) {
                prevSetTimeHour = setHour;
                prevSetTimeMinute = setMin;
                Clockface_setClockSetTime(setHour, setMin);
            }
        }
        prevDisplayState = currentState;

	}
	
}


