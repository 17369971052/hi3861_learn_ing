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
#include "MPU9250.h"



#define MPU9250Addr 	0xd0
 
/*****************************************************************************
* 功  能：写一个字节数据到 MPU9250 寄存器
* 参  数：reg： 寄存器地址
*         data: 要写入的数据
* 返回值：0成功 1失败
*****************************************************************************/
uint8_t MPU9250_WriteByte(uint8_t reg,uint8_t data)
{
	if(HALIIC_WriteByteToSlave(MPU9250Addr,reg,data))
		return 1;
	else
		return 0;
}
 
/*****************************************************************************
* 功  能：从指定MPU6050寄存器读取一个字节数据
* 参  数：reg： 寄存器地址
*         buf:  读取数据存放的地址
* 返回值：1失败 0成功
*****************************************************************************/
uint8_t MPU9250_ReadByte(uint8_t reg,uint8_t *buf)
{
	if(HALIIC_ReadByteFromSlave(MPU9250Addr,reg,buf))
		return 1;
	else
		return 0;
}
 
/*****************************************************************************
* 功  能：从指定寄存器写入指定长度数据
* 参  数：reg：寄存器地址
*         len：写入数据长度
*         buf: 写入数据存放的地址
* 返回值：0成功 1失败
*****************************************************************************/
uint8_t MPU9250_WriteMultBytes(uint8_t reg,uint8_t len,uint8_t *buf)
{
	if(HALIIC_WriteMultByteToSlave(MPU9250Addr,reg,len,buf))
		return 1;
	else
		return 0;
}
 
/*****************************************************************************
* 功  能：从指定寄存器读取指定长度数据
* 参  数：reg：寄存器地址
*         len：读取数据长度
*         buf: 读取数据存放的地址
* 返回值：0成功 0失败
*****************************************************************************/
uint8_t MPU9250_ReadMultBytes(uint8_t reg,uint8_t len,uint8_t *buf)
{
	if(HALIIC_ReadMultByteFromSlave(MPU9250Addr,reg,len,buf))
		return 1;
	else
		return 0;
}
