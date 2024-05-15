#include <stdio.h>
#include <string.h>
#include "include/iot_config.h"
#include "include/iot_main.h"
#include "include/iot_profile.h"
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "include/wifi_connecter.h"
#include "include/iot_gpio_ex.h"
#include "iot_gpio.h"
#include "iot_watchdog.h"
#include "include/cjson_init.h"
#include "hi_stdlib.h"
#include "include/sg92r_control.h"
/* attribute initiative to report */
#define TAKE_THE_INITIATIVE_TO_REPORT
/* oc request id */
#define CN_COMMAND_INDEX                    "commands/request_id="
/* oc report HiSpark attribute */
#define CMD_CONTROL_MODE      "Human_Existence_Service"  //属性名称
#define STRVO_RIGHT_ON_PAYLOAD        "Servo_Control"    //下发命令
#define Servo_ON "ON"
#define Servo_OFF "OFF"

#define TASK_SLEEP_1000MS (1000)
int g_lightStatus = -1;
int flag = 0;

//处理命令的回调函数
static void ServoControlMsgRcvCallBack(char *payload)
{
    //注意：strstr()这个函数是查找字符串中指定字符的函数
    //paylad是下发命令信息
    //strstr()可以在下发命令信息中找到华为云自己设置相应的“命令”字符串并返回，如果没有返回NULL
    printf("PAYLOAD:+++===%s===+++\r\n", payload);
    if (strstr(payload, CMD_CONTROL_MODE) != NULL) {
        if (strstr(payload, STRVO_RIGHT_ON_PAYLOAD) != NULL) { 
          if(flag == 0 && strstr(payload,Servo_ON) != NULL) {
            printf("==%s==\r\n",strstr(payload,Servo_ON));
            Turn_Where(Control_ON);
            g_lightStatus = 0;
            flag = 1;
          }
          
          if(g_lightStatus == 0 && strstr(payload,Servo_OFF) != NULL) {
            printf("==%s==\r\n",strstr(payload,Servo_OFF));
            Turn_Where(Control_OFF);
            g_lightStatus = 1;
            flag = 0;
          }   
        //   if(strstr(payload,Servo_ON) != NULL) {
        //     printf("==%s==\r\n",strstr(payload,Servo_ON));
        //     Turn_Where(Control_ON);
        //   }
          
        //   if(strstr(payload,Servo_OFF) != NULL) {
        //     printf("==%s==\r\n",strstr(payload,Servo_OFF));
        //     Turn_Where(Control_OFF);
        //   }

    }
    
  }
}

// /< this is the callback function, set to the mqtt, and if any messages come, it will be called
// /< The payload here is the json string
//上云命令
static void DemoMsgRcvCallBack(int qos, char *topic, char *payload)
{
    const char *requesID;
    char *tmp;
    IoTCmdResp resp;
    printf("RCVMSG:QOS:%d TOPIC:%s PAYLOAD:%s\r\n", qos, topic, payload);
    /* app 下发的操作 */
    ServoControlMsgRcvCallBack(payload);
    tmp = strstr(topic, CN_COMMAND_INDEX);
    if (tmp != NULL) {
        // /< now you could deal your own works here --THE COMMAND FROM THE PLATFORM
        // /< now er roport the command execute result to the platform
        requesID = tmp + strlen(CN_COMMAND_INDEX);
        resp.requestID = requesID;
        resp.respName = NULL;
        resp.retCode = 0;   ////< which means 0 success and others failed
        resp.paras = NULL;
        (void)IoTProfileCmdResp(CONFIG_DEVICE_PWD, &resp);
    }
}

//上云数据
void IotPublishSample(void)
{
    IoTProfileService service;
    IoTProfileKV property;
    printf("strove control:control module\r\n");
    memset_s(&property, sizeof(property), 0, sizeof(property));
    property.type = EN_IOT_DATATYPE_STRING;
    property.key = "Human_Existence_Service";
    if (g_lightStatus == 0) {
        property.value = "Strvo_Right_ON";
    } else {
        property.value = "Strvo_Left_OFF";
    }
    memset_s(&service, sizeof(service), 0, sizeof(service));
    service.serviceID = "Human_Existence_Service";
    service.serviceProperty = &property;
    IoTProfilePropertyReport(CONFIG_DEVICE_ID, &service);
}

// /< this is the demo main task entry,here we will set the wifi/cjson/mqtt ready ,and
// /< wait if any work to do in the while
static void Main_Entry(void)
{
    ConnectToHotspot();
    CJsonInit();
    IoTMain();
    IoTSetMsgCallback(DemoMsgRcvCallBack);
    TaskMsleep(30000); // 30000 = 3s连接华为云
    SG92RInit();
    /* 主动上报 */
    while (1) {
        // here you could add your own works here--we report the data to the IoTplatform
        TaskMsleep(TASK_SLEEP_1000MS);
        // know we report the data to the iot platform
        IotPublishSample();
    }
}

// This is the demo entry, we create a task here, and all the works has been done in the demo_entry
#define CN_IOT_TASK_STACKSIZE  0x1000
#define CN_IOT_TASK_PRIOR 28
#define CN_IOT_TASK_NAME "Strvo_Control"
static void Strvo_Control(void)
{
    osThreadAttr_t attr;
    IoTWatchDogDisable();
    attr.name = "Strvo_Control";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = CN_IOT_TASK_STACKSIZE;
    attr.priority = CN_IOT_TASK_PRIOR;

    if (osThreadNew((osThreadFunc_t)Main_Entry, NULL, &attr) == NULL) {
        printf("[TrafficLight] Failed to create Strvo_Control!\n");
    }
}

SYS_RUN(Strvo_Control);