/*
 * Lab3.c
 *
 *  Created on: Sep 14, 2018
 *      Author: drew
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "PLL.h"
#include "Timer1.h"
//#include "Switch.h"
#include "tm4c123gh6pm.h"
#include "clockface.h"
#include "SoundDriver.h"
#include "lab3.h"

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

void release() {

}

void Clock(void) {
//   PLL_Init(Bus80MHz);

	Clockface_Init();
	Sound_Init();

	//SwitchPE_Init(&onAlarmSilence, &onCycleHourSetTime, &onCycleMinuteSetTime);
	//SwitchPF4_Init(&onCycleDisplayState);

	Timer1_Init(80000000, &UpdateSeconds);
	Clockface_Draw();
  Clockface_setAlarmOn(false);
	EnableInterrupts();
    while (1) {
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



