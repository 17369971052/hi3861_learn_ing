# Hi3861 通过mqtt协议连接华为云IoT平台

## 软件设计

### 连接平台
在连接平台前需要设置获取CONFIG_APP_DEVICEID、CONFIG_APP_DEVICEPWD、CONFIG_APP_SERVERIP、CONFIG_APP_SERVERPORT，通过oc_mqtt_profile_connect()函数连接平台。
```c
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
```

### 推送数据


```c
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
```

### 电压采集

```c
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
```

华为云IoT平台需配置设备、服务、属性、事件等信息，具体配置方法请参考[华为云IoT平台](https://console.huaweicloud.com/iot/)
### 接口信息
| 接口名称 | 接口描述 |数据类型|
| --- | --- | --- |
|current|电流值|string|
|voltage|电压值|string|