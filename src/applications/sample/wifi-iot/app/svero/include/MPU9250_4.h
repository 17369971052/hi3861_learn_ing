#ifndef MPU9250_4
#define MPU9250_4


/*IIC通信地址*/
#define MPU9250_ADDRESS0 0x68    // AD0 0  68
#define MPU9250_ADDRESS1 0x69    // AD0 1


/* 寄存器地址 */
// 电源配置寄存器
#define PWR_MGMT_1          0x6B 
#define ACC_XYZ             0x0A

/* 寄存器配置 */

/*1.启用内部 20MHz 振荡时钟源
2.PTAT ADC 不掉电
3.不使用芯片待机模式
4.不进入休眠模式
5.重置所有寄存器和数据*/
#define PWR_MGMT_1_T   0x80

#define GYRO_FULL_SCALE_250_DPS 

#define ACC_FULL_SCALE_2_G  0x00
/*
*/
// #define 
#endif