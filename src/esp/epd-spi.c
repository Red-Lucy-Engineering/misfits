#include "epd-spi.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include <string.h>

#include "esp_heap_caps.h"

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
 * 	3. Send command CMD_SOFT_START
 * 	4. Send command CMD_UPDT_CTRL2
 * 	5. Send data 0xC7 for full display refresh
 * 	6. Send command CMD_MASTER_ACT to start physical refresh
 *
 *
 * Return: Non-zero code on error
 */
esp_err_t epd_update_display(uint8_t *buf, size_t size) {
	esp_err_t ret = 0;

	ESP_LOGI("epd_update_display", "start");

	if (size != 5000) {
		ESP_LOGE("epd_update_display",
			 "Error %d: buffer to update display has incorrect size",
			 ret);
		return -1;
	}


	ESP_LOGI("epd_update_display", "resetting RAM address positions");

	ret = epd_send_cmd(CMD_SETRAM_X);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error %d: Can't send X position", ret);
		return ret;
	}
	epd_send_data(0x00);
	epd_send_data(0x18);

	ESP_LOGI("epd_update_display", "X position set successfully");


	ret = epd_send_cmd(CMD_SETRAM_Y);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error %d: Can't send Y position", ret);
		return ret;
	}
	epd_send_data(0x00);
	epd_send_data(0x00);
	epd_send_data(0xC7);
	epd_send_data(0x00);

	ESP_LOGI("epd_update_display", "Y position set successfully");

	ESP_LOGI("epd_update_display", "Set Address counter X to 0");
	ret = epd_send_cmd(CMD_SETX_ADDR_CT);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error %d: Can't set X counter", ret);
		return ret;
	}
	epd_send_data(0x00);
	ESP_LOGI("epd_update_display", "X counter set successfully");

	ESP_LOGI("epd_update_display", "Set Address counter Y to 0");
	ret = epd_send_cmd(CMD_SETY_ADDR_CT);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error %d: Can't set Y counter", ret);
		return ret;
	}
	epd_send_data(0x00);
	epd_send_data(0x00);
	ESP_LOGI("epd_update_display", "Y counter set successfully");

	ESP_LOGI("epd_update_display", "sending command 0x26 (red RAM)");
	ret = epd_send_cmd(0x26);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error %d: Can't send command", ret);
		return ret;
	}
	ESP_LOGI("epd_update_display", "command successful");

	uint8_t *redbuf = (uint8_t *)heap_caps_malloc(5000,
						      MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

	if (redbuf != NULL) {
		memset(redbuf, 0xFF, 5000);
	} else {
		ESP_LOGE("epd_update_display", "couldn't allocate memory for redbuf");
		return -1;
	}

	ESP_LOGI("epd_update_display", "Sending buffer");
	ret = epd_send_data_bulk(redbuf, 5000); // testvalue
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error sending data to display: %d",
			 ret);
	}
	if (ret)
		return ret;
	ESP_LOGI("epd_update_display", "Buffer data sent");

	ESP_LOGI("epd_update_display", "resetting RAM address positions");

	ret = epd_send_cmd(CMD_SETRAM_X);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error %d: Can't send X position", ret);
		return ret;
	}
	epd_send_data(0x00);
	epd_send_data(0x18);

	ESP_LOGI("epd_update_display", "X position set successfully");


	ret = epd_send_cmd(CMD_SETRAM_Y);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error %d: Can't send Y position", ret);
		return ret;
	}
	epd_send_data(0x00);
	epd_send_data(0x00);
	epd_send_data(0xC7);
	epd_send_data(0x00);

	ESP_LOGI("epd_update_display", "Y position set successfully");

	ESP_LOGI("epd_update_display", "Set Address counter X to 0");
	ret = epd_send_cmd(CMD_SETX_ADDR_CT);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error %d: Can't set X counter", ret);
		return ret;
	}
	epd_send_data(0x00);
	ESP_LOGI("epd_update_display", "X counter set successfully");

	ESP_LOGI("epd_update_display", "Set Address counter Y to 0");
	ret = epd_send_cmd(CMD_SETY_ADDR_CT);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error %d: Can't set Y counter", ret);
		return ret;
	}
	epd_send_data(0x00);
	ESP_LOGI("epd_update_display", "Y counter set successfully");

	ESP_LOGI("epd_update_display", "sending command CMD_WRITE_RAM");
	ret = epd_send_cmd(CMD_WRITE_RAM);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error %d: Can't send write-RAM command", ret);
		return ret;
	}
	ESP_LOGI("epd_update_display", "command successful");

	ESP_LOGI("epd_update_display", "Sending buffer");
	ret = epd_send_data_bulk(buf, 5000);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error sending data to display: %d",
			 ret);
	}
	if (ret)
		return ret;

	ESP_LOGI("epd_update_display", "Buffer data sent");


	ESP_LOGI("epd_update_display", "sending command CMD_UPDT_CTRL2");
	ret = epd_send_cmd(CMD_UPDT_CTRL2);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error sending Display update command: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_update_display", "command successful");

	ESP_LOGI("epd_update_display", "sending refresh area for whole screen");
	ret = epd_send_data(0xF7);
	if (ret) {
		ESP_LOGE("epd_update_display",
			 "Error sending refresh area to whole display: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_update_display", "sending data successful");

	ESP_LOGI("epd_update_display", "sending command CMD_MASTER_ACT");
	ret = epd_send_cmd(CMD_MASTER_ACT);
	if (ret) {
		ESP_LOGE("epd_udpate_display",
			 "Error sending master activation command: %d", ret);
		return ret;
	}
	WAIT_BUSY;

	ESP_LOGI("epd_update_display", "sending command successful");

	heap_caps_free(redbuf);
	return ret;
}


