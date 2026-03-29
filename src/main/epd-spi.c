#include "epd-spi.h"
#include "driver/spi_master.h"


spi_device_handle_t epd_spi;

/**
 * epd_send_cmd() - Send command over SPI in command mode
 * @cmd: command to be sent
 *
 * Return: Non-zero code on error
 */
static esp_err_t epd_send_cmd(const uint8_t cmd) {
	int ret;
	
	ret = gpio_set_level(EPD_DC, GPIO_LVL_LOW);
	if (ret) {
		ESP_LOGE(TAG, "Error setting SPI to cmd mode: %d\n", ret);
		return ret;
	}


	spi_transaction_t t = {.length = 8, .tx_buffer = &cmd};
	ret = spi_device_polling_transmit(epd_spi, &t);
	if (ret) {
		ESP_LOGE(TAG, "Error sending cmd over SPI: %d\n", ret);
		return ret;
	}

	return ret;
}

/**
 * epd_send_data() - Send data over SPI in data mode
 * @cmd: command to be sent
 *
 * Return: Non-zero code on error
 */
static esp_err_t epd_send_data(const uint8_t data) {
	int ret;
	
	ret = gpio_set_level(EPD_DC, GPIO_LVL_HIGH);
	if (ret) {
		ESP_LOGE(TAG, "Error setting SPI to data mode: %d\n", ret);
		return ret;
	}

	spi_transaction_t t = {.length = 8, .tx_buffer = &data};
	ret = spi_device_polling_transmit(epd_spi, &t);
	if (ret) {
		ESP_LOGE(TAG, "Error sending data over SPI: %d\n", ret);
		return ret;
	}
	
	return ret;
}

/**
 * epd_update_display() - updates the display with given data
 * @buf: Pointer to buffer with data to be sent to the display
 * @size: Size of the buffer containing the data
 *
 * This function performs the following three steps:
 *	1. Send command CMD_WRITE_RAM
 * 	2. Writes the user-defined data to display RAM
 * 	3. Send command CMD_UPDT_CTRL2
 * 	4. Send data 0xC7 for full display refresh
 * 	5. Send command CMD_MASTER_ACT to start physical refresh
 *
 *
 * Return: Non-zero code on error
 */
esp_err_t epd_update_display(uint8_t *buf, size_t size) {
	esp_err_t ret = 0;

	if (size != 5000) {
		ESP_LOGE(TAG,
			 "Error %d: buffer to update display has incorrect size\n",
			 ret);
		return -1;
	}

	ret = epd_send_cmd(CMD_WRITE_RAM);
	if (ret) {
		ESP_LOGE(TAG, "Error %d: Can't send write-RAM command\n", ret);
		return ret;
	}

	for (size_t i = 0; i < size; i++) {
		ret = epd_send_data(buf[i]);
		if (ret) {
			ESP_LOGE(TAG, "Error sending data byte %d to display: %d\n",
				 i, ret);
			return ret;
		}
	}
	WAIT_BUSY;

	ret = epd_send_cmd(CMD_UPDT_CTRL2);
	if (ret) {
		ESP_LOGE(TAG, "Error sending Display update command: %d\n", ret);
		return ret;
	}

	ret = epd_send_data(0xC7);
	if (ret) {
		ESP_LOGE(TAG, "Error sending refresh area to whole display: %d\n", ret);
		return ret;
	}
	WAIT_BUSY;

	ret = epd_send_cmd(CMD_MASTER_ACT);
	if (ret) {
		ESP_LOGE(TAG, "Error sending master activation command: %d\n", ret);
		return ret;
	}
	WAIT_BUSY;

	return ret;
}

/**
 * epd_init_reset() - Performs reset routines necessary for initialization
 *
 * Return: non-zero value in case of error
 */
