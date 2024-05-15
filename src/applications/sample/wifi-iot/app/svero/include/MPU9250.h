/* 如果没有定义MPU_9250_H这个宏，那就对下面的代码进行编译 */
#ifndef MPU_9250_H
#define MPU_9250_H
/*====================================================================================================*/
#define MPU_CORRECTION_FLASH     0x0800F000        //存储校正数据的FLASH地址，SIZE=6*3*4字节

/*GPIO 相关的宏定义*/
#define WIFI_IOT_IO_FUNC_GPIO_0_I2C1_SDA 6
#define WIFI_IOT_IO_FUNC_GPIO_1_I2C1_SCL 6
#define WIFI_IOT_IO_FUNC_GPIO_8_GPIO 0
#define WIFI_IOT_IO_FUNC_GPIO_14_GPIO 4
#define WIFI_IOT_I2C_IDX_1 1
#define WIFI_IOT_I2C_BAUDRATE 400000

#define WIFI_IOT_IO_NAME_GPIO_0  0
#define WIFI_IOT_IO_NAME_GPIO_1  1

typedef struct 
{
    uint16_t Accel[3];
    uint16_t Gyro[3];
    uint16_t Mag[3];
} MPU9250_ReadData;

// 主函数使用的相关宏
#define MPU6050_DATA_2_BYTE 2

/*
* @brief 用于GPIO口的功能设置
*       这个函数里是将GPIO1以及GPIO9两个引脚用作IIC的
*/


/*
* 函数功能: 读取MPU9250的数据
* 参数：    自定义结构体，里面存储了要读取的数据
*/
int ReadData_MPU9250(MPU9250_ReadData *ReadData);

int MPU9250_I2C_Init(void);

int Mpu9250_Work_Mode_Init(void);
// int ReadData_MPU9250(MPU9250_ReadData *ReadData);
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
#define MPU6500_I2C_ADDR            ((uint8_t)0xD0)  //MPU9250设备地址
#define MPU9250_I2C_ADDR            ((uint8_t)0x69)  //MPU9250设备地址0x1101 0010  D2
#define MPU6500_Device_ID           ((uint8_t)0x71)  // In MPU9250
#define MPU6500_SELF_TEST_XG        ((uint8_t)0x00)
#define MPU6500_SELF_TEST_YG        ((uint8_t)0x01)
#define MPU6500_SELF_TEST_ZG        ((uint8_t)0x02)
#define MPU6500_SELF_TEST_XA        ((uint8_t)0x0D)
#define MPU6500_SELF_TEST_YA        ((uint8_t)0x0E)
#define MPU6500_SELF_TEST_ZA        ((uint8_t)0x0F)
#define MPU6500_XG_OFFSET_H         ((uint8_t)0x13)
#define MPU6500_XG_OFFSET_L         ((uint8_t)0x14)
#define MPU6500_YG_OFFSET_H         ((uint8_t)0x15)
#define MPU6500_YG_OFFSET_L         ((uint8_t)0x16)
#define MPU6500_ZG_OFFSET_H         ((uint8_t)0x17)
#define MPU6500_ZG_OFFSET_L         ((uint8_t)0x18)
#define MPU6500_SMPLRT_DIV          ((uint8_t)0x19)  // 采样率分频寄存器，输入采样时钟为1kHz
#define MPU6500_CONFIG              ((uint8_t)0x1A)  // 配置寄存器
#define MPU6500_GYRO_CONFIG         ((uint8_t)0x1B)  // 陀螺仪（角速度）配置寄存器
#define MPU6500_ACCEL_CONFIG        ((uint8_t)0x1C)  // 加速度配置寄存器
#define MPU6500_ACCEL_CONFIG_2      ((uint8_t)0x1D)
#define MPU6500_LP_ACCEL_ODR        ((uint8_t)0x1E)
#define MPU6500_MOT_THR             ((uint8_t)0x1F)
#define MPU6500_FIFO_EN             ((uint8_t)0x23)
#define MPU6500_I2C_MST_CTRL        ((uint8_t)0x24)
#define MPU6500_I2C_SLV0_ADDR       ((uint8_t)0x25)
#define MPU6500_I2C_SLV0_REG        ((uint8_t)0x26)
#define MPU6500_I2C_SLV0_CTRL       ((uint8_t)0x27)
#define MPU6500_I2C_SLV1_ADDR       ((uint8_t)0x28)
#define MPU6500_I2C_SLV1_REG        ((uint8_t)0x29)
#define MPU6500_I2C_SLV1_CTRL       ((uint8_t)0x2A)
#define MPU6500_I2C_SLV2_ADDR       ((uint8_t)0x2B)
#define MPU6500_I2C_SLV2_REG        ((uint8_t)0x2C)
#define MPU6500_I2C_SLV2_CTRL       ((uint8_t)0x2D)
#define MPU6500_I2C_SLV3_ADDR       ((uint8_t)0x2E)
#define MPU6500_I2C_SLV3_REG        ((uint8_t)0x2F)
#define MPU6500_I2C_SLV3_CTRL       ((uint8_t)0x30)
#define MPU6500_I2C_SLV4_ADDR       ((uint8_t)0x31)
#define MPU6500_I2C_SLV4_REG        ((uint8_t)0x32)
#define MPU6500_I2C_SLV4_DO         ((uint8_t)0x33)
#define MPU6500_I2C_SLV4_CTRL       ((uint8_t)0x34)
#define MPU6500_I2C_SLV4_DI         ((uint8_t)0x35)
#define MPU6500_I2C_MST_STATUS      ((uint8_t)0x36)
#define MPU6500_INT_PIN_CFG         ((uint8_t)0x37)   //INT引脚配置和Bypass模式配置寄存器
#define MPU6500_INT_ENABLE          ((uint8_t)0x38)
#define MPU6500_INT_STATUS          ((uint8_t)0x3A)

