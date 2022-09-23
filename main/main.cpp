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
#include "lib/rtos/freertos.h"
#include "lvglpp/lvgl.h"

#define TAG "MAIN"

/*
 * For this to work, use menuconfig to define the right screen and add following defines to lvgl_helpers.h
 * #define LV_HOR_RES_MAX 320
 * #define LV_VER_RES_MAX 480
 * #define SPI_HOST_MAX 3
 */


extern "C" {
   void app_main();
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    return ESP_OK;
}



void ButtonClicked(LVGL::Button* sender)
{
	ESP_LOGI("Button", "Clicked");
}

void app_main(void)
{
	nvs_flash_init();
	

	LVGL::Init();
	
	LVGL::Label label;
	LVGL::Button button;
	LVGL::Button button2;

	LVGL::ActScreen.AddWidget(button);
	button.OnClicked.Bind(ButtonClicked);
	
	LVGL::ActScreen.AddWidget(button2);
	button2.SetPos(0, 100);
	
	button.AddWidget(label);
	//label.SetAlignment(LV_ALIGN_CENTER);
	//label.SetText("Yeah");
	
	
	while (1)
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	
}










