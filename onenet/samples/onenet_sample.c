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

#include <unistd.h>

#include "MQTTClient.h"

#include "onenet.h"

#define ONENET_INFO_DEVID "597952816"
#define ONENET_INFO_AUTH "202005160951"
#define ONENET_INFO_APIKEY "zgQdlB5y3Bi9pNd2bUYmS8TJHIY="
#define ONENET_INFO_PROID "345377"
#define ONENET_MASTER_APIKEY "gwaK2wJT5wgnSbJYz67CVRGvwkI="

extern int rand(void);


void onenet_cmd_rsp_cb(uint8_t *recv_data, size_t recv_size, uint8_t **resp_data, size_t *resp_size)
{

    printf("recv data is %.*s\n", recv_size, recv_data);

    *resp_data = NULL;
    *resp_size = 0;
}



int mqtt_test(void)
{
	
    device_info_init(ONENET_INFO_DEVID, ONENET_INFO_PROID, ONENET_INFO_AUTH, ONENET_INFO_APIKEY, ONENET_MASTER_APIKEY);
    onenet_mqtt_init();

    onenet_set_cmd_rsp_cb(onenet_cmd_rsp_cb);

	while (1)
    {
		int value = 0;
		
        value = rand() % 100;

        if (onenet_mqtt_upload_digit("temperature", value) < 0)
        {
            printf("upload has an error, stop uploading");
            //break;
        }
        else
        {
            printf("buffer : {\"temperature\":%d} \r\n", value);
        }
        sleep(1);
    }

	return 0;
}

