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

#include "esp_log.h"
#include "lvgl/lvgl.h"
//#include "lvgl_helpers.h"


#define TAG "MAIN"

#define LV_HOR_RES_MAX 320
#define LV_VER_RES_MAX 480
#define SPI_HOST_MAX 3
#define DISP_SPI_MISO -1
#define DISP_SPI_MOSI 13
#define DISP_SPI_CLK 14
#define DISP_PIN_BCKL 23
#define DISP_SPI_CS 15
#define DISP_SPI_DC 21
#define DISP_SPI_RST 22
#define DISP_SPI_IO2 -1
#define DISP_SPI_IO3 -1
#define DISP_BUF_SIZE  (LV_HOR_RES_MAX * 40)
#define SPI_BUS_MAX_TRANSFER_SZ (DISP_BUF_SIZE * 2)
#define SPI_TFT_CLOCK_SPEED_HZ  (40*1000*1000)
#define SPI_TFT_SPI_MODE    (0)
#define DISP_SPI_INPUT_DELAY_NS (0)
#define SPI_TRANSACTION_POOL_SIZE 50	/* maximum number of DMA transactions simultaneously in-flight */
/* DMA Transactions to reserve before queueing additional DMA transactions. A 1/10th seems to be a good balance. Too many (or all) and it will increase latency. */
#define SPI_TRANSACTION_POOL_RESERVE_PERCENTAGE 10
#define SPI_TRANSACTION_POOL_RESERVE (SPI_TRANSACTION_POOL_SIZE / SPI_TRANSACTION_POOL_RESERVE_PERCENTAGE)	


static spi_host_device_t spi_host;
static spi_device_handle_t spi;
static QueueHandle_t TransactionPool = NULL;
static transaction_cb_t chained_post_cb;


static FreeRTOS::Mutex lvglMutex;
static FreeRTOS::Timer lvglTimer;

extern "C" {
   void app_main();
}



bool lvgl_spi_driver_init(spi_host_device_t host,
	int miso_pin,
	int mosi_pin,
	int sclk_pin,
	int max_transfer_sz,
	int dma_channel,
	int quadwp_pin,
	int quadhd_pin)
{
	assert((0 <= host) && (SPI_HOST_MAX > host));
	const char *spi_names[] = {
		"SPI1_HOST",
		"SPI2_HOST",
		"SPI3_HOST"
	};

	ESP_LOGI(TAG, "Configuring SPI host %s", spi_names[host]);
	ESP_LOGI(TAG,
		"MISO pin: %d, MOSI pin: %d, SCLK pin: %d, IO2/WP pin: %d, IO3/HD pin: %d",
		miso_pin,
		mosi_pin,
		sclk_pin,
		quadwp_pin,
		quadhd_pin);

	ESP_LOGI(TAG, "Max transfer size: %d (bytes)", max_transfer_sz);

	spi_bus_config_t buscfg = {
		.mosi_io_num = mosi_pin,
		.miso_io_num = miso_pin,
		.sclk_io_num = sclk_pin,
		.quadwp_io_num = quadwp_pin,
		.quadhd_io_num = quadhd_pin,
		.max_transfer_sz = max_transfer_sz
	};

	ESP_LOGI(TAG, "Initializing SPI bus...");
#if defined (CONFIG_IDF_TARGET_ESP32C3)
	dma_channel = SPI_DMA_CH_AUTO;
#endif
	esp_err_t ret = spi_bus_initialize(host, &buscfg, (spi_dma_chan_t)dma_channel);
	assert(ret == ESP_OK);

	return ESP_OK != ret;
}


typedef enum _disp_spi_send_flag_t {
	DISP_SPI_SEND_QUEUED = 0x00000000,
	DISP_SPI_SEND_POLLING = 0x00000001,
	DISP_SPI_SEND_SYNCHRONOUS = 0x00000002,
	DISP_SPI_SIGNAL_FLUSH = 0x00000004,
	DISP_SPI_RECEIVE = 0x00000008,
	DISP_SPI_CMD_8 = 0x00000010,
	/* Reserved */
	DISP_SPI_CMD_16 = 0x00000020,
	/* Reserved */
	DISP_SPI_ADDRESS_8 = 0x00000040, 
	DISP_SPI_ADDRESS_16 = 0x00000080, 
	DISP_SPI_ADDRESS_24 = 0x00000100, 
	DISP_SPI_ADDRESS_32 = 0x00000200, 
	DISP_SPI_MODE_DIO = 0x00000400, 
	DISP_SPI_MODE_QIO = 0x00000800, 
	DISP_SPI_MODE_DIOQIO_ADDR = 0x00001000, 
	DISP_SPI_VARIABLE_DUMMY = 0x00002000,
} disp_spi_send_flag_t;

