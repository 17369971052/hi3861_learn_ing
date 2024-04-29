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
//引入头文件
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "lwip/api_shell.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "wifi_device.h"
#include "wifi_error_code.h"
//定义函数
#define TASK_STACK_SIZE (1024 * 10)
#define TASK_PRIO 24
#define DEF_TIMEOUT 15
#define ONE_SECOND 1
#define DHCP_DELAY 100

static int WiFiInit(void);
static void WaitSacnResult(void);
static int WaitConnectResult(void);
static void OnWifiScanStateChangedHandler(int state, int size);
static void OnWifiConnectionChangedHandler(int state, WifiLinkedInfo *info);
static void OnHotspotStaJoinHandler(StationInfo *info);
static void OnHotspotStateChangedHandler(int state);
static void OnHotspotStaLeaveHandler(StationInfo *info);
//定义变量
static int g_staScanSuccess = 0;
static int g_connectSuccess = 0;
static int g_ssid_count = 0;
static struct netif *g_lwip_netif = NULL;
static WifiEvent g_wifiEventHandler = { 0 };
WifiErrorCode error;

#define SELECT_WLAN_PORT "wlan0"

#define SELECT_WIFI_SSID "BearPi"
#define SELECT_WIFI_PASSWORD "123456789"
#define SELECT_WIFI_SECURITYTYPE WIFI_SEC_TYPE_PSK
//定义函数连接至WIFI热点
static int WifiConnectAp(const char *ssid, const char *psk, WifiScanInfo *info, int i)
{
    //若SSID匹配成功
    if (strcmp(ssid, info[i].ssid) == 0) {
        int result;
        //打印热点信息
        printf("Select:%3d wireless, Waiting...\r\n", i + 1);
        //创建一个WifiDeviceConfig结构体用于保存要连接的热点的信息

        // 拷贝要连接的热点信息
        WifiDeviceConfig select_ap_config = { 0 };
        strcpy_s(select_ap_config.ssid, sizeof(select_ap_config.ssid), info[i].ssid);
        strcpy_s(select_ap_config.preSharedKey, sizeof(select_ap_config.preSharedKey), psk);
        select_ap_config.securityType = WIFI_SEC_TYPE_PSK;//设置安全类型为WIFI_SEC_TYPE_PSK

        //调用AddDeviceConfig函数添加设备配置
        if (AddDeviceConfig(&select_ap_config, &result) == WIFI_SUCCESS) {
            //如果连接成功且WaitConnectResult函数返回1则获取网络接口并返回0
            if (ConnectTo(result) == WIFI_SUCCESS && WaitConnectResult() == 1) {
                g_lwip_netif = netifapi_netif_find(SELECT_WLAN_PORT);
                return 0;
            }
        }
    }
    return -1;//若均失败则返回-1
}
//定义函数用于处理WIFI站点任务
static BOOL WifiStaTask(void)
{
    //定义整型变量以保存获取的WIFI热点数量
    unsigned int size = WIFI_SCAN_HOTSPOT_LIMIT;

    // 调用WiFiInit函数初始化WIFI
    if (WiFiInit() != WIFI_SUCCESS) {
        printf("WiFiInit failed, error = %d\r\n", error);//判断是否成功，返回错误信息
        return -1;//返回-1
    }
    // 分配空间，保存WiFi信息
    //用malloc函数分配内存以保存获取的WIFI热点的信息
    WifiScanInfo *info = malloc(sizeof(WifiScanInfo) * WIFI_SCAN_HOTSPOT_LIMIT);
    //若内存分配失败返回-1
    if (info == NULL) {
        return -1;
    }
    // 轮询查找WiFi列表，直到查找成功即g_staScanSuccess为1
    do {
        Scan();//调用scan函数扫描
        //调用WaitScanResult函数等待扫描结果
        WaitSacnResult();
        //调用GetScanInfoList函数等待扫描结果,并将结果保存在info中
        error = GetScanInfoList(info, &size);
    } while (g_staScanSuccess != 1);
    // 打印WiFi列表
    printf("********************\r\n");
    //使用for循环遍历所有扫描到的WIFI热点
    for (uint8_t i = 0; i < g_ssid_count; i++) {
        //打印每个WIFI热点的编号，SSID，RSSI
        printf("no:%03d, ssid:%-30s, rssi:%5d\r\n", i + 1, info[i].ssid, info[i].rssi);
    }
    printf("********************\r\n");
    // 循环以尝试连接到每个WiFi热点
    for (uint8_t i = 0; i < g_ssid_count; i++) {
        //调用WifiConnectAp函数尝试连接到WIFI热点，若连接成功则打印信息并退出循环
        if (WifiConnectAp(SELECT_WIFI_SSID, SELECT_WIFI_PASSWORD, info, i) == WIFI_SUCCESS) {
            //返回连接成功的信息
            printf("WiFi connect succeed!\r\n");
            //退出循环
            break;
        }

        //若均失败则返回错误信息并持续循环
        if (i == g_ssid_count - 1) {
            printf("ERROR: No wifi as expected\r\n");
            while (1)
                osDelay(DHCP_DELAY);//延迟时间
        }
    }

    // 启动DHCP（网络协议，用于自动分配IP地址给网络中的设备）
    //若网络接口存在则启动DHCP
    if (g_lwip_netif) {
        dhcp_start(g_lwip_netif);
        printf("begain to dhcp\r\n");
    }
    // 循环以等待DHCP绑定
    for (;;) {
        //若DHCP绑定成功则打印DHCP状态和IP信息并退出循环
        if (dhcp_is_bound(g_lwip_netif) == ERR_OK) {
            printf("<-- DHCP state:OK -->\r\n");
            // 打印获取到的IP信息
            netifapi_netif_common(g_lwip_netif, dhcp_clients_info_show, NULL);
            break;//退出
        }
        osDelay(DHCP_DELAY);
    }

    // 执行其他操作
    for (;;) {
        osDelay(DHCP_DELAY);//延迟一段时间
    }
}
//定义WiFiInit函数以初始化WIFI
int WiFiInit(void)
{
    //设置WIFI扫描状态改变时的回调函数
    g_wifiEventHandler.OnWifiScanStateChanged = OnWifiScanStateChangedHandler;
    //设置WIFI连接状态改变时的回调函数
    g_wifiEventHandler.OnWifiConnectionChanged = OnWifiConnectionChangedHandler;
    //设置热点有新的设备连接时的回调函数
    g_wifiEventHandler.OnHotspotStaJoin = OnHotspotStaJoinHandler;
    //设置热点有设备离开时的回调函数
    g_wifiEventHandler.OnHotspotStaLeave = OnHotspotStaLeaveHandler;
    //设置热点状态改变时的回调函数
    g_wifiEventHandler.OnHotspotStateChanged = OnHotspotStateChangedHandler;
    //调用RegisterWifiEvent函数注册这些回调函数，如果注册失败则返回错误信息与-1
    error = RegisterWifiEvent(&g_wifiEventHandler);
    if (error != WIFI_SUCCESS) {
        printf("register wifi event fail!\r\n");
        return -1;
    }
    // 调用EnableWifi函数使能WIFI，如果使能失败则返回错误信息与-1
    if (EnableWifi() != WIFI_SUCCESS) {
        printf("EnableWifi failed, error = %d\r\n", error);
        return -1;
    }
    // 调用IsWifiActive函数判断WIFI是否激活，如果激活失败则返回错误信息与-1
    if (IsWifiActive() == 0) {
        printf("Wifi station is not actived.\r\n");
        return -1;
    }
    return 0;//若均成功则返回0
}

