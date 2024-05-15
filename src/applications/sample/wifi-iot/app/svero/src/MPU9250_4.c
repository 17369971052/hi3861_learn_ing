#include <stdio.h>
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

#include "MPU9250_4.h"

static void MPU9250IoInit(void)
{
    IoTGpioInit(HI_GPIO_IDX_0);
    IoTGpioSetFunc(HI_GPIO_IDX_0, IOT_GPIO_FUNC_GPIO_0_I2C1_SDA);
    // IoTGpioSetDir
    IoTGpioInit(HI_GPIO_IDX_1);
    IoTGpioSetFunc(HI_GPIO_IDX_1, IOT_GPIO_FUNC_GPIO_1_I2C1_SCL);
    IoTI2cInit(HI_I2C_IDX_1, 400000);
}

// 六轴
static int MPU9250WriteDate(uint8_t Reg, uint8_t Value){
    uint32_t ret;
    uint8_t send_date[2] = { Reg, Value };
    ret = IoTI2cWrite(HI_I2C_IDX_1, (MPU9250_ADDRESS0 << 1) | 0x00, send_date, sizeof(send_date));
    if (ret != 0)
    { 
        printf("IIC_Write ret = %x", ret);
        return ret;
    }
    return 0;
}

// 磁力计
static int MPU9250WriteDate2(uint8_t Reg, uint8_t Value){
    uint32_t ret;
    uint8_t send_date[2] = { Reg, Value };
    ret = IoTI2cWrite(HI_I2C_IDX_1, 0x0C, send_date, sizeof(send_date));
    if (ret != 0)
    { 
        printf("IIC_Write ret = %x", ret);
        return ret;
    }
    return 0;
}

// 六轴 
static int MPU9250ReadBuffer(uint8_t Reg, uint8_t *pBuffer, uint16_t Length)
{
    IotI2cData mpu9250data = {0};
    uint8_t buffer[1] = { Reg };
    mpu9250data.sendBuf = buffer;
    mpu9250data.sendLen = 1;
    mpu9250data.receiveBuf = pBuffer;
    mpu9250data.receiveLen = Length;
    int ret = IoTI2cWriteread(HI_I2C_IDX_1, (MPU9250_ADDRESS0 << 1) | 0x00, &mpu9250data);
    if(ret != 0) {
        printf("===== Error: I2C writeread ret = 0x%x! =====\r\n", ret);
        return ret;
    }
    return 0;
}

// 读取磁力计的
static int MPU9250ReadBuffer2(uint8_t Reg, uint8_t *pBuffer, uint16_t Length)
{
    IotI2cData mpu9250data = {0};
    uint8_t buffer[1] = { Reg };
    mpu9250data.sendBuf = buffer;
    mpu9250data.sendLen = 1;
    mpu9250data.receiveBuf = pBuffer;
    mpu9250data.receiveLen = Length;
    int ret = IoTI2cWriteread(HI_I2C_IDX_1, 0x0C, &mpu9250data);
    if(ret != 0) {
        printf("===== Error: I2C writeread ret = 0x%x! =====\r\n", ret);
        return ret;
    }
    return 0;
}

static int MPU9250_Init(void){
    int ret;
    MPU9250WriteDate(PWR_MGMT_1, PWR_MGMT_1_T);// 重置寄存器
    hi_udelay(100); 
    MPU9250WriteDate(27,0x00);      // 陀螺仪灵敏度
    MPU9250WriteDate(28,0x00);      // 加速度计灵敏度
    MPU9250WriteDate(0x37, 0x02);   // 旁路模式
    MPU9250WriteDate(0x0A,0x11);    // 磁力计通信模式 
    
    return 0;
}

// 磁力计寄存器地址
static int ReadSTI(void){
    uint8_t ST1;
    do
    {
        MPU9250ReadBuffer2(0x02, &ST1, 1);
    }
    while (!(ST1&0x01));
}

typedef struct 
{
    short Mag_X;
    short Mag_Y;
    short Mag_Z;
} MPU9250_Mag_date;


// 磁力计
static int ReadAddress(void){
    MPU9250_Mag_date *Mag;
    uint8_t rawDate[6];

    MPU9250ReadBuffer2(0x03, rawDate, 6);
    Mag->Mag_X = (rawDate[1]<<8 | rawDate[0]); 
    Mag->Mag_Y = (rawDate[3]<<8 | rawDate[2]); 
    Mag->Mag_Z = (rawDate[5]<<8 | rawDate[4]);

    MPU9250WriteDate2(0x0A,0x11);

    return 0;
}

typedef struct 
{
    short ACC_X;
    short ACC_Y;
    short ACC_Z;
    short Gyro_X;
    short Gyro_Y;
    short Gyro_Z;
} MPU9250_AC_date;


static int MPU9250_Read_AC(void){

    uint8_t Buf[14];
    MPU9250_AC_date *data;
    MPU9250ReadBuffer2(0x3B,Buf, 14);
    
    // 加速度
    data->ACC_X=(Buf[0]<<8 | Buf[1]);
    data->ACC_Y=(Buf[2]<<8 | Buf[3]);
    data->ACC_Z=(Buf[4]<<8 | Buf[5]);
    
    // 陀螺仪
    data->Gyro_X=(Buf[8]<<8 | Buf[9]);
    data->Gyro_Y=(Buf[10]<<8 | Buf[11]);
    data->Gyro_Z=(Buf[12]<<8 | Buf[13]);
    return 0;
}