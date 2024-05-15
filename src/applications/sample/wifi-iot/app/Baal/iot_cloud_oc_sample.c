// * Internal Code: 103.Rebulid-Gen1
// * Module Series: Solomon
// * Module Code: Baal
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
#include "E53_IS1.h"
#include "wifi_connect.h"

#include "iot_adc.h"
#include "iot_errno.h"
#include "iot_gpio_ex.h"
#include <cJSON.h>

#define CONFIG_WIFI_SSID "HUAWEI Mate 60 Pro" // 修改为自己的WiFi 热点账号

#define CONFIG_WIFI_PWD "haozihan2004" // 修改为自己的WiFi 热点密码

#define CONFIG_APP_SERVERIP "117.78.5.125"

#define CONFIG_APP_SERVERPORT "1883"

#define CONFIG_APP_DEVICEID "6613b0b82ccc1a58388044e4_Hi3861V100" // 替换为注册设备后生成的deviceid

#define CONFIG_APP_DEVICEPWD "haozihan2004" // 替换为注册设备后生成的密钥

#define CONFIG_APP_LIFETIME 60 // seconds

#define CONFIG_QUEUE_TIMEOUT (5 * 1000)

#define MSGQUEUE_COUNT 16
#define MSGQUEUE_SIZE 10
#define CLOUD_TASK_STACK_SIZE (1024 * 30)
#define CLOUD_TASK_PRIO 27
#define BEEP_DELAY 5

#define SENSOR_TASK_STACK_SIZE (1024 * 2)
#define SENSOR_TASK_PRIO 25
#define TASK_DELAY_3S 3

#define ADC_TASK_STACK_SIZE 1024*8
#define TASK_PRIORITY 24
#define IOT_ADC_CHANNEL 5
#define IOT_SUCCESS 0

const float VCC = 5.0;            // ! ACS712 模块驱动电压为5V
const float Sensitivity = 0.185;   // ! 5A模块灵敏度为185mA/V，20A模块灵敏度为100mA/V，30A模块灵敏度为66mA/V
#define IOT_GPIO 11        // ! GPIO11对应的是ADC5
#define ADC_TASK_DELAY_1S 1000000 // ! 1s

osMessageQueueId_t app_msg;

int Status = 0;     // ! 0:缺省状态 1:已移交数据收集 2:已移交数据上报

typedef enum {
    en_msg_cmd = 0,
    en_msg_report,
} en_msg_type_t;

typedef struct {
    char *request_id;
    char *payload;
} cmd_t;

typedef struct {
    char voltage[50];
    char current[50];
} report_n;

typedef struct {
    en_msg_type_t msg_type;
    union {
        cmd_t cmd;
        report_n report;
    } msg;
} app_msg_t;

typedef struct {
    int connected;
} app_cb_t;

static report_n report_global;

static app_cb_t g_app_cb; 