static void IRAM_ATTR spi_ready(spi_transaction_t *trans)
{
	uint32_t address = reinterpret_cast<uint64_t>(trans->user);
	disp_spi_send_flag_t flags = (disp_spi_send_flag_t)address;

	if (flags & DISP_SPI_SIGNAL_FLUSH) {
		lv_disp_t * disp = NULL;

#if (LVGL_VERSION_MAJOR >= 7)
		disp = _lv_refr_get_disp_refreshing();
#else /* Before v7 */
		disp = lv_refr_get_disp_refreshing();
#endif

#if LVGL_VERSION_MAJOR < 8
		lv_disp_flush_ready(&disp->driver);
#else
		lv_disp_flush_ready(disp->driver);
#endif

	}

	if (chained_post_cb) {
		chained_post_cb(trans);
	}
}



void disp_spi_add_device_config(spi_host_device_t host, spi_device_interface_config_t *devcfg)
{
	spi_host = host;
	chained_post_cb = devcfg->post_cb;
	devcfg->post_cb = spi_ready;
	esp_err_t ret = spi_bus_add_device(host, devcfg, &spi);
	assert(ret == ESP_OK);
}

void disp_spi_add_device_with_speed(spi_host_device_t host, int clock_speed_hz)
{
	ESP_LOGI(TAG, "Adding SPI device");
	ESP_LOGI(TAG,
		"Clock speed: %dHz, mode: %d, CS pin: %d",
		clock_speed_hz,
		SPI_TFT_SPI_MODE,
		DISP_SPI_CS);

	spi_device_interface_config_t devcfg =  {
		.mode = SPI_TFT_SPI_MODE,
		.clock_speed_hz = clock_speed_hz,
		.input_delay_ns = DISP_SPI_INPUT_DELAY_NS,
		.spics_io_num = DISP_SPI_CS, // CS pin
		.queue_size = SPI_TRANSACTION_POOL_SIZE,
		.pre_cb = NULL,
		.post_cb = NULL,
	};

	
	disp_spi_add_device_config(host, &devcfg);

	/* create the transaction pool and fill it with ptrs to spi_transaction_ext_t to reuse */
	if (TransactionPool == NULL) {
		TransactionPool = xQueueCreate(SPI_TRANSACTION_POOL_SIZE, sizeof(spi_transaction_ext_t*));
		assert(TransactionPool != NULL);
		for (size_t i = 0; i < SPI_TRANSACTION_POOL_SIZE; i++)
		{
			spi_transaction_ext_t* pTransaction = (spi_transaction_ext_t*)heap_caps_malloc(sizeof(spi_transaction_ext_t), MALLOC_CAP_DMA);
			assert(pTransaction != NULL);
			memset(pTransaction, 0, sizeof(spi_transaction_ext_t));
			xQueueSend(TransactionPool, &pTransaction, portMAX_DELAY);
		}
	}
}

void disp_spi_add_device(spi_host_device_t host)
{
	disp_spi_add_device_with_speed(host, SPI_TFT_CLOCK_SPEED_HZ);
}

