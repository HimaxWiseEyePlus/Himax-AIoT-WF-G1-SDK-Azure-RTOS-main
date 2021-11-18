/*
 * azure_iothub.c
 *
 *  Created on: 2021年7月7日
 *      Author: 903990
 */


#if 1
#include <addons/azure_iot/nx_azure_iot_hub_client.h>
#include <addons/azure_iot/nx_azure_iot_json_reader.h>
#include <addons/azure_iot/nx_azure_iot_json_writer.h>
#include <addons/azure_iot/nx_azure_iot_provisioning_client.h>
#include <nx_api.h>
#include <hx_aiot_wifi_g1/inc/nx_azure_iot_cert.h>
#include <hx_aiot_wifi_g1/inc/nx_azure_iot_ciphersuites.h>
#include <hx_aiot_wifi_g1/inc/azure_iothub.h>
#include <hx_aiot_wifi_g1/inc/pmu.h>
#include "azure/core/az_span.h"
#include "azure/core/az_version.h"
#include "tx_port.h"
#include "hx_drv_iomux.h"
#define SAMPLE_DHCP_DISABLE
#ifndef SAMPLE_DHCP_DISABLE
#include <nxd_dhcp_client.h>
#endif /* SAMPLE_DHCP_DISABLE */
#include <nxd_dns.h>
#include <nx_secure_tls_api.h>
#include "BITOPS.h"


#include "esp8266.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Enable PMU mode. */
//#define ENABLE_PMU
/* Setup PMU Sleep time. */
#define PMU_SLEEP_MS	15000 /* unit:ms*/


#define NETWORK_EN
/* Define AZ IoT Provisioning Client topic format.  */
#define NX_AZURE_IOT_PROVISIONING_CLIENT_REG_SUB_TOPIC                "$dps/registrations/res/#"
#define NX_AZURE_IOT_PROVISIONING_CLIENT_PAYLOAD_START                "{\"registrationId\" : \""
#define NX_AZURE_IOT_PROVISIONING_CLIENT_QUOTE                        "\""
#define NX_AZURE_IOT_PROVISIONING_CLIENT_CUSTOM_PAYLOAD                ", \"payload\" : "
#define NX_AZURE_IOT_PROVISIONING_CLIENT_PAYLOAD_END                  "}"
#define NX_AZURE_IOT_PROVISIONING_CLIENT_POLICY_NAME                  "registration"

/* Useragent e.g: DeviceClientType=c%2F1.0.0-preview.1%20%28nx%206.0%3Bazrtos%206.0%29 */
#define NX_AZURE_IOT_HUB_CLIENT_STR(C)          #C
#define NX_AZURE_IOT_HUB_CLIENT_TO_STR(x)       NX_AZURE_IOT_HUB_CLIENT_STR(x)
#define NX_AZURE_IOT_HUB_CLIENT_USER_AGENT      "DeviceClientType=c%2F" AZ_SDK_VERSION_STRING "%20%28nx%20" \
                                                NX_AZURE_IOT_HUB_CLIENT_TO_STR(NETXDUO_MAJOR_VERSION) "." \
                                                NX_AZURE_IOT_HUB_CLIENT_TO_STR(NETXDUO_MINOR_VERSION) "%3Bazrtos%20"\
                                                NX_AZURE_IOT_HUB_CLIENT_TO_STR(THREADX_MAJOR_VERSION) "." \
                                                NX_AZURE_IOT_HUB_CLIENT_TO_STR(THREADX_MINOR_VERSION) "%29"

#define WIFI_ATCMD_RETRY_MAX_TIMES				12
#define WIFI_EN									1
#define DEBUG_TEST								0

#define AZURE_DEVICE_PROVISIONING_SERVICE_EN	0
#define AZURE_IOTHUB_DEVICE_EN					1


#if WIFI_EN
/*Key your AP "SSID", "PASSWORD"*/
#define WIFI_SSID 		"Key-in your SSID"
#define WIFI_PW  		"Key-in your PASSWORD"

#define NTP_TIMEZONE	"8" 				/*UTC+8*/
#define NTP_DOMAIN 		"ntp1.aliyun.com"

#endif //#if WIFI_EN

DEV_UART_PTR dev_uart_comm;

static SAMPLE_CONTEXT sample_context;

/* Define Azure RTOS TLS info.  */
static NX_SECURE_X509_CERT root_ca_cert;
static NX_AZURE_IOT_PROVISIONING_CLIENT dps_client;
static UCHAR nx_azure_iot_tls_metadata_buffer[NX_AZURE_IOT_TLS_METADATA_BUFFER_SIZE];
static NX_AZURE_IOT_HUB_CLIENT hub_client;
static UCHAR *buffer_ptr;
static UINT buffer_size = 1536;
VOID *buffer_context;
INT wifi_service_get_dps_key(ULONG expiry_time_secs, UCHAR *resource_dps_sas_token);
INT wifi_service_get_iothub_key(ULONG expiry_time_secs, UCHAR *resource_dps_sas_token);

/* Define the prototypes for AZ IoT.  */
static NX_AZURE_IOT nx_azure_iot;

/* Include the sample.  */
//extern VOID sample_entry(NX_IP* ip_ptr, NX_PACKET_POOL* pool_ptr, NX_DNS* dns_ptr, UINT (*unix_time_callback)(ULONG *unix_time));
extern VOID sample_entry(NX_IP *ip_ptr, NX_PACKET_POOL *pool_ptr, NX_DNS *dns_ptr, UINT (*unix_time_callback)(ULONG *unix_time), SAMPLE_CONTEXT *sample_context);
int get_time_flag = 0;

/* Define the helper thread for running Azure SDK on ThreadX (THREADX IoT Platform).  */
#ifndef SAMPLE_HELPER_STACK_SIZE
#define SAMPLE_HELPER_STACK_SIZE        (4096)
#endif /* SAMPLE_HELPER_STACK_SIZE  */

#ifndef WIFI_SERVICE_STACK_SIZE
#define WIFI_SERVICE_STACK_SIZE        (4096)
#endif /* SAMPLE_HELPER_STACK_SIZE  */

#ifndef ALGO_SEND_RESULT_STACK_SIZE
#define ALGO_SEND_RESULT_STACK_SIZE     (4096)
#endif /* SAMPLE_HELPER_STACK_SIZE  */

#ifndef CAPTURE_IMAGE_STACK_SIZE
#define CIS_CAPTURE_IMAGE_STACK_SIZE     (4096)
#endif /* CIS_CAPTURE_IMAGE_STACK_SIZE  */

#ifndef SAMPLE_HELPER_THREAD_PRIORITY
#define SAMPLE_HELPER_THREAD_PRIORITY   (4)
#endif /* SAMPLE_HELPER_THREAD_PRIORITY  */

#ifndef WIFI_SERVICE_THREAD_PRIORITY
#define WIFI_SERVICE_THREAD_PRIORITY   (5)
#endif /* WIFI_SERVICE_THREAD_PRIORITY  */

#ifndef ALGO_SEND_RESULT_THREAD_PRIORITY
#define ALGO_SEND_RESULT_THREAD_PRIORITY   (6)
#endif /* SAMPLE_HELPER_THREAD_PRIORITY  */

#ifndef CIS_CAPTURE_IMAGE_THREAD_PRIORITY
#define CIS_CAPTURE_IMAGE_THREAD_PRIORITY   (7)
#endif /* CIS_CAPTURE_IMAGE_THREAD_PRIORITY  */

/* Define user configurable symbols. */
#ifndef SAMPLE_IP_STACK_SIZE
#define SAMPLE_IP_STACK_SIZE            (2048)
#endif /* SAMPLE_IP_STACK_SIZE  */

#ifndef SAMPLE_PACKET_COUNT
#define SAMPLE_PACKET_COUNT             (32)
#endif /* SAMPLE_PACKET_COUNT  */

#ifndef SAMPLE_PACKET_SIZE
#define SAMPLE_PACKET_SIZE              (1536)
#endif /* SAMPLE_PACKET_SIZE  */

#define SAMPLE_POOL_SIZE                ((SAMPLE_PACKET_SIZE + sizeof(NX_PACKET)) * SAMPLE_PACKET_COUNT)

#ifndef SAMPLE_ARP_CACHE_SIZE
#define SAMPLE_ARP_CACHE_SIZE           (512)
#endif /* SAMPLE_ARP_CACHE_SIZE  */

#ifndef SAMPLE_IP_THREAD_PRIORITY
#define SAMPLE_IP_THREAD_PRIORITY       (1)
#endif /* SAMPLE_IP_THREAD_PRIORITY */

#ifdef SAMPLE_DHCP_DISABLE
#ifndef SAMPLE_IPV4_ADDRESS
#define SAMPLE_IPV4_ADDRESS           IP_ADDRESS(192, 168, 100, 33)
//#error "SYMBOL SAMPLE_IPV4_ADDRESS must be defined. This symbol specifies the IP address of device. "
#endif /* SAMPLE_IPV4_ADDRESS */

#ifndef SAMPLE_IPV4_MASK
#define SAMPLE_IPV4_MASK              0xFFFFFF00UL
//#error "SYMBOL SAMPLE_IPV4_MASK must be defined. This symbol specifies the IP address mask of device. "
#endif /* SAMPLE_IPV4_MASK */

#ifndef SAMPLE_GATEWAY_ADDRESS
#define SAMPLE_GATEWAY_ADDRESS        IP_ADDRESS(192, 168, 100, 1)
//#error "SYMBOL SAMPLE_GATEWAY_ADDRESS must be defined. This symbol specifies the gateway address for routing. "
#endif /* SAMPLE_GATEWAY_ADDRESS */

#ifndef SAMPLE_DNS_SERVER_ADDRESS
#define SAMPLE_DNS_SERVER_ADDRESS     IP_ADDRESS(192, 168, 100, 1)
//#error "SYMBOL SAMPLE_DNS_SERVER_ADDRESS must be defined. This symbol specifies the dns server address for routing. "
#endif /* SAMPLE_DNS_SERVER_ADDRESS */
#else
#define SAMPLE_IPV4_ADDRESS             IP_ADDRESS(192, 168, 52, 10)
#define SAMPLE_IPV4_MASK                IP_ADDRESS(255, 255, 255, 0)
#define SAMPLE_IPV4_GATEWAY             IP_ADDRESS(192, 168, 52, 254)
#endif /* SAMPLE_DHCP_DISABLE */


//0302static TX_THREAD        sample_helper_thread;
static TX_THREAD        wifi_service_thread;
static TX_THREAD        algo_send_result_thread;
static TX_THREAD        cis_capture_image_thread;
static NX_PACKET_POOL   pool_0;
static NX_IP            ip_0;
static NX_DNS           dns_0;
#ifndef SAMPLE_DHCP_DISABLE
static NX_DHCP          dhcp_0;
#endif /* SAMPLE_DHCP_DISABLE  */


/* Define the stack/cache for ThreadX.  */
static ULONG sample_ip_stack[SAMPLE_IP_STACK_SIZE / sizeof(ULONG)];
#ifndef SAMPLE_POOL_STACK_USER
static ULONG sample_pool_stack[SAMPLE_POOL_SIZE / sizeof(ULONG)];
static ULONG sample_pool_stack_size = sizeof(sample_pool_stack);
#else
extern ULONG sample_pool_stack[];
extern ULONG sample_pool_stack_size;
#endif

static ULONG wifi_service_thread_stack[WIFI_SERVICE_STACK_SIZE / sizeof(ULONG)];
static ULONG cis_capture_image_thread_stack[CIS_CAPTURE_IMAGE_STACK_SIZE / sizeof(ULONG)];
static ULONG algo_send_result_thread_stack[ALGO_SEND_RESULT_STACK_SIZE / sizeof(ULONG)];

/* Define the prototypes for type1sc mqtt initial. */
static void nb_type1sc_NQTT_initial_cfg();

/* Define the prototypes for sample thread.  */
static void sample_helper_thread_entry(ULONG parameter);

/* Define the prototypes for wifi service thread.  */
static void wifi_service_thread_entry(ULONG parameter);

/* Define the prototypes for cis capture image thread.  */
static void cis_capture_image_thread_entry(ULONG parameter);

/* Define the prototypes for algo send result thread.  */
static void algo_send_result_thread_entry(ULONG parameter);

/* Azure IoT DPS. */
static  char *azure_iotdps_connect_user_name;

//azure centrol iotdps pw
static  char azure_iotdps_connect_password[512];

AT_STRING azure_iotdps_get_registrations_publish_topic   = AZURE_IOTDPS_GET_CLIENT_REGISTER_STATUS_PUBLISH_TOPIC;//"$dps/registrations/GET/iotdps-get-operationstatus/?$rid=1&operationId=";

/* MQTT publish message for get operationID.*/
char azure_iotdps_registrations_msg[256];
/* MQTT publish message for get registration info.*/
char azure_iotdps_get_registrations_msg[256];

char azure_registrations_msg_len[2];

/* Azure Central IoT HUB. host name */
//static  char azure_iothub_connect_host_name[128];
char azure_iothub_connect_host_name[]= "WeiPlusIoTHub.azure-devices.net";

/* Azure Central IoT HUB. user_password*/
static  char azure_iothub_connect_password[512];
//char  azure_iothub_connect_password[]= "SharedAccessSignature sr=WeiPlusIoTHub.azure-devices.net%2Fdevices%2Fweiplus01&sig=raP%2FlzCaKD4Ll8ooN8xrX11A0EMxpYKS7m4SUZ3QSYs%3D&se=1668071384";
//char  azure_iothub_connect_password[256]= "\"SharedAccessSignature sr=WeiPlusIoTHub.azure-devices.net%2Fdevices%2Fweiplus01&sig=aC%2ByS6dSK%2F6lUgX70PkukGMMd8V%2Fw3svXwIZvr2qJXQ%3D&se=1638359512\"";

/* Azure Central IoT HUB. user name*/
static  char azure_iothub_connect_user_name[384];

//AT_STRING azure_iothub_publish_topic;
static char azure_iothub_publish_topic[256];
char azure_iothub_publish_msg_json_pkg[AT_MAX_LEN];

/* MQTT publish message info. */
char azure_iothub_publish_msg_len[2];

/* Azure PNP DPS vent initial. */
static uint8_t azure_pnp_iotdps_event = PNP_IOTDPS_INITIAL;

