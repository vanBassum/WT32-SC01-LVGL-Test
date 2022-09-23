/*
 * ST7796S LCD driver (optional built-in touchscreen driver)
 * 2020.11
*/

#include <string.h>
#include "st7796s.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "esp_system.h"
#include "esp_log.h"

#define TAG "st7796s"

static const int SPI_Command_Mode = 0;
static const int SPI_Data_Mode = 1;
//static const int SPI_Frequency = SPI_MASTER_FREQ_20M;
//static const int SPI_Frequency = SPI_MASTER_FREQ_26M;
static const int SPI_Frequency = SPI_MASTER_FREQ_40M;
//static const int SPI_Frequency = SPI_MASTER_FREQ_80M;

static const gpio_num_t io_CS	   = GPIO_NUM_15;
static const gpio_num_t io_DC	   = GPIO_NUM_21;
static const gpio_num_t io_RESET = GPIO_NUM_22;
static const gpio_num_t io_BL	   = GPIO_NUM_23;
static const gpio_num_t io_SCK   = GPIO_NUM_14;
static const gpio_num_t io_MOSI  = GPIO_NUM_13;
static const gpio_num_t io_MISO  = GPIO_NUM_NC;

static volatile uint8_t spi_pending_trans = 0;

spi_device_handle_t handle = NULL;


void reset()
{
	gpio_set_level(io_RESET, 1);
	vTaskDelay(pdMS_TO_TICKS(100));
	gpio_set_level(io_RESET, 0);
}

void io_init()
{
	ESP_LOGI(TAG, "GPIO_CS=%d", io_CS);
	gpio_pad_select_gpio(io_CS);
	gpio_set_direction(io_CS, GPIO_MODE_OUTPUT);
	gpio_set_level(io_CS, 0);

	ESP_LOGI(TAG, "GPIO_DC=%d", io_DC);
	gpio_pad_select_gpio(io_DC);
	gpio_set_direction(io_DC, GPIO_MODE_OUTPUT);
	gpio_set_level(io_DC, 0);

	ESP_LOGI(TAG, "GPIO_RESET=%d", io_RESET);
	if (io_RESET >= 0) {
		gpio_pad_select_gpio(io_RESET);
		gpio_set_direction(io_RESET, GPIO_MODE_OUTPUT);
		gpio_set_level(io_RESET, 0);
	}

	ESP_LOGI(TAG, "GPIO_BL=%d", io_BL);
	if (io_BL >= 0) {
		gpio_pad_select_gpio(io_BL);
		gpio_set_direction(io_BL, GPIO_MODE_OUTPUT);
		gpio_set_level(io_BL, 0);
	}
}

void spi_master_init()
{
	esp_err_t ret;

	spi_bus_config_t buscfg = {
		.mosi_io_num = io_MOSI,
		.miso_io_num = io_MISO,
		.sclk_io_num = io_SCK,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1
	};

	
	ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
	ESP_LOGD(TAG, "spi_bus_initialize=%d", ret);
	assert(ret == ESP_OK);

	spi_device_interface_config_t devcfg =  {
		.clock_speed_hz = SPI_Frequency,
		.spics_io_num = io_CS,
		.flags = SPI_DEVICE_NO_DUMMY,
		.queue_size = 7,
	};


	ret = spi_bus_add_device(HSPI_HOST, &devcfg, &handle);
	ESP_LOGD(TAG, "spi_bus_add_device=%d", ret);
	assert(ret == ESP_OK);
}


void disp_spi_send_data(void* data, size_t dataLength)
{
	spi_transaction_t SPITransaction;
	esp_err_t ret;

	if (dataLength > 0) {
		memset(&SPITransaction, 0, sizeof(spi_transaction_t));
		SPITransaction.length = dataLength * 8;
		SPITransaction.tx_buffer = data;
		ret = spi_device_transmit(handle, &SPITransaction);
		assert(ret == ESP_OK); 
	}
}


