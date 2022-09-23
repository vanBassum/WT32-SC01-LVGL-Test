#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/apps/sntp.h"
#include "lwip/apps/sntp_opts.h"
#include <string.h>
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "defines.h"
#include "lib/rtos/freertos.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"

#include "lvgl/lvgl.h"
#include "st7796s.h"

#define TAG "MAIN"



static FreeRTOS::Mutex lvglMutex;
static FreeRTOS::Timer lvglTimer;



extern "C" {
   void app_main();
}



void lvgl_driver_init(void)
{
	st7796s_Init();
	st7796s_Backlight(1);
	
	st7796s_WritePixel(0, 20, 0xAAAA);
}




void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
	//st7735s_flush(disp, area, color_p);
	lv_disp_flush_ready(disp);
}

void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{

}

static void lv_tick_task(FreeRTOS::Timer* timer)
{
	lv_tick_inc(1000);
}


void InitLVGL()
{
	static const uint16_t screenWidth = 480;
	static const uint16_t screenHeight = 320;
	static lv_disp_draw_buf_t draw_buf;
	static lv_color_t buf[screenWidth * 10];
	
	lvgl_driver_init();
	lv_init();			// Initialize lvgl
	

	/* LVGL : Setting up buffer to use for display */
	lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

	/*** LVGL : Setup & Initialize the display device driver ***/
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res = screenWidth;
	disp_drv.ver_res = screenHeight;
	disp_drv.flush_cb = display_flush;
	disp_drv.draw_buf = &draw_buf;
	lv_disp_drv_register(&disp_drv);

	/*** LVGL : Setup & Initialize the input device driver ***/
	static lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = touchpad_read;
	lv_indev_drv_register(&indev_drv);
	
	/* Create and start a periodic timer interrupt to call lv_tick_inc */
	
	lvglTimer.Init("lvgl", 1000 / portTICK_PERIOD_MS, true);
	lvglTimer.SetCallback(&lv_tick_task);
	lvglTimer.Start();
	
	
	
	/*** Create simple label and show LVGL version ***/
	char txt[100];
	lv_obj_t *tlabel; // touch x,y label
	
	sprintf(txt, "WT32-SC01 with LVGL v%d.%d.%d", lv_version_major(), lv_version_minor(), lv_version_patch());
	lv_obj_t *label = lv_label_create(lv_scr_act()); // full screen as the parent
	lv_label_set_text(label, txt);                   // set label text
	lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);    // Center but 20 from the top

	tlabel = lv_label_create(lv_scr_act());         // full screen as the parent
	lv_label_set_text(tlabel, "Touch:(000,000)");   // set label text
	lv_obj_align(tlabel, LV_ALIGN_TOP_RIGHT, 0, 0); // Center but 20 from the top
	
	
	while (1)
	{
		lv_timer_handler(); /* let the GUI do its work */
		vTaskDelay(1);
	}
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}

void app_main(void)
{
	nvs_flash_init();
	//tcpip_adapter_init();
	//ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
	//wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	//ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	//ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
	//ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	//
	//wifi_config_t sta_config = {};
	//memcpy(sta_config.sta.ssid, SSID, strlen(SSID));
	//memcpy(sta_config.sta.password, PSWD, strlen(PSWD));
	//sta_config.sta.bssid_set = false;
	//ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
	//ESP_ERROR_CHECK( esp_wifi_start() );
	//ESP_ERROR_CHECK( esp_wifi_connect() );
	//
	//setenv("TZ", "UTC-1UTC,M3.5.0,M10.5.0/3", 1);
	//tzset();
	//sntp_setoperatingmode(SNTP_OPMODE_POLL);
	//sntp_setservername(0, "pool.ntp.org");
	//sntp_init();
	InitLVGL();
}










