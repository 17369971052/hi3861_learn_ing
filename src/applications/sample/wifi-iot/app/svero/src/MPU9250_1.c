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
#include "iot_errno.h"
//加速度带宽设置和输出速率配置寄存器
/*
+-------+----+----+----+
|FCHOICE|DLPF|BW  |RATE|
+-------+----+----+----+
|0      |X   |1.13|4K  |
|1      |0   |460 |1K  |
|1      |1   |184 |1K  |
|1      |2   |92  |1K  |
|1      |3   |41  |1K  |
|1      |4   |20  |1K  |
|1      |5   |10  |1K  |
|1      |6   |10  |1K  |
|1      |7   |460 |1K  |
+-------+----+----+----+
*/

/*
  对于GPIO相应功能的设置以及初始化
*/
// static void i2c_Delay(void){
//     hi_udelay(3);  //已经定义的函数，在此处调用
// }

/***************************************************************
 Name: II2_Config
 Params: void
 Return: void
 Description: 配置引脚工作模式
***************************************************************/
/* SCL=>1 SDA=>0 */
// static int I2C_Init(void)
// {
//     uint8_t ret;
//     ret = IoTGpioInit(WIFI_IOT_IO_NAME_GPIO_0);
//     if (ret != IOT_SUCCESS) 
//     {
//         return -1;
//         printf("GPIOINIT Failed");
//     }
//     ret = IoTGpioSetFunc(WIFI_IOT_IO_NAME_GPIO_0, IOT_GPIO_FUNC_GPIO_0_I2C1_SDA); // GPIO_0复用为I2C1_SDA
//     if (ret != IOT_SUCCESS)
//     {
//         return -1;
//         printf("FUNC Failed");
//     }
//     IoTGpioInit(WIFI_IOT_IO_NAME_GPIO_1);
//     IoTGpioSetFunc(WIFI_IOT_IO_NAME_GPIO_1, IOT_GPIO_FUNC_GPIO_1_I2C1_SCL); // GPIO_1复用为I2C1_SCL
//     IoTI2cInit(WIFI_IOT_I2C_IDX_1, WIFI_IOT_I2C_BAUDRATE);              /* baudrate: 400kbps */
    
//     // 使能AD0口
//     IoTGpioInit(8);
//     IoTGpioSetFunc(8, IOT_GPIO_FUNC_GPIO_8_GPIO);
//     IoTGpioSetDir(8, IOT_GPIO_DIR_OUT);
//     IoTGpioSetOutputVal(8 , IOT_GPIO_VALUE1 );
//     return 0;
// }




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
static int MPU6050ReadBuffer(uint8_t Reg, uint8_t *pBuffer, uint16_t Length)
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


uint8_t i2c_dev;
int Mpu9250_Work_Mode_Init(void){

    MPU9250_Write(MPU6500_PWR_MGMT_1,0x80);   // PWR_MGMT_1  MPU9250电源管理寄存器解除休眠
    
    hi_udelay(100);

    MPU9250_WriteByte(MPU9250_RA_PWR_MGMT_1, 0x00); //唤醒MPU9250，并选择陀螺仪x轴PLL为时钟源 (MPU9250_RA_PWR_MGMT_1, 0x01)
    
    MPU6050ReadBuffer(MPU6500_WHO_AM_I, &i2c_dev, 1);   //i2c_dev  获取MPU_Read 返回值
    
    if(i2c_dev == 0x71 )    //设备ID
    {
        hi_udelay(1000);                        //   此处的延时很重要，开始比较延时小，导致读的全是0XFF，
        MPU9250_Write(MPU6500_PWR_MGMT_2,0x00);   // 使能寄存器 X，Y，Z加速度
        MPU9250_Write(MPU6500_SMPLRT_DIV,0x07);   // SMPLRT_DIV 采样率分频寄存器，输入采样时钟为1kHz，陀螺仪采样率1000/(1+7)=125HZ
        MPU9250_Write(MPU6500_CONFIG,0x06);       // 设为0x05时，GYRO的带宽为10Hz，延时为17.85ms，设为0x06时，带宽5Hz，延时33.48ms（建议使用0x05）
        MPU9250_Write(MPU6500_GYRO_CONFIG,0x10);  // =>±1000dps  加速度计测量范围 正负16g
        MPU9250_Write(MPU6500_USER_CTRL,0x00);    // 初始化I2C
        MPU9250_Write(MPU6500_ACCEL_CONFIG,0x10); // 加速度计测量范围 0X10 正负8g
        MPU9250_Write(MPU6500_INT_PIN_CFG,0x02);  // 进入Bypass模式，用于控制电子指南针
        return 0;
    }
    return -1;
}


