/*
 * esp8266.h
 *
 *  Created on: 2021¦~11¤ë11¤é
 *      Author: 903990
 */

#ifndef EXTERNAL_WIFI_ESP8266_ESP8266_H_
#define EXTERNAL_WIFI_ESP8266_ESP8266_H_


#include "board.h"
#include "board.h"
#include "hx_drv_uart.h"
#include "hx_drv_iomux.h"

//#include "external/nb_iot/type1sc/type1sc.h"
#ifdef WIFI_ESP8266

#define AT_ADD_POSTFIX

#define AT_PREFIX 			"AT"
#define AT_POSTFIX			"\r\n"
#define AT_OK_STR			"OK"
#define AT_ERROR_STR		"ERROR"

#define AT_MAX_LEN 	    	1536
#define AT_MAX_PARAMETER 	8

#define AT_OK		     	0
#define AT_ERROR	     	-1
#define UART_READ_TIMEOUT	-2

typedef char * AT_STRING;

typedef enum {
	AT_LIST,
	AT_READ,
	AT_WRITE,
	AT_PROPRIETARY,
	AT_EXECUTE,
	AT_CMD_TEST				= 99
}AT_MODE;

typedef enum {
	PUBLISH_TOPIC_DPS_IOTHUB = 0,
	PUBLISH_TOPIC_SEND_DATA  = 1,

}PUBLISH_TOPIC_TYPE;

typedef enum {
	SEND_IMAGE_DATA 	= 0,
	SEND_CSTM_JSON_DATA = 1,
}SEND_DATA_TYPE;

typedef enum {
	WIFI_ESP8266_SET_MODE_CFG 	  				= 0,
	WIFI_ESP8266_SSID_PW_CFG 					= 1,
	WIFI_ESP8266_QUERY_IP_ADDRESS_CFG  			= 2,
	WIFI_ESP8266_QUERY_NTP_NETWORK_TIME_CFG	 	= 3,
	WIFI_ESP8266_NETWORK_INIIAL_DONE	  		= 4,
}WIFI_ESP8266_NETWORK_INITIAL_CFG_STATE;

DEV_UART_PTR esp8266_init(USE_SS_UART_E uart_id, uint32_t baudrate);

int8_t esp8266_test(DEV_UART_PTR dev_uart_comm, char *recv_buf, uint32_t recv_len);

int8_t esp8266_drv_write_at_cmd(DEV_UART_PTR dev_uart_comm,AT_MODE mode, AT_STRING command, ...);
int8_t esp8266_drv_read(DEV_UART_PTR dev_uart_comm, char *recv_buf, uint32_t recv_len);

int8_t esp8266_wifi_mode_set(DEV_UART_PTR dev_uart_comm, AT_STRING wifi_mode, char *recv_buf, uint32_t recv_len);
int8_t esp8266_wifi_connect(DEV_UART_PTR dev_uart_comm, AT_STRING ssid, AT_STRING pwd, char *recv_buf, uint32_t recv_len);

int8_t esp8266_address_get(DEV_UART_PTR dev_uart_comm,char *recv_buf, uint32_t recv_len);
int8_t esp8266_set_ntp_time(DEV_UART_PTR dev_uart_comm, AT_STRING timezone, AT_STRING domain, char *recv_buf, uint32_t recv_len);
int8_t esp8266_get_ntp_time(DEV_UART_PTR dev_uart_comm, char *recv_buf, uint32_t recv_len);
/////////////////////////////////////////////////////////////////
////////////////////////////////MQTT////////////////////////////

int8_t esp8266_mqtt_set_user_cfg(DEV_UART_PTR dev_uart_comm, AT_STRING scheme,char *recv_buf, uint32_t recv_len);
int8_t esp8266_mqtt_set_user_client_id(DEV_UART_PTR dev_uart_comm, AT_STRING client_id,char *recv_buf, uint32_t recv_len);
int8_t esp8266_mqtt_set_user_name(DEV_UART_PTR dev_uart_comm, AT_STRING user_name,char *recv_buf, uint32_t recv_len);
int8_t esp8266_mqtt_set_user_password(DEV_UART_PTR dev_uart_comm, AT_STRING user_password,char *recv_buf, uint32_t recv_len);
int8_t esp8266_mqtt_publish(DEV_UART_PTR dev_uart_comm, AT_STRING topic, AT_STRING data,char *recv_buf, uint32_t recv_len);
int8_t esp8266_mqtt_conn(DEV_UART_PTR dev_uart_comm, AT_STRING domain, AT_STRING port,char *recv_buf, uint32_t recv_len);
int8_t esp8266_mqtt_disconn(DEV_UART_PTR dev_uart_comm, char *recv_buf, uint32_t recv_len);

#endif /* WIFI_ESP8266 */
#endif /* EXTERNAL_WIFI_ESP8266_ESP8266_H_ */
