/*
 * Copyright (c) 2020 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#define CONFIG_WIFI_SSID "BearPi" // 修改为自己的WiFi 热点账号

#define CONFIG_WIFI_PWD "123456789" // 修改为自己的WiFi 热点密码

#define CONFIG_APP_SERVERIP "117.78.5.125"//定义设备服务IP地址

#define CONFIG_APP_SERVERPORT "1883"//定义设备IP端口

#define CONFIG_APP_DEVICEID "65d31fb02ccc1a583878a9dd_2024223" // 替换为注册设备后生成的deviceid

#define CONFIG_APP_DEVICEPWD "123456789" // 替换为注册设备后生成的密钥

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
    int lum;
    int temp;
    int hum;
    int phvalue;
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
    int led;
    int motor;
    int phvalue;
} app_cb_t;
static app_cb_t g_app_cb;

static void deal_report_msg(report_t *report)  //上报消息
{
    oc_mqtt_profile_service_t service;  //创建结构体
    oc_mqtt_profile_kv_t temperature;  //温度属性
    oc_mqtt_profile_kv_t humidity;  //湿度属性
    oc_mqtt_profile_kv_t luminance;  //光照属性
    oc_mqtt_profile_kv_t phValue;  //ph属性
    oc_mqtt_profile_kv_t led;  //LED状态属性
    oc_mqtt_profile_kv_t motor;  //电机状态属性
    oc_mqtt_profile_kv_t fast_command;  //转速
//若连接失败返回
    if (g_app_cb.connected != 1) {
        return;
    }

    service.event_time = NULL;  //设置时间为空
    service.service_id = "Agriculture";  //设置服务ID
    service.service_property = &temperature;  //设置属性为温度
    service.nxt = NULL;  //设置下一个服务为空

    temperature.key = "Temperature";  
    temperature.value = &report->temp;
    temperature.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    temperature.nxt = &humidity;

    humidity.key = "Humidity";
    humidity.value = &report->hum;
    humidity.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    humidity.nxt = &luminance;

    luminance.key = "Luminance";
    luminance.value = &report->lum;
    luminance.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    luminance.nxt = &phValue;

    phValue.key = "PhValue";
    phValue.value = &report->phvalue;
    phValue.type = EN_OC_MQTT_PROFILE_VALUE_INT;
    phValue.nxt = &led;

    led.key = "LightStatus";
    led.value = g_app_cb.led ? "ON" : "OFF";
    led.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    led.nxt = &motor;

    motor.key = "MotorStatus";
    motor.value = g_app_cb.motor ? "ON" : "OFF";
    motor.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    motor.nxt = NULL;
//xinxeng
    /*fast_command.key = "LightStatus";
    fast_command.value = g_app_cb.fast ? "ON" : "OFF";
    fast_command.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    fast_command.nxt = NULL;*/

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
    if (strcmp(cJSON_GetStringValue(obj_para), "ON") == 0) {
        g_app_cb.led = 1;
        LightStatusSet(ON);
        printf("Light On!\r\n");
    } else {
        g_app_cb.led = 0;
        LightStatusSet(OFF);
        printf("Light Off!\r\n");
    }
    cmdret = 0;
    oc_cmdresp(cmd, cmdret);

    cJSON_Delete(obj_root);
    return;
}

static void deal_motor_cmd(cmd_t *cmd, cJSON *obj_root)  //电机处理函数
{
    cJSON *obj_paras;
    cJSON *obj_para;
    int cmdret;

    obj_paras = cJSON_GetObjectItem(obj_root, "Paras");
    if (obj_paras == NULL) {
        cJSON_Delete(obj_root);
    }
    obj_para = cJSON_GetObjectItem(obj_paras, "Motor");
    if (obj_para == NULL) {
        cJSON_Delete(obj_root);
    }
    ///< operate the Motor here
    if (strcmp(cJSON_GetStringValue(obj_para), "ON") == 0) {
        g_app_cb.motor = 1;
        MotorStatusSet(ON);
        printf("Motor On!\r\n");
    } else {
        g_app_cb.motor = 0;
        MotorStatusSet(OFF);
        printf("Motor Off!\r\n");
    }
    cmdret = 0;
    oc_cmdresp(cmd, cmdret);

    cJSON_Delete(obj_root);
    return;
}

//新的更改
static void deal_phValue_cmd(cmd_t *cmd, cJSON *obj_root)  //ph处理函数
{
    cJSON *obj_paras;
    cJSON *obj_para;
    int cmdret;

    obj_paras = cJSON_GetObjectItem(obj_root, "Paras");
    if (obj_paras == NULL) {
        cJSON_Delete(obj_root);
    }
    obj_para = cJSON_GetObjectItem(obj_paras, "PhValue");//这里要更改下发参数
    if (obj_para == NULL) {
        cJSON_Delete(obj_root);
    }
    ///< operate the Motor here
    if (strcmp(cJSON_GetStringValue(obj_para), "ON") == 0) {
        g_app_cb.phvalue = 1;
        //MotorStatusSet(ON);
        //......
        printf("Ph On!\r\n");
    } else {
        g_app_cb.phvalue = 0;
        //MotorStatusSet(OFF);
        //......
        printf("Ph Off!\r\n");
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
    if (strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_light") == 0) {
    /*if (strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_fast") == 0) {
        deal_fast_cmd(cmd, obj_root);*/
    } else if (strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_Motor") == 0) {
        deal_motor_cmd(cmd, obj_root);
    } else if (strcmp(cJSON_GetStringValue(obj_cmdname), "Agriculture_Control_light") == 0) {
        deal_motor_cmd(cmd, obj_root);
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
    //int fast = 1;
    app_msg_t *app_msg;
    int ret;
    E53IA1Data data;

    ret = E53IA1Init();  //初始化传感器
    if (ret != 0) {
        printf("E53_IA1 Init failed!\r\n");  //错误则返回报错信息
        return;
    }
    while (1) {
        ret = E53IA1ReadData(&data);  //获取传感器数据
        if (ret != 0) {
            printf("E53_IA1 Read Data failed!\r\n");  //失败则返回错误信息
            return;
        }
        app_msg = malloc(sizeof(app_msg_t));  //分配内存
        printf("SENSOR:lum:%.2f temp:%.2f hum:%.2f\r\n", data.Lux, data.Temperature, data.Humidity);
        if (app_msg != NULL) {
            app_msg->msg_type = en_msg_report;  //将 app_msg 的字段设置为相应的传感器数据
            app_msg->msg.report.hum = (int)data.Humidity;
            app_msg->msg.report.lum = (int)data.Lux;
            app_msg->msg.report.temp = (int)data.Temperature;
            app_msg->msg.report.phvalue = (int)data.PhValue;
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