#include <stdio.h>
#include <stdlib.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "hi_io.h"
#include "include/my_iot_gpio.h"
#include "iot_watchdog.h"
#include "hi_time.h"
#include "include/sg92r_control.h"

#define  COUNT   10
#define  FREQ_TIME    20000

void SetAngle(unsigned int duty)
{
    unsigned int time = FREQ_TIME;

    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_2, IOT_GPIO_VALUE1);
    hi_udelay(duty);
    IoTGpioSetOutputVal(IOT_IO_NAME_GPIO_2, IOT_GPIO_VALUE0);
    hi_udelay(time - duty);
}

/* The steering gear is centered
 * 1、依据角度与脉冲的关系，设置高电平时间为1500微秒
 * 2、不断地发送信号，控制舵机居中
*/
void RegressMiddle(void)
{
    unsigned int angle = 1500;
    for (int i = 0; i < COUNT; i++) {
        SetAngle(angle);
    }
}

/* Turn 90 degrees to the right of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为500微秒/1000微妙
 * 2、不断地发送信号，控制舵机向右旋转90度/转45度
*/
/*  Steering gear turn right */
void EngineTurnRight(void)
{
    unsigned int angle = 1000;
    for (int i = 0; i < COUNT; i++) {
        SetAngle(angle);
    }
}

/* Turn 90 degrees to the left of the steering gear
 * 1、依据角度与脉冲的关系，设置高电平时间为2500微秒/2000微秒
 * 2、不断地发送信号，控制舵机向左旋转90度/45度
*/
/* Steering gear turn left */
void EngineTurnLeft(void)
{
    unsigned int angle = 2000;
    for (int i = 0; i < COUNT; i++) {
        SetAngle(angle);
    }
}

void S92R_INNER_Init(void)
{
    IoTGpioInit(IOT_IO_NAME_GPIO_2);
    Io_Set_Func(IOT_IO_NAME_GPIO_2, IOT_IO_FUNC_GPIO_2_GPIO);
    IoTGpioSetDir(IOT_IO_NAME_GPIO_2, IOT_GPIO_DIR_OUT);
}

void Turn_Right_ON(void) {
    unsigned int time = 200;
    /*
    * 舵机左转90度
    * Steering gear turns 90 degrees to the left
    */
    EngineTurnLeft();
    Task_Msleep(time);
    /* 舵机归中 Steering gear centering */
    RegressMiddle();
    Task_Msleep(time);
}

void Turn_Left_OFF(void) {
    unsigned int time = 200;
    /*
        * 舵机右转90度
        * Steering gear turns right 90 degrees
        */
    EngineTurnRight();
    Task_Msleep(time);
    /* 舵机归中 Steering gear centering */
    RegressMiddle();
    Task_Msleep(time);
}

void SG92RInit(void)
{
    unsigned int time = 200;
    S92R_INNER_Init();
    /* 舵机归中 Steering gear centering */
    RegressMiddle();
    Task_Msleep(time);
}

void Turn_Where(Servo_Status_ENUM status) {
    if(status == Control_ON) {
       Turn_Right_ON();
    }
    if(status == Control_OFF) {
       Turn_Left_OFF();
    }
}
