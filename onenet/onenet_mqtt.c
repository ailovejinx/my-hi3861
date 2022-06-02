/*
 * File      : onenet_mqtt.c
 * COPYRIGHT (C) 2020-10, Lian Zhian
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-10-24     lianzhian     first version
 */
#include <stdio.h>
#include "MQTTClient.h"
#include "onenet.h"

/*
#define ONENET_INFO_DEVID "597952816"
#define ONENET_INFO_AUTH "202005160951"
#define ONENET_INFO_APIKEY "zgQdlB5y3Bi9pNd2bUYmS8TJHIY="
#define ONENET_INFO_PROID "345377"
#define ONENET_MASTER_APIKEY "gwaK2wJT5wgnSbJYz67CVRGvwkI="
*/
#define ONENET_INFO_DEVID "xxxxxx"
#define ONENET_INFO_AUTH "xxxxxx"
#define ONENET_INFO_APIKEY "xxxxxx="
#define ONENET_INFO_PROID "xxxxxx"
#define ONENET_MASTER_APIKEY "xxxxxx"




#define  ONENET_TOPIC_DP    "$dp"

static char init_ok = FALSE;
static MQTTClient mq_client;
struct rt_onenet_info onenet_info;

struct onenet_device
{
    struct rt_onenet_info *onenet_info;

    void(*cmd_rsp_cb)(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size);

} onenet_mqtt;

 void mqtt_callback(MessageData *msg_data)
{
    size_t res_len = 0;
    uint8_t *response_buf = NULL;
    char topicname[45] = { "$crsp/" };

    LOS_ASSERT(msg_data);

    //LOG_D("topic %.*s receive a message", msg_data->topicName->lenstring.len, msg_data->topicName->lenstring.data);

    //LOG_D("message length is %d", msg_data->message->payloadlen);

    if (onenet_mqtt.cmd_rsp_cb != NULL)
    {
        onenet_mqtt.cmd_rsp_cb((uint8_t *) msg_data->message->payload, msg_data->message->payloadlen, &response_buf,
                &res_len);

        if (response_buf != NULL || res_len != 0)
        {
            strncat(topicname, &(msg_data->topicName->lenstring.data[6]), msg_data->topicName->lenstring.len - 6);

            onenet_mqtt_publish(topicname, response_buf, strlen((const char *)response_buf));

            ONENET_FREE(response_buf);

        }

    }

}

 unsigned char *onenet_mqtt_buf;
 unsigned char *onenet_mqtt_readbuf;
 int buf_size;

Network n;
MQTTPacket_connectData data = MQTTPacket_connectData_initializer;  

static int onenet_mqtt_entry(void)
{
    int rc = 0;
    
	NetworkInit(&n);
	NetworkConnect(&n, ONENET_SERVER_IP, ONENET_SERVER_PORT);

    buf_size  = 2048;
    onenet_mqtt_buf = (unsigned char *) malloc(buf_size);
    onenet_mqtt_readbuf = (unsigned char *) malloc(buf_size);
    if (!(onenet_mqtt_buf && onenet_mqtt_readbuf))
    {
        printf("No memory for MQTT client buffer!");
        return -2;
    }

	MQTTClientInit(&mq_client, &n, 1000, onenet_mqtt_buf, buf_size, onenet_mqtt_readbuf, buf_size);
	
    MQTTStartTask(&mq_client);


    data.keepAliveInterval = 30;
    data.cleansession = 1;
	data.clientID.cstring = onenet_info.device_id;
	data.username.cstring = onenet_info.pro_id;
	data.password.cstring = onenet_info.auth_info;

	data.keepAliveInterval = 10;
	data.cleansession = 1;
	
    mq_client.defaultMessageHandler = mqtt_callback;

	rc = MQTTConnect(&mq_client, &data);


    return rc;
    
}

