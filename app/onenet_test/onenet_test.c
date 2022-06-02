#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "MQTTClient.h"
#include "onenet.h"
#include "ohos_init.h"
#include "cmsis_os2.h"

#include <unistd.h>
#include "hi_wifi_api.h"
//#include "wifi_sta.h"
#include "lwip/ip_addr.h"
#include "lwip/netifapi.h"

#include "lwip/sockets.h"

#include "iot_gpio.h" 
#include "hi_gpio.h"
#include "hi_io.h"
#include "hi_adc.h"
#include "hi_errno.h"

// oled相关的库
#include <hi_types_base.h>
#include <hi_i2c.h>
#include <hi_early_debug.h>
#include <hi_stdlib.h>
#include "oled_demo.h"
#include "oled_demo.c"
#include "oledfont.h"



#define ONENET_INFO_DEVID ""
#define ONENET_INFO_AUTH ""
#define ONENET_INFO_APIKEY ""
#define ONENET_INFO_PROID ""
#define ONENET_MASTER_APIKEY ""

#define LED_TASK_GPIO 10

extern int rand(void);

//------------WiFi Connect---------------//
#define APP_INIT_VAP_NUM    2
#define APP_INIT_USR_NUM    2

int wifi_ok_flg = 0;

static struct netif *g_lwip_netif = NULL;

/* clear netif's ip, gateway and netmask */
void hi_sta_reset_addr(struct netif *pst_lwip_netif)
{
    ip4_addr_t st_gw;
    ip4_addr_t st_ipaddr;
    ip4_addr_t st_netmask;

    if (pst_lwip_netif == NULL) {
        printf("hisi_reset_addr::Null param of netdev\r\n");
        return;
    }

    IP4_ADDR(&st_gw, 0, 0, 0, 0);
    IP4_ADDR(&st_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&st_netmask, 0, 0, 0, 0);

    netifapi_netif_set_addr(pst_lwip_netif, &st_ipaddr, &st_netmask, &st_gw);
}

void wifi_wpa_event_cb(const hi_wifi_event *hisi_event)
{
    if (hisi_event == NULL)
        return;

    switch (hisi_event->event) {
        case HI_WIFI_EVT_SCAN_DONE:
            printf("WiFi: Scan results available\n");
            break;
        case HI_WIFI_EVT_CONNECTED:
            printf("WiFi: Connected\n");
            netifapi_dhcp_start(g_lwip_netif);
            wifi_ok_flg = 1;
            break;
        case HI_WIFI_EVT_DISCONNECTED:
            printf("WiFi: Disconnected\n");
            netifapi_dhcp_stop(g_lwip_netif);
            hi_sta_reset_addr(g_lwip_netif);
            break;
        case HI_WIFI_EVT_WPS_TIMEOUT:
            printf("WiFi: wps is timeout\n");
            break;
        default:
            break;
    }
}

int hi_wifi_start_connect(void)
{
    int ret;
    errno_t rc;
    hi_wifi_assoc_request assoc_req = {0};

    /* copy SSID to assoc_req */
    rc = memcpy_s(assoc_req.ssid, HI_WIFI_MAX_SSID_LEN + 1, "1234567890", 10); /* 9:ssid length */
    if (rc != EOK) {
        return -1;
    }

    //热点加密方式
    assoc_req.auth = HI_WIFI_SECURITY_WPA2PSK;

    /* 热点密码 */
    memcpy(assoc_req.key, "11111111111", 11);

    ret = hi_wifi_sta_connect(&assoc_req);
    if (ret != HISI_OK) {
        return -1;
    }

    return 0;
}

int hi_wifi_start_sta(void)
{
    int ret;
    char ifname[WIFI_IFNAME_MAX_SIZE + 1] = {0};
    int len = sizeof(ifname);
    const unsigned char wifi_vap_res_num = APP_INIT_VAP_NUM;
    const unsigned char wifi_user_res_num = APP_INIT_USR_NUM;
    unsigned int  num = WIFI_SCAN_AP_LIMIT;

    //这里不需要重复进行WiFi init，因为系统启动后就自己会做WiFi init
#if 0
    printf("_______>>>>>>>>>> %s %d \r\n", __FILE__, __LINE__);
    ret = hi_wifi_init(wifi_vap_res_num, wifi_user_res_num);
    if (ret != HISI_OK) {
        return -1;
    }
#endif
    ret = hi_wifi_sta_start(ifname, &len);
    if (ret != HISI_OK) {
        return -1;
    }

    /* register call back function to receive wifi event, etc scan results event,
     * connected event, disconnected event.
     */
    ret = hi_wifi_register_event_callback(wifi_wpa_event_cb);
    if (ret != HISI_OK) {
        printf("register wifi event callback failed\n");
    }

    /* acquire netif for IP operation */
    g_lwip_netif = netifapi_netif_find(ifname);
    if (g_lwip_netif == NULL) {
        printf("%s: get netif failed\n", __FUNCTION__);
        return -1;
    }

    /* 开始扫描附件的WiFi热点 */
    ret = hi_wifi_sta_scan();
    if (ret != HISI_OK) {
        return -1;
    }

    sleep(5);   /* sleep 5s, waiting for scan result. */

    hi_wifi_ap_info *pst_results = malloc(sizeof(hi_wifi_ap_info) * WIFI_SCAN_AP_LIMIT);
    if (pst_results == NULL) {
        return -1;
    }

    //把扫描到的热点结果存储起来
    ret = hi_wifi_sta_scan_results(pst_results, &num);
    if (ret != HISI_OK) {
        free(pst_results);
        return -1;
    }

    //打印扫描到的所有热点
    for (unsigned int loop = 0; (loop < num) && (loop < WIFI_SCAN_AP_LIMIT); loop++) {
        printf("SSID: %s\n", pst_results[loop].ssid);
    }
    free(pst_results);

    /* 开始接入热点 */
    ret = hi_wifi_start_connect();
    if (ret != 0) {
        return -1;
    }
    return 0;
}