#define    MPU6500_ACCEL_XOUT_H    0x3B   //三轴加速度寄存器
#define    MPU6500_ACCEL_XOUT_L    0x3C
#define    MPU6500_ACCEL_YOUT_H    0x3D
#define    MPU6500_ACCEL_YOUT_L    0x3E
#define    MPU6500_ACCEL_ZOUT_H    0x3F
#define    MPU6500_ACCEL_ZOUT_L    0x40
#define    MPU6500_TEMP_OUT_H      ((uint8_t)0x41)
#define    MPU6500_TEMP_OUT_L      ((uint8_t)0x42)
#define    GYRO_XOUT_H        0x43
#define    GYRO_XOUT_L        0x44    
#define    GYRO_YOUT_H        0x45
#define    GYRO_YOUT_L        0x46
#define    GYRO_ZOUT_H        0x47
#define    GYRO_ZOUT_L        0x48
//三轴陀螺仪（角速度）寄存器
#define MPU6500_GYRO_XOUT_H         ((uint8_t)0x43)
#define MPU6500_GYRO_XOUT_L         ((uint8_t)0x44)
#define MPU6500_GYRO_YOUT_H         ((uint8_t)0x45)
#define MPU6500_GYRO_YOUT_L         ((uint8_t)0x46)
#define MPU6500_GYRO_ZOUT_H         ((uint8_t)0x47)
#define MPU6500_GYRO_ZOUT_L         ((uint8_t)0x48)
#define MPU6500_EXT_SENS_DATA_00    ((uint8_t)0x49)
#define MPU6500_EXT_SENS_DATA_01    ((uint8_t)0x4A)
#define MPU6500_EXT_SENS_DATA_02    ((uint8_t)0x4B)
#define MPU6500_EXT_SENS_DATA_03    ((uint8_t)0x4C)
#define MPU6500_EXT_SENS_DATA_04    ((uint8_t)0x4D)
#define MPU6500_EXT_SENS_DATA_05    ((uint8_t)0x4E)
#define MPU6500_EXT_SENS_DATA_06    ((uint8_t)0x4F)
#define MPU6500_EXT_SENS_DATA_07    ((uint8_t)0x50)
#define MPU6500_EXT_SENS_DATA_08    ((uint8_t)0x51)
#define MPU6500_EXT_SENS_DATA_09    ((uint8_t)0x52)
#define MPU6500_EXT_SENS_DATA_10    ((uint8_t)0x53)
#define MPU6500_EXT_SENS_DATA_11    ((uint8_t)0x54)
#define MPU6500_EXT_SENS_DATA_12    ((uint8_t)0x55)
#define MPU6500_EXT_SENS_DATA_13    ((uint8_t)0x56)
#define MPU6500_EXT_SENS_DATA_14    ((uint8_t)0x57)
#define MPU6500_EXT_SENS_DATA_15    ((uint8_t)0x58)
#define MPU6500_EXT_SENS_DATA_16    ((uint8_t)0x59)
#define MPU6500_EXT_SENS_DATA_17    ((uint8_t)0x5A)
#define MPU6500_EXT_SENS_DATA_18    ((uint8_t)0x5B)
#define MPU6500_EXT_SENS_DATA_19    ((uint8_t)0x5C)
#define MPU6500_EXT_SENS_DATA_20    ((uint8_t)0x5D)
#define MPU6500_EXT_SENS_DATA_21    ((uint8_t)0x5E)
#define MPU6500_EXT_SENS_DATA_22    ((uint8_t)0x5F)
#define MPU6500_EXT_SENS_DATA_23    ((uint8_t)0x60)
#define MPU6500_I2C_SLV0_DO         ((uint8_t)0x63)
#define MPU6500_I2C_SLV1_DO         ((uint8_t)0x64)
#define MPU6500_I2C_SLV2_DO         ((uint8_t)0x65)
#define MPU6500_I2C_SLV3_DO         ((uint8_t)0x66)
#define MPU6500_I2C_MST_DELAY_CTRL  ((uint8_t)0x67)
#define MPU6500_SIGNAL_PATH_RESET   ((uint8_t)0x68)
#define MPU6500_MOT_DETECT_CTRL     ((uint8_t)0x69)
#define MPU6500_USER_CTRL           ((uint8_t)0x6A)
#define MPU6500_PWR_MGMT_1          ((uint8_t)0x6B)  //MPU9250电源管理寄存器
#define MPU6500_PWR_MGMT_2          ((uint8_t)0x6C)
#define MPU6500_FIFO_COUNTH         ((uint8_t)0x72)
#define MPU6500_FIFO_COUNTL         ((uint8_t)0x73)    //
#define MPU6500_FIFO_R_W            ((uint8_t)0x74)
#define MPU6500_WHO_AM_I            ((uint8_t)0x75)    //    //MPU9250设备ID寄存器 0x75
#define MPU6500_XA_OFFSET_H         ((uint8_t)0x77)
#define MPU6500_XA_OFFSET_L         ((uint8_t)0x78)
#define MPU6500_YA_OFFSET_H         ((uint8_t)0x7A)
#define MPU6500_YA_OFFSET_L         ((uint8_t)0x7B)
#define MPU6500_ZA_OFFSET_H         ((uint8_t)0x7D)
#define MPU6500_ZA_OFFSET_L         ((uint8_t)0x7E)
/* ---- AK8963 Reg In MPU9250 ----------------------------------------------- */
#define AK8963_I2C_ADDR             ((uint8_t)0x0C)   //指南针设备地址
#define AK8963_DEV_ID               ((uint8_t)0x48)  //指南针设备ID
//Read-only Reg
#define AK8963_WIA                  ((uint8_t)0x00)    //指南针设备ID寄存器
#define AK8963_INFO                 ((uint8_t)0x01)
#define AK8963_ST1                  ((uint8_t)0x02)
#define AK8963_HXL                  ((uint8_t)0x03)
#define AK8963_HXH                  ((uint8_t)0x04)
#define AK8963_HYL                  ((uint8_t)0x05)
#define AK8963_HYH                  ((uint8_t)0x06)
#define AK8963_HZL                  ((uint8_t)0x07)
#define AK8963_HZH                  ((uint8_t)0x08)
#define AK8963_ST2                  ((uint8_t)0x09)
//Write/Read Reg
#define AK8963_CNTL1                ((uint8_t)0x0A) //启动单次传输
#define AK8963_CNTL2                ((uint8_t)0x0B)
#define AK8963_ASTC                 ((uint8_t)0x0C)
#define AK8963_TS1                  ((uint8_t)0x0D)
#define AK8963_TS2                  ((uint8_t)0x0E)
#define AK8963_I2CDIS               ((uint8_t)0x0F)
//Read-only Reg ( ROM )
#define AK8963_ASAX                 ((uint8_t)0x10)
#define AK8963_ASAY                 ((uint8_t)0x11)
#define AK8963_ASAZ                 ((uint8_t)0x12)

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
#define TEMP_OUT_H        0x41
#define EMP_OUT_L         0x42
#define MAG_XOUT_L        0x03
#define MAG_XOUT_H        0x04
#define MAG_YOUT_L        0x05
#define MAG_YOUT_H        0x06
#define MAG_ZOUT_L        0x07
#define MAG_ZOUT_H        0x08
#endif