static int onenet_get_info(void)
{
    char dev_id[ONENET_INFO_DEVID_LEN] = { 0 };
    char api_key[ONENET_INFO_APIKEY_LEN] = { 0 };
    char auth_info[ONENET_INFO_AUTH_LEN] = { 0 };

#ifdef ONENET_USING_AUTO_REGISTER
    char name[ONENET_INFO_NAME_LEN] = { 0 };
    
    if (!onenet_port_is_registed())
    {
        if (onenet_port_get_register_info(name, auth_info) < 0)
        {
            //LOG_E("onenet get register info fail!");
            return -1;
        }

        if (onenet_http_register_device(name, auth_info) < 0)
        {
            //LOG_E("onenet register device fail! name is %s,auth info is %s", name, auth_info);
            return -1;
        }
    }

    if (onenet_port_get_device_info(dev_id, api_key, auth_info))
    {
        //LOG_E("onenet get device id fail,dev_id is %s,api_key is %s,auth_info is %s", dev_id, api_key, auth_info);
        return -1;
    }

#else
    strncpy(dev_id, ONENET_INFO_DEVID, strlen(ONENET_INFO_DEVID));
    strncpy(api_key, ONENET_INFO_APIKEY, strlen(ONENET_INFO_APIKEY));
    strncpy(auth_info, ONENET_INFO_AUTH, strlen(ONENET_INFO_AUTH));
#endif

    if (strlen(api_key) < 15)
    {
        strncpy(api_key, ONENET_MASTER_APIKEY, strlen(ONENET_MASTER_APIKEY));
    }

    //使用默认的设备信息
    if(onenet_info.user_device_id_flg == 0)
    {
        strncpy(onenet_info.device_id, dev_id, strlen(dev_id));
        strncpy(onenet_info.pro_id, ONENET_INFO_PROID, strlen(ONENET_INFO_PROID));
        strncpy(onenet_info.auth_info, auth_info, strlen(auth_info));
        strncpy(onenet_info.api_key, api_key, strlen(api_key));
    }

    strncpy(onenet_info.server_uri, ONENET_SERVER_URL, strlen(ONENET_SERVER_URL));
    return 0;
}

void device_info_init(char *device_id, char * pro_id, char *auth_info, char *api_key, char *master_api_key)
{
    onenet_info.user_device_id_flg = 1;
    strncpy(onenet_info.device_id,  device_id, strlen(device_id));
    strncpy(onenet_info.pro_id,     pro_id, strlen(pro_id));
    strncpy(onenet_info.auth_info,  auth_info, strlen(auth_info));

    if (strlen(api_key) < 15)
    {
        strncpy(api_key, master_api_key, strlen(master_api_key));
    }else{
        strncpy(onenet_info.api_key,    api_key, strlen(api_key));
    }
}

/**
 * onenet mqtt client init.
 *
 * @param   NULL
 *
 * @return  0 : init success
 *         -1 : get device info fail
 *         -2 : onenet mqtt client init fail
 */
int onenet_mqtt_init(void)
{
    int result = 0;

    if (init_ok)
    {
        //LOG_D("onenet mqtt already init!");
        return 0;
    }

    if (onenet_get_info() < 0)
    {
        result = -1;
        goto __exit;
    }

    onenet_mqtt.onenet_info = &onenet_info;
    onenet_mqtt.cmd_rsp_cb = NULL;

    if (onenet_mqtt_entry() < 0)
    {
        result = -2;
        goto __exit;
    }

__exit:
    if (!result)
    {
        //LOG_I("RT-Thread OneNET package(V%s) initialize success.", ONENET_SW_VERSION);
        init_ok = 0;
    }
    else
    {
        //LOG_E("RT-Thread OneNET package(V%s) initialize failed(%d).", ONENET_SW_VERSION, result);
    }
    return result;
}

/**
 * mqtt publish msg to topic
 *
 * @param   topic   target topic
 * @param   msg     message to be sent
 * @param   len     message length
 *
 * @return  0 : publish success
 *         -1 : publish fail
 */
int onenet_mqtt_publish(const char *topic, const uint8_t *msg, size_t len)
{
    MQTTMessage message;

    LOS_ASSERT(topic);
    LOS_ASSERT(msg);

    message.qos = QOS1;
    message.retained = 0;
    message.payload = (void *) msg;
    message.payloadlen = len;

    if (MQTTPublish(&mq_client, topic, &message) < 0)
    {
        return -1;
    }

    return 0;
}

