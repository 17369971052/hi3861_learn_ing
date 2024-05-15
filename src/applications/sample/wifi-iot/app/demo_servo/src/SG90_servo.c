#include "hi_gpio.h"
#include "hi_io.h"
#include "hi_pwm.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cmsis_os2.h"
#include "SG90_servo.h"
/*
static void Servo_init(void)
{
    IoTGpioInit(HI_GPIO_IDX_2);
    IoTGpioSetFunc(HI_GPIO_IDX_2,HI_IO_FUNC_GPIO_2_GPIO);
    IoTGpioSetDir(HI_GPIO_IDX_2, HI_GPIO_DIR_OUT);
    IoTGpioSetOutputVal(HI_GPIO_IDX_2,HI_GPIO_VALUE0);
}


void set_angle(hi_u32 utime) {
     Servo_init();
     IoTGpioSetOutputVal(HI_GPIO_IDX_2,HI_GPIO_VALUE1);
     hi_udelay(utime);
     IoTGpioSetOutputVal(HI_GPIO_IDX_2,HI_GPIO_VALUE0);
     hi_udelay(19350-utime);
}*/