typedef struct {
	uint8_t cmd;
	uint8_t data[16];
	uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;


static void st7796s_send_cmd(uint8_t cmd)
{
	gpio_set_level(io_DC, 0);
	disp_spi_send_data(&cmd, 1);
}

static void st7796s_send_data(void * data, uint16_t length)
{
	gpio_set_level(io_DC, 1);
	disp_spi_send_data(data, length);
}

static void st7796s_send_color(void * data, uint16_t length)
{
	gpio_set_level(io_DC, 1);
	disp_spi_send_data(data, length);
}


//-----------------------------------------------------------------------------
/**
  * @brief  ST7796S initialization
  * @param  None
  * @retval None
  */
void st7796s_Init(void)
{
	io_init();
	
	if (handle == NULL)
	{
		spi_master_init();
	}
	
	reset();
	
	lcd_init_cmd_t st7796s_init_cmds[] = {
		{ 0x11, { 0 }, 0x80 },
		{ 0xF0, { 0xc3 }, 1 },
		{ 0xf0, { 0x96 }, 1 },
		{ 0x36, { 0x28 }, 1 },
		{ 0x3a, { 0x55 }, 1 },
		{ 0xb4, { 0x01 }, 1 },
		{ 0xb7, { 0xc6 }, 1 },
		{ 0xe8, { 0x40, 0x8a, 0x00, 0x00, 0x29, 0x19, 0xa5, 0x33 }, 8 },
		{ 0xc1, { 0x06 }, 1 },
		{ 0xc2, { 0xa7 }, 1 },
		{ 0xc5, { 0x18 }, 1 }, 
		{ 0xe0, { 0xf0, 0x09, 0x0b, 0x06, 0x04, 0x15, 0x2f, 0x54, 0x42, 0x3c, 0x17, 0x14, 0x18, 0x1b }, 14 },
		{ 0xe1, { 0xf0, 0x09, 0x0b, 0x06, 0x04, 0x03, 0x2d, 0x43, 0x42, 0x3b, 0x16, 0x14, 0x17, 0x1b }, 14 },
		{ 0xF0, { 0x3c }, 1 },
		{ 0xF0, { 0x69 }, 0x81 },
		{ 0x29, { 0 }, 0 },
		{ 0x2c, { 0 }, 0 },
		{ 0, { 0 }, 0xff },
	};
	
	
	//Send all the commands
	uint16_t cmd = 0;
	while (st7796s_init_cmds[cmd].databytes != 0xff) {
		st7796s_send_cmd(st7796s_init_cmds[cmd].cmd);
		st7796s_send_data(st7796s_init_cmds[cmd].data, st7796s_init_cmds[cmd].databytes & 0x1F);
		if (st7796s_init_cmds[cmd].databytes & 0x80) {
			vTaskDelay(150 / portTICK_RATE_MS);
		}
		cmd++;
	}

	st7796s_Backlight(true);
}


void st7796s_Backlight(uint8_t val)
{
	gpio_set_level(io_BL, val);
}


void st7796s_WritePixel(uint16_t Xpos, uint16_t Ypos, uint16_t RGBCode)
{
	uint8_t data[4] = { 0 };

	uint16_t offsetx1 = 10;
	uint16_t offsetx2 = 20;
	uint16_t offsety1 = 10;
	uint16_t offsety2 = 20;
	
	/*Column addresses*/
	st7796s_send_cmd(ST7796S_CASET);
	data[0] = (offsetx1 >> 8) & 0xFF;
	data[1] = offsetx1 & 0xFF;
	data[2] = (offsetx2 >> 8) & 0xFF;
	data[3] = offsetx2 & 0xFF;
	st7796s_send_data(data, 4);

	/*Page addresses*/
	st7796s_send_cmd(ST7796S_RASET);
	data[0] = (offsety1 >> 8) & 0xFF;
	data[1] = offsety1 & 0xFF;
	data[2] = (offsety2 >> 8) & 0xFF;
	data[3] = offsety2 & 0xFF;
	st7796s_send_data(data, 4);

	/*Memory write*/
	st7796s_send_cmd(ST7796S_RAMWR);

	uint32_t size = 100;
	uint16_t colorMap[100];
	
	memset(colorMap, 0xFF, 100 * 2);

	st7796s_send_color((void*)colorMap, size * 2);
}

