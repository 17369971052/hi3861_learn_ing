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

#include "MPU92502.h"

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
static int I2CWriteByte(uint8_t addr,uint8_t Reg, uint8_t Value){
    uint32_t ret;
    uint8_t send_date[2] = { Reg, Value };
    ret = IoTI2cWrite(HI_I2C_IDX_1, (addr << 1) | 0x00, send_date, sizeof(send_date));
    if (ret != 0)
    { 
        printf("IIC_Write ret = %x", ret);
        return ret;
    }
    return 0;
}



// 六轴 
static int I2CRead(uint8_t addr, uint8_t Reg,  uint16_t Length, uint8_t *pBuffer)
{
    IotI2cData mpu9250data = {0};
    uint8_t buffer[1] = { Reg };
    mpu9250data.sendBuf = buffer;
    mpu9250data.sendLen = 1;
    mpu9250data.receiveBuf = pBuffer;
    mpu9250data.receiveLen = Length;
    int ret = IoTI2cWriteread(HI_I2C_IDX_1, (addr << 1) | 0x00, &mpu9250data);
    if(ret != 0) {
        printf("===== Error: I2C writeread ret = 0x%x! =====\r\n", ret);
        return ret;
    }
    return 0;
}



int mpuconfig(int ways)
{
 if(ways==1)
 {	 
  I2CWriteByte(MPU9250_ADDRESS, 0x6B, 0x80); //重置寄存器
  delay(100);
  I2CWriteByte(MPU9250_ADDRESS,0x1C,ACC_FULL_SCALE_2_G);//设置重力计单位值
  I2CWriteByte(MPU9250_ADDRESS,0x1B,GYRO_FULL_SCALE_250_DPS);//设置陀螺仪单位值
  I2CWriteByte(MPU9250_ADDRESS,0x37,0x02);//设置旁路模式
  I2CWriteByte(MAG_ADDRESS,0x0A,0x11);//设置磁力计通讯模式
 }
}

void readAG(void)	//读取加速度和陀螺仪的数据
{
  uint8_t Buf[14];
  I2CRead(MPU9250_ADDRESS,0x3B,14,Buf);

    // 加速度
  ACC_X=(Buf[0]<<8 | Buf[1]);
  ACC_Y=(Buf[2]<<8 | Buf[3]);
  ACC_Z=Buf[4]<<8 | Buf[5];
  
	//陀螺仪
  Gyro_X=(Buf[8]<<8 | Buf[9]);
  Gyro_Y=(Buf[10]<<8 | Buf[11]);
  Gyro_Z=(Buf[12]<<8 | Buf[13]);
  
}

void readMag(void)
{ /*
  uint8_t Buf[6];	
  I2CRead(MAG_ADDRESS,0x10,3,Buf);  //读取磁力计数据

  Mag_X=(Buf[1]<<8 | Buf[0]);
  Mag_Y=(Buf[3]<<8 | Buf[2]);
  Mag_Z=-(Buf[5]<<8 | Buf[4]);
 */
  uint8_t rawData[6];  // x/y/z gyro calibration data stored here
  I2CWriteByte(MPU9250_ADDRESS, 0x37, 0x02); // Power down magnetometer  
  I2CWriteByte(MAG_ADDRESS, 0x0A, 0x01); // Enter Fuse ROM access mode
  I2CRead(MAG_ADDRESS, 0x10, 3, rawData);
  short Mag_X =  (rawData[0]<<8 | rawData[1]);   
  short Mag_Y =  (rawData[2]<<8 | rawData[3]);
  short Mag_Z =  (rawData[4]<<8 | rawData[5]);
  I2CWriteByte(MAG_ADDRESS, 0x0A, 0x00); // Power down magnetometer  
  delay(10);
  
}