# 基于舵机的非侵入式开关开发
基于SG90舵机控制的非侵入式家居灯光开关，通过控制舵机旋转角度实现开关控制。


## 软件设计


### 连接平台
在连接平台前需要设置获取CONFIG_APP_DEVICEID、CONFIG_APP_DEVICEPWD、CONFIG_APP_SERVERIP、CONFIG_APP_SERVERPORT，通过oc_mqtt_profile_connect()函数连接平台。
```c
    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
    dtls_al_init();
    mqtt_al_init();
    oc_mqtt_init();
    
    g_app_cb.app_msg = queue_create("queue_rcvmsg",10,1);
    if(g_app_cb.app_msg == NULL){
        printf("Create receive msg queue failed");
        
    }
    oc_mqtt_profile_connect_t  connect_para;
    (void) memset( &connect_para, 0, sizeof(connect_para));

    connect_para.boostrap =      0;
    connect_para.device_id =     CONFIG_APP_DEVICEID;
    connect_para.device_passwd = CONFIG_APP_DEVICEPWD;
    connect_para.server_addr =   CONFIG_APP_SERVERIP;
    connect_para.server_port =   CONFIG_APP_SERVERPORT;
    connect_para.life_time =     CONFIG_APP_LIFETIME;
    connect_para.rcvfunc =       msg_rcv_callback;
    connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE;
    //连接平台
    ret = oc_mqtt_profile_connect(&connect_para);
    if((ret == (int)en_oc_mqtt_err_ok)){
        g_app_cb.connected = 1;
        printf("oc_mqtt_profile_connect succed!\r\n");
    }
    else
    {
        printf("oc_mqtt_profile_connect faild!\r\n");
    }
```

### 推送数据

当需要上传数据时，需要先拼装数据，让后通过oc_mqtt_profile_propertyreport上报数据。代码示例如下： 

```c

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

```




### 命令接收

华为IoT平台支持下发命令，命令是用户自定义的。接收到命令后会将命令数据发送到队列中，task_main_entry函数中读取队列数据并调用deal_cmd_msg函数进行处理，代码示例如下： 

```c

static int msg_rcv_callback(oc_mqtt_profile_msgrcv_t *msg)
{
    int ret = 0;
    char *buf;
    int buf_len;
    app_msg_t *app_msg;

    if ((msg == NULL) || (msg->request_id == NULL) || (msg->type != EN_OC_MQTT_PROFILE_MSG_TYPE_DOWN_COMMANDS)) {
        return ret;
    }

    buf_len = sizeof(app_msg_t) + strlen(msg->request_id) + 1 + msg->msg_len + 1;
    buf = malloc(buf_len);
    if (buf == NULL) {
        return ret;
    }
    app_msg = (app_msg_t *)buf;
    buf += sizeof(app_msg_t);

    app_msg->msg_type = en_msg_cmd;
    app_msg->msg.cmd.request_id = buf;
    buf_len = strlen(msg->request_id);
    buf += buf_len + 1;
    memcpy_s(app_msg->msg.cmd.request_id, buf_len, msg->request_id, buf_len);
    app_msg->msg.cmd.request_id[buf_len] = '\0';

    buf_len = msg->msg_len;
    app_msg->msg.cmd.payload = buf;
    memcpy_s(app_msg->msg.cmd.payload, buf_len, msg->msg, buf_len);
    app_msg->msg.cmd.payload[buf_len] = '\0';

    ret = osMessageQueuePut(g_app_cb.app_msg, &app_msg, 0U, CONFIG_QUEUE_TIMEOUT);
    if (ret != 0) {
        free(app_msg);
    }

    return ret;
}

static void oc_cmdresp(cmd_t *cmd, int cmdret)
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
static void deal_light_cmd(cmd_t *cmd, cJSON *obj_root)
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
        engine_145();
        printf("Light On!\r\n");
    } else {
        g_app_cb.led = 0;
        engine_45();
        printf("Light Off!\r\n");
    }
    cmdret = 0;
    oc_cmdresp(cmd, cmdret);

    cJSON_Delete(obj_root);
    return;
}
```


## 编译调试


### 登录

设备接入华为云平台
### 创建产品

在设备接入页面可看到总览界面，展示了华为云平台接入的协议与域名信息，根据需要选取MQTT通讯必要的信息备用，如下图所示。

接入协议（端口号）：MQTT 1883

服务ID：`ServoControl`(必须一致)

服务类型：`ServoControl`(可自定义)

创建产品

在“ServoControl”的下拉菜单下点击“添加属性”填写相关信息“Angle”，
如下图所示。

在“ServoControl”的下拉菜单下点击“添加命令”填写相关信息。

命令名称：`ServoControl`

参数名称：`Angle`

数据类型：`int`

长度：`3`

命令名称：`ServoControl`

参数名称：`Light`

数据类型：`enum`

长度：`3`

枚举值：`ON,OFF`


#### 注册设备

在侧边栏中单击“设备”，进入设备页面，单击右上角“注册设备”，勾选对应所属资源空间并选中刚刚创建的产品，注意设备认证类型选择“秘钥”，按要求填写秘钥，如下图所示。


### 修改代码中设备信息

### 修改BUILD.gn

修改applications/sample/wifi-iot/app/目录下的BUILD.gn，在features字段中添加demo_servo:servo_demolink.

```c
import("//build/lite/config/component/lite_component.gni")

lite_component("app") {
features = [ "demo_servo:servo_demolink", ]
}
```
### 编译烧录
点击DevEco Device Tool工具“Rebuild”按键，编译程序。

![image-20230103154607638](/doc/pic/image-20230103154607638.png)

点击DevEco Device Tool工具“Upload”按键，等待提示（出现Connecting，please reset device...），手动进行开发板复位（按下开发板RESET键），将程序烧录到开发板中。

![image-20230103154836005](/doc/pic/image-20230103154836005.png)

### 运行结果

示例代码编译烧录代码后，按下开发板的RESET按键，通过串口助手查看日志，会打印舵机状态信息，平台上的设备显示为在线状态，如下图所示。

在华为云平台设备详情页，单击“命令”，选择同步命令下发，选中创建的命令属性，单击“确定”，即可发送下发命令控制设备，如下图所示。
