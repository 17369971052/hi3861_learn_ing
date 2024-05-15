#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
#include <math.h>
#include "iot_gpio_ex.h"
#include "iot_gpio.h"
// I2C相关的头文件以及函数
#include "iot_i2c.h"
#include "iot_i2c_ex.h"
#include "MPU9250.h"
#include "hi_time.h"

#define MPU9250_RA_ACCEL_XOUT_H     0x3B
 
#define MPU9250_RA_TEMP_OUT_H       0x41
 
#define MPU9250_RA_GYRO_XOUT_H      0x43
 
//MPU9250内部封装了一个AK8963磁力计,地址和ID如下:
#define AK8963_ADDR			0X0C	//AK8963的I2C地址
#define AK8963_ID			0X48	//AK8963的器件ID
//AK8963的内部寄存器
#define MAG_WIA				0x00	//AK8963的器件ID寄存器地址
#define MAG_XOUT_L			0X03


#include "iot_errno.h"

/*
* @brief  用于与相关的寄存器进行通信，同时进行相应计时器的赋值
* 输入参数: reg 要写的寄存器地址
*           value 要在对应寄存器写入的数值
* 返回值： 无
*/
int MPU9250_Write(uint8_t Reg, uint8_t Value)
{
    uint32_t ret;
    uint8_t send_data[MPU6050_DATA_2_BYTE] = { Reg, Value };
    ret = IoTI2cWrite(WIFI_IOT_I2C_IDX_1, (MPU9250_I2C_ADDR << 1) | 0x00, send_data, sizeof(send_data));
    if (ret != 0) {
        printf("===== Error: I2C write ret = 0x%x! =====\r\n", ret);
        return -1;
    }
    return 0;
}

int AK8963_Write(uint8_t Reg, uint8_t Value)
{
    uint32_t ret;
    uint8_t send_data[MPU6050_DATA_2_BYTE] = { Reg, Value };
    ret = IoTI2cWrite(WIFI_IOT_I2C_IDX_1, (AK8963_ADDR << 1) | 0x00, send_data, sizeof(send_data));
    if (ret != 0) {
        printf("===== Error: I2C write ret = 0x%x! =====\r\n", ret);
        return -1;
    }
    return 0;
}
/***************************************************************
 * 函数功能: 通过I2C读取一段寄存器内容存放到指定的缓冲区内
 * 输入参数: Addr：I2C设备地址
 *           Reg：目标寄存器
 *           RegSize：寄存器尺寸(8位或者16位)
 *           pBuffer：缓冲区指针
 *           Length：缓冲区长度
 * 返 回 值: HAL_StatusTypeDef：操作结果
 * 说    明: 无
 **************************************************************/
static int MPU9250ReadBuffer(uint8_t Reg, uint8_t *pBuffer, uint16_t Length)
{
    uint32_t ret = 0;
    IotI2cData mpu9250_i2c_data = { 0 };
    uint8_t buffer[1] = { Reg };
    mpu9250_i2c_data.sendBuf = buffer;
    mpu9250_i2c_data.sendLen = 1;
    mpu9250_i2c_data.receiveBuf = pBuffer;
    mpu9250_i2c_data.receiveLen = Length;
    ret = IoTI2cWriteread(WIFI_IOT_I2C_IDX_1, (MPU9250_I2C_ADDR << 1) | 0x00 , &mpu9250_i2c_data);
    if (ret != 0) {
        printf("===== Error: I2C writeread ret = 0x%x! =====\r\n", ret);
        return -1;
    }
    return 0;
}

static int AK8963ReadBuffer(uint8_t Reg, uint8_t *pBuffer, uint16_t Length)
{
    uint32_t ret = 0;
    IotI2cData mpu9250_i2c_data = { 0 };
    uint8_t buffer[1] = { Reg };
    mpu9250_i2c_data.sendBuf = buffer;
    mpu9250_i2c_data.sendLen = 1;
    mpu9250_i2c_data.receiveBuf = pBuffer;
    mpu9250_i2c_data.receiveLen = Length;
    ret = IoTI2cWriteread(WIFI_IOT_I2C_IDX_1, (MPU9250_I2C_ADDR << 1) | 0x00 , &mpu9250_i2c_data);
    if (ret != 0) {
        printf("===== Error: I2C writeread ret = 0x%x! =====\r\n", ret);
        return -1;
    }
    return 0;
}