typedef struct
{
	uint8_t cmd;
	uint8_t data[16];
	uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;


void disp_wait_for_pending_transactions(void)
{
	spi_transaction_t *presult;

	while (uxQueueMessagesWaiting(TransactionPool) < SPI_TRANSACTION_POOL_SIZE) {
		/* service until the transaction reuse pool is full again */
		if (spi_device_get_trans_result(spi, &presult, 1) == ESP_OK) {
			xQueueSend(TransactionPool, &presult, portMAX_DELAY);
		}
	}
}


void disp_spi_transaction(const uint8_t *data,
	size_t length,
	disp_spi_send_flag_t flags,
	uint8_t *out,
	uint64_t addr,
	uint8_t dummy_bits)
{
	if (0 == length) {
		return;
	}

	spi_transaction_ext_t t;
	
	memset(&t, 0, sizeof(spi_transaction_ext_t));

	/* transaction length is in bits */
	t.base.length = length * 8;

	if (length <= 4 && data != NULL) {
		t.base.flags = SPI_TRANS_USE_TXDATA;
		memcpy(t.base.tx_data, data, length);
	}
	else {
		t.base.tx_buffer = data;
	}

	if (flags & DISP_SPI_RECEIVE) {
		assert(out != NULL && (flags & (DISP_SPI_SEND_POLLING | DISP_SPI_SEND_SYNCHRONOUS)));
		t.base.rx_buffer = out;

#if defined(DISP_SPI_HALF_DUPLEX)
		t.base.rxlength = t.base.length;
		t.base.length = 0;	/* no MOSI phase in half-duplex reads */
#else
		t.base.rxlength = 0; /* in full-duplex mode, zero means same as tx length */
#endif
	}

	if (flags & DISP_SPI_ADDRESS_8) {
		t.address_bits = 8;
	}
	else if (flags & DISP_SPI_ADDRESS_16) {
		t.address_bits = 16;
	}
	else if (flags & DISP_SPI_ADDRESS_24) {
		t.address_bits = 24;
	}
	else if (flags & DISP_SPI_ADDRESS_32) {
		t.address_bits = 32;
	}
	if (t.address_bits) {
		t.base.addr = addr;
		t.base.flags |= SPI_TRANS_VARIABLE_ADDR;
	}

#if defined(DISP_SPI_HALF_DUPLEX)
	if (flags & DISP_SPI_MODE_DIO) {
		t.base.flags |= SPI_TRANS_MODE_DIO;
	}
	else if (flags & DISP_SPI_MODE_QIO) {
		t.base.flags |= SPI_TRANS_MODE_QIO;
	}

	if (flags & DISP_SPI_MODE_DIOQIO_ADDR) {
		t.base.flags |= SPI_TRANS_MODE_DIOQIO_ADDR;
	}

	if ((flags & DISP_SPI_VARIABLE_DUMMY) && dummy_bits) {
		t.dummy_bits = dummy_bits;
		t.base.flags |= SPI_TRANS_VARIABLE_DUMMY;
	}
#endif

	/* Save flags for pre/post transaction processing */
	t.base.user = (void *) flags;

	/* Poll/Complete/Queue transaction */
	if (flags & DISP_SPI_SEND_POLLING) {
		disp_wait_for_pending_transactions();	/* before polling, all previous pending transactions need to be serviced */
		spi_device_polling_transmit(spi, (spi_transaction_t *) &t);
	}
	else if (flags & DISP_SPI_SEND_SYNCHRONOUS) {
		disp_wait_for_pending_transactions();	/* before synchronous queueing, all previous pending transactions need to be serviced */
		spi_device_transmit(spi, (spi_transaction_t *) &t);
	}
	else {
		
		/* if necessary, ensure we can queue new transactions by servicing some previous transactions */
		if (uxQueueMessagesWaiting(TransactionPool) == 0) {
			spi_transaction_t *presult;
			while (uxQueueMessagesWaiting(TransactionPool) < SPI_TRANSACTION_POOL_RESERVE) {
				if (spi_device_get_trans_result(spi, &presult, 1) == ESP_OK) {
					xQueueSend(TransactionPool, &presult, portMAX_DELAY);	/* back to the pool to be reused */
				}
			}
		}

		spi_transaction_ext_t *pTransaction = NULL;
		xQueueReceive(TransactionPool, &pTransaction, portMAX_DELAY);
		memcpy(pTransaction, &t, sizeof(t));
		if (spi_device_queue_trans(spi, (spi_transaction_t *) pTransaction, portMAX_DELAY) != ESP_OK) {
			xQueueSend(TransactionPool, &pTransaction, portMAX_DELAY);	/* send failed transaction back to the pool to be reused */
		}
	}
}

static inline void disp_spi_send_data(uint8_t *data, size_t length) {
	disp_spi_transaction(data, length, DISP_SPI_SEND_POLLING, NULL, 0, 0);
}

static inline void disp_spi_send_colors(uint8_t *data, size_t length) {
	disp_spi_transaction(data,
		length,
		(_disp_spi_send_flag_t)(DISP_SPI_SEND_QUEUED | DISP_SPI_SIGNAL_FLUSH),
		NULL,
		0,
		0);
}

static void st7796s_send_cmd(uint8_t cmd)
{
	disp_wait_for_pending_transactions();
	gpio_set_level((gpio_num_t)DISP_SPI_DC, 0); /*Command mode*/
	disp_spi_send_data(&cmd, 1);
}

static void st7796s_send_data(void *data, uint16_t length)
{
	disp_wait_for_pending_transactions();
	gpio_set_level((gpio_num_t)DISP_SPI_DC, 1); /*Data mode*/
	disp_spi_send_data((uint8_t*)data, length);
}


static void st7796s_set_orientation(uint8_t orientation)
{
	// ESP_ASSERT(orientation < 4);

	const char *orientation_str[] = {
		"PORTRAIT",
		"PORTRAIT_INVERTED",
		"LANDSCAPE",
		"LANDSCAPE_INVERTED"
	 };

	ESP_LOGI(TAG, "Display orientation: %s", orientation_str[orientation]);

#if defined CONFIG_LV_PREDEFINED_DISPLAY_M5STACK
	uint8_t data[] = { 0x68, 0x68, 0x08, 0x08 };
#elif defined(CONFIG_LV_PREDEFINED_DISPLAY_WROVER4)
	uint8_t data[] = { 0x4C, 0x88, 0x28, 0xE8 };
#elif defined(CONFIG_LV_PREDEFINED_DISPLAY_WT32_SC01)
	uint8_t data[] = { 0x48, 0x88, 0x28, 0xE8 };
#elif defined(CONFIG_LV_PREDEFINED_DISPLAY_NONE)
	uint8_t data[] = { 0x48, 0x88, 0x28, 0xE8 };
#endif

	ESP_LOGI(TAG, "0x36 command value: 0x%02X", data[orientation]);

	st7796s_send_cmd(0x36);
	st7796s_send_data((void *)&data[orientation], 1);
}

void st7796s_init(void)
{
	lcd_init_cmd_t init_cmds[] = {
		{ 0xCF, { 0x00, 0x83, 0X30 }, 3 },
		{ 0xED, { 0x64, 0x03, 0X12, 0X81 }, 4 },
		{ 0xE8, { 0x85, 0x01, 0x79 }, 3 },
		{ 0xCB, { 0x39, 0x2C, 0x00, 0x34, 0x02 }, 5 },
		{ 0xF7, { 0x20 }, 1 },
		{ 0xEA, { 0x00, 0x00 }, 2 },
		{ 0xC0, { 0x26 }, 1 }, /*Power control*/
		{ 0xC1, { 0x11 }, 1 }, /*Power control */
		{ 0xC5, { 0x35, 0x3E }, 2 },
		/*VCOM control*/
		{ 0xC7, { 0xBE }, 1 }, /*VCOM control*/
		{ 0x36, { 0x28 }, 1 }, /*Memory Access Control*/
		{ 0x3A, { 0x55 }, 1 }, /*Pixel Format Set*/
		{ 0xB1, { 0x00, 0x1B }, 2 },
		{ 0xF2, { 0x08 }, 1 },
		{ 0x26, { 0x01 }, 1 },
		{ 0xE0, { 0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0X87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00 }, 15 },
		{ 0XE1, { 0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F }, 15 },
		{ 0x2A, { 0x00, 0x00, 0x00, 0xEF }, 4 },
		{ 0x2B, { 0x00, 0x00, 0x01, 0x3f }, 4 },
		{ 0x2C, { 0 }, 0 },
		{ 0xB7, { 0x07 }, 1 },
		{ 0xB6, { 0x0A, 0x82, 0x27, 0x00 }, 4 },
		{ 0x11, { 0 }, 0x80 },
		{ 0x29, { 0 }, 0x80 },
		{ 0, { 0 }, 0xff },
	};

	//Initialize non-SPI GPIOs
	gpio_pad_select_gpio(DISP_SPI_DC);
	gpio_set_direction((gpio_num_t)DISP_SPI_DC, GPIO_MODE_OUTPUT);
	
	gpio_pad_select_gpio(DISP_PIN_BCKL);
	gpio_set_direction((gpio_num_t)DISP_PIN_BCKL, GPIO_MODE_OUTPUT);
	gpio_set_level((gpio_num_t)DISP_PIN_BCKL, 1);
	
	
#if ST7796S_USE_RST
	gpio_pad_select_gpio(ST7796S_RST);
	gpio_set_direction(ST7796S_RST, GPIO_MODE_OUTPUT);

	//Reset the display
	gpio_set_level(ST7796S_RST, 0);
	vTaskDelay(100 / portTICK_RATE_MS);
	gpio_set_level(ST7796S_RST, 1);
	vTaskDelay(100 / portTICK_RATE_MS);
#endif

	ESP_LOGI(TAG, "Initialization.");

	//Send all the commands
	uint16_t cmd = 0;
	while (init_cmds[cmd].databytes != 0xff)
	{
		st7796s_send_cmd(init_cmds[cmd].cmd);
		st7796s_send_data(init_cmds[cmd].data, init_cmds[cmd].databytes & 0x1F);
		if (init_cmds[cmd].databytes & 0x80)
		{
			vTaskDelay(100 / portTICK_RATE_MS);
		}
		cmd++;
	}

	st7796s_set_orientation(CONFIG_LV_DISPLAY_ORIENTATION);

#if ST7796S_INVERT_COLORS == 1
	st7796s_send_cmd(0x21);
#else
	st7796s_send_cmd(0x20);
#endif
}




static void st7796s_send_color(void *data, uint16_t length)
{
	disp_wait_for_pending_transactions();
	gpio_set_level((gpio_num_t)DISP_SPI_DC, 1); /*Data mode*/
	disp_spi_send_colors((uint8_t*)data, length);
}

void st7796s_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
	uint8_t data[4];

	/*Column addresses*/
	st7796s_send_cmd(0x2A);
	data[0] = (area->x1 >> 8) & 0xFF;
	data[1] = area->x1 & 0xFF;
	data[2] = (area->x2 >> 8) & 0xFF;
	data[3] = area->x2 & 0xFF;
	st7796s_send_data(data, 4);

	/*Page addresses*/
	st7796s_send_cmd(0x2B);
	data[0] = (area->y1 >> 8) & 0xFF;
	data[1] = area->y1 & 0xFF;
	data[2] = (area->y2 >> 8) & 0xFF;
	data[3] = area->y2 & 0xFF;
	st7796s_send_data(data, 4);

	/*Memory write*/
	st7796s_send_cmd(0x2C);

	uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);

	st7796s_send_color((void *)color_map, size * 2);
}