static int onenet_mqtt_get_digit_data(const char *ds_name, const double digit, char **out_buff, size_t *length)
{
    int result = 0;
    cJSON *root = NULL;
    char *msg_str = NULL;

    LOS_ASSERT(ds_name);
    LOS_ASSERT(out_buff);
    LOS_ASSERT(length);

    root = cJSON_CreateObject();
    if (!root)
    {
        //LOG_E("MQTT publish digit data failed! cJSON create object error return NULL!");
        return -2;
    }

    cJSON_AddNumberToObject(root, ds_name, digit);

    /* render a cJSON structure to buffer */
    msg_str = cJSON_PrintUnformatted(root);
    if (!msg_str)
    {
        //LOG_E("MQTT publish digit data failed! cJSON print unformatted error return NULL!");
        result = -2;
        goto __exit;
    }

    *out_buff = ONENET_MALLOC(strlen(msg_str) + 3);
    if (!(*out_buff))
    {
        //LOG_E("ONENET mqtt upload digit data failed! No memory for send buffer!");
        return -2;
    }

    strncpy(&(*out_buff)[3], msg_str, strlen(msg_str));
    *length = strlen(&(*out_buff)[3]);

    /* mqtt head and json length */
    (*out_buff)[0] = 0x03;
    (*out_buff)[1] = (*length & 0xff00) >> 8;
    (*out_buff)[2] = *length & 0xff;
    *length += 3;

__exit:
    if (root)
    {
        cJSON_Delete(root);
    }
    if (msg_str)
    {
        cJSON_free(msg_str);
    }

    return result;
}

/**
 * Upload digit data to OneNET cloud.
 *
 * @param   ds_name     datastream name
 * @param   digit       digit data
 *
 * @return  0 : upload digit data success
 *         -5 : no memory
 */
int onenet_mqtt_upload_digit(const char *ds_name, const double digit)
{
    char *send_buffer = NULL;
    int result = 0;
    size_t length = 0;

    LOS_ASSERT(ds_name);

    result = onenet_mqtt_get_digit_data(ds_name, digit, &send_buffer, &length);
    if (result < 0)
    {
        goto __exit;
    }

    result = onenet_mqtt_publish(ONENET_TOPIC_DP, (uint8_t *)send_buffer, length);
    if (result < 0)
    {
        //LOG_E("onenet publish failed (%d)!", result);
        goto __exit;
    }

__exit:
    if (send_buffer)
    {
        ONENET_FREE(send_buffer);
    }

    return result;
}

static int onenet_mqtt_get_string_data(const char *ds_name, const char *str, char **out_buff, size_t *length)
{
    int result = 0;
    cJSON *root = NULL;
    char *msg_str = NULL;

    LOS_ASSERT(ds_name);
    LOS_ASSERT(str);
    LOS_ASSERT(out_buff);
    LOS_ASSERT(length);

    root = cJSON_CreateObject();
    if (!root)
    {
        //LOG_E("MQTT publish string data failed! cJSON create object error return NULL!");
        return -2;
    }

    cJSON_AddStringToObject(root, ds_name, str);

    /* render a cJSON structure to buffer */
    msg_str = cJSON_PrintUnformatted(root);
    if (!msg_str)
    {
        //LOG_E("MQTT publish string data failed! cJSON print unformatted error return NULL!");
        result = -2;
        goto __exit;
    }

    *out_buff = ONENET_MALLOC(strlen(msg_str) + 3);
    if (!(*out_buff))
    {
        //LOG_E("ONENET mqtt upload string data failed! No memory for send buffer!");
        return -2;
    }

    strncpy(&(*out_buff)[3], msg_str, strlen(msg_str));
    *length = strlen(&(*out_buff)[3]);

    /* mqtt head and json length */
    (*out_buff)[0] = 0x03;
    (*out_buff)[1] = (*length & 0xff00) >> 8;
    (*out_buff)[2] = *length & 0xff;
    *length += 3;

__exit:
    if (root)
    {
        cJSON_Delete(root);
    }
    if (msg_str)
    {
        cJSON_free(msg_str);
    }

    return result;
}

/**
 * upload string data to OneNET cloud.
 *
 * @param   ds_name     datastream name
 * @param   str         string data
 *
 * @return  0 : upload digit data success
 *         -5 : no memory
 */