#define MPU9250_RA_ACCEL_XOUT_H     0x3B
 
#define MPU9250_RA_TEMP_OUT_H       0x41
 
#define MPU9250_RA_GYRO_XOUT_H      0x43
 
//MPU9250内部封装了一个AK8963磁力计,地址和ID如下:
#define AK8963_ADDR			0X0C	//AK8963的I2C地址
#define AK8963_ID			0X48	//AK8963的器件ID
//AK8963的内部寄存器
#define MAG_WIA				0x00	//AK8963的器件ID寄存器地址
#define MAG_XOUT_L			0X03
 
// /******************************************************************************
// * 功  能：读取加速度的原始数据
// * 参  数：*accData 原始数据的指针
// * 返回值：无
// *******************************************************************************/
// void MPU9250_AccRead(int16_t *accData)
// {
//     uint8_t buf[6];
//    	MPU9250_ReadMultBytes(MPU9250_RA_ACCEL_XOUT_H,6,buf);
//     accData[0] = (int16_t)((buf[0] << 8) | buf[1]);
//     accData[1] = (int16_t)((buf[2] << 8) | buf[3]);
//     accData[2] = (int16_t)((buf[4] << 8) | buf[5]);
// }
 
// /******************************************************************************
// * 功  能：读取陀螺仪的原始数据
// * 参  数：*gyroData 原始数据的指针
// * 返回值：无
// *******************************************************************************/
// void MPU9250_GyroRead(int16_t *gyroData)
// {
//     uint8_t buf[6];
// 	MPU9250_ReadMultBytes(MPU9250_RA_GYRO_XOUT_H, 6, buf);
//     gyroData[0] = (int16_t)((buf[0] << 8) | buf[1]) ;
//     gyroData[1] = (int16_t)((buf[2] << 8) | buf[3]) ;
//     gyroData[2] = (int16_t)((buf[4] << 8) | buf[5]) ;
// }
 
// /******************************************************************************
// * 功  能：读取磁力计的原始数据
// * 参  数：*magData原始数据的指针
// * 返回值：无
// *******************************************************************************/
// void MPU9250_MagRead(int16_t *magData)
// {
//     uint8_t buf[6];
//     HALIIC_WriteByteToSlave(MPU9250Addr,0x37,0x02);//turn on Bypass Mode
//     HAL_Delay(10);
//     HALIIC_WriteByteToSlave(AK8963_MAG_ADDRESS,0x0A,0x11);
//     HAL_Delay(10);
 
//     HALIIC_ReadMultByteFromSlave(AK8963_MAG_ADDRESS,MAG_XOUT_L, 6, buf);
//     magData[0] = (int16_t)((buf[1] << 8) | buf[0]) ;
//     magData[1] = (int16_t)((buf[3] << 8) | buf[2]) ;
//     magData[2] = (int16_t)((buf[5] << 8) | buf[4]) ;
// }

// ————————————————

//                             版权声明：本文为博主原创文章，遵循 CC 4.0 BY-SA 版权协议，转载请附上原文出处链接和本声明。
                        
// 原文链接：https://blog.csdn.net/u010779035/article/details/104362532

/*读取MPU9250数据*/
uint8_t TX_DATA[4];//显示据缓存区
uint8_t BUF[10];//接收数据缓存区

uint16_t T_X,T_Y,T_Z,T_T;//X,Y,Z轴，温度

