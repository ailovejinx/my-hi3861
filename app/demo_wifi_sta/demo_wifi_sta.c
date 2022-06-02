#include <stdio.h>

#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"

#include <unistd.h>
#include "hi_wifi_api.h"
//#include "wifi_sta.h"
#include "lwip/ip_addr.h"
#include "lwip/netifapi.h"

#include "lwip/sockets.h"

#define APP_INIT_VAP_NUM    2
#define APP_INIT_USR_NUM    2

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
    rc = memcpy_s(assoc_req.ssid, HI_WIFI_MAX_SSID_LEN + 1, "1234567890", 10); /* 10:ssid length */
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


/* 设置 按键中断响应 */
void wifi_sta_task(void *arg)
{
    arg = arg;
    
    hi_wifi_start_sta();

    while(1)
    {
        usleep(30000);
    }
    

}



void wifi_sta_entry(void)
{
    osThreadAttr_t attr;
    
    attr.name = "wifi_sta_demo";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 2048;
    attr.priority = 26;

    if (osThreadNew((osThreadFunc_t)wifi_sta_task, NULL, &attr) == NULL) {
        printf("[wifi_sta_demo] Falied to create wifi_sta_demo!\n");
    }
    
}


SYS_RUN(wifi_sta_entry);
