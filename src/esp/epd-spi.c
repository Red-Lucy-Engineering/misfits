#include "epd-spi.h"
#include "driver/spi_master.h"


spi_device_handle_t epd_spi;


/**
 * epd_update_display() - updates the display with given data
 * @buf: Pointer to buffer with data to be sent to the display
 * @size: Size of the buffer containing the data in bytes (expected to be
 *	  5000 bytes (40000 bits))
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

	ESP_LOGI("epd_update_display", "start\n");

	if (size != 5000) {
		ESP_LOGE("epd_update_display",
			 "Error %d: buffer to update display has incorrect size\n",
			 ret);
		return -1;
	}

	ESP_LOGI("epd_update_display", "sending command CMD_WRITE_RAM\n");
	ret = epd_send_cmd(CMD_WRITE_RAM);
	if (ret) {
		ESP_LOGE("epd_update_display", "Error %d: Can't send write-RAM command\n", ret);
		return ret;
	}
	ESP_LOGI("epd_update_display", "command successful\n");

	for (size_t i = 0; i < size; i++) {
		ESP_LOGI("epd_update_display", "sending byte %d as data\n",
			  i);
		ret = epd_send_data(buf[i]);
		vTaskDelay(pdMS_TO_TICKS(10));
		if (ret) {
			ESP_LOGE("epd_update_display", "Error sending data byte %d to display: %d\n",
				 i, ret);
			break;
		}
		ESP_LOGI("epd_update_display", "byte %d sent successful\n", i);
	}
	if (ret)
		return ret;
	WAIT_BUSY;

	ESP_LOGI("epd_update_display", "sending command CMD_UPDT_CTRL2\n");
	ret = epd_send_cmd(CMD_UPDT_CTRL2);
	if (ret) {
		ESP_LOGE("epd_update_display", "Error sending Display update command: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_update_display", "command successful\n");

	ESP_LOGI("epd_update_display", "sending refresh area for whole screen\n");
	ret = epd_send_data(0xC7);
	if (ret) {
		ESP_LOGE("epd_update_display", "Error sending refresh area to whole display: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_update_display", "sending data successful\n");
	WAIT_BUSY;

	ESP_LOGI("epd_update_display", "sending command CMD_MASTER_ACT\n");
	ret = epd_send_cmd(CMD_MASTER_ACT);
	if (ret) {
		ESP_LOGE("epd_udpate_display", "Error sending master activation command: %d\n", ret);
		return ret;
	}
	WAIT_BUSY;
	ESP_LOGI("epd_update_display", "sending command successful\n");

	return ret;
}


/**
 * epd_init() - Initialization function for the ePaper display
 *
 * Return: Non-zero code on error
 */
esp_err_t epd_init() {
	esp_err_t ret;
	
	ESP_LOGI("epd_init", "Starting to configure gpio output pins\n");

	// Initialize output pins
	gpio_config_t io_conf = {
		.pin_bit_mask	= (1ULL << EPD_DC) | (1ULL << EPD_RESET),
		.mode		= GPIO_MODE_OUTPUT,
		.pull_up_en	= GPIO_PULLUP_DISABLE,
	};
	ret = gpio_config(&io_conf);
	if (ret) {
		ESP_LOGE("epd_init", "Error initializing GPIO Output pins: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init", "configured gpio output pins\n");

	ESP_LOGI("epd_init", "Configure input gpio pin\n");
	// Initialize input pin (BUSY pin)
	io_conf.pin_bit_mask	= (1UL << EPD_BUSY);
	io_conf.mode		= GPIO_MODE_INPUT;
	io_conf.pull_up_en	= GPIO_PULLUP_ENABLE;
	ret = gpio_config(&io_conf);
	if (ret) {
		ESP_LOGE("epd_init", "Error initializing GPIO input pin: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init", "configured gpio input pins\n");

	ESP_LOGI("epd_init", "Configure SPI bus\n");
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
		ESP_LOGE("epd_init", "Error initializing SPI bus: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init", "Configured SPI bus\n");

	ESP_LOGI("epd_init", "Configure SPI interface\n");
	spi_device_interface_config_t devcfg = {
		.clock_speed_hz	= EPD_CLOCK_SPEED,
		.mode		= 0,
		.spics_io_num	= EPD_CS,
		.queue_size	= 7,
	};
	ret = spi_bus_add_device(SPI2_HOST, &devcfg, &epd_spi);
	if (ret) {
		ESP_LOGE("epd_update", "Error adding device to SPI bus: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init", "Configured SPI interface\n");

	ESP_LOGI("epd_init", "Begin reset routines\n");
	// Perform reset routines for initialization
	ret = epd_init_reset();
	if (ret) {
		ESP_LOGE("epd_udpate", "Error performing reset routines for init: %d\n",
			       ret);
	}
	ESP_LOGI("epd_init", "Finished reset routines, ePaper display initialized\n");

	return ret;
}

