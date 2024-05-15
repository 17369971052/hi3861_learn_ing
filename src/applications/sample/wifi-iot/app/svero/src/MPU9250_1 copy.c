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
#include "E53_IA1.h"
#include "wifi_connect.h"

// 关于GPIO以及ADC的头文件
#include "iot_adc.h"
#include "iot_errno.h"
#include <math.h>
#include "iot_gpio_ex.h"
// I2C相关的头文件以及函数
#include "iot_i2c.h"
#include "iot_i2c_ex.h"

/*====================================================================================================*/
#define MPU_CORRECTION_FLASH     0x0800F000        //存储校正数据的FLASH地址，SIZE=6*3*4字节

//相关常数定义
#define G 9.86
#define M 0.27

//10000GS(高斯)等于1T(特斯拉)，地磁场约0.6Gs
//5883输出-2048-2047，超量程后，输出为-4096
#define MPU_ACCE_K (8 * G / 32768.0)            //单位换算，将LSB化为m*s^-2，前面的"2"需根据量程修改
#define MPU_GYRO_K (1000 / 32768.0 * PI / 180)    //单位换算，将LSB化为rad/s
#define MPU_MAGN_K (49.12 / 32760.0)            //单位换算，将LSB化为Gs
#define MPU_TEMP_K (0.002995177763f)            //degC/LSB
#define MPU_GYRO_ZERO_CALI_FACT 512                //陀螺仪零点校正采样次数，512次约600ms，需保持陀螺仪静止
/*====================================================================================================*/
/*
|     |      ACCELEROMETER      |        GYROSCOPE        |
| LPF | BandW | Delay  | Sample | BandW | Delay  | Sample |
+-----+-------+--------+--------+-------+--------+--------+
|  0  | 260Hz |    0ms |  1kHz  | 256Hz | 0.98ms |  8kHz  |
|  1  | 184Hz |  2.0ms |  1kHz  | 188Hz |  1.9ms |  1kHz  |
|  2  |  94Hz |  3.0ms |  1kHz  |  98Hz |  2.8ms |  1kHz  |
|  3  |  44Hz |  4.9ms |  1kHz  |  42Hz |  4.8ms |  1kHz  |
|  4  |  21Hz |  8.5ms |  1kHz  |  20Hz |  8.3ms |  1kHz  |
|  5  |  10Hz | 13.8ms |  1kHz  |  10Hz | 13.4ms |  1kHz  |
|  6  |   5Hz | 19.0ms |  1kHz  |   5Hz | 18.6ms |  1kHz  |
|  7  | -- Reserved -- |  1kHz  | -- Reserved -- |  8kHz  |
*/
typedef enum {
    MPU_GYRO_LPS_250HZ   = 0x00,
    MPU_GYRO_LPS_184HZ   = 0x01,
    MPU_GYRO_LPS_92HZ    = 0x02,
    MPU_GYRO_LPS_41HZ    = 0x03,
    MPU_GYRO_LPS_20HZ    = 0x04,
    MPU_GYRO_LPS_10HZ    = 0x05,
    MPU_GYRO_LPS_5HZ     = 0x06,
    MPU_GYRO_LPS_DISABLE = 0x07,
} MPU_GYRO_LPF_TypeDef;

typedef enum {
    MPU_ACCE_LPS_460HZ   = 0x00,
    MPU_ACCE_LPS_184HZ   = 0x01,
    MPU_ACCE_LPS_92HZ    = 0x02,
    MPU_ACCE_LPS_41HZ    = 0x03,
    MPU_ACCE_LPS_20HZ    = 0x04,
    MPU_ACCE_LPS_10HZ    = 0x05,
    MPU_ACCE_LPS_5HZ     = 0x06,
    MPU_ACCE_LPS_DISABLE = 0x08,
} MPU_ACCE_LPF_TypeDef;