#define MPU9250_RA_INT_ENABLE       0x38
#define MPU9250_RA_PWR_MGMT_1       0x6B
#define MPU9250_RA_GYRO_CONFIG      0x1B
#define MPU9250_RA_ACCEL_CONFIG     0x1C
#define MPU9250_RA_CONFIG           0x1A
#define MPU9250_RA_SMPLRT_DIV       0x19
#define MPU9250_RA_INT_PIN_CFG      0x37
 
//设置低通滤波
#define MPU9250_DLPF_BW_256         0x00
#define MPU9250_DLPF_BW_188         0x01
#define MPU9250_DLPF_BW_98          0x02
#define MPU9250_DLPF_BW_42          0x03
#define MPU9250_DLPF_BW_20          0x04
#define MPU9250_DLPF_BW_10          0x05
#define MPU9250_DLPF_BW_5           0x06

uint8_t MPU9250_Check(void){
    
    MPU9250ReadBuffer();
}

 
void MPU9250_Init(void)
{
	MPU9250_Check(); //通过读取ID，检查MPU9250是否连接
 
	MPU9250_WriteByte(MPU9250_RA_PWR_MGMT_1, 0x80); //复位MPU9250
	HAL_Delay(100);
	MPU9250_WriteByte(MPU9250_RA_PWR_MGMT_1, 0x00); //唤醒MPU9250，并选择陀螺仪x轴PLL为时钟源 (MPU9250_RA_PWR_MGMT_1, 0x01)
	MPU9250_WriteByte(MPU9250_RA_INT_ENABLE, 0x00); //禁止中断
	MPU9250_WriteByte(MPU9250_RA_GYRO_CONFIG, 0x18); //陀螺仪满量程+-2000度/秒 (最低分辨率 = 2^15/2000 = 16.4LSB/度/秒
	MPU9250_WriteByte(MPU9250_RA_ACCEL_CONFIG, 0x08); //加速度满量程+-4g   (最低分辨率 = 2^15/4g = 8196LSB/g )
	MPU9250_WriteByte(MPU9250_RA_CONFIG, MPU9250_DLPF_BW_20);//设置陀螺的输出为1kHZ,DLPF=20Hz
	MPU9250_WriteByte(MPU9250_RA_SMPLRT_DIV, 0x00);  //采样分频 (采样频率 = 陀螺仪输出频率 / (1+DIV)，采样频率1000hz）
	MPU9250_WriteByte(MPU9250_RA_INT_PIN_CFG, 0x02); //MPU 可直接访问MPU9250辅助I2C
 
}
                        
// 原文链接：https://blog.csdn.net/u010779035/article/details/104362532




 
/******************************************************************************
* 功  能：读取加速度的原始数据
* 参  数：*accData 原始数据的指针
* 返回值：无
*******************************************************************************/
void MPU9250_AccRead(int16_t *accData)
{
    uint8_t buf[6];
   	MPU9250_ReadMultBytes(MPU9250_RA_ACCEL_XOUT_H,6,buf);
    accData[0] = (int16_t)((buf[0] << 8) | buf[1]);
    accData[1] = (int16_t)((buf[2] << 8) | buf[3]);
    accData[2] = (int16_t)((buf[4] << 8) | buf[5]);
}
 
/******************************************************************************
* 功  能：读取陀螺仪的原始数据
* 参  数：*gyroData 原始数据的指针
* 返回值：无
*******************************************************************************/
void MPU9250_GyroRead(int16_t *gyroData)
{
    uint8_t buf[6];
	MPU9250_ReadMultBytes(MPU9250_RA_GYRO_XOUT_H, 6, buf);
    gyroData[0] = (int16_t)((buf[0] << 8) | buf[1]) ;
    gyroData[1] = (int16_t)((buf[2] << 8) | buf[3]) ;
    gyroData[2] = (int16_t)((buf[4] << 8) | buf[5]) ;
}
 
/******************************************************************************
* 功  能：读取磁力计的原始数据
* 参  数：*magData原始数据的指针
* 返回值：无
*******************************************************************************/
void MPU9250_MagRead(int16_t *magData)
{
    uint8_t buf[6];
    MPU9250_Write(0x37,0x02);//turn on Bypass Mode
    HAL_Delay(10);
    AK8963_Write(0x0A,0x11);
    HAL_Delay(10);
 
    AK8963ReadBuffer(MAG_XOUT_L, buf, 6 );
    magData[0] = (int16_t)((buf[1] << 8) | buf[0]) ;
    magData[1] = (int16_t)((buf[3] << 8) | buf[2]) ;
    magData[2] = (int16_t)((buf[5] << 8) | buf[4]) ;
}
 
