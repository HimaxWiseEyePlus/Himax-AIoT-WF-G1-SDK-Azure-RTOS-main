/*
 * esp8266.c
 *
 *  Created on: 2021¦~11¤ë11¤é
 *      Author: 903990
 */


#include <esp8266.h>
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stdarg.h"
#include "embARC_debug.h"
#include "tx_api.h"
#include "hx_drv_iomux.h"

//#define DEBUG_TEST
#define UART_READ_CNT 12	// tx_thread_sleep(9000) * 15 : about 7.14s , Note: tx_thread_sleep(9000):514ms

/*esp8266_init*/
DEV_UART_PTR esp8266_init(USE_SS_UART_E uart_id, uint32_t baudrate){

	DEV_UART_PTR dev_uart_comm = NULL;
	dev_uart_comm = hx_drv_uart_get_dev(uart_id);
	if(dev_uart_comm != NULL){
		dev_uart_comm->uart_open(baudrate);
	}

	return dev_uart_comm;
}

/*esp8266_drv_write_at_cmd*/
int8_t esp8266_drv_write_at_cmd(DEV_UART_PTR dev_uart_comm,AT_MODE mode, AT_STRING command, ...)
{
	TX_INTERRUPT_SAVE_AREA
		char at_cmd[AT_MAX_LEN] = AT_PREFIX;

		va_list vl;
		char * str = command;
		if(str == NULL){
			xprintf("[%s]%d: command is NULL, send AT test command\r\n", __FUNCTION__, __LINE__);
		} else {
			if(mode == AT_PROPRIETARY){
				strcat(at_cmd,"%");
			}else{
				strcat(at_cmd,"+");
			}

			strcat(at_cmd, command);

			switch(mode){
				case AT_LIST:
					strcat(at_cmd, "=?");
					break;
				case AT_READ:
					strcat(at_cmd, "?");
					break;
				case AT_WRITE:
					strcat(at_cmd, "=");
					va_start(vl, command);
					for(int i = 0; i < AT_MAX_PARAMETER; i++){
						str = va_arg(vl, AT_STRING);
						if(str == NULL){
							break;
						}
						if(i != 0){
							strcat(at_cmd, ",");
						}
						strcat(at_cmd, str);
					}
					va_end(vl);
					break;
				case AT_PROPRIETARY:
					strcat(at_cmd, "=");
					va_start(vl, command);
					for(int i = 0; i < AT_MAX_PARAMETER; i++){
						str = va_arg(vl, AT_STRING);
						if(str == NULL){
							break;
						}
						if(i != 0){
							strcat(at_cmd, ",");
							//strcat(at_cmd, "\\,"); //mqtt publish 20211112@jason
						}
						strcat(at_cmd, str);
					}
					va_end(vl);
					break;
				case AT_EXECUTE:
				default:
					break;
			}
		}
		#ifdef AT_ADD_POSTFIX
			strcat(at_cmd, AT_POSTFIX);
		#endif /*AT_ADD_POSTFIX*/

		//xprintf("at_cmd:\"%s\" ,length:(%d)\r\n",at_cmd, strlen(at_cmd));
		int len = 0;
		len = dev_uart_comm->uart_write(at_cmd,strlen(at_cmd));
//		xprintf("\nat_cmd length:(%d),%s\r\n", len,at_cmd);
		//if( 0 > dev_uart_comm->uart_write(at_cmd,strlen(at_cmd)))
		if(0 > len)
		{
			xprintf("at cmd send fail.\n");
			return AT_ERROR;
		}

		return AT_OK;
}

/*esp8266_drv_read*/
int8_t esp8266_drv_read(DEV_UART_PTR dev_uart_comm, char *recv_buf, uint32_t recv_len)
{

	int ret = 0;
	ret = dev_uart_comm->uart_read(recv_buf,recv_len);
//	if(0 > ret)
//	{
//		xprintf("uart read fail.\n");
//		return AT_ERROR;
//	}
	return ret;//AT_OK;
}

/*esp8266_test*/
int8_t esp8266_test(DEV_UART_PTR dev_uart_comm, char *recv_buf, uint32_t recv_len)
{
	uint8_t uart_read_cnt = 0;

	//AT+CGPADDR
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_EXECUTE, NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);

		xprintf("[\"AT\" Test..]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		if(strstr(recv_buf,AT_OK_STR)){
			xprintf("[\"AT\" Test OK]\n");
			break;
		}

		if(uart_read_cnt == 3){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;

}