static void lv_tick_task(FreeRTOS::Timer* timer)
{
	lv_tick_inc(1000);
}


void InitLVGL()
{
	ESP_LOGI(TAG, "Initializing SPI master for display");

	lvgl_spi_driver_init(
		SPI2_HOST,
		DISP_SPI_MISO,
		DISP_SPI_MOSI,
		DISP_SPI_CLK,
		SPI_BUS_MAX_TRANSFER_SZ,
		1,
		DISP_SPI_IO2,
		DISP_SPI_IO3);

	disp_spi_add_device(SPI2_HOST);

	st7796s_init();

	lv_init();			// Initialize lvgl
	
	static lv_disp_draw_buf_t draw_buf;
	static lv_color_t buf[LV_HOR_RES_MAX * 10];

	/* LVGL : Setting up buffer to use for display */
	lv_disp_draw_buf_init(&draw_buf, buf, NULL, LV_HOR_RES_MAX * 10);


	/*** LVGL : Setup & Initialize the display device driver ***/
	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res = LV_HOR_RES_MAX;
	disp_drv.ver_res = LV_VER_RES_MAX;
	disp_drv.flush_cb = st7796s_flush;
	disp_drv.draw_buf = &draw_buf;
	lv_disp_drv_register(&disp_drv);

	
	/*** LVGL : Setup & Initialize the input device driver ***/
	//static lv_indev_drv_t indev_drv;
	//lv_indev_drv_init(&indev_drv);
	//indev_drv.type = LV_INDEV_TYPE_POINTER;
	//indev_drv.read_cb = touch_driver_read;
	//lv_indev_drv_register(&indev_drv);
	
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










