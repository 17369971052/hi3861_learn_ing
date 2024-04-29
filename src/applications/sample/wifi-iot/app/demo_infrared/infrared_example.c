#include <stdio.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "ohos_init.h"

#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_adc.h"
#include "iot_uart.h"

#define INFR_TASK_STACK_SIZE 1024 * 8
#define INFR_TASK_PRIO 25
#define INFR_BUFF_SIZE 1000


#define BUTTON_F1_GPIO 11
#define BUTTON_F2_GPIO 12

typedef unsigned char byte;

byte trg = 0, cont = 0;
byte buf[32], len;

//求校验和
byte getSum(byte *data, byte len) {
  byte i, sum = 0;
  for (i = 0; i < len; i++) {
    sum += data[i];
  }
  return sum;
}

//红外内码学习
byte IrStudy(byte *data, byte group) {
  byte *offset = data, cs;
  //帧头
  *offset++ = 0x68;
  //帧长度
  *offset++ = 0x08;
  *offset++ = 0x00;
  //模块地址
  *offset++ = 0xff;
  //功能码
  *offset++ = 0x10;
  //内码索引号，代表第几组
  *offset++ = group;
  cs = getSum(&data[3], offset - data - 3);
  *offset++ = cs;
  *offset++ = 0x16;
  return offset - data; 
}

//红外内码发送
byte IrSend(byte *data, byte group) {
  byte *offset = data, cs;
  //帧头
  *offset++ = 0x68;
  //帧长度
  *offset++ = 0x08;
  *offset++ = 0x00;
  //模块地址
  *offset++ = 0xff;
  //功能码
  *offset++ = 0x12;
  //内码索引号，代表第几组
  *offset++ = group;
  cs = getSum(&data[3], offset - data - 3);
  *offset++ = cs;
  *offset++ = 0x16;
  return offset - data; 
}
/*
//按键状态回读
void keyRead() {
  byte data = 0x00;
  if (digitalRead(K1) == 0) {
    data |= K1_MASK;   
  }
  if (digitalRead(K2) == 0) {
    data |= K2_MASK;  
  }
  trg = data & (data ^ cont);
  cont = data;
}

//按键按下检测处理
void keyHandle() {
  if (trg == 0x00) {
      return;
  }
*/
byte sstudy(){
  //按键1按下，开始红外内码学习

    len = IrStudy(buf, 0);
    //Serial.write(buf, len);
    IoTGpioSetOutputVal(buf, len);
  
}

byte ssend(){
  //按键2按下，开始红外内码发射

    len = IrSend(buf, 0);
    //Serial.write(buf, len);
    IoTGpioSetOutputVal(buf, len);

}

/*
void setup() {
  // put your setup code here, to run once:
      IotUartAttribute uart_attr = 
    {
        //波特率: 115200
        .baudRate = 115200,

        //data_bits: 8bits
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };

  IoTGpioSetDir(BUTTON_F1_GPIO, IOT_GPIO_DIR_OUT);
  IoTGpioSetDir(BUTTON_F2_GPIO, IOT_GPIO_DIR_OUT);
}
*/
/*
void loop() {
  // put your main code here, to run repeatedly:
  keyRead();
  keyHandle();
  delay(20);
}
*/

/*
static void InfrExampleEntry(void)
{
    osThreadAttr_t attr;

    attr.name = "keyHandle";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = INFR_TASK_STACK_SIZE;
    attr.priority = INFR_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)keyHandle, NULL, &attr) == NULL) {
        printf("Failed to create LedTask!\n");
    }

}
*/

static void ButtonExampleEntry(void)
{


    // init gpio of F1 key and set it as the falling edge to trigger interrupt
    IoTGpioInit(BUTTON_F1_GPIO);
    IoTGpioSetDir(BUTTON_F1_GPIO, IOT_GPIO_DIR_IN);
    IoTGpioRegisterIsrFunc(BUTTON_F1_GPIO, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, sstudy, NULL);
    // init gpio of F2 key and set it as the falling edge to trigger interrupt
    IoTGpioInit(BUTTON_F2_GPIO);
    IoTGpioSetDir(BUTTON_F2_GPIO, IOT_GPIO_DIR_IN);
    IoTGpioRegisterIsrFunc(BUTTON_F2_GPIO, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, ssend, NULL);
}

APP_FEATURE_INIT(ButtonExampleEntry);
