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
#include "SG90_servo.h"

#define CONFIG_WIFI_SSID "HUAWEI nova 5z" // 修改为自己的WiFi 热点账号

#define CONFIG_WIFI_PWD "56771231" // 修改为自己的WiFi 热点密码

#define CONFIG_APP_SERVERIP "117.78.5.125"//定义设备服务IP地址

#define CONFIG_APP_SERVERPORT "1883"//定义设备IP端口

#define CONFIG_APP_DEVICEID "664de1286bc31504f06b75f5_servodemo" // 替换为注册设备后生成的deviceid

#define CONFIG_APP_DEVICEPWD "557414c99cf4e34bc39972888353bfe0" // 替换为注册设备后生成的密钥

#define CONFIG_APP_LIFETIME 60 // seconds // 设备生命周期

#define CONFIG_QUEUE_TIMEOUT (5 * 1000)  //消息队列超时

#define MSGQUEUE_COUNT 16  //消息数量
#define MSGQUEUE_SIZE 10  //消息空间
#define CLOUD_TASK_STACK_SIZE (1024 * 10)  //任务栈空间
#define CLOUD_TASK_PRIO 24  //优先级
#define SENSOR_TASK_STACK_SIZE (1024 * 4)  //--
#define SENSOR_TASK_PRIO 25  //--
#define TASK_DELAY 3  //任务延时

osMessageQueueId_t mid_MsgQueue; // message queue id
typedef enum {  //创建消息类型
    en_msg_cmd = 0,
    en_msg_report,
    en_msg_conn,
    en_msg_disconn,
} en_msg_type_t;

typedef struct {  //创建命令
    char *request_id;
    char *payload;
} cmd_t;

typedef struct {  //定义各个属性
    int angle;

} report_t;

typedef struct {  //消息结构体
    en_msg_type_t msg_type;
    union {
        cmd_t cmd;
        report_t report;
    } msg;
} app_msg_t;

typedef struct {  //回调结构体
    osMessageQueueId_t app_msg;
    int connected;
    int angle;
    int led;
} app_cb_t;
static app_cb_t g_app_cb;



typedef struct oc_data_upload
{
    int angle;

}oc_iot_data;

oc_iot_data oc_1 = {.angle = 360};

hi_void InitServo(void)
{
    IoTGpioInit(HI_GPIO_IDX_2);
    IoTGpioSetFunc(HI_GPIO_IDX_2,HI_IO_FUNC_GPIO_2_GPIO);
    IoTGpioSetDir(HI_GPIO_IDX_2, HI_GPIO_DIR_OUT);
    IoTGpioSetOutputVal(HI_GPIO_IDX_2,HI_GPIO_VALUE0);
}

static void set_angle(hi_u32 utime)
 {
     InitServo();
     IoTGpioSetOutputVal(HI_GPIO_IDX_2,HI_GPIO_VALUE1);
     hi_udelay(utime);
     IoTGpioSetOutputVal(HI_GPIO_IDX_2,HI_GPIO_VALUE0);
     hi_udelay(19350-utime);
 }

hi_void engine_0(hi_void)
 {
     for (int i = 0; i <10; i++) 
     {
         set_angle(500);
         
     }
     oc_1.angle = 0;
 }
 
  //左转
 hi_void engine_45(hi_void)
 {
     for (int i = 0; i <10; i++) {
         set_angle(1000);
     }
     oc_1.angle = 45;
 }
 //右转
 hi_void engine_90(hi_void)
 {
     for (int i = 0; i <10; i++) {
         set_angle(1500);
     }
     oc_1.angle = 90;
 }
  hi_void engine_145(hi_void)
 {
     for (int i = 0; i <10; i++) {
         set_angle(2000);
     }
     oc_1.angle = 145;
 }
  hi_void engine_180(hi_void)
 {
     for (int i = 0; i <10; i++) {
         set_angle(2500);
     }
     oc_1.angle = 180;
 }