static void deal_report_msg()
{
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t voltage;
    oc_mqtt_profile_kv_t current;

    if (g_app_cb.connected != 1) {
        return;
    }
    service.event_time = NULL;
    service.service_id = "ADC";
    service.service_property = &voltage;
    service.nxt = NULL;

    voltage.key = "voltage";
    voltage.value = report_global.voltage;
    voltage.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    voltage.nxt = &current;

    current.key = "current";
    current.value = report_global.current;
    current.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    current.nxt = NULL;

    printf("Ready to report\n");
    oc_mqtt_profile_propertyreport(NULL, &service);
    printf("Reported voltage: %s, current: %s\n", voltage.value, current.value);
    memset(report_global.voltage, 0x00, sizeof(report_global.voltage));
    memset(report_global.current, 0x00, sizeof(report_global.current));
    printf("%d, %d\n", strlen(report_global.voltage), strlen(report_global.current));
    return;
}
static int CloudMainTaskEntry(void)
{
    uint32_t ret;
    //app_msg_t *app_msg_2;
    //app_msg_2 = (app_msg_t *)malloc(sizeof(app_msg_t));
    g_app_cb.connected = 0;
    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
    dtls_al_init();
    mqtt_al_init();
    oc_mqtt_init();

    app_msg = osMessageQueueNew(MSGQUEUE_COUNT, MSGQUEUE_SIZE, NULL);
    if (app_msg == NULL) {
        printf("Create receive msg queue failed");
    }
    oc_mqtt_profile_connect_t connect_para;
    (void)memset_s(&connect_para, sizeof(connect_para), 0, sizeof(connect_para));
    connect_para.boostrap = 0;
    connect_para.device_id = CONFIG_APP_DEVICEID;
    connect_para.device_passwd = CONFIG_APP_DEVICEPWD;
    connect_para.server_addr = CONFIG_APP_SERVERIP;
    connect_para.server_port = CONFIG_APP_SERVERPORT;
    connect_para.life_time = CONFIG_APP_LIFETIME;
    connect_para.rcvfunc = NULL;
    connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE;
    ret = oc_mqtt_profile_connect(&connect_para);
    if ((ret == (int)en_oc_mqtt_err_ok)) {
        g_app_cb.connected = 1;
        printf("oc_mqtt_profile_connect succed!\r\n");
    } else {
        printf("oc_mqtt_profile_connect faild!\r\n");
    }
    Status = 1;
    while (1) {
        // app_msg_2 = (app_msg_t *)malloc(sizeof(app_msg_t));
        // if (app_msg_2 == NULL) {
        //     printf("Memory allocation failed\n");
        //     continue;  // Skip this iteration
        // }
        //if(osMessageQueueGet(app_msg, &app_msg_2, NULL, 0)){
            if (Status == 2) {
                printf("CloudMainTaskEntry\n");
                deal_report_msg();
                //free(app_msg_2);
                Status=1;
            }
        //}
        sleep(1);
    }
    return 0;
}
static float GetVoltage(void) { // ! 获取电压值，返回值为电压值
    unsigned int ret;
    unsigned short data;
    ret = IoTAdcRead(IOT_ADC_CHANNEL, &data, IOT_ADC_EQU_MODEL_8, IOT_ADC_CUR_BAIS_DEFAULT, 0xff);  // ! 8位精度
    if (ret != IOT_SUCCESS) {
        printf("ADC Read Fail\n");
        printf("Raw data is: %d\n", data);
    }
    return (float)data * 1.8 * 4.0 / 4096.0;
}
static float FakeData(void) { // ! 生成随机数据，返回值为电压值
    return (float)(rand() % 100) / 10.0;
}
static int SensorTaskEntry(void) {
    int ret;
    float voltage;
    float current;

    //app_msg_t *app_msg_1;
    while (1) {
        char Buffer_Voltage[50];
        char Buffer_Current[50];
        if(g_app_cb.connected == 1 && Status == 1){
            //printf("SensorTaskEntry\n");
            //app_msg_1 = (app_msg_t *)malloc(sizeof(app_msg_t));
            //voltage = (int)(GetVoltage()*1000) / 1000.0;
            //current = (voltage - (VCC * 0.5)) / Sensitivity * 1000;
            /*----------------------------------------------------------*/
            // ! 以下为模拟数据
            voltage = FakeData();
            current = FakeData();
            /*----------------------------------------------------------*/
            if (app_msg != NULL) {
                //app_msg_1->msg_type = en_msg_report;
                sprintf(Buffer_Voltage, "%.3f", voltage);
                printf("Voltage: %s\n", Buffer_Voltage);
                strcpy(report_global.voltage, Buffer_Voltage);
                sprintf(Buffer_Current, "%.3f", current);
                printf("Current: %s\n", Buffer_Current);
                strcpy(report_global.current, Buffer_Current);
                memset(Buffer_Voltage, 0x00, sizeof(Buffer_Voltage));
                memset(Buffer_Current, 0x00, sizeof(Buffer_Current));
                //if (osMessageQueuePut(app_msg, &app_msg_1, 0U, 0U) != 0) {
                //    free(app_msg_1);
                //}
            }
        }
        Status = 2;
        sleep(1);
    }
    return 0;
}
static void IotMainTaskEntry(void)
{
    osThreadAttr_t attr;
    attr.name = "CloudMainTaskEntry";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = CLOUD_TASK_STACK_SIZE;
    attr.priority = CLOUD_TASK_PRIO;

    if (osThreadNew((osThreadFunc_t)CloudMainTaskEntry, NULL, &attr) == NULL) {
        printf("Failed to create CloudMainTaskEntry!\n");
    }

    printf("CloudMainTaskEntry\n");
    attr.stack_size = SENSOR_TASK_STACK_SIZE;
    attr.priority = SENSOR_TASK_PRIO;
    attr.name = "SensorTaskEntry";
    if (osThreadNew((osThreadFunc_t)SensorTaskEntry, NULL, &attr) == NULL) {
        printf("Failed to create SensorTaskEntry!\n");
    }
    printf("SensorTaskEntry\n");
}

APP_FEATURE_INIT(IotMainTaskEntry);