/*esp8266_wifi_mode_set*/
int8_t esp8266_wifi_mode_set(DEV_UART_PTR dev_uart_comm, AT_STRING wifi_mode, char *recv_buf, uint32_t recv_len){

	uint8_t uart_read_cnt = 0;

	//AT+CWMODE
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_WRITE,"CWMODE",wifi_mode, NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);

		xprintf("[WIFI MODE Setting]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		if(strstr(recv_buf,AT_OK_STR)){
			xprintf("[WIFI MODE Setting ok!!]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
			break;
		}

		if(uart_read_cnt == UART_READ_CNT){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;
}

/*esp8266_wifi_connect*/
int8_t esp8266_wifi_connect(DEV_UART_PTR dev_uart_comm, AT_STRING ssid, AT_STRING pwd, char *recv_buf, uint32_t recv_len){

	uint8_t uart_read_cnt = 0;

	//AT+CWJAP="iPhone11","00000000"
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_WRITE, "CWJAP", ssid, pwd, NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);

		xprintf("[WIFI CONNECT to AP Setting]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		if(strstr(recv_buf,"WIFI GOT IP")){
			xprintf("[[WIFI CONNECT to AP ok!!]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
			break;
		}

		if(uart_read_cnt == UART_READ_CNT){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;
}

/*esp8266_address_get*/
int8_t esp8266_address_get(DEV_UART_PTR dev_uart_comm,char *recv_buf, uint32_t recv_len){

	uint8_t uart_read_cnt = 0;

	//AT+CIFSR
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_EXECUTE,"CIFSR", NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);

		xprintf("[GET IP ADDRESS..]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		if(strstr(recv_buf,AT_OK_STR)){
			xprintf("[GET IP ADDRESS ok!!]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
			break;
		}

		if(uart_read_cnt == UART_READ_CNT){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;
}

/*esp8266_set_ntp_time*/
int8_t esp8266_set_ntp_time(DEV_UART_PTR dev_uart_comm, AT_STRING timezone, AT_STRING domain, char *recv_buf, uint32_t recv_len)
{
	uint8_t uart_read_cnt = 0;

	//AT+CIPSNTPCFG=1,8,"ntp1.aliyun.com"
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_WRITE, "CIPSNTPCFG", "1", timezone , domain, NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);

		xprintf("[SET NTP NETWORK TIME..]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		if(strstr(recv_buf,AT_OK_STR)){
			xprintf("[SET NTP NETWORK CFG ok!!]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
			break;
		}

		if(uart_read_cnt == UART_READ_CNT){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;
}

/*esp8266_get_ntp_time*/
int8_t esp8266_get_ntp_time(DEV_UART_PTR dev_uart_comm, char *recv_buf, uint32_t recv_len)
{
	uint8_t uart_read_cnt = 0;

	//AT+CIPSNTPTIME?
	//No NETWORK:+CIPSNTPTIME:Thu Jan  1 08:03:21 1970
	//NETWORK   :+CIPSNTPTIME:Mon Oct 04 18:50:04 2021
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_READ, "CIPSNTPTIME", NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);

		xprintf("[GET NTP NETWORK TIME..]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		if(strstr(recv_buf,"+CIPSNTPTIME:")){
			if(strstr(recv_buf,"1970")){
					xprintf("[PLEASE CHECK WIFI CONNECT to NETWORK?]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
					return AT_ERROR;
				}else{
					xprintf("[GET NTP NETWORK TIME]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
					break;
				}
		}

		if(uart_read_cnt == UART_READ_CNT){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;
}

/////////////////////////////////////////////////////////////////
////////////////////////////////MQTT////////////////////////////

/* esp8266_mqtt_set_user_cfg */
int8_t esp8266_mqtt_set_user_cfg(DEV_UART_PTR dev_uart_comm, AT_STRING scheme,char *recv_buf, uint32_t recv_len)
{
	uint8_t uart_read_cnt = 0;

	//AT+MQTTUSERCFG=0,2,"NULL","NULL","NULL",0,0,""
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_WRITE, "MQTTUSERCFG", "0", scheme,\
									"\"NULL\"", "\"NULL\"", "\"NULL\"", "0", "0", "\"\"", NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);

		xprintf("[SET MQTT USER CFG..]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		if(strstr(recv_buf,AT_OK_STR)){
			xprintf("[SET MQTT USER CFG ok!!]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
			break;
		}

		if(uart_read_cnt == UART_READ_CNT){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;
}


/*esp8266_mqtt_set_user_client_id*/
int8_t esp8266_mqtt_set_user_client_id(DEV_UART_PTR dev_uart_comm, AT_STRING client_id,char *recv_buf, uint32_t recv_len)
{
	uint8_t uart_read_cnt = 0;

	//AT+MQTTCLIENTID=0,"weiplus01"
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_WRITE, "MQTTCLIENTID", "0", client_id, NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);

		xprintf("[SET MQTT CLIENT ID..]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		if(strstr(recv_buf,AT_OK_STR)){
			xprintf("[SET MQTT CLIENT ID ok!!]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
			break;
		}

		if(uart_read_cnt == UART_READ_CNT){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;
}

/*esp8266_mqtt_set_user_name*/
int8_t esp8266_mqtt_set_user_name(DEV_UART_PTR dev_uart_comm, AT_STRING user_name,char *recv_buf, uint32_t recv_len)
{
	uint8_t uart_read_cnt = 0;

	//AT+MQTTUSERNAME=0,"WeiPlusIoTHub.azure-devices.net/weiplus01/?api-version=2020-09-30&model-id=dtmi:himax:himax_aiot_nb_g2;2"
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_WRITE, "MQTTUSERNAME", "0", user_name, NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);

		xprintf("[SET MQTT USER NAME..]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		if(strstr(recv_buf,AT_OK_STR)){
			xprintf("[SET MQTT USER NAME ok!!]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
			break;
		}

		if(uart_read_cnt == UART_READ_CNT){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;
}

/*esp8266_mqtt_set_user_password*/
int8_t esp8266_mqtt_set_user_password(DEV_UART_PTR dev_uart_comm, AT_STRING user_password,char *recv_buf, uint32_t recv_len)
{
	uint8_t uart_read_cnt = 0;

	//AT+MQTTPASSWORD
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_WRITE, "MQTTPASSWORD", "0", user_password, NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);

		xprintf("[SET MQTT USER PASSWORD..]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		if(strstr(recv_buf,AT_OK_STR)){
			xprintf("[SET MQTT USER PASSWORD ok!!]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
			break;
		}

		if(uart_read_cnt == UART_READ_CNT){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;
}

/*esp8266_mqtt_publish*/
int8_t esp8266_mqtt_publish(DEV_UART_PTR dev_uart_comm, AT_STRING topic, AT_STRING data,char *recv_buf, uint32_t recv_len)
{
	uint8_t uart_read_cnt = 0;
	//AT+MQTTPUB=0,"$dps/registrations/PUT/iotdps-register/?$rid=1","{\"registrationId\":\"weiplus\"\,\"payload\":{\"modelId\":\"dtmi:himax:weiplus;2\"}}",1,0
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_WRITE, "MQTTPUB", "0", topic, data, "1", "0", NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);

		xprintf("[MQTT PUBLISH DATA to Server..]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		if(strstr(recv_buf,AT_OK_STR)){
			xprintf("[MQTT PUBLISH DATA to Server ok!!]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
			break;
		}

		if(uart_read_cnt == UART_READ_CNT){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;
}

/*esp8266_mqtt_conn*/
int8_t esp8266_mqtt_conn(DEV_UART_PTR dev_uart_comm, AT_STRING domain, AT_STRING port,char *recv_buf, uint32_t recv_len)
{
	uint8_t uart_read_cnt = 0;

	//AT+MQTTCONN=<LinkID>,<"host">,<port>,<reconnect>
	if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_WRITE, "MQTTCONN", "0", domain, port, "1", NULL))
	{
		return AT_ERROR;
	}

	// get at reply
	while(1){
		uart_read_cnt++;
		esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);
		xprintf("[SET MQTT CONNECT to Server..]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
		//+MQTTCONNECTED:0,2,"WeiPlusIoTHub.azure-devices.net","8883","",1
		if(strstr(recv_buf,"+MQTTCONNECTED:") || strstr(recv_buf,AT_OK_STR)){
			xprintf("[MQTT CONNECT to Server ok!!]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
			break;
		}

		if(uart_read_cnt == UART_READ_CNT){
			xprintf("uart_read timeout.\n");
			return UART_READ_TIMEOUT;
		}
		tx_thread_sleep(7000);//400ms
	}//while

	return AT_OK;
}

/*esp8266_mqtt_disconn*/
int8_t esp8266_mqtt_disconn(DEV_UART_PTR dev_uart_comm, char *recv_buf, uint32_t recv_len)
{
	uint8_t uart_read_cnt = 0;

		//AT+MQTTCLEAN=<LinkID>
		if(0 > esp8266_drv_write_at_cmd(dev_uart_comm, AT_WRITE, "MQTTCLEAN", "0", NULL))
		{
			return AT_ERROR;
		}

		// get at reply
		while(1){
			uart_read_cnt++;
			esp8266_drv_read(dev_uart_comm,recv_buf,recv_len);
			xprintf("[SET MQTT DISCONNECT ..]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
			if(strstr(recv_buf,AT_OK_STR)){
				xprintf("[SET MQTT DISCONNECT ok!!]\nrecv_len:%d %s\n",strlen(recv_buf),recv_buf);
				break;
			}

			if(uart_read_cnt == UART_READ_CNT){
				xprintf("uart_read timeout.\n");
				return UART_READ_TIMEOUT;
			}
			tx_thread_sleep(7000);//400ms
		}//while

		return AT_OK;
}