static void deal_report_msg(report_t *report)  //上报消息
{
    oc_mqtt_profile_service_t service;  //创建结构体
    oc_mqtt_profile_kv_t angle;  //属性
//若连接失败返回
    if (g_app_cb.connected != 1) {
        return;
    }

    service.event_time = NULL;  //设置时间为空
    service.service_id = "ServoControl";  //设置服务ID
    service.service_property = &angle;  //设置属性为温度
    service.nxt = NULL;  //设置下一个服务为空

    angle.key = "Angle";
    angle.value = &report->angle;
    angle.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    angle.nxt = NULL;

    oc_mqtt_profile_propertyreport(NULL, &service);  //调用oc_mqtt_profile_propertyreport函数上报属性

    return;  // 返回函数
}

// use this function to push all the message to the buffer
//处理回调命令
static int msg_rcv_callback(oc_mqtt_profile_msgrcv_t *msg)
{
    int ret = 0;  //定义变量
    char *buf;
    int buf_len;
    app_msg_t *app_msg;

    if ((msg == NULL) || (msg->request_id == NULL) || (msg->type != EN_OC_MQTT_PROFILE_MSG_TYPE_DOWN_COMMANDS)) {
        return ret;  //若消息或id或类型错误返回ret值
    }

    buf_len = sizeof(app_msg_t) + strlen(msg->request_id) + 1 + msg->msg_len + 1;  //定义消息长度
    buf = malloc(buf_len);  //分配内存空间
    if (buf == NULL) {
        return ret;  //无空间则返回
    }
    app_msg = (app_msg_t *)buf;  //指针类型转换
    buf += sizeof(app_msg_t);  //累加空间

    app_msg->msg_type = en_msg_cmd;  //定义消息类型为命令
    app_msg->msg.cmd.request_id = buf;  //定义消息ID
    buf_len = strlen(msg->request_id);  //定义消息长度
    buf += buf_len + 1;  //累加空间
    memcpy_s(app_msg->msg.cmd.request_id, buf_len, msg->request_id, buf_len);
    app_msg->msg.cmd.request_id[buf_len] = '\0';

    buf_len = msg->msg_len;
    app_msg->msg.cmd.payload = buf;
    memcpy_s(app_msg->msg.cmd.payload, buf_len, msg->msg, buf_len);
    app_msg->msg.cmd.payload[buf_len] = '\0';

    ret = osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT);
    if (ret != 0) {
        free(app_msg);  //释放内存
    }

    return ret;
}

static void oc_cmdresp(cmd_t *cmd, int cmdret)  //生成命令回复
{
    oc_mqtt_profile_cmdresp_t cmdresp;
    ///< do the response
    cmdresp.paras = NULL;
    cmdresp.request_id = cmd->request_id;
    cmdresp.ret_code = cmdret;
    cmdresp.ret_name = NULL;
    (void)oc_mqtt_profile_cmdresp(NULL, &cmdresp);
}

///< COMMAND DEAL
#include <cJSON.h>
static void deal_light_cmd(cmd_t *cmd, cJSON *obj_root)  //灯处理函数
{
    cJSON *obj_paras;
    cJSON *obj_para;
    int cmdret;

    obj_paras = cJSON_GetObjectItem(obj_root, "paras");
    if (obj_paras == NULL) {
        cJSON_Delete(obj_root);
    }
    obj_para = cJSON_GetObjectItem(obj_paras, "Light");
    if (obj_paras == NULL) {
        cJSON_Delete(obj_root);
    }
    ///< operate the LED here
    if (strcmp(cJSON_GetStringValue(obj_para), "ON") == 0) 
    {
        g_app_cb.led = 1;
        engine_145();
        printf("Light On!\r\n");
    } 
    else 
    {
        g_app_cb.led = 0;
        engine_45();
        printf("Light Off!\r\n");
    }
    cmdret = 0;
    oc_cmdresp(cmd, cmdret);

    cJSON_Delete(obj_root);
    return;
}


static void deal_cmd_msg(cmd_t *cmd)  //处理命令消息函数
{
    cJSON *obj_root;
    cJSON *obj_cmdname;

    int cmdret = 1;
    obj_root = cJSON_Parse(cmd->payload);
    if (obj_root == NULL) {
        oc_cmdresp(cmd, cmdret);
    }
    obj_cmdname = cJSON_GetObjectItem(obj_root, "command_name");
    if (obj_cmdname == NULL) {
        cJSON_Delete(obj_root);
    }
    if (strcmp(cJSON_GetStringValue(obj_cmdname), "Control_light") == 0) 
    {
        deal_light_cmd(cmd, obj_root);
    } 

    return;
}

