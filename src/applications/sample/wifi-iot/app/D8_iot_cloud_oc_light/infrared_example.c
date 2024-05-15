#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "ohos_init.h"
#include "iot_errno.h"
#include "iot_uart.h"

#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_adc.h"
#include "iot_uart.h"

#define INFR_TASK_STACK_SIZE 1024 * 8
#define INFR_TASK_PRIO 25
#define INFR_BUFF_SIZE 1000
#define TASK_DELAY_1S 1000000
#define UART_TASK_STACK_SIZE (1024 * 8)
#define UART_TASK_PRIO 25
#define UART_BUFF_SIZE 1000
#define WIFI_IOT_UART_IDX_1 1
#define TASK_DELAY_1S 1000000

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
  int r1 = offset - data;
  return r1; 
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
  int r2 = offset - data;
  return r2; 
}

static void UartTask1(void)
{
    uint8_t uart_buff[UART_BUFF_SIZE] = { 0 };
    uint8_t *uart_buff_ptr = uart_buff;
    uint8_t ret;

    IotUartAttribute uart_attr = {

        // baud_rate: 9600
        .baudRate = 9600,
        // data_bits: 8bits
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };

    // Initialize uart driver
    ret = IoTUartInit(WIFI_IOT_UART_IDX_1, &uart_attr);
    if (ret != IOT_SUCCESS) {
        printf("Failed to init uart! Err code = %d\n", ret);
        return;
    }
    int r1 = Irstudy();

    // send data through uart1
    IoTUartWrite(WIFI_IOT_UART_IDX_1, (unsigned char *)r1, strlen(r1));

    // receive data through uart1
    //IoTUartRead(WIFI_IOT_UART_IDX_1, uart_buff_ptr, UART_BUFF_SIZE);
    //printf("Uart1 read data:%s\n", uart_buff_ptr);
    usleep(TASK_DELAY_1S);
    
}


static void UartTask2(void)
{
    uint8_t uart_buff[UART_BUFF_SIZE] = { 0 };
    uint8_t *uart_buff_ptr = uart_buff;
    uint8_t ret;

    IotUartAttribute uart_attr = {

        // baud_rate: 9600
        .baudRate = 9600,
        // data_bits: 8bits
        .dataBits = 8,
        .stopBits = 1,
        .parity = 0,
    };

    // Initialize uart driver
    ret = IoTUartInit(WIFI_IOT_UART_IDX_1, &uart_attr);
    if (ret != IOT_SUCCESS) {
        printf("Failed to init uart! Err code = %d\n", ret);
        return;
    }

    //红外内码发送
    int r2 = Irstudy();

    // send data through uart1
    IoTUartWrite(WIFI_IOT_UART_IDX_1, (unsigned char *)r2, strlen(r2));

    // receive data through uart1
    //IoTUartRead(WIFI_IOT_UART_IDX_1, uart_buff_ptr, UART_BUFF_SIZE);
    //printf("Uart1 read data:%s\n", uart_buff_ptr);
    usleep(TASK_DELAY_1S);
    
}



static void ButtonExampleEntry(void)
{
    osThreadAttr_t attr;

    attr.name = "UartTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = UART_TASK_STACK_SIZE;
    attr.priority = UART_TASK_PRIO;

    //init gpio of LED
    IoTGpioInit(LED_GPIO);
    IoTGpioSetDir(LED_GPIO, IOT_GPIO_DIR_OUT);

    //init gpio of F1 key and set it as the falling edge to trigger interrupt
    IoTGpioInit(BUTTON_F1_GPIO);
    IoTGpioSetDir(BUTTON_F1_GPIO, IOT_GPIO_DIR_IN);
    IoTGpioSetPull(BUTTON_F1_GPIO, IOT_GPIO_PULL_UP);
    IoTGpioRegisterIsrFunc(BUTTON_F1_GPIO, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, F1Pressed, NULL);

    //init gpio of F2 key and set it as the falling edge to trigger interrupt
    IoTGpioInit(BUTTON_F2_GPIO);
    IoTGpioSetDir(BUTTON_F2_GPIO, IOT_GPIO_DIR_IN);
    IoTGpioSetPull(BUTTON_F2_GPIO, IOT_GPIO_PULL_UP);
    IoTGpioRegisterIsrFunc(BUTTON_F2_GPIO, IOT_INT_TYPE_EDGE, IOT_GPIO_EDGE_FALL_LEVEL_LOW, F2Pressed, NULL);
}
APP_FEATURE_INIT(ButtonExampleEntry);