typedef enum {
    MPU_GYRO_FS_250  = 0x00,
    MPU_GYRO_FS_500  = 0x08,
    MPU_GYRO_FS_1000 = 0x10,
    MPU_GYRO_FS_2000 = 0x18,
} MPU_GYRO_FS_TypeDef;

typedef enum {
    MPU_ACCE_FS_2G  = 0x00,
    MPU_ACCE_FS_4G  = 0x08,
    MPU_ACCE_FS_8G  = 0x10,
    MPU_ACCE_FS_16G = 0x18,
} MPU_ACCE_FS_TypeDef;

typedef enum {
    MPU_READ_ACCE = 1 << 0,
    MPU_READ_TEMP = 1 << 1,
    MPU_READ_GYRO = 1 << 2,
    MPU_READ_MAGN = 1 << 3,
    MPU_READ_ALL  = 0x0F,
} MPU_READ_TypeDef;

typedef enum {
    MPU_CORRECTION_PX = 0x01,
    MPU_CORRECTION_NX = 0x02,
 
    //三轴磁强寄存器    
    MPU_CORRECTION_PY = 0x03,
    MPU_CORRECTION_NY = 0x04,
    MPU_CORRECTION_PZ = 0x05,
    MPU_CORRECTION_NZ = 0x06,
    MPU_CORRECTION_GYRO = 0x07,
    MPU_CORRECTION_CALCX = 0x08,
    MPU_CORRECTION_CALCY = 0x09,
    MPU_CORRECTION_CALCZ = 0x0A,
    MPU_CORRECTION_SAVE = 0x0B,
    MPU_CORRECTION_CIRCLE = 0x0C,
    MPU_CORRECTION_CIRCLEZ = 0x0D,
} MPU_CORRECTION_TypeDef;


// 原文链接：https://blog.csdn.net/u010779035/article/details/104362532
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
/********************************************************************/
                        