/**
 * epd_init() - Initialization function for the ePaper display
 *
 * Return: Non-zero code on error
 */
esp_err_t epd_init() {
	esp_err_t ret;
	
	ESP_LOGI("epd_init", "Starting to configure gpio output pins");

	// Initialize output pins
	gpio_config_t io_conf = {
		.pin_bit_mask	= (1ULL << EPD_DC) | (1ULL << EPD_RESET),
		.mode		= GPIO_MODE_OUTPUT,
		.pull_up_en	= GPIO_PULLUP_DISABLE,
	};
	ret = gpio_config(&io_conf);
	if (ret) {
		ESP_LOGE("epd_init", "Error initializing GPIO Output pins: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init", "configured gpio output pins");

	ESP_LOGI("epd_init", "Configure input gpio pin");
	// Initialize input pin (BUSY pin)
	io_conf.pin_bit_mask	= (1UL << EPD_BUSY);
	io_conf.mode		= GPIO_MODE_INPUT;
	io_conf.pull_up_en	= GPIO_PULLUP_ENABLE;
	ret = gpio_config(&io_conf);
	if (ret) {
		ESP_LOGE("epd_init", "Error initializing GPIO input pin: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init", "configured gpio input pins");

	ESP_LOGI("epd_init", "Configure SPI bus");
	// Initialize SPI bus
	spi_bus_config_t buscfg = {
		.miso_io_num	 = -1,
		.mosi_io_num	 = EPD_MOSI,
		.sclk_io_num	 = EPD_SCL,
		.quadwp_io_num	 = -1,
		.quadhd_io_num	 = -1,
		.max_transfer_sz = 6000
	};
	ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
	if (ret) {
		ESP_LOGE("epd_init", "Error initializing SPI bus: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init", "Configured SPI bus");

	ESP_LOGI("epd_init", "Configure SPI interface");
	spi_device_interface_config_t devcfg = {
		.clock_speed_hz	= EPD_CLOCK_SPEED,
		.mode		= 0,
		.spics_io_num	= EPD_CS,
		.queue_size	= 7,
	};
	ret = spi_bus_add_device(SPI2_HOST, &devcfg, &epd_spi);
	if (ret) {
		ESP_LOGE("epd_update", "Error adding device to SPI bus: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init", "Configured SPI interface");

	ESP_LOGI("epd_init", "Begin reset routines");
	// Perform reset routines for initialization
	ret = epd_init_reset();
	if (ret) {
		ESP_LOGE("epd_udpate", "Error performing reset routines for init: %d",
			       ret);
	}
	ESP_LOGI("epd_init", "Finished reset routines, ePaper display initialized");

	return ret;
}

