#include <hx_aiot_wifi_g1/inc/azure_iothub.h>
#include <hx_aiot_wifi_g1/inc/hii.h>
#include <hx_aiot_wifi_g1/inc/pmu.h>
#include <hx_aiot_wifi_g1/inc/tflitemicro_algo.h>
#include "spi_master_protocol.h"
#include "hx_drv_tflm.h"
#include "inc/hx_drv_pmu.h"
#include "tx_api.h"
#include "powermode.h"
#include "library/ota/ota.h"
#include "board.h"
#include "BITOPS.h"
#include "external/wifi/esp8266/esp8266.h"
//datapath boot up reason flag
volatile uint8_t g_bootup_md_detect = 0;

#define IMAGE_HEIGHT 	480
#define IMAGE_WIDTH		640
struct_algoResult algo_result;
uint32_t g_imgsize;
unsigned char *g_img_cur_addr_pos;


hx_drv_sensor_image_config_t g_pimg_config;
static bool is_initialized = false;
int GetImage(int image_width,int image_height, int channels) {
	int ret = 0;
	//xprintf("is_initialized : %d \n",is_initialized);
	  if (!is_initialized) {
	    if (hx_drv_sensor_initial(&g_pimg_config) != HX_DRV_LIB_PASS) {
	    	xprintf("hx_drv_sensor_initial error\n");
	      return ERROR;
	    }
	    is_initialized = true;
		xprintf("is_initialized : %d \n",is_initialized);
	  }

	  //capture image by sensor
	  hx_drv_sensor_capture(&g_pimg_config);

	  g_img_cur_addr_pos = (unsigned char *)g_pimg_config.jpeg_address;
	  g_imgsize = g_pimg_config.jpeg_size;
	  xprintf("g_pimg_config.jpeg_address:0x%x size : %d \n",g_pimg_config.jpeg_address,g_pimg_config.jpeg_size);

	  return OK;
}

int img_cnt = 0;
static uint8_t power_start_flag  = 1;
void tflitemicro_start() {
	int size = 0 , ret = 0;
	img_cnt++;

	GetImage(IMAGE_WIDTH,IMAGE_HEIGHT,1);

#ifdef TFLITE_MICRO_GOOGLE_PERSON
	xprintf("### img_cnt:%d ###\n",img_cnt);
	xprintf("[tflitemicro_algo_run_result]:\n");
	tflitemicro_algo_run(g_pimg_config.raw_address, g_pimg_config.img_width, g_pimg_config.img_height, &algo_result);
	
	xprintf("humanPresence:%d\n",algo_result.humanPresence);
//	xprintf("det_box_x:%d\ndet_box_y:%d\ndet_box_width:%d\ndet_box_height:%d\n",\
			algo_result.ht[0].upper_body_bbox.x,\
			algo_result.ht[0].upper_body_bbox.y, \
			algo_result.ht[0].upper_body_bbox.width,\
			algo_result.ht[0].upper_body_bbox.height);
#endif


	/* azure_active_event
	 * ALGO_EVENT_SEND_RESULT_TO_CLOUD :Send Algorithm Metadata.
	 * ALGO_EVENT_SEND_IMAGE_TO_CLOUD  :Send Image.
	 * ALGO_EVENT_SEND_RESULT_AND_IMAGE:Send Metadata and Image.
	 *
	 * */
	if(algo_result.humanPresence){
		img_cnt= 0;
		azure_active_event = ALGO_EVENT_SEND_RESULT_TO_CLOUD;
	}
}

void setup(){
#ifdef TFLITE_MICRO_GOOGLE_PERSON
	xprintf("### TFLITE_MICRO_GOOGLE_PERSON ALGO INITIAL... ###\n");
	tflitemicro_algo_init();
#endif /* TFLITE_MICRO_GOOGLE_PERSON */

	/*Azure TX Task. */
	xprintf("#############################################################################\n");
	xprintf("**** Enter TX Thread ****\n");
	xprintf("#############################################################################\n");
	wifi_task_define();
}


hx_drv_sensor_image_config_t g_pimg_config;
void hx_aiot_wifi_g1()
{

#if BOARD_PLL_CLK_40M  //for 40MH
	xprintf("===== Work Freq. 40MHZ_External_LDO =====\n");
#else
	xprintf("===== Work Freq. 400MHZ_External_LDO =====\n");
	//xprintf("===== Work Freq. 400MHZ_Internal_LDO =====\n");
#endif

	/* Active Mode. */
	uint32_t addr = 0;
	uint32_t val = 0;
	PM_CFG_T aCfg;
	PM_CFG_PWR_MODE_E mode = PM_MODE_ALL_ON;

	// for external power saving
	addr = 0xB0000074;//  0xB0000074 = 0x0001
	val = _arc_read_uncached_32((void*)addr);
	xprintf("1######val = %d\n", val);

	xprintf("COLD BOOT PMU_WE1_POWERPLAN_EXTERNAL_LDO\n######val = %d\n", val);

	if(!val){
		val = val + 1;//0x0001;
		_arc_write_uncached_32((void *)addr, val);
		xprintf("2######\nval = %d\n", val);
		hx_drv_pmu_set_ctrl(PMU_PWR_PLAN, PMU_WE1_POWERPLAN_EXTERNAL_LDO);
		board_delay_ms(200);
		// disable sldo, 0xb0000410=0x00000001
		hx_lib_pm_sldo_en(0);
		board_delay_ms(200);
		hx_lib_pm_cldo_en(0);
		EnterToPMU(3000);
	}

	if(val){
		xprintf("PMU_WE1_POWERPLAN_EXTERNAL_LDO\n######val = %d\n", val);
		/* External_LDO*/
		hx_drv_pmu_set_ctrl(PMU_PWR_PLAN, PMU_WE1_POWERPLAN_EXTERNAL_LDO);
		board_delay_ms(200);
		// disable sldo, 0xb0000410=0x00000001
		hx_lib_pm_sldo_en(0);
		board_delay_ms(200);
		hx_lib_pm_cldo_en(0);
	}

	setup();
}