int onenet_mqtt_upload_string(const char *ds_name, const char *str)
{
    char *send_buffer = NULL;
    int result = 0;
    size_t length = 0;

    LOS_ASSERT(ds_name);
    LOS_ASSERT(str);

    result = onenet_mqtt_get_string_data(ds_name, str, &send_buffer, &length);
    if (result < 0)
    {
        goto __exit;
    }

    result = onenet_mqtt_publish(ONENET_TOPIC_DP, (uint8_t *)send_buffer, length);
    if (result < 0)
    {
        //LOG_E("onenet mqtt publish digit data failed!");
        goto __exit;
    }

__exit:
    if (send_buffer)
    {
        ONENET_FREE(send_buffer);
    }

    return result;
}

/**
 * set the command responses call back function
 *
 * @param   cmd_rsp_cb  command responses call back function
 *
 * @return  0 : set success
 *         -1 : function is null
 */
void onenet_set_cmd_rsp_cb(void (*cmd_rsp_cb)(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size))
{

    onenet_mqtt.cmd_rsp_cb = cmd_rsp_cb;

}

static int onenet_mqtt_get_bin_data(const char *str, const uint8_t *bin, int binlen, uint8_t **out_buff, size_t *length)
{
    int result = 0;
    cJSON *root = NULL;
    char *msg_str = NULL;

    LOS_ASSERT(str);
    LOS_ASSERT(bin);
    LOS_ASSERT(out_buff);
    LOS_ASSERT(length);

    root = cJSON_CreateObject();
    if (!root)
    {
        //LOG_E("MQTT online push failed! cJSON create object error return NULL!");
        return -2;
    }

    cJSON_AddStringToObject(root, "ds_id", str);

    /* render a cJSON structure to buffer */
    msg_str = cJSON_PrintUnformatted(root);
    if (!msg_str)
    {
        //LOG_E("Device online push failed! cJSON print unformatted error return NULL!");
        result = -2;
        goto __exit;
    }

    /* size = header(3) + json + binary length(4) + binary length +'\0' */
    *out_buff = (uint8_t *) ONENET_MALLOC(strlen(msg_str) + 3 + 4 + binlen + 1);

    strncpy((char *)&(*out_buff)[3], msg_str, strlen(msg_str));
    *length = strlen((const char *)&(*out_buff)[3]);

    /* mqtt head and cjson length */
    (*out_buff)[0] = 0x02;
    (*out_buff)[1] = (*length & 0xff00) >> 8;
    (*out_buff)[2] = *length & 0xff;
    *length += 3;

    /* binary data length */
    (*out_buff)[(*length)++] = (binlen & 0xff000000) >> 24;
    (*out_buff)[(*length)++] = (binlen & 0x00ff0000) >> 16;
    (*out_buff)[(*length)++] = (binlen & 0x0000ff00) >> 8;
    (*out_buff)[(*length)++] = (binlen & 0x000000ff);

    memcpy(&((*out_buff)[*length]), bin, binlen);
    *length = *length + binlen;

__exit:
    if (root)
    {
        cJSON_Delete(root);
    }
    if (msg_str)
    {
        cJSON_free(msg_str);
    }

    return result;
}

/**
 * upload binary data to onenet cloud by path
 *
 * @param   ds_name     datastream name
 * @param   bin         binary file
 * @param   len         binary file length
 *
 * @return  0 : upload success
 *         -1 : invalid argument or open file fail
 */
int onenet_mqtt_upload_bin(const char *ds_name, uint8_t *bin, size_t len)
{
    size_t length = 0;
    int result = 0;
    uint8_t *send_buffer = NULL;

    LOS_ASSERT(ds_name);
    LOS_ASSERT(bin);

    result = onenet_mqtt_get_bin_data(ds_name, bin, len, &send_buffer, &length);
    if (result < 0)
    {
        result = -1;
        goto __exit;
    }

    result = onenet_mqtt_publish(ONENET_TOPIC_DP, send_buffer, length);
    if (result < 0)
    {
        //LOG_E("onenet publish data failed(%d)!", result);
        result = -1;
        goto __exit;
    }

__exit:
    if (send_buffer)
    {
        ONENET_FREE(send_buffer);
    }

    return result;
}