/* ---- Sensitivity --------------------------------------------------------- */
#define MPU9250A_2g       ((fp32)0.000061035156f) // 0.000061035156 g/LSB
#define MPU9250A_4g       ((fp32)0.000122070312f) // 0.000122070312 g/LSB
#define MPU9250A_8g       ((fp32)0.000244140625f) // 0.000244140625 g/LSB
#define MPU9250A_16g      ((fp32)0.000488281250f) // 0.000488281250 g/LSB
#define MPU9250G_250dps   ((fp32)0.007633587786f) // 0.007633587786 dps/LSB
#define MPU9250G_500dps   ((fp32)0.015267175572f) // 0.015267175572 dps/LSB
#define MPU9250G_1000dps  ((fp32)0.030487804878f) // 0.030487804878 dps/LSB
#define MPU9250G_2000dps  ((fp32)0.060975609756f) // 0.060975609756 dps/LSB
#define MPU9250M_4800uT   ((fp32)0.6f)            // 0.6 uT/LSB
#define MPU9250T_85degC   ((fp32)0.002995177763f) // 0.002995177763 degC/LSB
/* ---- MPU6500 Reg In MPU9250 ---------------------------------------------- */
#define MPU6500_I2C_ADDR            ((u8)0xD0)  //MPU9250设备地址
#define MPU6500_Device_ID           ((u8)0x71)  // In MPU9250
#define MPU6500_SELF_TEST_XG        ((u8)0x00)
#define MPU6500_SELF_TEST_YG        ((u8)0x01)
#define MPU6500_SELF_TEST_ZG        ((u8)0x02)
#define MPU6500_SELF_TEST_XA        ((u8)0x0D)
#define MPU6500_SELF_TEST_YA        ((u8)0x0E)
#define MPU6500_SELF_TEST_ZA        ((u8)0x0F)
#define MPU6500_XG_OFFSET_H         ((u8)0x13)
#define MPU6500_XG_OFFSET_L         ((u8)0x14)
#define MPU6500_YG_OFFSET_H         ((u8)0x15)
#define MPU6500_YG_OFFSET_L         ((u8)0x16)
#define MPU6500_ZG_OFFSET_H         ((u8)0x17)
#define MPU6500_ZG_OFFSET_L         ((u8)0x18)
#define MPU6500_SMPLRT_DIV          ((u8)0x19)  //采样率分频寄存器，输入采样时钟为1kHz
#define MPU6500_CONFIG              ((u8)0x1A) //配置寄存器
#define MPU6500_GYRO_CONFIG         ((u8)0x1B)   //陀螺仪（角速度）配置寄存器
#define MPU6500_ACCEL_CONFIG        ((u8)0x1C)//加速度配置寄存器
#define MPU6500_ACCEL_CONFIG_2      ((u8)0x1D)
#define MPU6500_LP_ACCEL_ODR        ((u8)0x1E)
#define MPU6500_MOT_THR             ((u8)0x1F)
#define MPU6500_FIFO_EN             ((u8)0x23)
#define MPU6500_I2C_MST_CTRL        ((u8)0x24)
#define MPU6500_I2C_SLV0_ADDR       ((u8)0x25)
#define MPU6500_I2C_SLV0_REG        ((u8)0x26)
#define MPU6500_I2C_SLV0_CTRL       ((u8)0x27)
#define MPU6500_I2C_SLV1_ADDR       ((u8)0x28)
#define MPU6500_I2C_SLV1_REG        ((u8)0x29)
#define MPU6500_I2C_SLV1_CTRL       ((u8)0x2A)
#define MPU6500_I2C_SLV2_ADDR       ((u8)0x2B)
#define MPU6500_I2C_SLV2_REG        ((u8)0x2C)
#define MPU6500_I2C_SLV2_CTRL       ((u8)0x2D)
#define MPU6500_I2C_SLV3_ADDR       ((u8)0x2E)
#define MPU6500_I2C_SLV3_REG        ((u8)0x2F)
#define MPU6500_I2C_SLV3_CTRL       ((u8)0x30)
#define MPU6500_I2C_SLV4_ADDR       ((u8)0x31)
#define MPU6500_I2C_SLV4_REG        ((u8)0x32)
#define MPU6500_I2C_SLV4_DO         ((u8)0x33)
#define MPU6500_I2C_SLV4_CTRL       ((u8)0x34)
#define MPU6500_I2C_SLV4_DI         ((u8)0x35)
#define MPU6500_I2C_MST_STATUS      ((u8)0x36)
#define MPU6500_INT_PIN_CFG         ((u8)0x37)   //INT引脚配置和Bypass模式配置寄存器
#define MPU6500_INT_ENABLE          ((u8)0x38)
#define MPU6500_INT_STATUS          ((u8)0x3A)