/* Azure PNP IoTHub Event initial. */
static uint8_t azure_pnp_iothub_event = PNP_IOTHUB_WIFI_IDLE;

/* Azure Algorithm Event initial. */
uint8_t azure_active_event = ALGO_EVENT_IDLE;

/* uart read buffer*/
char recv_buf[AT_MAX_LEN];
/* only save operationID, remove other info from receive data. */
char azure_iotdps_reg_opid[256];
//char *recv_buf;
//char *azure_iotdps_reg_opid;
uint32_t recv_len = AT_MAX_LEN;

/*#################Algorithm Info#################*/
/* unit:BYTE */
#define SEND_PKG_MAX_SIZE 	256//128

#if 0
uint32_t azure_iotdps_reg_year,azure_iotdps_reg_month,azure_iotdps_reg_day;
uint32_t azure_iotdps_reg_hour,azure_iotdps_reg_minute,azure_iotdps_reg_sec;
#else
struct tm azure_iotdps_network_tm;
static time_t azure_iotdps_network_epoch_time;

struct tm azure_iotdps_reg_tm;
static time_t azure_iotdps_epoch_time;
#endif

#ifndef SAMPLE_DHCP_DISABLE
static void dhcp_wait();
#endif /* SAMPLE_DHCP_DISABLE */

static UINT dns_create();

static UINT unix_time_get(ULONG *unix_time);

/* Include the platform IP driver. */
void _nx_ram_network_driver(struct NX_IP_DRIVER_STRUCT *driver_req);

/*
 * azure_iotdps_parsing_network_time
 * */