/*模拟IIC端口输出输入定义*/
/****************************************/
static int READ_MPU9250_ACCEL(void)
{
    int ret;
    //读取计算X Y Z轴数据    T_X =advalue/
    ret = MPU6050ReadBuffer( MPU6500_ACCEL_XOUT_L ,BUF,6);
    if (ret != 0) {
        return -1;
    }
    T_X= (BUF[1]<<8)|BUF[0];
    T_X/=4096;
 
    //读取计算Y轴数据
    T_Y= (BUF[3]<<8)|BUF[2];
    T_Y/=4096;
 
    //读取计算Z轴数据
    T_Z= (BUF[5]<<8)|BUF[4];
    T_Z/=4096;
    return 0;
}


static int READ_MPU9250_GYRO(void)
{
    int ret;
    //读取计算X Y Z轴数据    T_X =advalue/
    ret = MPU6050ReadBuffer(MPU6500_GYRO_XOUT_L,BUF,6);
    if (ret != 0) {
        return -1;
    }
    T_X=(BUF[1]<<8)|BUF[0];
    T_X/=32.8;
 
    //读取计算Y轴数据
    T_Y= (BUF[3]<<8)|BUF[2];
    T_Y/=32.8;
 
    //读取计算Z轴数据
    T_Z=(BUF[5]<<8)|BUF[4];
    T_Z/=32.8;
    return 0;
}


static int READ_MPU9250_MAG(void)
{
    MPU9250_Write(0x37,0x02);  //turn on Bypass Mode
    MPU9250_Write(0x0A,0x01);
    //读取计算X轴数据
     int ret;
    //读取计算X Y Z轴数据    T_X =advalue/
    ret = MPU6050ReadBuffer(AK8963_HXL,BUF,6);
    if (ret != 0) {
        return -1;
    }
    T_X=(BUF[1]<<8)|BUF[0];

    // 读取计算Y轴数据
    T_Y=(BUF[3]<<8)|BUF[2];

    //读取计算Z轴数据
    T_Z= (BUF[5]<<8)|BUF[4];
    return 0;
 }

#define ONE 1
#define TWO 2
#define THREE 3
 /***************************************************************
 * 函数名称: ReadData_MPU9250
 * 说    明: 读取MPU9250的数据
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
int ReadData_MPU9250(MPU9250_ReadData *ReadData)
{
    int ret;
    // MPU6050ReadBuffer(MPU6500_WHO_AM_I, &i2c_dev,1);   //i2c_dev  获取MPU_Read 返回值
    // if(i2c_dev == 0x71 )
    // {
    //     return -1;
    // }
    ret = Mpu9250_Work_Mode_Init();
    if (ret != 0) {
        // return -1;
        printf("Mode_Init fail\r\n");
    }
    ret = READ_MPU9250_ACCEL();
    if (ret != 0) {
        // return -1;
        printf("Read_accel fail\r\n");
    }
    // 读加速度数据
    ReadData->Accel[1] = T_X;
    ReadData->Accel[2] = T_Y;
    ReadData->Accel[3] = T_Z;

    ret = READ_MPU9250_GYRO();
    if (ret != 0) {
        // return -1;
        printf("Read_Gyro fail\r\n");
    }
    // 传角速度数据
    ReadData->Gyro[1] = T_X;
    ReadData->Gyro[2] = T_Y;
    ReadData->Gyro[3] = T_Z;

    ret = READ_MPU9250_MAG();
    if (ret != 0) {
        // return -1;
        printf("Read_Mag fail\r\n");
    }
    // 传角速度数据
    ReadData->Mag[1] = T_X;
    ReadData->Mag[2] = T_Y;
    ReadData->Mag[3] = T_Z;
    hi_udelay(50000);

    return 0;
}

int MPU9250_I2C_Init(void)
{
    int ret;
    // ret = I2C_Init();
    // if (ret != 0) {
    //     // return -1;
    //     printf("IIC fail\r\n");
    // }
    ret = Mpu9250_Work_Mode_Init();
    if (ret != 0) {
        // return -1;
        printf("Mode_Init fail\r\n");
    }
    return 0;
}