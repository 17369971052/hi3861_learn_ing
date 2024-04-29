#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h" //调用头文件
#include "ohos_init.h"

osMessageQueueId_t message_id;
osStatus_t status;

typedef struct { //定义结构体
    char *name;
    uint8_t number;
}MSG;

MSG msg; //命名结构体msg

void message_Put(void)  //设定函数
{
    //(void)ptr;
    uint8_t num=0;
    msg.name="Hello BearPi-HM_Nano!";
    
    while(1){  //一直运行，防止进入退出态
        msg.number=num;//赋值num
        status=osMessageQueuePut(message_id,&msg,0U,0U);
        num++;//num变量累加
        if (status != osOK){
            printf("Failed to create message_Put!\r\n");//输出错误提示
        }
        osThreadYield();   //挂起任务
        osDelay(100U);
    }
}

void message_Get(void)  //设定函数
{
    while(1){   //一直运行，防止进入退出态
        uint32_t count;
        count = osMessageQueueGetCount(message_id);
        printf("message Queue count is %d\r\n",count);
        if (count > 14){
            osMessageQueueDelete(message_id);
        }
        status=osMessageQueueGet(message_id,&msg,NULL,osWaitForever);
        if (status != osOK){
            printf("Failed to create message_Get!\r\n");
        }
        else {
            printf("message Get msg name:%s, number:%d\r\n",msg.name,msg.number);
        }
        osDelay(300U);
    }
}

void message_auto_Delete(void)  //消息获取一次后，删除消息队列函数
{
    while(1){
        uint32_t count;
        count = osMessageQueueGetCount(message_id);//获取消息数
        printf("message Queue count is %d\r\n",count);
        status=osMessageQueueGet(message_id,&msg,NULL,osWaitForever);
        if (status != osOK){
            printf("Failed to create message_Get!\r\n");//输出错误提示
        }
        else {
            printf("message Get msg name:%s, number:%d\r\n",msg.name,msg.number);//输出消息
            osMessageQueueDelete(message_id);  //删除消息队列
        }
        osDelay(300U);
    }
}
static void message_example(void)   //设置函数
{
    message_id=osMessageQueueNew(16,100,NULL);   // (队列的节点数量 ,队列的单个节点空间大小 ,队列参数，若参数为NULL则使用默认配置)
    if (message_id == NULL){
        printf("Failed to create message!\r\n");  //当空时输出创建失败提示
    }

    osThreadAttr_t attr;  //初始化该线程的各项配置
    attr.attr_bits=0U;    //线程属性位
    attr.cb_mem=NULL;     //用户指定的控制块指针
    attr.cb_size=0U;      //用户指定的控制块大小（以字节为单位）
    attr.stack_mem=NULL;  //用户指定的线程栈指针
    attr.stack_size=1024*4;//线程栈大小（以字节为单位）
    //第一个任务
    attr.name="Thread_1"; //任务名称
    attr.priority=25;     //线程优先级
    if (osThreadNew((osThreadFunc_t)message_Put,NULL,&attr) == NULL){
        printf("Failed to create message_Put!\r\n");
    }
    //创建第二个任务
    attr.name="Thread_2";//任务名称
    if (osThreadNew((osThreadFunc_t)message_Get,NULL,&attr) == NULL){
        printf("Failed to create message_Get!\r\n");
    }    
}
APP_FEATURE_INIT(message_example);