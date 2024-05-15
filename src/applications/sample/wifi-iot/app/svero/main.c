/*
 * Copyright (c) 2020 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>





#include <math.h>
#include "cmsis_os2.h"
#include "ohos_init.h"

#include <dtls_al.h>
#include <mqtt_al.h>
#include <oc_mqtt_al.h>
#include <oc_mqtt_profile.h>

#include "wifi_connect.h"

// ADC的头文件
#include "iot_adc.h"
// 库函数返回值
#include "iot_errno.h"
#include <math.h>
// GPIO有关的函数
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
// 系统关于时间的函数
#include "hi_time.h"



#include "cmsis_os2.h"
#include "ohos_init.h"

#include <dtls_al.h>
#include <mqtt_al.h>
#include <oc_mqtt_al.h>
#include <oc_mqtt_profile.h>


#include "wifi_connect.h"

// 关于GPIO以及ADC的头文件
#include "iot_adc.h"
#include "iot_errno.h"

#include "iot_gpio_ex.h"
#include "iot_gpio.h"
#include "hi_gpio.h"
// I2C相关的头文件以及函数
#include "hi_i2c.h"
#include "iot_i2c.h"
#include "iot_i2c_ex.h"
// #include "MPU9250.h"
#include "hi_time.h"
#include "iot_errno.h"

#include "MPU92502.h"



void svero_gpio(void)
{
    IoTGpioInit(1);
    IoTGpioSetFunc(1,IOT_GPIO_FUNC_GPIO_1_GPIO);
    IoTGpioSetDir(1,IOT_GPIO_DIR_OUT);
    IoTGpioSetPull(1,IOT_GPIO_PULL_DOWN);
}
void svero(void)
{
    for (int i = 0; i < 1000; i++)
    {
        /* code */
        IoTGpioSetOutputVal(1,IOT_GPIO_VALUE1);
        hi_udelay(1000);
        IoTGpioSetOutputVal(1,IOT_GPIO_VALUE0);
        hi_udelay(20000-1000);
    }
    
}



// 传感器数据采集主函数
static int SensorTaskEntry(void)
{
    svero_gpio();
    
    sleep(1);
    while (1) { 
        svero();
        sleep(1);
    }
    return 0;
}

static void IotMainTaskEntry(void)
{
    osThreadAttr_t attr;


    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024*10;
    attr.priority = 25;
    attr.name = "SensorTaskEntry";
    if (osThreadNew((osThreadFunc_t)SensorTaskEntry, NULL, &attr) == NULL) {
        printf("Failed to create SensorTaskEntry!\n");
    }
}

APP_FEATURE_INIT(IotMainTaskEntry);