#define    MPU6500_ACCEL_XOUT_H    0x3B   //三轴加速度寄存器
#define    MPU6500_ACCEL_XOUT_L    0x3C
#define    MPU6500_ACCEL_YOUT_H    0x3D
#define    MPU6500_ACCEL_YOUT_L    0x3E
#define    MPU6500_ACCEL_ZOUT_H    0x3F
#define    MPU6500_ACCEL_ZOUT_L    0x40
#define MPU6500_TEMP_OUT_H          ((u8)0x41)
#define MPU6500_TEMP_OUT_L          ((u8)0x42)
#define    GYRO_XOUT_H        0x43
#define    GYRO_XOUT_L        0x44    
#define    GYRO_YOUT_H        0x45
#define    GYRO_YOUT_L        0x46
#define    GYRO_ZOUT_H        0x47
#define    GYRO_ZOUT_L        0x48
//三轴陀螺仪（角速度）寄存器
#define MPU6500_GYRO_XOUT_H         ((u8)0x43)
#define MPU6500_GYRO_XOUT_L         ((u8)0x44)
#define MPU6500_GYRO_YOUT_H         ((u8)0x45)
#define MPU6500_GYRO_YOUT_L         ((u8)0x46)
#define MPU6500_GYRO_ZOUT_H         ((u8)0x47)
#define MPU6500_GYRO_ZOUT_L         ((u8)0x48)
#define MPU6500_EXT_SENS_DATA_00    ((u8)0x49)
#define MPU6500_EXT_SENS_DATA_01    ((u8)0x4A)
#define MPU6500_EXT_SENS_DATA_02    ((u8)0x4B)
#define MPU6500_EXT_SENS_DATA_03    ((u8)0x4C)
#define MPU6500_EXT_SENS_DATA_04    ((u8)0x4D)
#define MPU6500_EXT_SENS_DATA_05    ((u8)0x4E)
#define MPU6500_EXT_SENS_DATA_06    ((u8)0x4F)
#define MPU6500_EXT_SENS_DATA_07    ((u8)0x50)
#define MPU6500_EXT_SENS_DATA_08    ((u8)0x51)
#define MPU6500_EXT_SENS_DATA_09    ((u8)0x52)
#define MPU6500_EXT_SENS_DATA_10    ((u8)0x53)
#define MPU6500_EXT_SENS_DATA_11    ((u8)0x54)
#define MPU6500_EXT_SENS_DATA_12    ((u8)0x55)
#define MPU6500_EXT_SENS_DATA_13    ((u8)0x56)
#define MPU6500_EXT_SENS_DATA_14    ((u8)0x57)
#define MPU6500_EXT_SENS_DATA_15    ((u8)0x58)
#define MPU6500_EXT_SENS_DATA_16    ((u8)0x59)
#define MPU6500_EXT_SENS_DATA_17    ((u8)0x5A)
#define MPU6500_EXT_SENS_DATA_18    ((u8)0x5B)
#define MPU6500_EXT_SENS_DATA_19    ((u8)0x5C)
#define MPU6500_EXT_SENS_DATA_20    ((u8)0x5D)
#define MPU6500_EXT_SENS_DATA_21    ((u8)0x5E)
#define MPU6500_EXT_SENS_DATA_22    ((u8)0x5F)
#define MPU6500_EXT_SENS_DATA_23    ((u8)0x60)
#define MPU6500_I2C_SLV0_DO         ((u8)0x63)
#define MPU6500_I2C_SLV1_DO         ((u8)0x64)
#define MPU6500_I2C_SLV2_DO         ((u8)0x65)
#define MPU6500_I2C_SLV3_DO         ((u8)0x66)
#define MPU6500_I2C_MST_DELAY_CTRL  ((u8)0x67)
#define MPU6500_SIGNAL_PATH_RESET   ((u8)0x68)
#define MPU6500_MOT_DETECT_CTRL     ((u8)0x69)
#define MPU6500_USER_CTRL           ((u8)0x6A)
#define MPU6500_PWR_MGMT_1          ((u8)0x6B)  //MPU9250电源管理寄存器
#define MPU6500_PWR_MGMT_2          ((u8)0x6C)
#define MPU6500_FIFO_COUNTH         ((u8)0x72)
#define MPU6500_FIFO_COUNTL         ((u8)0x73)    //
#define MPU6500_FIFO_R_W            ((u8)0x74)
#define MPU6500_WHO_AM_I            ((u8)0x75)    //    //MPU9250设备ID寄存器 0x75
#define MPU6500_XA_OFFSET_H         ((u8)0x77)
#define MPU6500_XA_OFFSET_L         ((u8)0x78)
#define MPU6500_YA_OFFSET_H         ((u8)0x7A)
#define MPU6500_YA_OFFSET_L         ((u8)0x7B)
#define MPU6500_ZA_OFFSET_H         ((u8)0x7D)
#define MPU6500_ZA_OFFSET_L         ((u8)0x7E)
/* ---- AK8963 Reg In MPU9250 ----------------------------------------------- */
#define AK8963_I2C_ADDR             ((u8)0x0C)   //指南针设备地址
#define AK8963_DEV_ID               ((u8)0x48)  //指南针设备ID
//Read-only Reg
#define AK8963_WIA                  ((u8)0x00)    //指南针设备ID寄存器
#define AK8963_INFO                 ((u8)0x01)
#define AK8963_ST1                  ((u8)0x02)
#define AK8963_HXL                  ((u8)0x03)
#define AK8963_HXH                  ((u8)0x04)
#define AK8963_HYL                  ((u8)0x05)
#define AK8963_HYH                  ((u8)0x06)
#define AK8963_HZL                  ((u8)0x07)
#define AK8963_HZH                  ((u8)0x08)
#define AK8963_ST2                  ((u8)0x09)
//Write/Read Reg
#define AK8963_CNTL1                ((u8)0x0A) //启动单次传输
#define AK8963_CNTL2                ((u8)0x0B)
#define AK8963_ASTC                 ((u8)0x0C)
#define AK8963_TS1                  ((u8)0x0D)
#define AK8963_TS2                  ((u8)0x0E)
#define AK8963_I2CDIS               ((u8)0x0F)
//Read-only Reg ( ROM )
#define AK8963_ASAX                 ((u8)0x10)
#define AK8963_ASAY                 ((u8)0x11)
#define AK8963_ASAZ                 ((u8)0x12)
# if 0
const unsigned char MPU_INIT_REG[][2] = {
    {MPU6500_PWR_MGMT_1,        0x80},        // Reset Device
    {0xFF, 20},                                // ÑÓÊ±
    {MPU6500_PWR_MGMT_1,        0x03},        // Clock Source
    {MPU6500_PWR_MGMT_2,        0x00},        // Enable Acc & Gyro
    {MPU6500_SMPLRT_DIV,        0x07},    // 默认代码是 0x00
    {MPU6500_CONFIG,            MPU_GYRO_LPS_184HZ},
    {MPU6500_GYRO_CONFIG,        MPU_GYRO_FS_1000},
    {MPU6500_ACCEL_CONFIG,        MPU_ACCE_FS_8G}, //sample rate
    {MPU6500_ACCEL_CONFIG_2,    MPU_ACCE_LPS_460HZ},

    {MPU6500_INT_PIN_CFG,        0x30},        // Disable Interrupt
    {MPU6500_I2C_MST_CTRL,        0x4D},        // I2C Speed 400 kHz
    {MPU6500_USER_CTRL,            0x00},        // Disable AUX

    {MPU6500_I2C_SLV4_CTRL,        0x13},  //
    {MPU6500_I2C_MST_DELAY_CTRL,0x01},

    {MPU6500_I2C_SLV0_ADDR,        AK8963_I2C_ADDR},
    {MPU6500_I2C_SLV0_CTRL,        0x81},            // Enable slave0 Length=1

    {MPU6500_I2C_SLV0_REG,        AK8963_CNTL2},    // reg
    {MPU6500_I2C_SLV0_DO,        0x01},            // dat
    {0xFF, 50},

    {MPU6500_I2C_SLV0_REG,        AK8963_CNTL1},    // reg
    {MPU6500_I2C_SLV0_DO,        0x16},            // dat
    {0xFF, 10},

    {MPU6500_I2C_SLV0_ADDR,        0x80 | AK8963_I2C_ADDR},
    {MPU6500_I2C_SLV0_REG,        AK8963_HXL},
    {MPU6500_I2C_SLV0_CTRL,        0x87},            // Enable slave0 Length=6
};

