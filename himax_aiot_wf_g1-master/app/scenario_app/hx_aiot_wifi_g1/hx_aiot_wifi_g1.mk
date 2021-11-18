override SCENARIO_APP_SUPPORT_LIST := $(APP_TYPE)
override OS_SEL := tx
override OS_TYPE := TX
APPL_DEFINES += -DHX_AIOT_WIFI_G1
override ALGO_TYPE = TFLITE_MICRO_GOOGLE_PERSON
LIB_SEL +=  sensordp audio tflm_sdk_lib pwrmgmt
override HEAPSZ = 80000

ifeq ("$(ALGO_TYPE)","TFLITE_MICRO_GOOGLE_PERSON")
$(warning "tflitemicro_24")
LIB_SEL += tflitemicro_24
else #default algo
override ALGO_TYPE := DATA_BSS_SRAM
endif

$(warning "hx_aiot_nb")

#---------- Azure IoTHub ------------
$(warning "AZURE_IOTHUB")
LIB_SEL += az_sdk netxduo
$(warning "OTA")	
LIB_SEL += ota i2c_comm
APPL_DEFINES += -DNB_IOT_BOARD
APPL_DEFINES += -D__Xdmac

#for  mqtt
APPL_DEFINES += -DNXD_MQTT_REQUIRE_TLS
APPL_DEFINES += -DNXD_MQTT_CLOUD_ENABLE
APPL_DEFINES += -DNX_SECURE_ENABLE
APPL_DEFINES += -DNX_ENABLE_EXTENDED_NOTIFY_SUPPORT
APPL_DEFINES += -DNX_ENABLE_IP_PACKET_FILTER
APPL_DEFINES += -DNX_DISABLE_IPV6
APPL_DEFINES += -DNX_AZURE_DISABLE_IOT_SECURITY_MODULE