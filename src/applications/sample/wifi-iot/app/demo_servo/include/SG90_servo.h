#ifndef __SG90_servo__
#define __SG90_servo__

#define IOT_GPIO_PWM 2
#define PWM_SIGNAL_PERIOD 20000
#define PWM_SIGNAL_MIN_VALUE 500
#define PWM_SIGNAL_MAX_VALUE 20000
#define PWM_SIGNAL_BASE_VALUE 1500
#define minInAngle 0

#define COUNT 10
#define FREQ_TIME 20000
#define IOT_IO_NAME_GPIO_2 HI_IO_NAME_GPIO_2

#include "hi_gpio.h"
#include "hi_io.h"
#include "hi_pwm.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cmsis_os2.h"

/*
typedef struct {
    int angle;
} ServoController;
*/
void Servo_init(void);
//void set_angle();

#endif