#endif

//传感器返回的原始数据
extern signed short mpu_acce[3];
extern signed short mpu_gyro[3];
extern signed short mpu_magn[3];
extern signed short mpu_temp;

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
//温度值寄存器
#define    TEMP_OUT_H        0x41
#define    TEMP_OUT_L        0x42
#define MAG_XOUT_L        0x03
#define MAG_XOUT_H        0x04
#define MAG_YOUT_L        0x05
#define MAG_YOUT_H        0x06
#define MAG_ZOUT_L        0x07
#define MAG_ZOUT_H        0x08
void Mpu9250_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
     RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;        //
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);


}

/******************************************************************************/
void TurnOnMpu(void)
{
    GPIO_SetBits(GPIOB, GPIO_Pin_2);    
}

/******************************************************************************
函数名:
参数:
功能描述:    关MPU
输出:
******************************************************************************/
void TurnOffMpu(void)
{
    GPIO_ResetBits(GPIOB, GPIO_Pin_2);    
}

/******************************************************************************/


u8 MPU_Write(u8 MPU_Adr,u8 address,u8 val)
{
    I2C_Start();
    I2C_SendByte(MPU_Adr);//设置器件地址
    I2C_Ack();
    I2C_SendByte(address);   //设置低起始地址
    I2C_Ack();
    I2C_SendByte(val);
    delay_ms(10);
    I2C_Ack();
    I2C_Stop();
    delay_ms(10);
    //注意：因为这里要等待EEPROM写完，可以采用查询或延时方式(10ms)
 
    return 1;
}

