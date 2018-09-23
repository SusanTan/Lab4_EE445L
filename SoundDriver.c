/*
 * SoundDriver.c
 *
 *  Created on: Sep 14, 2018
 */

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "PWM.h"

void Sound_Init(void) {
    int pwm_period = 90909; //440hz
    PWM0A_Init(pwm_period, pwm_period/4);
    PWM0A_Disable();
}

void Sound_Play() {
    PWM0A_Enable();
}

void Sound_Stop() {
    PWM0A_Disable();
}