static void OnWifiScanStateChangedHandler(int state, int size)
{
    (void)state;
    if (size > 0) {
        g_ssid_count = size;
        g_staScanSuccess = 1;
    }
    return;
}

static void OnWifiConnectionChangedHandler(int state, WifiLinkedInfo *info)
{
    (void)info;

    if (state > 0) {
        g_connectSuccess = 1;
        printf("callback function for wifi connect\r\n");
    } else {
        printf("connect error,please check password\r\n");
    }
    return;
}

static void OnHotspotStaJoinHandler(StationInfo *info)
{
    (void)info;
    printf("STA join AP\n");
    return;
}

static void OnHotspotStaLeaveHandler(StationInfo *info)
{
    (void)info;
    printf("HotspotStaLeave:info is null.\n");
    return;
}

static void OnHotspotStateChangedHandler(int state)
{
    printf("HotspotStateChanged:state is %d.\n", state);
    return;
}
//定义函数以等待WIFI扫描结果
static void WaitSacnResult(void)
{
    //定义整型变量用于保存扫描超时的时间
    int scanTimeout = DEF_TIMEOUT;
    //使用while循环等待扫描结果，直到成功或超时
    while (scanTimeout > 0) {
        //调用sleep函数延迟一秒
        sleep(ONE_SECOND);
        //扫描超时时间并减1
        scanTimeout--;
        //若扫描成功即g_staScanSuccess为1则打印成功信息并推出循环
        if (g_staScanSuccess == 1) {
            //打印信息成功
            printf("WaitSacnResult:wait success[%d]s\n", (DEF_TIMEOUT - scanTimeout));
            //推出循环
            break;
        }
    }
    if (scanTimeout <= 0) {
        //若超时打印超时信息
        printf("WaitSacnResult:timeout!\n");
    }
}
//创建函数检查是否连接成功
static int WaitConnectResult(void)
{
    //定义变量记录连接超时时间
    int ConnectTimeout = DEF_TIMEOUT;
    //使用循环不断检测连接状态
    while (ConnectTimeout > 0) {
        //延时1秒
        sleep(1);
         //每次循环都将ConnectTimeout减1
        ConnectTimeout--;
        //如果g_connectSuccess为1，表示连接成功，打印等待成功的时间
        if (g_connectSuccess == 1) {
            printf("WaitConnectResult:wait success[%d]s\n", (DEF_TIMEOUT - ConnectTimeout));
            break;
        }
    }
    //如果ConnectTimeout小于等于0，表示已经超过了定义的超时时间，连接失败
    if (ConnectTimeout <= 0) {
        printf("WaitConnectResult:timeout!\n");//返回错误信息与0
        return 0;
    }
    //正常结束则返回1

    return 1;
}
//定义一个函数用于启动一个新的线程来处理WIFI站点任务
static void WifiClientSTA(void)
{
    osThreadAttr_t attr;//定义线程属性

    attr.name = "WifiStaTask";//设置线程名称
    attr.attr_bits = 0U;//设置线程属性标志，0表示没有特殊属性
    attr.cb_mem = NULL;//设置线程控制块内存，NULL表示使用默认的内存分配
    attr.cb_size = 0U;//设置线程控制块大小，0表示使用默认的大小
    attr.stack_mem = NULL;//设置线程堆栈内存，NULL表示使用默认的内存分配
    attr.stack_size = TASK_STACK_SIZE;//设置线程堆栈大小，TASK_STACK_SIZE是预定义的大小
    attr.priority = TASK_PRIO;//设置线程优先级，TASK_PRIO是预定义的优先级
//创建新线程以执行WifiStaTask函数，如果创建失败，则打印错误信息
    if (osThreadNew((osThreadFunc_t)WifiStaTask, NULL, &attr) == NULL) {
        printf("Failed to create WifiStaTask!\n");
    }
}
//使用app入口函数初始化WIFIclientSTA函数
APP_FEATURE_INIT(WifiClientSTA);