static esp_err_t epd_init_reset() {
	esp_err_t ret;

	// Initialization sequence: Hardware reset
	ret = gpio_set_level(EPD_RESET, GPIO_LVL_LOW);
	if (ret) {
		ESP_LOGE(TAG, "Error pulling EPD_RESET low: %d\n", ret);
		return ret;
	}
	vTaskDelay(pdMS_TO_TICKS(10));

	ret = gpio_set_level(EPD_RESET, GPIO_LVL_HIGH);
	if (ret) {
		ESP_LOGE(TAG, "Error pulling EPD_RESET high: %d\n", ret);
		return ret;
	}

	// wait while busy
	while (!gpio_get_level(EPD_BUSY))
		vTaskDelay(pdMS_TO_TICKS(10));

	// Initialization sequence: Software reset
	ret = epd_send_cmd(CMD_SOFT_RESET);
	if (ret) {
		ESP_LOGE(TAG, "Error sending cmd 0x12: %d\n", ret);
		return ret;
	}

	// wait while busy
	while (!gpio_get_level(EPD_BUSY))
		vTaskDelay(pdMS_TO_TICKS(10));

	// Initialization sequence: Driver output control
	ret = epd_send_cmd(CMD_DRV_OUT_CTRL);
	if (ret) {
		ESP_LOGE(TAG, "Error sending cmd 0x01: %d\n", ret);
		return ret;
	}
	// Set resolution to 200x200
	ret = epd_send_data(0xC7);
	if (ret) {
		ESP_LOGE(TAG, "Error sending data for full refresh: %d\n", ret);
		return ret;
	}
	epd_send_data(0x00);
	epd_send_data(0x00);

	// Border Waveform Control
	ret = epd_send_cmd(CMD_BRD_WVFRM);
	if (ret) {
		ESP_LOGE(TAG, "Error sending Border Waveform Control cmd: %d\n", ret);
		return ret;
	}
	ret = epd_send_data(0x05);
	if (ret) {
		ESP_LOGE(TAG, "Error sending Border behavior config: %d\n", ret);
		return ret;
	}

	// Temperature sensor select
	ret = epd_send_cmd(CMD_TMP_SENSOR);
	if (ret) {
		ESP_LOGE(TAG, "Error sending Temp Sensor select data: %d\n", ret);
		return ret;
	}
	ret = epd_send_data(0x80);
	if (ret) {
		ESP_LOGE(TAG, "Error selecting internal Temp Sensor: %d\n", ret);
		return ret;
	}

	return ret;
}

/**
 * epd_init() - Initialization function for the ePaper display
 *
 * Return: Non-zero code on error
 */
esp_err_t epd_init() {
	esp_err_t ret;
	
	// Initialize output pins
	gpio_config_t io_conf = {
		.pin_bit_mask	= (1ULL << EPD_DC) | (1ULL << EPD_RESET),
		.mode		= GPIO_MODE_OUTPUT,
		.pull_up_en	= GPIO_PULLUP_DISABLE,
	};
	ret = gpio_config(&io_conf);
	if (ret) {
		ESP_LOGE(TAG, "Error initializing GPIO Output pins: %d\n", ret);
		return ret;
	}

	// Initialize input pin (BUSY pin)
	io_conf.pin_bit_mask	= (1UL << EPD_BUSY);
	io_conf.mode		= GPIO_MODE_INPUT;
	io_conf.pull_up_en	= GPIO_PULLUP_ENABLE;
	ret = gpio_config(&io_conf);
	if (ret) {
		ESP_LOGE(TAG, "Error initializing GPIO input pin: %d\n", ret);
		return ret;
	}

	// Initialize SPI bus
	spi_bus_config_t buscfg = {
		.miso_io_num	= -1,
		.mosi_io_num	= EPD_MOSI,
		.sclk_io_num	= EPD_SCL,
		.quadwp_io_num	= -1,
		.quadhd_io_num	= -1,
	};
	ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
	if (ret) {
		ESP_LOGE(TAG, "Error initializing SPI bus: %d\n", ret);
		return ret;
	}

	spi_device_interface_config_t devcfg = {
		.clock_speed_hz	= EPD_CLOCK_SPEED,
		.mode		= 0,
		.spics_io_num	= EPD_CS,
		.queue_size	= 7,
	};
	ret = spi_bus_add_device(SPI2_HOST, &devcfg, &epd_spi);
	if (ret) {
		ESP_LOGE(TAG, "Error adding device to SPI bus: %d\n", ret);
		return ret;
	}

	// Perform reset routines for initialization
	ret = epd_init_reset();
	if (ret) {
		ESP_LOGE(TAG, "Error performing reset routines for init: %d\n",
			       ret);
	}

	return ret;
}

