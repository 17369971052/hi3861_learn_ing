/*
 * Copyright (c) 2020 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */// 引入所需的头文件
#include <stdio.h>
#include <unistd.h>
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "ohos_init.h"

// 定义LED任务的堆栈大小和优先级，以及LED连接的GPIO引脚
#define LED_TASK_STACK_SIZE (1024 * 4)
#define LED_TASK_PRIO 25
#define LED_GPIO 2 //定义引脚2为LED_GPIO

/*
* name  : Led_Task
* brief : LED任务函数
* param : 无
* return: 无
* note  : 请大家给此函数添加详细的注释
*/
static void LedTask(void)
{
    //初始化LED连接的GPIO引脚
    IoTGpioInit(LED_GPIO); // pinMode(LED_GPIO, OUTPUT);
    IoTGpioSetDir(LED_GPIO, IOT_GPIO_DIR_OUT);//设置该引脚为输出模式
    IoTGpioSetOutputVal(LED_GPIO, IOT_GPIO_VALUE1); // digitalWrite(LED_GPIO, HIGH);//设置该引脚为高电平

    while (1)//循环
    {
        IoTGpioSetOutputVal(LED_GPIO, 1); // digitalWrite(LED_GPIO, HIGH);设置为高电平
        sleep(1);//延时一秒
        IoTGpioSetOutputVal(LED_GPIO, 0);//设置为低电平
        sleep(1);//延时一秒
    }
}

// 主程序入口
static void LedExampleEntry(void)
{
    osThreadAttr_t attr;
    // 设置任务属性
    attr.name = "LedTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = LED_TASK_STACK_SIZE;
    attr.priority = LED_TASK_PRIO;
    // 创建LED任务
    if (osThreadNew((osThreadFunc_t)LedTask, NULL, &attr) == NULL)
    {
        printf("Failed to create LedTask!\n");
    }
}
// HarmonyOS的应用程序入口，会在系统启动时自动调用LedExampleEntry函数
APP_FEATURE_INIT(LedExampleEntry);