u8 MPU_Read(u8 MPU_Adr,u8 address)//读字节
{
 
    u8  temp=0;
    I2C_Start();
    I2C_SendByte(MPU_Adr);//设置器件地址
    I2C_Ack();
    I2C_SendByte(address);   //设置低起始地址

    I2C_Ack();
    I2C_Start();
    I2C_SendByte(MPU_Adr|0x01);//设置器件地址   

 
    I2C_Ack();
    temp=I2C_ReceiveByte();    //
    I2C_NAck();
    I2C_Stop();
    return temp;     
}

u8 MPU_write_String (u8 MPU_Adr,u8 *buff,u8 address,u8 length)
{
 
    I2C_SendByte(MPU_Adr);//设置器件地址
    if(!I2C_WaitAck())
    {
        I2C_Stop();
        return 0;
    }
 
    I2C_SendByte(address);   //设置低起始地址      
    I2C_WaitAck();
    while(length--)
    {
        I2C_SendByte(*buff);
        I2C_WaitAck();
        buff++;
    }       
    I2C_Stop();
    //注意：因为这里要等待EEPROM写完，可以采用查询或延时方式(10ms)
delay_ms(10);
    return 1;   
}
u8 MPU_Read_String(u8 MPU_Adr,u8 address,u8 *buff,u8 length)//读字符串

{

    I2C_Start();
   I2C_SendByte(MPU_Adr);//设置器件地址
   I2C_WaitAck();
   I2C_SendByte(address);   //设置低起始地址      
   I2C_WaitAck();
   I2C_Start();
   I2C_SendByte(MPU_Adr|0x01);//设置器件地址
   I2C_WaitAck();
    while(length)
    {
        *buff=I2C_ReceiveByte();
        if(length==1)
            I2C_NAck();
        else
            I2C_Ack();
        buff++;
        length--;
    }
    I2C_Stop();
    return 1;    
}



u8 i2c_dev;
int i=0,j=0;  
int Mpu9250_Work_Mode_Init(void)
{

    MPU_Write(MPU6500_I2C_ADDR,MPU6500_PWR_MGMT_1,0x00);   // PWR_MGMT_1  MPU9250电源管理寄存器解除休眠
  delay_ms(100);
    i2c_dev=MPU_Read(MPU6500_I2C_ADDR,MPU6500_WHO_AM_I);   //i2c_dev  获取MPU_Read 返回值
    if(i2c_dev == 0x71 )    //设备ID
    {
 
    delay_ms(1000);   //   此处的延时很重要，开始比较延时小，导致读的全是0XFF，
    MPU_Write(MPU6500_I2C_ADDR,MPU6500_PWR_MGMT_2,0x00);   //使能寄存器 X，Y，Z加速度
    MPU_Write(MPU6500_I2C_ADDR,MPU6500_SMPLRT_DIV,0x07);   //    SMPLRT_DIV 采样率分频寄存器，输入采样时钟为1kHz，陀螺仪采样率1000/(1+7)=125HZ
    MPU_Write(MPU6500_I2C_ADDR,MPU6500_CONFIG,0x06);//设为0x05时，GYRO的带宽为10Hz，延时为17.85ms，设为0x06时，带宽5Hz，延时33.48ms（建议使用0x05）
    MPU_Write(MPU6500_I2C_ADDR,MPU6500_GYRO_CONFIG,0x10);//=>±1000dps  加速度计测量范围 正负16g
    MPU_Write(MPU6500_I2C_ADDR,MPU6500_USER_CTRL,0x00); // 初始化I2C
    MPU_Write(MPU6500_I2C_ADDR,MPU6500_ACCEL_CONFIG,0x10);// 加速度计测量范围 0X10 正负8g
    MPU_Write(MPU6500_I2C_ADDR,MPU6500_INT_PIN_CFG,0x02);//进入Bypass模式，用于控制电子指南针
    return 0;
    }
    return 1;
}