int8_t azure_iotdps_parsing_network_time()
{
	xprintf("### Parsing NTP Network time and Generate DPS SAS Token... ###\n");

	char azure_iotdps_netwoerk_time[48];
	char *azure_iotdps_get_network_time_loc = NULL;
	char *str_month_list[12] ={"Jan", "Feb", "Mar", "Apr","May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	ULONG current_time;
	UCHAR *resource_dps_sas_token = NULL;
	get_time_flag = 0;

#if DEBUG_TEST
	//char recv_buf[1024] ="CIPSNTPTIME:Sat Nov 13 11:35:22 2021";//for test
	char recv_buf[1024] ="AT+CIPSNTPTIME?\r\n+CIPSNTPTIME:Sat Nov 13 11:35:22 2021\r\nOK";//for test
	xprintf("### DEBUG_TEST recv_buf_time:%s ###\n",recv_buf);
#endif

	azure_iotdps_get_network_time_loc = strstr(recv_buf,"+CIPSNTPTIME:");
	strncpy(recv_buf, (recv_buf+(azure_iotdps_get_network_time_loc - recv_buf+1)),(strlen(recv_buf) - (azure_iotdps_get_network_time_loc - recv_buf)));
	xprintf("### Parsing recv_buf:%s ###\n",recv_buf);

	/* parsing network time month. */
	for(uint8_t mon_num = 0; mon_num < 12;mon_num++){
		//xprintf("str_month_list[%d]:%s\n",mon_num,str_month_list[mon_num]);
		//search string month and covert to number.
		if(strstr(recv_buf,str_month_list[mon_num])){
			azure_iotdps_network_tm.tm_mon = mon_num+1-1;
			xprintf("### CURRENT Month:%d ###\n",azure_iotdps_network_tm.tm_mon+1);
			//xprintf("\n!!!Mon recv_buf:%s !!!\n",recv_buf);
		}
	}//for mon_num

	/* parsing network time day. */
	//recv_buf location 21 is day start_loc
	strncpy(azure_iotdps_netwoerk_time, recv_buf+20,2);
	xprintf("### CURRENT Day:%s ###\n",azure_iotdps_netwoerk_time);
	azure_iotdps_network_tm.tm_mday = atoi(azure_iotdps_netwoerk_time);
//	xprintf("### azure_iotdps_netwoerk_time:%s ###\n",azure_iotdps_netwoerk_time);

	/* parsing network time hour. */
	//recv_buf location 24 is hour start_loc
	strncpy(azure_iotdps_netwoerk_time, recv_buf+23,2);
	xprintf("### CURRENT Hour:%s ###\n",azure_iotdps_netwoerk_time);
	azure_iotdps_network_tm.tm_hour = (atoi(azure_iotdps_netwoerk_time) - 1);
//	xprintf("### azure_iotdps_netwoerk_time:%s ###\n",azure_iotdps_netwoerk_time);

	/* parsing network time minute. */
	//recv_buf location 27 is minute start_loc
	strncpy(azure_iotdps_netwoerk_time, recv_buf+26,2);
	xprintf("### CURRENT Minute:%s ###\n",azure_iotdps_netwoerk_time);
	azure_iotdps_network_tm.tm_min = (atoi(azure_iotdps_netwoerk_time) - 1);
//	xprintf("### azure_iotdps_netwoerk_time:%s ###\n",azure_iotdps_netwoerk_time);

	/* parsing network time second. */
	//recv_buf location 30 is second start_loc
	strncpy(azure_iotdps_netwoerk_time, recv_buf+29,2);
	xprintf("### CURRENT Second:%s ###\n",azure_iotdps_netwoerk_time);
	azure_iotdps_network_tm.tm_sec = (atoi(azure_iotdps_netwoerk_time) - 1);
//	xprintf("### azure_iotdps_netwoerk_time:%s ###\n",azure_iotdps_netwoerk_time);

	/* parsing network time year. */
	//recv_buf location 33 is second start_loc
	strncpy(azure_iotdps_netwoerk_time, recv_buf+32,4);
	xprintf("### CURRENT Year:%s ###\n",azure_iotdps_netwoerk_time);
	azure_iotdps_network_tm.tm_year = (atoi(azure_iotdps_netwoerk_time) - 1900);
//	xprintf("### azure_iotdps_netwoerk_time:%s ###\n",azure_iotdps_netwoerk_time);

	azure_iotdps_network_epoch_time = mktime(&azure_iotdps_network_tm);
	get_time_flag = 1;
	unix_time_get(&current_time);
	xprintf(" azure_iotdps_network_epoch_time:%ld \n", azure_iotdps_network_epoch_time);
	//xprintf("*** current_time:%ld ***\n", current_time);
#if AZURE_DEVICE_PROVISIONING_SERVICE_EN
	wifi_service_get_dps_key(azure_iotdps_network_epoch_time, resource_dps_sas_token);
#else
	wifi_service_get_iothub_key(azure_iotdps_network_epoch_time, resource_dps_sas_token);
#endif
	return 1;
}

/*
 * Send Algorithm Metadata to Cloud.
 * */
int8_t send_algo_result_to_cloud()
{
	int8_t ret = 0;
	char azure_iothub_publish_human[10];
	char azure_iothub_publish_det_box_x[10];
	char azure_iothub_publish_det_box_y[10];
	char azure_iothub_publish_det_box_width[10];
	char azure_iothub_publish_det_box_height[10];

	char azure_iothub_publish_msg_json[1024];
	char azure_iothub_publish_msg_json_write[1024];
	char azure_iothub_publish_msg_json_len[2];

	/* for dtmi:himax:weiplus;2 */
	/* eX:{"human":3,"det_box_x":320,"det_box_y":240,"det_box_width":50,"det_box_height":50} */
#if 0
	algo_result.humanPresence				= 1;
	algo_result.ht[0].upper_body_bbox.x		= 320;
	algo_result.ht[0].upper_body_bbox.y 	= 240;
	algo_result.ht[0].upper_body_bbox.width	= 50;
	algo_result.ht[0].upper_body_bbox.height= 50;
#endif
	xprintf("### WIFI IOTHUB Send Metadata... ###\n");
	memset(azure_iothub_publish_msg_json_pkg,0,AT_MAX_LEN);

	sprintf(azure_iothub_publish_human, "%d", algo_result.humanPresence);
	sprintf(azure_iothub_publish_det_box_x, "%d", algo_result.ht[0].upper_body_bbox.x);
	sprintf(azure_iothub_publish_det_box_y, "%d", algo_result.ht[0].upper_body_bbox.y);
	sprintf(azure_iothub_publish_det_box_width, "%d", algo_result.ht[0].upper_body_bbox.width);
	sprintf(azure_iothub_publish_det_box_height, "%d", algo_result.ht[0].upper_body_bbox.height);

#ifdef MODEL_ID_HIMAX_AIOT_NB_G2_V1
	/*
	 * JSON Example:
	 * {"human":1,"det_box_x":320,"det_box_y":320,"det_box_width":50,"det_box_height":50}
	 * */

	/*  {"human": */
	strcpy(azure_iothub_publish_msg_json,AZURE_IOTHUB_PUBLISH_MESSAGE_JSON_PREFIX);
	/* {"human":1 */
	strcat(azure_iothub_publish_msg_json, azure_iothub_publish_human);
//	xprintf("\n*** azure_iothub_publish_msg_json_1:%s ***\n",azure_iothub_publish_msg_json);

	/* {"human":1,"det_box_x": */
	strcat(azure_iothub_publish_msg_json, AZURE_IOTHUB_PUBLISH_MESSAGE_JSON_DET_BOX_X);
	/*{"human":1,"det_box_x":320 */
	strcat(azure_iothub_publish_msg_json, azure_iothub_publish_det_box_x);
//	xprintf("\n*** azure_iothub_publish_msg_json_2:%s ***\n",azure_iothub_publish_msg_json);

	/* {"human":1,"det_box_x":320,"det_box_y": */
	strcat(azure_iothub_publish_msg_json, AZURE_IOTHUB_PUBLISH_MESSAGE_JSON_DET_BOX_Y);
	/* {"human":1,"det_box_x":320,"det_box_y":320 */
	strcat(azure_iothub_publish_msg_json, azure_iothub_publish_det_box_y);
//	xprintf("\n*** azure_iothub_publish_msg_json_3:%s ***\n",azure_iothub_publish_msg_json);

	/* {"human":1,"det_box_x":320,"det_box_y":320,"det_box_width": */
	strcat(azure_iothub_publish_msg_json, AZURE_IOTHUB_PUBLISH_MESSAGE_JSON_DET_BOX_WIDTH);
	/* {"human":1,"det_box_x":320,"det_box_y":320,"det_box_width":50 */
	strcat(azure_iothub_publish_msg_json, azure_iothub_publish_det_box_width);
//	xprintf("\n*** azure_iothub_publish_msg_json_4:%s ***\n",azure_iothub_publish_msg_json);

	/* {"human":1,"det_box_x":320,"det_box_y":240,"det_box_width":50,"det_box_height": */
	strcat(azure_iothub_publish_msg_json, AZURE_IOTHUB_PUBLISH_MESSAGE_JSON_DET_BOX_HEIGHT);
	/* {"human":1,"det_box_x":320,"det_box_y":240,"det_box_width":50,"det_box_height":50 */
	strcat(azure_iothub_publish_msg_json, azure_iothub_publish_det_box_height);
//	xprintf("\n*** azure_iothub_publish_msg_json_5:%s ***\n",azure_iothub_publish_msg_json);

	/* {"human":1,"det_box_x":320,"det_box_y":320,"det_box_width":50,"det_box_height":50} */
	strcat(azure_iothub_publish_msg_json, AZURE_IOTHUB_PUBLISH_MESSAGE_JSON_SUFFIX);
	xprintf("\n*** azure_iothub_publish_msg_json:%s ***\n",azure_iothub_publish_msg_json);
#endif /*#ifdef MODEL_ID_HIMAX_AIOT_NB_G2_V1*/

	/* json data length. */
	memset(azure_iothub_publish_msg_json_len,0,2);
	sprintf(azure_iothub_publish_msg_json_len, "%d", strlen(azure_iothub_publish_msg_json));
	/* json data.*/
	memset(azure_iothub_publish_msg_json_write,0,1024);
	//strncat(azure_iothub_publish_msg_json_write,azure_iothub_publish_msg_json_len,strlen(azure_iothub_publish_msg_json_len));
	strncat(azure_iothub_publish_msg_json_write,azure_iothub_publish_msg_json,strlen(azure_iothub_publish_msg_json));

	xprintf("**** azure_iothub_publish_msg_json_write_metadata:\n%s ****\n\n",azure_iothub_publish_msg_json_write);
#if WIFI_EN //20211008 test comment out jason.

#if DEBUG_TEST
	char azure_iothub_publish_msg_json_write_test[] = "\"{\\\"human\\\":1\\\,\\\"det_box_x\\\":320\\\,\\\"det_box_y\\\":320\\\,\\\"det_box_width\\\":320\\\,\\\"det_box_height\\\":320}\"";
	char azure_iothub_publish_topic_test[] = "\"devices/weiplus01/messages/events/\"";
#endif /*#if DEBUG_TEST*/
	ret = esp8266_mqtt_publish(dev_uart_comm, azure_iothub_publish_topic, azure_iothub_publish_msg_json_write, recv_buf, recv_len);
#endif//#if WIFI_EN

	if(ret == UART_READ_TIMEOUT)
	{
		xprintf("### WIFI IOTHUB Send Metadata to Cloud ing... ###\n");
			return UART_READ_TIMEOUT;
	}else if(ret == AT_ERROR){
		xprintf("### WIFI IOTHUB Send Metadata to Cloud Fail.. ###\n");
		return AT_ERROR;
	}


	return 1;
}

/*
 * Send custom json data to cloud.
 *
 * */
int8_t send_cstm_data_to_cloud(unsigned char *databuf, int size, uint8_t send_type)
{
	int8_t ret = 0;
	char azure_iothub_publish_data2str[1024];
	char azure_iothub_publish_msg_json_cstm_write[1024];

	static char azureIothubPublishTopic[256];
	memset(azureIothubPublishTopic, 0, 256);
	strcat(azureIothubPublishTopic,"\"devices/");
	strcat(azureIothubPublishTopic,AZURE_IOTHUB_DEVICE_ID);
	strcat(azureIothubPublishTopic,"/messages/events/\"");

	/* json data length. */
	memset(azure_iothub_publish_msg_len,0,2);
	/* json data.*/
	memset(azure_iothub_publish_msg_json_cstm_write,0,1024);
	if(send_type == SEND_IMAGE_DATA)
	{
		/*Send Image*/
		/* Image data Hex convert to hex string. */
		for (int j = 0; j < size; j++){
			sprintf((azure_iothub_publish_data2str + (j * 2)), "%02X", *(databuf + j));
		}
	//	xprintf("\n*** azure_iothub_publish_data2str_cstm:%s ***\n",azure_iothub_publish_data2str);
		sprintf(azure_iothub_publish_msg_len, "%d", (strlen(azure_iothub_publish_data2str)+2));
		strncat(azure_iothub_publish_msg_json_cstm_write,azure_iothub_publish_msg_len,strlen(azure_iothub_publish_msg_len));
		strcat(azure_iothub_publish_msg_json_cstm_write,"\r\n");
		strncat(azure_iothub_publish_msg_json_cstm_write,azure_iothub_publish_data2str,strlen(azure_iothub_publish_data2str));
	}else{
		/* SEND_CSTM_JSON_DATA. */
		sprintf(azure_iothub_publish_msg_len, "%d", size);
		strncat(azure_iothub_publish_msg_json_cstm_write,databuf,size);
	}
	xprintf("**** azure_iothub_publish_msg_json_write_cstm:\n%s ****\n\n",azure_iothub_publish_msg_json_cstm_write);

#if WIFI_EN
	esp8266_mqtt_publish(dev_uart_comm, azure_iothub_publish_topic, azure_iothub_publish_msg_json_cstm_write, recv_buf, recv_len);
#endif //#if WIFI_EN

	if(ret == UART_READ_TIMEOUT)
	{
		xprintf("### WIFI IOTHUB Send Data to Cloud ing... ###\n");
		return UART_READ_TIMEOUT;
	}else if(ret == AT_ERROR){
		xprintf("### WIFI IOTHUB Send Data to Cloud Fail.. ###\n");
		return AT_ERROR;
	}

	return 1;
}

#if AZURE_DEVICE_PROVISIONING_SERVICE_EN
/* Parsing Azure IoT DPS Registration Operation ID. */
static uint8_t azure_pnp_iotdps_get_registration_operation_id()
{
	char azure_iotdps_reg_opid_str[512];//need parsing get operationID from receive data

	char *azure_iotdps_reg_opid_loc;
	char azure_iotdps_reg_opid_str_len[2];
	const UINT azure_iotdps_reg_opid_len = 55;

#if 0
	//20210709 jason for test
	char *recv_buf = "%MQTTEVU:\"PUBRCV\",1,2,\"$dps/registrations/res/202/?$rid=1&retry-after=3\",94\r\n\"{\"operationId\":\"4.17252aac68733575.90bf37d0-51ea-48d2-8d8f-cace5e14cf20\",\"status\":\"assigning\"}";
	xprintf("recb_buf:%s\n",recv_buf);
#endif

		/* 20210723 add*/
		/* Deserialization error if the response code is 400 */
		if (strstr(recv_buf, "$dps/registrations/res/400/")!= NULL){
			xprintf("**** Deserialization error ****\n");
			return 0;
		}
		/* 20210429 add end*/

	if (strstr(recv_buf, "%MQTTEVU:\"PUBRCV\"")!= NULL){
		xprintf("### Parsing DPS Registration Operation ID... ###\n");
		xprintf("recb_buf:%s\n",recv_buf);
		azure_iotdps_reg_opid_loc = strstr(recv_buf, "\"operationId\":");
		strncpy(azure_iotdps_reg_opid_str, (recv_buf+(azure_iotdps_reg_opid_loc - recv_buf+1)),(strlen(recv_buf) - (azure_iotdps_reg_opid_loc - recv_buf)));
		xprintf("**** azure_iotdps_reg_opid_str:\n%d,%s ****\n\n",strlen(azure_iotdps_reg_opid_str),azure_iotdps_reg_opid_str);

		/* Ex:
		 * 92,
		 * operationId":"4.17252aac68733575.90bf37d0-51ea-48d2-8d8f-cace5e14cf20","status":"assigning"}
		 */
		azure_iotdps_reg_opid_loc = strstr(azure_iotdps_reg_opid_str, ":");
		strncpy(azure_iotdps_reg_opid, (azure_iotdps_reg_opid_str+(azure_iotdps_reg_opid_loc - azure_iotdps_reg_opid_str+2)),azure_iotdps_reg_opid_len);
		xprintf("**** azure_iotdps_reg_opid:\n%d,%s ****\n\n",strlen(azure_iotdps_reg_opid),azure_iotdps_reg_opid);

		/* get registration publish topic. */
		strncat(azure_iotdps_get_registrations_publish_topic, azure_iotdps_reg_opid, strlen(azure_iotdps_reg_opid));
//		xprintf("**** azure_iotdps_get_registrations_publish_topic:\n%d,%s ****\n\n",strlen(azure_iotdps_get_registrations_publish_topic),azure_iotdps_get_registrations_publish_topic);

		/* publish message for get registration status.*/
		//20210726 sprintf(azure_iotdps_reg_opid_str_len, "%d", strlen(azure_iotdps_get_registrations_publish_topic));
		sprintf(azure_iotdps_reg_opid_str_len, "%d", (strlen(azure_iotdps_get_registrations_publish_topic)+1));
		strcpy(azure_iotdps_get_registrations_msg,azure_iotdps_reg_opid_str_len);
		//xprintf("****len azure_iotdps_get_registrations_msg:\n%s ****\n\n",azure_iotdps_get_registrations_msg);
		strcat(azure_iotdps_get_registrations_msg,"\r\n");
		strncat(azure_iotdps_get_registrations_msg,azure_iotdps_get_registrations_publish_topic,strlen(azure_iotdps_get_registrations_publish_topic));
		// xprintf("**** azure_iotdps_get_registrations_msg:\n%s ****\n\n",azure_iotdps_get_registrations_msg);

		//20210726
		char azure_iotdps_get_registrations_publish_topic_tmp[256];
		memset(azure_iotdps_get_registrations_publish_topic_tmp,0,256);
		strcat(azure_iotdps_get_registrations_publish_topic_tmp,"\"");
		strncat(azure_iotdps_get_registrations_publish_topic_tmp,azure_iotdps_get_registrations_publish_topic,strlen(azure_iotdps_get_registrations_publish_topic));
		strcat(azure_iotdps_get_registrations_publish_topic_tmp,"\"");
		// xprintf("****[TMP] azure_iotdps_get_registrations_publish_topic_tmp:\n%s ****\n\n",azure_iotdps_get_registrations_publish_topic_tmp);
		strcpy(azure_iotdps_get_registrations_publish_topic,azure_iotdps_get_registrations_publish_topic_tmp);
		// xprintf("****[STR] azure_iotdps_get_registrations_publish_topic_str:\n%s ****\n\n",azure_iotdps_get_registrations_publish_topic);

		return 1;
	}else{
		return 0;
	}
}
#endif

#if AZURE_DEVICE_PROVISIONING_SERVICE_EN
/* Parsing Azure IoT DPS Registration Status. */
static uint8_t azure_pnp_iotdps_get_registration_status()
{
	/* azure iotdps registration info. */
	char azure_iotdps_reg_time_str[AT_MAX_LEN];

//	UINT azure_iotdps_parsing_reg_time			= 0;
	const UINT azure_iotdps_parsing_reg_time_len= 19;

	char azure_iotdps_get_create_reg_time_tmp[19];
	char azure_iotdps_reg_assignedhub_tmp[128];

	static char *azure_iotdps_reg_create_time_loc	= NULL;
	static char *azure_iotdps_reg_assignedhub_loc	= NULL;
	static char *azure_iotdps_reg_deviceid_loc		= NULL;

  	//for sas token
	ULONG current_time;
	UCHAR *resource_dps_sas_token = NULL;
	get_time_flag = 0;


//	/* 20210429 add*/
//	/* keep polling the service if the response code is 202 */
//	if (strstr(recv_buf, "$dps/registrations/res/202/")!= NULL){
//		return 0;
//	}
//	/* 20210429 add end*/

#if 0
	//for test
	char *recv_buf = "%MQTTEVU:\"PUBRCV\",1,2,\"$dps/registrations/res/200/?$rid=1\",435\r\n{\"operationId\":\"4.17252aac68733575.69153b4c-3a91-4dab-b634-98ca044fdde0\",\"status\":\"assigned\",\"registrationState\":{\"registrationId\":\"weiplus\",\"createdDateTimeUtc\":\"2021-06-30T06:52:07.7289225Z\",\"assignedHub\":\"WeiPlusIoTHub.azure-devices.net\",\"deviceId\":\"weiplus01\",\"status\":\"assigned\",\"substatus\":\"initialAssignment\",\"lastUpdatedDateTimeUtc\":\"2021-06-30T06:52:07.8873405Z\",\"etag\":\"IjFmMDU5Y2FmLTAwMDAtMjQwMC0wMDAwLTYwZGMxNDk3MDAwMCI=\"}}";
	xprintf("\n[For TEST]recv_buf:\n%s\n\n",recv_buf);
#endif

	/*parsing registered time*/
	if (strstr(recv_buf, "%MQTTEVU:\"PUBRCV\"")!= NULL){
		xprintf("### Parsing DPS Registration Status... ###\n");
		azure_iotdps_reg_create_time_loc = strstr(recv_buf, "\"createdDateTimeUtc\":");
		strncpy(azure_iotdps_reg_time_str, (recv_buf+(azure_iotdps_reg_create_time_loc - recv_buf+1)),(strlen(recv_buf) - (azure_iotdps_reg_create_time_loc - recv_buf)));
		xprintf("**** azure_iotdps_reg_time_str:\n%d,%s ****\n\n",strlen(azure_iotdps_reg_time_str),azure_iotdps_reg_time_str);

		azure_iotdps_reg_create_time_loc = strstr(azure_iotdps_reg_time_str, "createdDateTimeUtc");
		memset(azure_iotdps_get_create_reg_time_tmp, 0, sizeof(azure_iotdps_get_create_reg_time_tmp));
		strncpy(azure_iotdps_get_create_reg_time_tmp, (azure_iotdps_reg_time_str+(azure_iotdps_reg_create_time_loc - azure_iotdps_reg_time_str+21)),azure_iotdps_parsing_reg_time_len);
		azure_iotdps_get_create_reg_time_tmp[19]='\0';
		xprintf("**** azure_iotdps_get_create_reg_time_tmp:\n%d,%s ****\n\n",strlen(azure_iotdps_get_create_reg_time_tmp),azure_iotdps_get_create_reg_time_tmp);

		/*
		 * eX: "createdDateTimeUtc":"2021-06-30T06:52:07.7289225Z"
		 * */
		/* parsing registered time year. */
//		azure_iotdps_reg_year = atoi(azure_iotdps_get_create_reg_time_tmp);
//		xprintf("### azure_iotdps_reg_year:%d ###\n", azure_iotdps_reg_year);
		azure_iotdps_reg_tm.tm_year = (atoi(azure_iotdps_get_create_reg_time_tmp) - 1900);
		//xprintf("*** azure_iotdps_reg_tm.tm_year:%d\n", (azure_iotdps_reg_tm.tm_year + 1900));

		/* parsing registered time month. */
		azure_iotdps_reg_create_time_loc = strstr(azure_iotdps_get_create_reg_time_tmp, "-");
		strncpy(azure_iotdps_get_create_reg_time_tmp, (azure_iotdps_get_create_reg_time_tmp+(azure_iotdps_reg_create_time_loc - azure_iotdps_get_create_reg_time_tmp+1)), azure_iotdps_parsing_reg_time_len- (azure_iotdps_reg_create_time_loc - azure_iotdps_get_create_reg_time_tmp));
//		azure_iotdps_reg_month = atoi(azure_iotdps_get_create_reg_time_tmp);
//		xprintf("### azure_iotdps_reg_month:%d ###\n", azure_iotdps_reg_month);
		azure_iotdps_reg_tm.tm_mon = (atoi(azure_iotdps_get_create_reg_time_tmp) - 1);
		//xprintf("*** azure_iotdps_reg_tm.tm_mon:%d ***\n", (azure_iotdps_reg_tm.tm_mon + 1));

		/* parsing registered time day. */
		azure_iotdps_reg_create_time_loc = strstr(azure_iotdps_get_create_reg_time_tmp, "-");
		strncpy(azure_iotdps_get_create_reg_time_tmp, (azure_iotdps_get_create_reg_time_tmp+(azure_iotdps_reg_create_time_loc - azure_iotdps_get_create_reg_time_tmp+1)), azure_iotdps_parsing_reg_time_len- (azure_iotdps_reg_create_time_loc - azure_iotdps_get_create_reg_time_tmp));
//		azure_iotdps_reg_day = atoi(azure_iotdps_get_create_reg_time_tmp);
//		xprintf("### azure_iotdps_reg_day:%d ###\n", azure_iotdps_reg_day);
		azure_iotdps_reg_tm.tm_mday = atoi(azure_iotdps_get_create_reg_time_tmp);
		//xprintf("*** azure_iotdps_reg_tm.tm_mday:%d ***\n", azure_iotdps_reg_tm.tm_mday);

		/* parsing registered time hour. */
		azure_iotdps_reg_create_time_loc = strstr(azure_iotdps_get_create_reg_time_tmp, "T");
		strncpy(azure_iotdps_get_create_reg_time_tmp, (azure_iotdps_get_create_reg_time_tmp+(azure_iotdps_reg_create_time_loc - azure_iotdps_get_create_reg_time_tmp+1)), azure_iotdps_parsing_reg_time_len- (azure_iotdps_reg_create_time_loc - azure_iotdps_get_create_reg_time_tmp));
//		azure_iotdps_reg_hour = atoi(azure_iotdps_get_create_reg_time_tmp);
//		xprintf("### azure_iotdps_reg_hour:%d ###\n", azure_iotdps_reg_hour);
		azure_iotdps_reg_tm.tm_hour = (atoi(azure_iotdps_get_create_reg_time_tmp) - 1);
		//xprintf("*** azure_iotdps_reg_tm.tm_hour:%d ***\n",azure_iotdps_reg_tm.tm_hour + 1);

		/* parsing registered time minute. */
		azure_iotdps_reg_create_time_loc = strstr(azure_iotdps_get_create_reg_time_tmp, ":");
		strncpy(azure_iotdps_get_create_reg_time_tmp, (azure_iotdps_get_create_reg_time_tmp+(azure_iotdps_reg_create_time_loc - azure_iotdps_get_create_reg_time_tmp+1)), azure_iotdps_parsing_reg_time_len- (azure_iotdps_reg_create_time_loc - azure_iotdps_get_create_reg_time_tmp));
//		azure_iotdps_reg_minute = atoi(azure_iotdps_get_create_reg_time_tmp);
//		xprintf("### azure_iotdps_reg_minute:%d ###\n", azure_iotdps_reg_minute);
		azure_iotdps_reg_tm.tm_min = (atoi(azure_iotdps_get_create_reg_time_tmp) - 1);
		//xprintf("*** azure_iotdps_reg_tm.tm_min:%d ***\n", (azure_iotdps_reg_tm.tm_min + 1));

		/* parsing registered time second. */
		azure_iotdps_reg_create_time_loc = strstr(azure_iotdps_get_create_reg_time_tmp, ":");
		strncpy(azure_iotdps_get_create_reg_time_tmp, (azure_iotdps_get_create_reg_time_tmp+(azure_iotdps_reg_create_time_loc - azure_iotdps_get_create_reg_time_tmp+1)), azure_iotdps_parsing_reg_time_len- (azure_iotdps_reg_create_time_loc - azure_iotdps_get_create_reg_time_tmp));
//		azure_iotdps_reg_sec = atoi(azure_iotdps_get_create_reg_time_tmp);
//		xprintf("### azure_iotdps_reg_sec:%d ###\n", azure_iotdps_reg_sec);
		azure_iotdps_reg_tm.tm_sec = (atoi(azure_iotdps_get_create_reg_time_tmp) - 1);
		//xprintf("*** azure_iotdps_reg_tm.tm_sec:%d ***\n", (azure_iotdps_reg_tm.tm_sec + 1));

		azure_iotdps_epoch_time = mktime(&azure_iotdps_reg_tm);
		get_time_flag = 1;
		unix_time_get(&current_time);
		//xprintf("*** azure_iotdps_epoch_time:%ld ***\n", azure_iotdps_epoch_time);
		//xprintf("*** current_time:%ld ***\n", current_time);

		/* Get assignedHub name. */
		azure_iotdps_reg_assignedhub_loc = strstr(recv_buf, "\"assignedHub\":");
		strncpy(azure_iotdps_reg_time_str, (recv_buf+(azure_iotdps_reg_assignedhub_loc - recv_buf+1)),(strlen(recv_buf) - (azure_iotdps_reg_assignedhub_loc - recv_buf)));
		//xprintf("**** azure_iotdps_reg_assignedHub_str:\n%d,%s ****\n\n",strlen(azure_iotdps_reg_time_str),azure_iotdps_reg_time_str);
		azure_iotdps_reg_deviceid_loc = strstr(azure_iotdps_reg_time_str, "\"deviceId\":");
		//xprintf("azure_iotdps_reg_deviceId_str_idx:%d\n",(azure_iotdps_reg_deviceid_loc-azure_iotdps_reg_time_str));
		memset(azure_iotdps_reg_assignedhub_tmp, 0, sizeof(azure_iotdps_reg_assignedhub_tmp));
		//20210803 strncpy(azure_iotdps_reg_assignedhub_tmp, (azure_iotdps_reg_time_str+(azure_iotdps_reg_assignedhub_loc - azure_iotdps_reg_time_str+14)),(azure_iotdps_reg_deviceid_loc - azure_iotdps_reg_time_str)-14);
		strncpy(azure_iotdps_reg_assignedhub_tmp, (azure_iotdps_reg_time_str+(azure_iotdps_reg_assignedhub_loc - azure_iotdps_reg_time_str+15)),(azure_iotdps_reg_deviceid_loc - azure_iotdps_reg_time_str)-16);
		//xprintf("**** azure_iotdps_reg_assignedhub_tmp:\n%s ****\n\n", azure_iotdps_reg_assignedhub_tmp);

		strcpy(azure_iothub_connect_host_name,azure_iotdps_reg_assignedhub_tmp);
		xprintf("**** azure_iothub_connect_host_name:\n%s ****\n", azure_iothub_connect_host_name);

		wifi_service_get_iothub_key(azure_iotdps_epoch_time, resource_dps_sas_token);

		return 1;
	}else{
		xprintf("\nWaiting azure_pnp_iotdps_get_registration_status...\n\n");
		return 0;
	}
}
#endif

/* Define Azure RTOS TLS info.  */
static NX_SECURE_X509_CERT root_ca_cert;
static UCHAR nx_azure_iot_tls_metadata_buffer[NX_AZURE_IOT_TLS_METADATA_BUFFER_SIZE];
static ULONG nx_azure_iot_thread_stack[NX_AZURE_IOT_STACK_SIZE / sizeof(ULONG)];

/* Define what the initial system looks like.  */
void    wifi_task_define(void *first_unused_memory)
{
	UINT  status;

    NX_PARAMETER_NOT_USED(first_unused_memory);

#if WIFI_EN
    xprintf("### WIFI Driver Initial... ###\n");
    dev_uart_comm = esp8266_init(DFSS_UART_0_ID, UART_BAUDRATE_115200);
    if(dev_uart_comm == NULL)
    {
    	xprintf("WIFI Initial Fail\n");
    	return;
    }
#endif

#ifdef NETWORK_EN
    /* Initialize the NetX system.  */
    nx_system_initialize();
#endif

    /* Create a packet pool.  */
    status = nx_packet_pool_create(&pool_0, "NetX Main Packet Pool", SAMPLE_PACKET_SIZE,
                                   (UCHAR *)sample_pool_stack , sample_pool_stack_size);
    /* Check for pool creation error.  */
    if (status)
    {
        xprintf("nx_packet_pool_create fail: %u\r\n", status);
        return;
    }

#ifdef NETWORK_EN
    /* Create an IP instance.  */
#if 1
//20210411 jason-
    status = nx_ip_create(&ip_0, "NetX IP Instance 0",
                          SAMPLE_IPV4_ADDRESS, SAMPLE_IPV4_MASK,
                          &pool_0, NULL,
                          (UCHAR*)sample_ip_stack, sizeof(sample_ip_stack),
                          SAMPLE_IP_THREAD_PRIORITY);
#else
    status = nx_ip_create(&ip_0, "NetX IP Instance 0", 0, 0, &pool_0, NULL, NULL, 0, 0);
#endif

    /* Check for IP create errors.  */
    if (status)
    {
        xprintf("nx_ip_create fail: %u\r\n", status);
        return;
    }
#endif

#ifdef NETWORK_EN
    /* Initialize TLS.  */
    nx_secure_tls_initialize();
#endif

    /* Create wifi service thread. */
    status = tx_thread_create(&wifi_service_thread, "wifi service Thread",
    							wifi_service_thread_entry, 0,
								wifi_service_thread_stack, WIFI_SERVICE_STACK_SIZE,
								WIFI_SERVICE_THREAD_PRIORITY, WIFI_SERVICE_THREAD_PRIORITY,
								TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Check status.  */
    if (status)
    {
        xprintf("wifi service thread creation fail: %u\r\n", status);
        return;
    }

    /* Create cis capture image thread. */
    status = tx_thread_create(&cis_capture_image_thread, "cis capture image Thread",
    							cis_capture_image_thread_entry, 0,
								cis_capture_image_thread_stack, CIS_CAPTURE_IMAGE_STACK_SIZE,
								CIS_CAPTURE_IMAGE_THREAD_PRIORITY, CIS_CAPTURE_IMAGE_THREAD_PRIORITY,
								TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Check status.  */
    if (status)
    {
        xprintf("CIS capture image thread creation fail: %u\r\n", status);
        return;
    }

    /* Create algo send result thread. */
    status = tx_thread_create(&algo_send_result_thread, "algo send result Thread",
    							algo_send_result_thread_entry, 0,
								algo_send_result_thread_stack, ALGO_SEND_RESULT_STACK_SIZE,
								ALGO_SEND_RESULT_THREAD_PRIORITY, ALGO_SEND_RESULT_THREAD_PRIORITY,
								TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Check status.  */
    if (status)
    {
        xprintf("algo send result thread creation fail: %u\r\n", status);
        return;
    }

}


/* Define the prototypes for Azure RTOS IoT.  */
UINT nbiot_service_iot_provisioning_client_initialize(ULONG expiry_time_secs)
{
	UINT status;
	UINT mqtt_user_name_length;
	NXD_MQTT_CLIENT *mqtt_client_ptr;
	NX_AZURE_IOT_RESOURCE *resource_ptr;
	UCHAR *buffer_ptr;
	UINT buffer_size;
	VOID *buffer_context;
	az_span endpoint_span = az_span_create((UCHAR *)ENDPOINT, (INT)sizeof(ENDPOINT) - 1);
	az_span id_scope_span = az_span_create((UCHAR *)ID_SCOPE, (INT)sizeof(ID_SCOPE) - 1);
	az_span registration_id_span = az_span_create((UCHAR *)REGISTRATION_ID, (INT)sizeof(REGISTRATION_ID) - 1);

    memset(&dps_client, 0, sizeof(NX_AZURE_IOT_PROVISIONING_CLIENT));
    /* Set resource pointer.  */
    resource_ptr = &(dps_client.nx_azure_iot_provisioning_client_resource);
    mqtt_client_ptr = &(dps_client.nx_azure_iot_provisioning_client_resource.resource_mqtt);

    dps_client.nx_azure_iot_ptr = &nx_azure_iot;
    dps_client.nx_azure_iot_provisioning_client_endpoint = (UCHAR *)ENDPOINT;
    dps_client.nx_azure_iot_provisioning_client_endpoint_length = sizeof(ENDPOINT) - 1;
    dps_client.nx_azure_iot_provisioning_client_id_scope = (UCHAR *)ID_SCOPE;
    dps_client.nx_azure_iot_provisioning_client_id_scope_length = sizeof(ID_SCOPE) - 1;
    dps_client.nx_azure_iot_provisioning_client_registration_id = (UCHAR *)REGISTRATION_ID;
    dps_client.nx_azure_iot_provisioning_client_registration_id_length = sizeof(REGISTRATION_ID) - 1;
    dps_client.nx_azure_iot_provisioning_client_resource.resource_crypto_array = _nx_azure_iot_tls_supported_crypto;
    dps_client.nx_azure_iot_provisioning_client_resource.resource_crypto_array_size = _nx_azure_iot_tls_supported_crypto_size;
    dps_client.nx_azure_iot_provisioning_client_resource.resource_cipher_map = _nx_azure_iot_tls_ciphersuite_map;
    dps_client.nx_azure_iot_provisioning_client_resource.resource_cipher_map_size = _nx_azure_iot_tls_ciphersuite_map_size;
    dps_client.nx_azure_iot_provisioning_client_resource.resource_metadata_ptr = nx_azure_iot_tls_metadata_buffer;
    dps_client.nx_azure_iot_provisioning_client_resource.resource_metadata_size = sizeof(nx_azure_iot_tls_metadata_buffer);
#ifdef NX_SECURE_ENABLE /*YUN*/
    dps_client.nx_azure_iot_provisioning_client_resource.resource_trusted_certificate = &root_ca_cert;
#endif
    dps_client.nx_azure_iot_provisioning_client_resource.resource_hostname = (UCHAR *)ENDPOINT;
    dps_client.nx_azure_iot_provisioning_client_resource.resource_hostname_length = sizeof(ENDPOINT) - 1;
    resource_ptr->resource_mqtt_client_id_length = dps_client.nx_azure_iot_provisioning_client_registration_id_length;
    resource_ptr->resource_mqtt_client_id = (UCHAR *)dps_client.nx_azure_iot_provisioning_client_registration_id;

    //expiry_time_secs = (ULONG)time(NULL);//(ULONG)azure_iotdps_epoch_time;
    dps_client.nx_azure_iot_provisioning_client_symmetric_key = (UCHAR *)DEVICE_SYMMETRIC_KEY;
    dps_client.nx_azure_iot_provisioning_client_symmetric_key_length = sizeof(DEVICE_SYMMETRIC_KEY) - 1;
    expiry_time_secs += NX_AZURE_IOT_PROVISIONING_CLIENT_TOKEN_EXPIRY;
    //xprintf("expiry_time_secs %ld\r\n", expiry_time_secs);

    if (az_result_failed(az_iot_provisioning_client_init(&(dps_client.nx_azure_iot_provisioning_client_core),
                                                         endpoint_span, id_scope_span,
                                                         registration_id_span, NULL)))
    {
    	xprintf("IoTProvisioning client initialize fail: failed to initialize core client\n");
        return(NX_AZURE_IOT_SDK_CORE_ERROR);
    }

    //buffer_size = 2048;
    status = nx_azure_iot_buffer_allocate(dps_client.nx_azure_iot_ptr,
                                          &buffer_ptr, &buffer_size, &buffer_context);
    //xprintf("buffer_size %d\n", buffer_size);
    if (status)
    {
    	xprintf("IoTProvisioning client failed initialization: BUFFER ALLOCATE FAIL\n");
        return(status);
    }

    /* Build user name.  */
    if (az_result_failed(az_iot_provisioning_client_get_user_name(&(dps_client.nx_azure_iot_provisioning_client_core),
                                                                  (CHAR *)buffer_ptr, buffer_size, &mqtt_user_name_length)))
    {
    	xprintf("IoTProvisioning client connect fail: NX_AZURE_IOT_Provisioning_CLIENT_USERNAME_SIZE is too small.\n");
        return(NX_AZURE_IOT_INSUFFICIENT_BUFFER_SPACE);
    }
    //xprintf("mqtt_user_name_length %d\n",mqtt_user_name_length);
    //xprintf("buffer_ptr %s\n",buffer_ptr);
    /* Save the resource buffer.  */
    resource_ptr -> resource_mqtt_buffer_context = buffer_context;
    resource_ptr -> resource_mqtt_buffer_size = buffer_size;
    resource_ptr -> resource_mqtt_user_name_length = mqtt_user_name_length;
    resource_ptr -> resource_mqtt_user_name = buffer_ptr;
    resource_ptr -> resource_mqtt_sas_token = buffer_ptr + mqtt_user_name_length;
    dps_client.nx_azure_iot_provisioning_client_sas_token_buff_size = buffer_size - mqtt_user_name_length;

    /* Link the resource.  */
    dps_client.nx_azure_iot_provisioning_client_resource.resource_data_ptr = &dps_client;
    dps_client.nx_azure_iot_provisioning_client_resource.resource_type = NX_AZURE_IOT_RESOURCE_IOT_PROVISIONING;
    nx_azure_iot_resource_add(&nx_azure_iot, &(dps_client.nx_azure_iot_provisioning_client_resource));
    return(NX_AZURE_IOT_SUCCESS);
}

static UINT nbiot_service_iot_hub_client_process_publish_packet(UCHAR *start_ptr,
                                                           ULONG *topic_offset_ptr,
                                                           USHORT *topic_length_ptr)
{
UCHAR *byte = start_ptr;
UINT byte_count = 0;
UINT multiplier = 1;
UINT remaining_length = 0;
UINT topic_length;

    /* Validate packet start contains fixed header.  */
    do
    {
        if (byte_count >= 4)
        {
            LogError(LogLiteralArgs("Invalid mqtt packet start position"));
            return(NX_AZURE_IOT_INVALID_PACKET);
        }

        byte++;
        remaining_length += (((*byte) & 0x7F) * multiplier);
        multiplier = multiplier << 7;
        byte_count++;
    } while ((*byte) & 0x80);

    if (remaining_length < 2)
    {
        return(NX_AZURE_IOT_INVALID_PACKET);
    }

    /* Retrieve topic length.  */
    byte++;
    topic_length = (UINT)(*(byte) << 8) | (*(byte + 1));

    if (topic_length > remaining_length - 2u)
    {
        return(NX_AZURE_IOT_INVALID_PACKET);
    }

    *topic_offset_ptr = (ULONG)((byte + 2) - start_ptr);
    *topic_length_ptr = (USHORT)topic_length;

    /* Return.  */
    return(NX_AZURE_IOT_SUCCESS);
}

static VOID nbiot_service_iot_hub_client_mqtt_receive_callback(NXD_MQTT_CLIENT* client_ptr,
                                                          UINT number_of_messages)
{
NX_AZURE_IOT_RESOURCE *resource = nx_azure_iot_resource_search(client_ptr);
NX_AZURE_IOT_HUB_CLIENT *hub_client_ptr = NX_NULL;
NX_PACKET *packet_ptr;
NX_PACKET *packet_next_ptr;
ULONG topic_offset;
USHORT topic_length;

    /* This function is protected by MQTT mutex.  */

    NX_PARAMETER_NOT_USED(number_of_messages);

    if (resource && (resource -> resource_type == NX_AZURE_IOT_RESOURCE_IOT_HUB))
    {
        hub_client_ptr = (NX_AZURE_IOT_HUB_CLIENT *)resource -> resource_data_ptr;
    }

    if (hub_client_ptr)
    {
        for (packet_ptr = client_ptr -> message_receive_queue_head;
             packet_ptr;
             packet_ptr = packet_next_ptr)
        {

            /* Store next packet in case current packet is consumed.  */
            packet_next_ptr = packet_ptr -> nx_packet_queue_next;

            /* Adjust packet to simply process logic.  */
            nx_azure_iot_mqtt_packet_adjust(packet_ptr);

            if (nbiot_service_iot_hub_client_process_publish_packet(packet_ptr -> nx_packet_prepend_ptr, &topic_offset,
                                                               &topic_length))
            {

                /* Message not supported. It will be released.  */
                nx_packet_release(packet_ptr);
                continue;
            }

            if ((topic_offset + topic_length) >
                (ULONG)(packet_ptr -> nx_packet_append_ptr - packet_ptr -> nx_packet_prepend_ptr))
            {

                /* Only process topic in the first packet since the fixed topic is short enough to fit into one packet.  */
                topic_length = (USHORT)(((ULONG)(packet_ptr -> nx_packet_append_ptr - packet_ptr -> nx_packet_prepend_ptr) -
                                         topic_offset) & 0xFFFF);
            }

            if (hub_client_ptr -> nx_azure_iot_hub_client_direct_method_message.message_process &&
                (hub_client_ptr -> nx_azure_iot_hub_client_direct_method_message.message_process(hub_client_ptr, packet_ptr,
                                                                                                 topic_offset,
                                                                                                 topic_length) == NX_AZURE_IOT_SUCCESS))
            {

                /* Direct method message is processed.  */
                continue;
            }

            if (hub_client_ptr -> nx_azure_iot_hub_client_c2d_message.message_process &&
                (hub_client_ptr -> nx_azure_iot_hub_client_c2d_message.message_process(hub_client_ptr, packet_ptr,
                                                                                       topic_offset,
                                                                                       topic_length) == NX_AZURE_IOT_SUCCESS))
            {

                /* Could to Device message is processed.  */
                continue;
            }

            if ((hub_client_ptr -> nx_azure_iot_hub_client_device_twin_message.message_process) &&
                (hub_client_ptr -> nx_azure_iot_hub_client_device_twin_message.message_process(hub_client_ptr,
                                                                                               packet_ptr, topic_offset,
                                                                                               topic_length) == NX_AZURE_IOT_SUCCESS))
            {

                /* Device Twin message is processed.  */
                continue;
            }

            /* Message not supported. It will be released.  */
            nx_packet_release(packet_ptr);
        }

        /* Clear all message from MQTT receive queue.  */
        client_ptr -> message_receive_queue_head = NX_NULL;
        client_ptr -> message_receive_queue_tail = NX_NULL;
        client_ptr -> message_receive_queue_depth = 0;
    }
}

UINT nbiot_service_iot_hub_client_initialize(NX_AZURE_IOT_HUB_CLIENT* hub_client_ptr,
                                        NX_AZURE_IOT *nx_azure_iot_ptr,
                                        const UCHAR *host_name, UINT host_name_length,
                                        const UCHAR *device_id, UINT device_id_length,
                                        const UCHAR *module_id, UINT module_id_length,
                                        const NX_CRYPTO_METHOD **crypto_array, UINT crypto_array_size,
                                        const NX_CRYPTO_CIPHERSUITE **cipher_map, UINT cipher_map_size,
                                        UCHAR * metadata_memory, UINT memory_size
#ifdef NX_SECURE_ENABLE /*YUN*/
										,NX_SECURE_X509_CERT *trusted_certificate
#endif
										)
{
UINT status;
NX_AZURE_IOT_RESOURCE *resource_ptr;
az_span hostname_span = az_span_create((UCHAR *)host_name, (INT)host_name_length);
az_span device_id_span = az_span_create((UCHAR *)device_id, (INT)device_id_length);
az_iot_hub_client_options options = az_iot_hub_client_options_default();
az_result core_result;

    if ((nx_azure_iot_ptr == NX_NULL) || (hub_client_ptr == NX_NULL) || (host_name == NX_NULL) ||
        (device_id == NX_NULL) || (host_name_length == 0) || (device_id_length == 0))
    {
        LogError(LogLiteralArgs("IoTHub client initialization fail: INVALID POINTER"));
        return(NX_AZURE_IOT_INVALID_PARAMETER);
    }

    memset(hub_client_ptr, 0, sizeof(NX_AZURE_IOT_HUB_CLIENT));

    hub_client_ptr -> nx_azure_iot_ptr = nx_azure_iot_ptr;
    hub_client_ptr -> nx_azure_iot_hub_client_resource.resource_crypto_array = crypto_array;
    hub_client_ptr -> nx_azure_iot_hub_client_resource.resource_crypto_array_size = crypto_array_size;
    hub_client_ptr -> nx_azure_iot_hub_client_resource.resource_cipher_map = cipher_map;
    hub_client_ptr -> nx_azure_iot_hub_client_resource.resource_cipher_map_size = cipher_map_size;
    hub_client_ptr -> nx_azure_iot_hub_client_resource.resource_metadata_ptr = metadata_memory;
    hub_client_ptr -> nx_azure_iot_hub_client_resource.resource_metadata_size = memory_size;
#ifdef NX_SECURE_ENABLE /*YUN*/
    hub_client_ptr -> nx_azure_iot_hub_client_resource.resource_trusted_certificate = trusted_certificate;
#endif
    hub_client_ptr -> nx_azure_iot_hub_client_resource.resource_hostname = host_name;
    hub_client_ptr -> nx_azure_iot_hub_client_resource.resource_hostname_length = host_name_length;
    options.module_id = az_span_create((UCHAR *)module_id, (INT)module_id_length);
    options.user_agent = AZ_SPAN_FROM_STR(NX_AZURE_IOT_HUB_CLIENT_USER_AGENT);

    core_result = az_iot_hub_client_init(&hub_client_ptr -> iot_hub_client_core,
                                         hostname_span, device_id_span, &options);
    if (az_result_failed(core_result))
    {
        LogError(LogLiteralArgs("IoTHub client failed initialization with error status: %d"), core_result);
        return(NX_AZURE_IOT_SDK_CORE_ERROR);
    }

    /* Set resource pointer.  */
    resource_ptr = &(hub_client_ptr -> nx_azure_iot_hub_client_resource);

    /* Link the resource.  */
    resource_ptr -> resource_data_ptr = (VOID *)hub_client_ptr;
    resource_ptr -> resource_type = NX_AZURE_IOT_RESOURCE_IOT_HUB;
    nx_azure_iot_resource_add(nx_azure_iot_ptr, resource_ptr);


    return(NX_AZURE_IOT_SUCCESS);
}

static UINT nbiot_service_dps_entry(void)
{
	UINT status;
	ULONG expiry_time_secs;
/* 20210310 jason+*/
#ifdef ENABLE_AZURE_PORTAL_DPS_HUB_DEVICE
#ifndef AZURE_PNP_CERTIFICATION_EN //20210330 jaosn+
	UCHAR *iothub_hostname = (UCHAR *)HOST_NAME;
	UINT iothub_hostname_length = sizeof(HOST_NAME) - 1;
#else
	UCHAR *iothub_hostname = (UCHAR *)ENDPOINT;
	UINT iothub_hostname_length = sizeof(ENDPOINT) - 1;
#endif
#else
	UCHAR *iothub_hostname = (UCHAR *)ENDPOINT;
	UINT iothub_hostname_length = sizeof(ENDPOINT) - 1;
#endif

	UCHAR *iothub_device_id = (UCHAR *)AZURE_IOTHUB_DEVICE_ID;
	UINT iothub_device_id_length = sizeof(AZURE_IOTHUB_DEVICE_ID) - 1;
	expiry_time_secs = (ULONG)time(NULL);

	//xprintf("Create Azure IoT handler...\r\n");
	/* Create Azure IoT handler.  */
	if ((status = nx_azure_iot_create(&nx_azure_iot, (UCHAR *)"Azure IoT", &ip_0, &pool_0, &dns_0,
									  nx_azure_iot_thread_stack, sizeof(nx_azure_iot_thread_stack),
									  NX_AZURE_IOT_THREAD_PRIORITY, unix_time_get)))
	{
		xprintf("Failed on nx_azure_iot_create!: error code = 0x%08x\r\n", status);
		return(NX_AZURE_IOT_SDK_CORE_ERROR);
	}

    //xprintf("Start Provisioning Client...\r\n");

    /* Initialize IoT provisioning client.  */
    if (status = nbiot_service_iot_provisioning_client_initialize(expiry_time_secs))
    {
        xprintf("Failed on nx_azure_iot_provisioning_client_initialize!: error code = 0x%08x\r\n", status);
        return(status);
    }

    /* Initialize IoTHub client.  */
    if ((status = nbiot_service_iot_hub_client_initialize(&hub_client, &nx_azure_iot,
                                                     iothub_hostname, iothub_hostname_length,
                                                     iothub_device_id, iothub_device_id_length,
                                                     (UCHAR *)MODEL_ID, sizeof(MODEL_ID) - 1,
                                                     _nx_azure_iot_tls_supported_crypto,
                                                     _nx_azure_iot_tls_supported_crypto_size,
                                                     _nx_azure_iot_tls_ciphersuite_map,
                                                     _nx_azure_iot_tls_ciphersuite_map_size,
                                                     nx_azure_iot_tls_metadata_buffer,
                                                     sizeof(nx_azure_iot_tls_metadata_buffer),
                                                     &root_ca_cert)))
    {
        xprintf("Failed on nx_azure_iot_hub_client_initialize!: error code = 0x%08x\r\n", status);
        return(status);
    }

    return(status);
}

INT wifi_service_get_dps_key(ULONG expiry_time_secs, UCHAR *resource_dps_sas_token)
{
		UINT status;
		NX_AZURE_IOT_RESOURCE *resource_ptr;
		UCHAR *output_ptr;
		UINT output_len;
		az_span span;
		az_result core_result;
		az_span buffer_span;
		az_span policy_name = AZ_SPAN_LITERAL_FROM_STR(NX_AZURE_IOT_PROVISIONING_CLIENT_POLICY_NAME);

	    /* Set resource pointer.  */
	    resource_ptr = &(dps_client.nx_azure_iot_provisioning_client_resource);

	    span = az_span_create(resource_ptr->resource_mqtt_sas_token,
	                          (INT)dps_client.nx_azure_iot_provisioning_client_sas_token_buff_size);

	    status = nx_azure_iot_buffer_allocate(dps_client.nx_azure_iot_ptr,
	                                          &buffer_ptr, &buffer_size,
	                                          &buffer_context);
	    //xprintf("nx_azure_iot_buffer_allocate: BUFFER size %d\r\n", buffer_size);
	    if (status)
	    {
	        xprintf("IoTProvisioning client sas token fail: BUFFER ALLOCATE FAI\r\n");
	        return(status);
	    }

	    //xprintf("expiry_time_secs %ld\r\n", expiry_time_secs);

	    core_result = az_iot_provisioning_client_sas_get_signature(&(dps_client.nx_azure_iot_provisioning_client_core),
	                                                               expiry_time_secs, span, &span);
	    //xprintf("az_iot_provisioning_client_sas_get_signature\r\n");
	    if (az_result_failed(core_result))
	    {
	        xprintf("IoTProvisioning failed failed to get signature with error status: %d\r\n", core_result);
	        return(NX_AZURE_IOT_SDK_CORE_ERROR);
	    }
	    //xprintf("prov_client.nx_azure_iot_provisioning_client_symmetric_key %s\r\n", dps_client.nx_azure_iot_provisioning_client_symmetric_key);
	    //xprintf("prov_client.nx_azure_iot_provisioning_client_symmetric_key_length %d\r\n", dps_client.nx_azure_iot_provisioning_client_symmetric_key_length);

	    status = nx_azure_iot_base64_hmac_sha256_calculate(resource_ptr,
	    												   dps_client.nx_azure_iot_provisioning_client_symmetric_key,
														   dps_client.nx_azure_iot_provisioning_client_symmetric_key_length,
	                                                       az_span_ptr(span), (UINT)az_span_size(span), buffer_ptr, buffer_size,
	                                                       &output_ptr, &output_len);
	    //xprintf("output_ptr %s\r\n", output_ptr);
	    if (status)
	    {
	        xprintf("IoTProvisioning failed to encoded hash\r\n");
	        return(status);
	    }

	    buffer_span = az_span_create(output_ptr, (INT)output_len);

	    //xprintf("11expiry_time_secs %ld\r\n", expiry_time_secs);

	    core_result = az_iot_provisioning_client_sas_get_password(&(dps_client.nx_azure_iot_provisioning_client_core),
	                                                              buffer_span, expiry_time_secs, policy_name,
	                                                              (CHAR *)resource_ptr -> resource_mqtt_sas_token,
																  dps_client.nx_azure_iot_provisioning_client_sas_token_buff_size,
	                                                              &(resource_ptr -> resource_mqtt_sas_token_length));
	    if (az_result_failed(core_result))
	    {
	        xprintf("IoTProvisioning failed to generate token with error : %d\r\n", core_result);
	        return(NX_AZURE_IOT_SDK_CORE_ERROR);
	    }
	    //xprintf("resource_mqtt_sas_token %s\n",resource_ptr -> resource_mqtt_sas_token);
	    //strcpy(azure_iotdps_connect_password, resource_ptr -> resource_mqtt_sas_token);
	    strcpy(azure_iotdps_connect_password,"\"");
	    strcat(azure_iotdps_connect_password, resource_ptr -> resource_mqtt_sas_token);
	    strcat(azure_iotdps_connect_password, "\"");
	   	xprintf("\n*** azure_iotdps_connect_password:%s ***\n", azure_iotdps_connect_password);

	   	resource_dps_sas_token = resource_ptr -> resource_mqtt_sas_token;
	    return(NX_AZURE_IOT_SUCCESS);
}

static UINT nbiot_service_iot_hub_client_sas_token_get(NX_AZURE_IOT_HUB_CLIENT *hub_client_ptr,
                                                  ULONG expiry_time_secs, const UCHAR *key, UINT key_len,
                                                  UCHAR *sas_buffer, UINT sas_buffer_len, UINT *sas_length)
{
UCHAR *buffer_ptr;
UINT buffer_size;
VOID *buffer_context;
az_span span = az_span_create(sas_buffer, (INT)sas_buffer_len);
az_span buffer_span;
UINT status;
UCHAR *output_ptr;
UINT output_len;
az_result core_result;

    status = nx_azure_iot_buffer_allocate(hub_client_ptr -> nx_azure_iot_ptr, &buffer_ptr, &buffer_size, &buffer_context);
    if (status)
    {
        LogError(LogLiteralArgs("IoTHub client sas token fail: BUFFER ALLOCATE FAIL"));
        return(status);
    }

    core_result = az_iot_hub_client_sas_get_signature(&(hub_client_ptr -> iot_hub_client_core),
                                                      expiry_time_secs, span, &span);
    if (az_result_failed(core_result))
    {
        LogError(LogLiteralArgs("IoTHub failed failed to get signature with error status: %d"), core_result);
        nx_azure_iot_buffer_free(buffer_context);
        return(NX_AZURE_IOT_SDK_CORE_ERROR);
    }

    status = nx_azure_iot_base64_hmac_sha256_calculate(&(hub_client_ptr -> nx_azure_iot_hub_client_resource),
                                                       key, key_len, az_span_ptr(span), (UINT)az_span_size(span),
                                                       buffer_ptr, buffer_size, &output_ptr, &output_len);
    if (status)
    {
        LogError(LogLiteralArgs("IoTHub failed to encoded hash"));
        nx_azure_iot_buffer_free(buffer_context);
        return(status);
    }

    buffer_span = az_span_create(output_ptr, (INT)output_len);
    core_result= az_iot_hub_client_sas_get_password(&(hub_client_ptr -> iot_hub_client_core),
                                                    expiry_time_secs, buffer_span, AZ_SPAN_EMPTY,
                                                    (CHAR *)sas_buffer, sas_buffer_len, &sas_buffer_len);
    if (az_result_failed(core_result))
    {
        LogError(LogLiteralArgs("IoTHub failed to generate token with error status: %d"), core_result);
        nx_azure_iot_buffer_free(buffer_context);
        return(NX_AZURE_IOT_SDK_CORE_ERROR);
    }

    *sas_length = sas_buffer_len;
    nx_azure_iot_buffer_free(buffer_context);

    return(NX_AZURE_IOT_SUCCESS);
}

INT wifi_service_get_iothub_key(ULONG expiry_time_secs, UCHAR *resource_dps_sas_token)
//INT wifi_service_get_iothub_key(ULONG expiry_time_secs, UCHAR *resource_dps_sas_token,UCHAR *host_name )
{
	UINT            status;
	NXD_ADDRESS     server_address;
	NX_AZURE_IOT_RESOURCE *resource_ptr;
	NXD_MQTT_CLIENT *mqtt_client_ptr;
	UCHAR           *buffer_ptr;
	UINT            buffer_size;
	VOID            *buffer_context;
	UINT            buffer_length;
	az_result       core_result;

    /* Allocate buffer for client id, username and sas token.  */
    status = nx_azure_iot_buffer_allocate(hub_client.nx_azure_iot_ptr,
                                          &buffer_ptr, &buffer_size, &buffer_context);
    if (status)
    {
    	xprintf("IoTHub client failed initialization: BUFFER ALLOCATE FAIL\n");
        return(status);
    }

    /* Set resource pointer and buffer context.  */
    resource_ptr = &(hub_client.nx_azure_iot_hub_client_resource);

    /* Build client id.  */
    buffer_length = buffer_size;
    core_result = az_iot_hub_client_get_client_id(&(hub_client.iot_hub_client_core),
                                                  (CHAR *)buffer_ptr, buffer_length, &buffer_length);
    if (az_result_failed(core_result))
    {
        nx_azure_iot_buffer_free(buffer_context);
        xprintf("IoTHub client failed to get clientId with error status: \n", core_result);
        return(NX_AZURE_IOT_SDK_CORE_ERROR);
    }
    resource_ptr -> resource_mqtt_client_id = buffer_ptr;
    resource_ptr -> resource_mqtt_client_id_length = buffer_length;

    /* Update buffer for user name.  */
    buffer_ptr += resource_ptr -> resource_mqtt_client_id_length;
    buffer_size -= resource_ptr -> resource_mqtt_client_id_length;

    /* Build user name.  */
    buffer_length = buffer_size;
    core_result = az_iot_hub_client_get_user_name(&hub_client.iot_hub_client_core,
                                                  (CHAR *)buffer_ptr, buffer_length, &buffer_length);
    if (az_result_failed(core_result))
    {
        nx_azure_iot_buffer_free(buffer_context);
        xprintf("IoTHub client connect fail, with error status: %d\n", core_result);
        return(NX_AZURE_IOT_SDK_CORE_ERROR);
    }
    resource_ptr -> resource_mqtt_user_name = buffer_ptr;
    resource_ptr -> resource_mqtt_user_name_length = buffer_length;

    /* Build sas token.  */
    resource_ptr -> resource_mqtt_sas_token = buffer_ptr + buffer_length;
    resource_ptr -> resource_mqtt_sas_token_length = buffer_size - buffer_length;

    hub_client.nx_azure_iot_hub_client_symmetric_key = (UCHAR *)DEVICE_SYMMETRIC_KEY;
    hub_client.nx_azure_iot_hub_client_symmetric_key_length = sizeof(DEVICE_SYMMETRIC_KEY) - 1;

    /* Host Name. 20210309 jason+*/
    hub_client.iot_hub_client_core._internal.iot_hub_hostname._internal.ptr = (UCHAR *)azure_iothub_connect_host_name;
    hub_client.iot_hub_client_core._internal.iot_hub_hostname._internal.size = strlen(azure_iothub_connect_host_name) - 1;

    xprintf("\n*** iot_hub_client_iothub_host_name:%s ***\n", hub_client.iot_hub_client_core._internal.iot_hub_hostname._internal.ptr);

    expiry_time_secs += NX_AZURE_IOT_HUB_CLIENT_TOKEN_EXPIRY;
    status = nbiot_service_iot_hub_client_sas_token_get(&hub_client,
                                                       expiry_time_secs,
                                                       hub_client.nx_azure_iot_hub_client_symmetric_key,
                                                       hub_client.nx_azure_iot_hub_client_symmetric_key_length,
                                                       resource_ptr -> resource_mqtt_sas_token,
                                                       resource_ptr -> resource_mqtt_sas_token_length,
                                                       &(resource_ptr -> resource_mqtt_sas_token_length));
    if (status)
    {
        nx_azure_iot_buffer_free(buffer_context);
        xprintf("IoTHub client connect fail: Token generation failed status: %d", status);
        return(status);
    }
    resource_dps_sas_token = resource_ptr -> resource_mqtt_sas_token;
    xprintf("resource_mqtt_sas_token %s\n",resource_ptr -> resource_mqtt_sas_token);
    //strcpy(azure_iothub_connect_password, resource_ptr -> resource_mqtt_sas_token);
    strcpy(azure_iothub_connect_password,"\"");
    strcat(azure_iothub_connect_password,resource_ptr -> resource_mqtt_sas_token);
    strcat(azure_iothub_connect_password,"\"");
    xprintf("\n*** azure_iothub_connect_password:%s ***\n", azure_iothub_connect_password);

    return(NX_AZURE_IOT_SUCCESS);
}

/* Define wifi service thread entry.  */
void wifi_service_thread_entry(ULONG parameter)
{
	xprintf("### Start wifi_service_thread ###\n");

	//ULONG expiry_time_secs = 0;
	//UCHAR *resource_dps_sas_token = NULL;
	//TX_INTERRUPT_SAVE_AREA
	unsigned int wifi_atcmd_retry_cnt = 0;

	nbiot_service_dps_entry();
	//expiry_time_secs = (ULONG)time(NULL);
	//wifi_service_get_dps_key(expiry_time_secs, resource_dps_sas_token);
	//wifi_service_get_iothub_key(expiry_time_secs, resource_dps_sas_token);

	TX_INTERRUPT_SAVE_AREA

#if DEBUG_TEST
	xprintf("###### azure_iotdps_parsing_network_time ######\n");
	if (0> azure_iotdps_parsing_network_time()){
		xprintf("### azure_iotdps_parsing_network_time fail.. ###\n");
	}
#endif // #if DEBUG_TEST

#if WIFI_EN
{
	/* ESP8266 Network Initial configuration.
	 * 99: AT TEST
	 * 0: SET STA MODE
	 * 1: SET CONNECT "SSID","PASSWORD"
	 * 2: QUERY IP ADDRESS
	 * 3: QUERY NTP NETWORK TIME
	 * 4: WIFI INITIAL DONE
	 *
	 * */
	 static int  esp8266_network_setup_event = AT_CMD_TEST;
	 while(1){
		 memset(recv_buf,0,AT_MAX_LEN);//clear buffer
		 switch(esp8266_network_setup_event){
		 case AT_CMD_TEST:
			 /* AT CMD Test.*/
			 if(esp8266_test(dev_uart_comm, recv_buf, recv_len) == AT_OK){
				 xprintf("### ESP8266 AT TEST OK!! ###\n");
				 esp8266_network_setup_event = WIFI_ESP8266_SET_MODE_CFG;
			 }else{
				 xprintf("### ESP8266 AT TEST FAIL.. ###\n");
			 }
		 break;
		 case WIFI_ESP8266_SET_MODE_CFG:
			 /* SET STA MODE. */
			 if( esp8266_wifi_mode_set(dev_uart_comm, "1", recv_buf, recv_len) == AT_OK){
				 xprintf("### SET STA MODE OK!! ###\n");
				 esp8266_network_setup_event = WIFI_ESP8266_SSID_PW_CFG;
			 }else{
				 xprintf("### SET STA MODE FAIL.. ###\n");
			 }
		 break;
		 case WIFI_ESP8266_SSID_PW_CFG:
			 /* SET CONNECT "SSID","PASSWORD". */
			 if(esp8266_wifi_connect(dev_uart_comm, "\"" WIFI_SSID"\"" , "\"" WIFI_PW"\"", recv_buf, recv_len) == AT_OK){
				 xprintf("### WIFI CONNECTE to AP OK!! ###\n");
				 esp8266_network_setup_event = WIFI_ESP8266_QUERY_IP_ADDRESS_CFG;
			 }else{
				 xprintf("### WIFI CONNECTE to AP FAIL.. ###\n");
			 }
		 break;
		 case WIFI_ESP8266_QUERY_IP_ADDRESS_CFG:
			 /*QUERY IP ADDRESS. */
			 if(esp8266_address_get(dev_uart_comm, recv_buf, recv_len) == AT_OK){
				 xprintf("### GET IP ADDRESS OK!! ###\n");
#if !AZURE_DEVICE_PROVISIONING_SERVICE_EN
				 esp8266_network_setup_event = WIFI_ESP8266_QUERY_NTP_NETWORK_TIME_CFG;
#else
				 esp8266_network_setup_event = WIFI_ESP8266_NETWORK_INIIAL_DONE;
#endif //#if DEBUG_TEST
			 }else{
				 xprintf("### GET IP ADDRESS FAIL.. ###\n");
			 }
		 break;
#if !AZURE_DEVICE_PROVISIONING_SERVICE_EN
		 case WIFI_ESP8266_QUERY_NTP_NETWORK_TIME_CFG:
			 /*QUERY NTP NETWORK TIME. */
			 if( esp8266_set_ntp_time(dev_uart_comm, NTP_TIMEZONE, "\""NTP_DOMAIN"\"", recv_buf, recv_len) == AT_OK){
				 xprintf("### SET NTP NETWORK TIME CFG OK!! ###\n");
				 xprintf("### GET NTP NETWORK TIME.. ###\n");
				 if( esp8266_get_ntp_time(dev_uart_comm, recv_buf, recv_len) == AT_OK){
					 xprintf("### GET NTP NETWORK OK!! ###\n");

					 azure_iotdps_parsing_network_time();//gen iothub sas token
					 esp8266_network_setup_event = WIFI_ESP8266_NETWORK_INIIAL_DONE;

				 }else{
					 xprintf("### GET NTP NETWORK TIME FAIL.. ###\n");
				 }

			 }else{
				 xprintf("### SET NTP NETWORK TIME CFGFAIL.. ###\n");
			 }
		 break;
#endif //#if DEBUG_TEST
		 }

		 if(esp8266_network_setup_event == WIFI_ESP8266_NETWORK_INIIAL_DONE){
			 xprintf("### ESP8266 Network setting Setup ok!! ###\n");
			 break;
		 }
		tx_thread_sleep(7000);//400ms = (sleep_ms/3500)*200ms
	 }//while
}
#endif/* #if WIFI_EN */


#ifdef NETWORK_EN

	/* Azure PNP DPS Event initial. */
	azure_pnp_iotdps_event = PNP_IOTDPS_INITIAL;
	/* Azure PNP IoTHub Event initial. */
	azure_pnp_iothub_event = PNP_IOTHUB_WIFI_IDLE;

	/*"ID_SCOPE/registrations/REGISTRATION_ID/api-version=2019-03-31"*/
	azure_iotdps_connect_user_name ="\"" ID_SCOPE "/registrations/" REGISTRATION_ID
		                                           "/api-version=" AZURE_IOTDPS_SERVICE_VERSION "\"";
	xprintf("**** azure_iotdps_connect_user_name: %s ****\n",azure_iotdps_connect_user_name);

	/* {"registrationId":REGISTRATION_ID,"payload":{"modelId":MODEL_ID}}*/
	char azure_iotdps_registrations_msg_str[256] = "{\"registrationId\":\"" REGISTRATION_ID "\""
			",\"payload\":{\"modelId\":\"" MODEL_ID "\"}}";
	//xprintf("**** azure_iotdps_registrations_msg_str:\n%s ****\n\n",azure_iotdps_registrations_msg_str);

	//20210726 sprintf(azure_registrations_msg_len, "%d", strlen(azure_iotdps_registrations_msg_str));
	sprintf(azure_registrations_msg_len, "%d", (strlen(azure_iotdps_registrations_msg_str)+1));
	strcpy(azure_iotdps_registrations_msg,azure_registrations_msg_len);
	//strncat(azure_iotdps_registrations_msg,"\r\n",2);
	//xprintf("****len azure_iotdps_registrations_msg:\n%s ****\n\n",azure_iotdps_registrations_msg);
	strncat(azure_iotdps_registrations_msg,azure_iotdps_registrations_msg_str,strlen(azure_iotdps_registrations_msg_str));
	//xprintf("**** azure_iotdps_registrations_msg:\n%s ****\n\n",azure_iotdps_registrations_msg);

#if AZURE_DEVICE_PROVISIONING_SERVICE_EN //DPS
	xprintf("\n#############################################################################\n");
	xprintf("**** Enter Azure DPS Connect... ****\n");
	xprintf("#############################################################################\n");

#if 0
	/* Azure DPS　single Event Test for debug. */
	azure_pnp_iotdps_event = PNP_IOTDPS_GET_REGISTRATION_STATUS;
#endif

	while(1){
		memset(recv_buf,0,AT_MAX_LEN);//clear buffer
		switch(azure_pnp_iotdps_event)
		{
		/* WIFI Query/check IP Address. */
		case PNP_IOTDPS_INITIAL:
			xprintf("### GET IP ADDRESS.. ###\n");
			if(esp8266_address_get(dev_uart_comm, recv_buf, recv_len) == AT_OK){
				xprintf("### GET IP ADDRESS OK!! ###\n");
				azure_pnp_iotdps_event = PNP_IOTDPS_SET_NETWORK_TIME;
			}else{
				xprintf("### GET IP ADDRESS FAIL.. ###\n");
			}
		break;
		/* WIFI　Set　NTP Network time. */
		case PNP_IOTDPS_SET_NETWORK_TIME:
			 if( esp8266_set_ntp_time(dev_uart_comm, NTP_TIMEZONE, "\""NTP_DOMAIN"\"", recv_buf, recv_len) == AT_OK){
				 xprintf("### SET NTP NETWORK TIME CFG OK!! ###\n");
				 wifi_atcmd_retry_cnt = 0;
				 azure_pnp_iotdps_event = PNP_IOTDPS_GET_NETWORK_TIME;
			 }else{
				 ++wifi_atcmd_retry_cnt;
				 xprintf("### SET NTP NETWORK TIME CFGFAIL.. ###\n");
			 }
		break;
		/* WIFI　Get　NTP Network time. */
		case PNP_IOTDPS_GET_NETWORK_TIME:
			 if( esp8266_get_ntp_time(dev_uart_comm, recv_buf, recv_len) == AT_OK){
				 xprintf("### GET NTP NETWORK OK!! ###\n");
				 azure_iotdps_parsing_network_time();
				 azure_pnp_iotdps_event	= PNP_IOTDPS_USER_CFG;
				 wifi_atcmd_retry_cnt	= 0;
			 }else{
				 ++wifi_atcmd_retry_cnt;
				 xprintf("### GET NTP NETWORK TIME FAIL.. ###\n");
			 }
		break;
		/*WIFI MQTTUSERCFG. */
		case PNP_IOTDPS_USER_CFG:
			xprintf("SET MQTT USER CONFIG..\n");
			if(esp8266_mqtt_set_user_cfg(dev_uart_comm, "2", recv_buf, recv_len)== AT_OK)
			{
				xprintf("WIFI MQTT USER CONFIG OK!!\n");
				azure_pnp_iotdps_event	= PNP_IOTDPS_USER_CLIENT_ID;
				wifi_atcmd_retry_cnt	= 0;
			}else{
				++wifi_atcmd_retry_cnt;
				xprintf("WIFI MQTT USER CONFIG FAIL..\n");
			}
		break;
		/* WIFI MQTTCLIENTID. */
		case PNP_IOTDPS_USER_CLIENT_ID:
			xprintf("SET MQTT CLIENT ID..\n");
			if( esp8266_mqtt_set_user_client_id(dev_uart_comm, "\""REGISTRATION_ID"\"", recv_buf, recv_len)== AT_OK){
				xprintf("SET MQTT CLIENT ID OK!!\n");
				azure_pnp_iotdps_event	= PNP_IOTDPS_USER_NAME;
				wifi_atcmd_retry_cnt	= 0;
			}else{
				++wifi_atcmd_retry_cnt;
				xprintf("SET MQTT CLIENT ID FAIL..\n");
			}
		break;
		/*WIFI MQTTUSERNAME. */
		case PNP_IOTDPS_USER_NAME:
			xprintf("SET MQTT USER NAME..\n");
			if( esp8266_mqtt_set_user_name(dev_uart_comm, azure_iotdps_connect_user_name, recv_buf, recv_len)== AT_OK)
			{
				xprintf("SET MQTT USER NAME OK!!\n");
				azure_pnp_iotdps_event	= PNP_IOTDPS_USER_PASSWORD;
				wifi_atcmd_retry_cnt	= 0;
			}else{
				++wifi_atcmd_retry_cnt;
				xprintf("SET MQTT USER NAME FAIL..\n");
			}
		break;
		/*WIFI MQTTPASSWORD. */
		case PNP_IOTDPS_USER_PASSWORD:
			xprintf("SET MQTT USERR PASSWORD..\n");
			if(esp8266_mqtt_set_user_password(dev_uart_comm, azure_iotdps_connect_password, recv_buf, recv_len) == AT_OK)
			{
				xprintf("SET MQTT USERR PASSWORD OK!!\n");
				azure_pnp_iotdps_event	= PNP_IOTDPS_CONNECT_TO_DPS;
				wifi_atcmd_retry_cnt	= 0;
			}else{
				++wifi_atcmd_retry_cnt;
				xprintf("SET MQTT USERR PASSWORD FAIL..\n");
			}
		break;
		/*WIFI MQTTCONN. */
		case PNP_IOTDPS_CONNECT_TO_DPS:
			xprintf("WIFI CONNECT to IOTDPS SERVICE..\n");
			if( esp8266_mqtt_conn(dev_uart_comm, "\""ENDPOINT"\"", "8883", recv_buf, recv_len)== AT_OK)
			{
				xprintf("WIFI CONNECT to IOTDPS SERVICEE OK!!\n");
				azure_pnp_iotdps_event	= PNP_IOTDPS_REGISTRATION_DONE;
				wifi_atcmd_retry_cnt	= 0;
			}else{
				++wifi_atcmd_retry_cnt;
				xprintf("WIFI CONNECT to IOTDPS SERVICE FAIL..\n");
			}
		break;
		/* DPS Registration Done. */
		case PNP_IOTDPS_REGISTRATION_DONE:
			xprintf("WIFI DPS Connect OK!!\n");
			wifi_atcmd_retry_cnt = 0;
		break;
		}//switch case

		/* MQTT DPS Re-Connect. */
		if(wifi_atcmd_retry_cnt == WIFI_ATCMD_RETRY_MAX_TIMES){
			wifi_atcmd_retry_cnt = 0;
			azure_pnp_iotdps_event = PNP_IOTDPS_RECONNECT;
		}

		tx_thread_sleep(9000);//600ms
	}//while
#endif /* #if AZURE_DEVICE_PROVISIONING_SERVICE_EN  */

	xprintf("\n#############################################################################\n");
	xprintf("**** Enter Azure IoTHUB Device Connect... ****\n");
	xprintf("#############################################################################\n");

	/* IOTHUB Connect User Name...*/
	strcat(azure_iothub_connect_user_name,"\"");
	//strcat(azure_iothub_connect_user_name,azure_iothub_connect_host_name);
	strcat(azure_iothub_connect_user_name,"WeiPlusIoTHub.azure-devices.net");
	strcat(azure_iothub_connect_user_name,"/");
	char azure_iothub_connect_user_name_tmp[] = AZURE_IOTHUB_DEVICE_ID "/?api-version=" AZURE_IOTHUB_SERVICE_VERSION "&model-id=" MODEL_ID "\"";
	//char azure_iothub_connect_user_name_tmp[] = AZURE_IOTHUB_DEVICE_ID "/?api-version=" AZURE_IOTHUB_SERVICE_VERSION "&model-id=" MODEL_ID;

	/* host_name/device_id/?api-version=2020-09-30&model-id=dtmi:himax:weiplus;1*/
	strcat(azure_iothub_connect_user_name,azure_iothub_connect_user_name_tmp);
	xprintf("**** azure_iothub_connect_user_name: %s ****\n",azure_iothub_connect_user_name);

	/* azure iothub publish topic. */
	strcat(azure_iothub_publish_topic,"\"devices/");
	strcat(azure_iothub_publish_topic,AZURE_IOTHUB_DEVICE_ID);
	strcat(azure_iothub_publish_topic,"/messages/events/\"");
	/* devices/device_id/messages/events/ */
	xprintf("**** azure_iothub_publish_topic: %s ****\n",azure_iothub_publish_topic);

#if DEBUG_TEST
	send_algo_result_to_cloud();
#endif/*#if DEBUG_TEST*/

#if !AZURE_DEVICE_PROVISIONING_SERVICE_EN //for test , IOTHUB-> Cloud
	azure_pnp_iotdps_event = PNP_IOTDPS_REGISTRATION_DONE;
	azure_pnp_iothub_event = PNP_IOTHUB_INITIAL;
#else
	//for test , IOTDPS -> IOTHUB-> Cloud
	azure_pnp_iothub_event = PNP_IOTHUB_INITIAL;
#endif

#if AZURE_IOTHUB_DEVICE_EN //IOTHUB

	memset(recv_buf,0,AT_MAX_LEN);//clear buffer
	wifi_atcmd_retry_cnt = 0;
	while(1){
		if(azure_pnp_iothub_event != PNP_IOTHUB_CONNECTIING){
			memset(recv_buf,0,AT_MAX_LEN);//clear buffer
		}

		switch(azure_pnp_iothub_event)
		{
		/* WIFI Query/check IP Address. */
		case PNP_IOTHUB_INITIAL:
			 xprintf("### GET IP ADDRESS.. ###\n");
			if(esp8266_address_get(dev_uart_comm, recv_buf, recv_len) == AT_OK){
				 xprintf("### GET IP ADDRESS OK!! ###\n");
				 azure_pnp_iothub_event =PNP_IOTHUB_USER_CFG;
			 }else{
				 xprintf("### GET IP ADDRESS FAIL.. ###\n");
			 }
		break;
		/*WIFI MQTTUSERCFG. */
		case PNP_IOTHUB_USER_CFG:
			xprintf("SET MQTT USER CONFIG..\n");
			if(esp8266_mqtt_set_user_cfg(dev_uart_comm, "2", recv_buf, recv_len)== AT_OK)
			{
				xprintf("WIFI MQTT USER CONFIG OK!!\n");
				azure_pnp_iothub_event	= PNP_IOTHUB_USER_CLIENT_ID;
				wifi_atcmd_retry_cnt	= 0;
			}else{
				++wifi_atcmd_retry_cnt;
				xprintf("WIFI MQTT USER CONFIG FAIL..\n");
			}
		break;
		/* WIFI MQTTCLIENTID. */
		case PNP_IOTHUB_USER_CLIENT_ID:
			xprintf("SET MQTT CLIENT ID..\n");
			if( esp8266_mqtt_set_user_client_id(dev_uart_comm, "\""DEVICE_ID"\"", recv_buf, recv_len)== AT_OK){
				xprintf("SET MQTT CLIENT ID OK!!\n");
				azure_pnp_iothub_event	= PNP_IOTHUB_USER_NAME;
				wifi_atcmd_retry_cnt	= 0;
			}else{
				++wifi_atcmd_retry_cnt;
				xprintf("SET MQTT CLIENT ID FAIL..\n");
			}
		break;
		/* WIFI MQTTUSERNAME. */
		case PNP_IOTHUB_USER_NAME:
			xprintf("SET MQTT USER NAME..\n");
			if( esp8266_mqtt_set_user_name(dev_uart_comm, azure_iothub_connect_user_name, recv_buf, recv_len)== AT_OK)
			{
				xprintf("SET MQTT USER NAME OK!!\n");
				azure_pnp_iothub_event	= PNP_IOTHUB_USER_PASSWORD;
				wifi_atcmd_retry_cnt	= 0;
			}else{
				++wifi_atcmd_retry_cnt;
				xprintf("SET MQTT USER NAME FAIL..\n");
			}
		break;
		/* WIFI MQTTPASSWORD. */
		case PNP_IOTHUB_USER_PASSWORD:
			xprintf("SET MQTT USERR PASSWORD..\n");
			if(esp8266_mqtt_set_user_password(dev_uart_comm, azure_iothub_connect_password, recv_buf, recv_len) == AT_OK)
			{
				xprintf("SET MQTT USERR PASSWORD OK!!\n");
				azure_pnp_iothub_event	= PNP_IOTHUB_CONNECT_TO_DEVICE;
				wifi_atcmd_retry_cnt	= 0;
			}else{
				++wifi_atcmd_retry_cnt;
				xprintf("SET MQTT USERR PASSWORD FAIL..\n");
			}
		break;
		/* WIFI MQTTCONN. */
		case PNP_IOTHUB_CONNECT_TO_DEVICE:
			xprintf("WIFI CONNECT to IOTHUB DEVICE..\n");
			if( esp8266_mqtt_conn(dev_uart_comm, "\""HOST_NAME"\"", "8883", recv_buf, recv_len)== AT_OK)
			{
				xprintf("WIFI CONNECT to IOTHUB DEVICE OK!!\n");
				azure_pnp_iothub_event	= PNP_IOTHUB_CONNECT_TO_DEVICE_DONE;
				wifi_atcmd_retry_cnt	= 0;
			}else{
				++wifi_atcmd_retry_cnt;
				xprintf("WIFI CONNECT to IOTHUB DEVICE FAIL..\n");
			}
		break;
		/* WIFI Connect IOTHUB Device Done. */
		case PNP_IOTHUB_CONNECT_TO_DEVICE_DONE:
			azure_pnp_iothub_event	= PNP_IOTHUB_CONNECTIING;
			azure_active_event		= ALGO_EVENT_IDLE;
		break;
		case PNP_IOTHUB_RECONNECT:
			xprintf("### WIFI IOTHUB Device MQTT Re-Connect... ###\n");
			/* WIFI Disconnect IOTHUB. */
			if(esp8266_mqtt_disconn(dev_uart_comm, recv_buf, recv_len) == AT_OK)
			{
				xprintf("WIFI IOTHUB Device MQTT Disconnect OK!!\n");
				azure_pnp_iothub_event 		= PNP_IOTHUB_INITIAL;
				azure_active_event 			= ALGO_EVENT_IDLE;
			}else{
				xprintf("### WIFI IOTHUB Device MQTT Disconnect ing... ###\n");
			}
		break;
		}//switch case

		/* MQTT IOTHUB Re-Connect. */
		if(wifi_atcmd_retry_cnt == WIFI_ATCMD_RETRY_MAX_TIMES){
			wifi_atcmd_retry_cnt	= 0;
			azure_pnp_iothub_event = PNP_IOTHUB_RECONNECT;
		}

		tx_thread_sleep(9000);//600ms
	}//while
#endif /* #if AZURE_IOTHUB_DEVICE_EN */
#endif //NETWORK_EN
}


/* Define cis capture image thread entry.  */
void cis_capture_image_thread_entry(ULONG parameter)
{
	xprintf("### Start cis_capture_image_thread_entry ###\n");

	while(1){
		switch(azure_active_event)
		{
		/* Algorithm event idle*/
		case ALGO_EVENT_IDLE:

			if( azure_pnp_iotdps_event	== PNP_IOTDPS_REGISTRATION_DONE && \
				azure_pnp_iothub_event	== PNP_IOTHUB_CONNECTIING)
			{
				/*
				 * GetImage ->
				 * -> tflitemicro_algo_run
				 * */
				tflitemicro_start();
			}
		break;
		}//switch case
		tx_thread_sleep(7000);
	}//while
}

/* Define algo send result thread entry.  */
void algo_send_result_thread_entry(ULONG parameter)
{
	xprintf("### Start algo_send_result_thread ###\n");
	int ret											= 0;
	unsigned int wifi_atcmd_retry_cnt				= 0;
	unsigned int azure_img_idx_cnt					= 0;
	unsigned int azure_algo_snd_res_img_event_tmp	= ALGO_EVENT_IDLE;


	memset(recv_buf,0,AT_MAX_LEN);//clear buffer
	while(1){
		switch (azure_active_event)
		{
		/* 0:Not do thing. */
		case ALGO_EVENT_IDLE:
			/* Capture image in cis_capture_image_thread_entry. */
		break;
		/* 1:Send Metadata to Cloud. */
		case ALGO_EVENT_SEND_RESULT_TO_CLOUD:
			ret = send_algo_result_to_cloud();
			if(ret == 1)
			{
				xprintf("WIFI IOTHUB Send MetaData to Cloud OK!!\n");

				wifi_atcmd_retry_cnt	= 0;
				/* clear buffer. */
				memset(recv_buf,0,AT_MAX_LEN);
				memset(azure_iothub_publish_msg_json_pkg,0,AT_MAX_LEN);

				if(azure_algo_snd_res_img_event_tmp == ALGO_EVENT_SEND_RESULT_AND_IMAGE){
					azure_active_event = ALGO_EVENT_SEND_IMAGE_TO_CLOUD;
				}else{
#ifdef ENABLE_PMU
					azure_active_event = ENTER_PMU_MODE;
#else
					azure_active_event = ALGO_EVENT_IDLE;
#endif
				}
			}else if(ret == AT_ERROR){
				xprintf("!!!!WIFI Send Metadata Retry_Cnt:%d!!!!\n",wifi_atcmd_retry_cnt);
				++wifi_atcmd_retry_cnt;
				//20210811 jason+
				if( wifi_atcmd_retry_cnt > (WIFI_ATCMD_RETRY_MAX_TIMES/2)){
					tx_thread_sleep(8750);//500ms
				}
			}
		break;
		/* 2:Send Image to Cloud. */
		case ALGO_EVENT_SEND_IMAGE_TO_CLOUD:
#if 1
			if(g_imgsize > SEND_PKG_MAX_SIZE){
				ret = send_cstm_data_to_cloud((unsigned char *)g_img_cur_addr_pos, SEND_PKG_MAX_SIZE, SEND_IMAGE_DATA);
				if(ret == 1)
				{
					++azure_img_idx_cnt;
					xprintf("### Send Data:%d/%d Bytes... ###\n",(azure_img_idx_cnt*SEND_PKG_MAX_SIZE),g_pimg_config.jpeg_size,SEND_IMAGE_DATA);

					g_img_cur_addr_pos += SEND_PKG_MAX_SIZE;
					g_imgsize		   -= SEND_PKG_MAX_SIZE;

					wifi_atcmd_retry_cnt = 0;
					/* clear buffer. */
					memset(recv_buf,0,AT_MAX_LEN);
					memset(azure_iothub_publish_msg_json_pkg,0,AT_MAX_LEN);
				}else if(ret == AT_ERROR){
					xprintf("!!!!WIFI Send Image Retry_Cnt:%d!!!!\n",wifi_atcmd_retry_cnt);
					++wifi_atcmd_retry_cnt;
					//20210811 jason+
					if( wifi_atcmd_retry_cnt > (WIFI_ATCMD_RETRY_MAX_TIMES/2)){
						tx_thread_sleep(8750);//500ms
					}
				}
			}else{
				/* g_pimg_config.jpeg_size < SEND_PKG_MAX_SIZE. */
				g_img_cur_addr_pos += g_imgsize;
				ret = send_cstm_data_to_cloud((unsigned char *)g_img_cur_addr_pos, g_imgsize,SEND_IMAGE_DATA);
				//if(send_cstm_data_to_cloud((unsigned char *)g_img_cur_addr_pos, g_imgsize))
				if(ret == 1)
				{
					xprintf("### Send Data:%d/%d Bytes... ###\n",(azure_img_idx_cnt*SEND_PKG_MAX_SIZE) + g_imgsize
							,g_pimg_config.jpeg_size);

					xprintf("WIFI IOTHUB Send Image to Cloud OK!!\n");

					azure_algo_snd_res_img_event_tmp = ALGO_EVENT_IDLE;
					azure_img_idx_cnt		= 0;
					wifi_atcmd_retry_cnt	= 0;
					/* clear buffer. */
					memset(recv_buf,0,AT_MAX_LEN);
					memset(azure_iothub_publish_msg_json_pkg,0,AT_MAX_LEN);
#ifdef ENABLE_PMU
					azure_active_event = ENTER_PMU_MODE;
#if 1
					/*MQTT Disconnect IOTHUB. */
					if(esp8266_mqtt_disconn(dev_uart_comm, recv_buf, recv_len) == AT_OK){
						xprintf("WIFI IOTHUB MQTT Disconnect OK!!\n");
					}else{
						xprintf("### WIFI IOTHUB MQTT Disconnect ing... ###\n");
					}
#endif
#else
					azure_active_event = ALGO_EVENT_IDLE;
#endif
				}else if(ret == AT_ERROR){
					xprintf("!!!!WIFI Send Image Retry_Cnt:%d!!!!\n",wifi_atcmd_retry_cnt);
					++wifi_atcmd_retry_cnt;
					//20210811 jason+
					if( wifi_atcmd_retry_cnt > (WIFI_ATCMD_RETRY_MAX_TIMES/2)){
						tx_thread_sleep(8750);//500ms
					}
				}
			}
#endif
		break;
		/* Send Metadata and Imag. */
		case ALGO_EVENT_SEND_RESULT_AND_IMAGE:
			azure_algo_snd_res_img_event_tmp = ALGO_EVENT_SEND_RESULT_AND_IMAGE;
			azure_active_event				 = ALGO_EVENT_SEND_RESULT_TO_CLOUD;
		break;
		/* Azure DPS_IOTHUB Re-Connect... */
		case ALGO_EVENT_IOTHUB_RECONNECT:
			xprintf("\n### Azure DPS_IOTHUB Re-Connect... ###\n");
			azure_active_event 		= ALGO_EVENT_IDLE;
			azure_pnp_iothub_event  = PNP_IOTHUB_RECONNECT;
		break;
		/*Enter PMU Mode. */
		case ENTER_PMU_MODE:
#if WIFI_EN
			/* MQTT Disconnect IOTHUB Device. */
			if(esp8266_mqtt_disconn(dev_uart_comm, recv_buf, recv_len) == AT_OK){
				xprintf("WIFI IOTHUB Device MQTT Disconnect OK!!\n");
			}else{
				xprintf("### WIFI IOTHUB Device MQTT Disconnect ing... ###\n");
			}
#endif //#if WIFI_EN

			xprintf("### EnterToPMU() ###\n");
			EnterToPMU(PMU_SLEEP_MS); 
		break;
		}//switch case

		if(wifi_atcmd_retry_cnt == WIFI_ATCMD_RETRY_MAX_TIMES){
			wifi_atcmd_retry_cnt = 0;
			azure_active_event = ALGO_EVENT_IOTHUB_RECONNECT;
		}
		tx_thread_sleep(7000);
	}//while
}


#ifndef SAMPLE_DHCP_DISABLE
static void dhcp_wait()
{
ULONG   actual_status;

    xprintf("DHCP In Progress...\r\n");

    /* Create the DHCP instance.  */
    nx_dhcp_create(&dhcp_0, &ip_0, "DHCP Client");

    /* Start the DHCP Client.  */
    nx_dhcp_start(&dhcp_0);

    /* Wait util address is solved. */
    nx_ip_status_check(&ip_0, NX_IP_ADDRESS_RESOLVED, &actual_status, NX_WAIT_FOREVER);
}
#endif /* SAMPLE_DHCP_DISABLE  */

static UINT dns_create()
{

UINT    status;
ULONG   dns_server_address[3];
//0302UINT    dns_server_address_size = 12;

    /* Create a DNS instance for the Client.  Note this function will create
       the DNS Client packet pool for creating DNS message packets intended
       for querying its DNS server. */
    status = nx_dns_create(&dns_0, &ip_0, (UCHAR *)"DNS Client");
    if (status)
    {
        return(status);
    }

    /* Is the DNS client configured for the host application to create the packet pool? */
#ifdef NX_DNS_CLIENT_USER_CREATE_PACKET_POOL

    /* Yes, use the packet pool created above which has appropriate payload size
       for DNS messages. */
    status = nx_dns_packet_pool_set(&dns_0, ip_0.nx_ip_default_packet_pool);
    if (status)
    {
        nx_dns_delete(&dns_0);
        return(status);
    }
#endif /* NX_DNS_CLIENT_USER_CREATE_PACKET_POOL */

#ifndef SAMPLE_DHCP_DISABLE
    /* Retrieve DNS server address.  */
    nx_dhcp_interface_user_option_retrieve(&dhcp_0, 0, NX_DHCP_OPTION_DNS_SVR, (UCHAR *)(dns_server_address),
                                           &dns_server_address_size);
#else
    dns_server_address[0] = SAMPLE_DNS_SERVER_ADDRESS;
#endif /* SAMPLE_DHCP_DISABLE */

    /* Add an IPv4 server address to the Client list. */
    status = nx_dns_server_add(&dns_0, dns_server_address[0]);
    if (status)
    {
        nx_dns_delete(&dns_0);
        return(status);
    }

    /* Output DNS Server address.  */
    xprintf("DNS Server address: %lu.%lu.%lu.%lu\r\n",
           (dns_server_address[0] >> 24),
           (dns_server_address[0] >> 16 & 0xFF),
           (dns_server_address[0] >> 8 & 0xFF),
           (dns_server_address[0] & 0xFF));

    return(NX_SUCCESS);
}

static UINT unix_time_get(ULONG *unix_time)
{

    /* Using time() to get unix time on x86.
       Note: User needs to implement own time function to get the real time on device, such as: SNTP.  */
	*unix_time = (ULONG)time(NULL);

    return(NX_SUCCESS);
}
#endif