void wifi_sta_task(void *arg)
{

    //初始化oled
    hi_i2c_init(HI_I2C_IDX_0, 100000); /* baudrate: 100000 */
    led_init();

    OLED_ColorTurn(0);//0正常显示，1 反色显示
    OLED_DisplayTurn(0);//0正常显示 1 屏幕翻转显示
    OLED_Clear();

    OLED_ShowString(16, 24, "Wifi init...", 16);
    OLED_Refresh();
    arg = arg;
    
    //连接热点
    hi_wifi_start_sta();

    while(wifi_ok_flg == 0)
    {
        usleep(30000);
    }
    
    
    usleep(2000000);

    //开始进入MQTT测试
    onenet_test();
}



void wifi_sta_entry(void)
{
    osThreadAttr_t attr;
    
    attr.name = "wifi_sta_demo";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = 26;

    if (osThreadNew((osThreadFunc_t)wifi_sta_task, NULL, &attr) == NULL) {
        printf("[wifi_sta_demo] Falied to create wifi_sta_demo!\n");
    }
    
}

//-----ADC读取温度传感器的值----------------//
static hi_u16 AdcGpioTask(){

    hi_u16 value;

    
    hi_io_set_func(HI_GPIO_IDX_11, HI_IO_FUNC_GPIO_11_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_11, HI_GPIO_DIR_IN);

    if(hi_adc_read(HI_ADC_CHANNEL_5, &value, HI_ADC_EQU_MODEL_8, HI_ADC_CUR_BAIS_DEFAULT, 0) != HI_ERR_SUCCESS){
        printf("ADC read error!\n");
    }else{
        printf("ADC_VALUE = %d\n", (unsigned int)value);

    }
    return value;

}


//------------Onenet Connect---------------//

void onenet_cmd_rsp_cb(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size)
{
    // printf("recv data is %.*s\n", recv_size, recv_data);

    // 根据接收到的指令判断是否开启LED，100为关闭，102为开启
    if (strcmp(recv_data, "101") < 0){
        printf("off\n");
        hi_gpio_set_ouput_val(HI_GPIO_IDX_10, 0);
    }else if (strcmp(recv_data, "101") > 0){
        printf("on\n");
        hi_gpio_set_ouput_val(HI_GPIO_IDX_10, 1);
    }
    *resp_data = NULL;
    *resp_size = 0;
}

int onenet_test(void)
{

    OLED_ShowString(8, 24, "Onenet init...", 16);
    OLED_Refresh();

    hi_gpio_init();
	// 定义蜂鸣器GPIO
    int BUZ_GPIO = 12;
    IoTGpioInit(BUZ_GPIO);
    IoTGpioSetDir(BUZ_GPIO,IOT_GPIO_DIR_OUT);

    // 定义非板载LED的GPIO
    hi_io_set_func(HI_GPIO_IDX_10, HI_IO_FUNC_GPIO_10_GPIO);
    hi_gpio_set_dir(HI_GPIO_IDX_10, HI_GPIO_DIR_OUT);
    // hi_gpio_set_ouput_val(HI_GPIO_IDX_10, 1);
    




    // 初始化onenet设备
    device_info_init(ONENET_INFO_DEVID, ONENET_INFO_PROID, ONENET_INFO_AUTH, ONENET_INFO_APIKEY, ONENET_MASTER_APIKEY);
    onenet_mqtt_init();
    OLED_ShowString(8, 24, "    Ready!    ", 16);
    OLED_Refresh();
    usleep(1);
    

	while (1)
    {
        // 读取ADC的值
		hi_u16 value = AdcGpioTask();
        // 换算成温度
        float tem = 720.0 * value / 4096;

        // 把float类型转为char*
        char str[10];
        sprintf(str, "%.6lf", tem);
        

        
        OLED_ShowString(8, 24, "Tem: ", 16);
        OLED_ShowString(48, 24, str, 16);

        OLED_Refresh();
        
        
		// 把温度值上传到Onenet
        int res = onenet_mqtt_upload_digit("temperature", tem);
        // 根据上传是否成功返回结果
        if (res < 0)
        {
            printf(res);
            printf("upload has an error, stop uploading");
            //break;
        }
        else
        {
            printf("buffer : {\"temperature\":%.2f} \r\n", tem);
        }

        // 判断温度
        if (tem >= 32)
        {
            IoTGpioSetOutputVal(BUZ_GPIO, 1);
        }else
        {
            IoTGpioSetOutputVal(BUZ_GPIO, 0);
        }

        // 判断Onenet下达命令
        onenet_set_cmd_rsp_cb(onenet_cmd_rsp_cb);

        sleep(0.5);
    }
	return 0;
}
SYS_RUN(wifi_sta_entry);