static int CloudMainTaskEntry(void)  //云端任务入口函数
{
    app_msg_t *app_msg;  //定义变量
    uint32_t ret;

    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);  //wifi连接
    dtls_al_init();  //初始化函数
    mqtt_al_init();
    oc_mqtt_init();

    g_app_cb.app_msg = osMessageQueueNew(MSGQUEUE_COUNT, MSGQUEUE_SIZE, NULL);  //创建消息队列
    if (g_app_cb.app_msg == NULL) {
        printf("Create receive msg queue failed");  //返回错误信息
    }
    oc_mqtt_profile_connect_t connect_para;  //初始化结构体connect_para，并设置其各个字段的值。其中包括设备的相关信息、服务器的地址和端口、连接的超时时间等。
    (void)memset_s(&connect_para, sizeof(connect_para), 0, sizeof(connect_para));

    connect_para.boostrap = 0;
    connect_para.device_id = CONFIG_APP_DEVICEID;
    connect_para.device_passwd = CONFIG_APP_DEVICEPWD;
    connect_para.server_addr = CONFIG_APP_SERVERIP;
    connect_para.server_port = CONFIG_APP_SERVERPORT;
    connect_para.life_time = CONFIG_APP_LIFETIME;
    connect_para.rcvfunc = msg_rcv_callback;
    connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE;
    ret = oc_mqtt_profile_connect(&connect_para);  ////尝试与云端建立连接，并将返回值赋值给 ret
    if ((ret == (int)en_oc_mqtt_err_ok)) {
        g_app_cb.connected = 1;
        printf("oc_mqtt_profile_connect succed!\r\n");
    } else {
        printf("oc_mqtt_profile_connect faild!\r\n");
    }

    while (1) {  //处理消息队列
        app_msg = NULL;
        (void)osMessageQueueGet(g_app_cb.app_msg, (void **)&app_msg, NULL, 0xFFFFFFFF);
        if (app_msg != NULL) {
            switch (app_msg->msg_type) {
                case en_msg_cmd:
                    deal_cmd_msg(&app_msg->msg.cmd);
                    break;
                case en_msg_report:
                    deal_report_msg(&app_msg->msg.report);
                    break;
                default:
                    break;
            }
            free(app_msg);
        }
    }
    return 0;
}




static int SensorTaskEntry(void)  //传感器任务入口
{
    app_msg_t *app_msg;
    while (1) {
        app_msg = malloc(sizeof(app_msg_t));  //分配内存
        printf("SENSOR:angle:%d\n", oc_1.angle);
        if (app_msg != NULL) {
            app_msg->msg_type = en_msg_report;  //将 app_msg 的字段设置为相应的传感器数据
            app_msg->msg.report.angle = (int)oc_1.angle;
            if (osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT != 0)) {
                free(app_msg);  //释放内存
            }
        }
        sleep(TASK_DELAY);
    }
    return 0;
}

static void IotMainTaskEntry(void)  //Iot任务入口函数
{
    osThreadAttr_t attr;  //配置任务属性，字段包括任务的名称、属性、回调函数和堆栈大小等

    attr.name = "CloudMainTaskEntry";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = CLOUD_TASK_STACK_SIZE;
    attr.priority = CLOUD_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)CloudMainTaskEntry, NULL, &attr) == NULL) {
        printf("Failed to create CloudMainTaskEntry!\n");  //创建新任务，若失败返回错误信息
    }
    attr.stack_size = SENSOR_TASK_STACK_SIZE;  //回调函数
    attr.priority = SENSOR_TASK_PRIO;
    attr.name = "SensorTaskEntry";
    if (osThreadNew((osThreadFunc_t)SensorTaskEntry, NULL, &attr) == NULL) {
        printf("Failed to create SensorTaskEntry!\n");
    }
}

APP_FEATURE_INIT(IotMainTaskEntry);  //注册为初始化函数