/*读取MPU9250数据*/
u8 TX_DATA[4];//显示据缓存区
u8 BUF[10];//接收数据缓存区

u8 T_X,T_Y,T_Z,T_T;//X,Y,Z轴，温度
/*模拟IIC端口输出输入定义*/
/****************************************/
void READ_MPU9250_ACCEL(void)
{
    //读取计算X轴数据    T_X =advalue/
    BUF[0]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_ACCEL_XOUT_L );
    BUF[1]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_ACCEL_XOUT_H );
    T_X= (BUF[1]<<8)|BUF[0];
     T_X/=4096;
 
    //读取计算Y轴数据
    BUF[2]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_ACCEL_YOUT_L);
    BUF[3]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_ACCEL_YOUT_H);
    T_Y= (BUF[3]<<8)|BUF[2];
    T_Y/=4096;
 
    //读取计算Z轴数据
    BUF[4]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_ACCEL_ZOUT_L);
    BUF[5]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_ACCEL_ZOUT_H);
    T_Z= (BUF[5]<<8)|BUF[4];
    T_Z/=4096;
}

void READ_MPU9250_GYRO(void)
{
    //读取计算X轴数据
    BUF[0]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_GYRO_XOUT_L);
    BUF[1]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_GYRO_XOUT_H);
    T_X=(BUF[1]<<8)|BUF[0];
    T_X/=32.8;
 
    //读取计算Y轴数据
    BUF[2]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_GYRO_YOUT_L);
    BUF[3]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_GYRO_YOUT_H);
    T_Y= (BUF[3]<<8)|BUF[2];
    T_Y/=32.8;
 
    //读取计算Z轴数据
    BUF[4]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_GYRO_ZOUT_L);
    BUF[5]=MPU_Read(MPU6500_I2C_ADDR,MPU6500_GYRO_ZOUT_H);
    T_Z=(BUF[5]<<8)|BUF[4];
    T_Z/=32.8;


}

void READ_MPU9250_MAG(void)
{
    MPU_Write(AK8963_I2C_ADDR,0x37,0x02);  //turn on Bypass Mode
    MPU_Write(AK8963_I2C_ADDR ,0x0A,0x01);
    //读取计算X轴数据
    BUF[0]=MPU_Read (AK8963_I2C_ADDR,AK8963_HXL);
    BUF[1]=MPU_Read (AK8963_I2C_ADDR,AK8963_HXH);
    T_X=(BUF[1]<<8)|BUF[0];

    // 读取计算Y轴数据
    BUF[2]=MPU_Read(AK8963_I2C_ADDR,AK8963_HYL);
    BUF[3]=MPU_Read(AK8963_I2C_ADDR,AK8963_HYH);
    T_Y=(BUF[3]<<8)|BUF[2];
    //读取计算Z轴数据
    BUF[4]=MPU_Read(AK8963_I2C_ADDR,AK8963_HZL);
    BUF[5]=MPU_Read(AK8963_I2C_ADDR,AK8963_HZL);
    T_Z= (BUF[5]<<8)|BUF[4];
 }