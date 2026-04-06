#ifndef _EPD_SPI_H
#define _EPD_SPI_H

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_err.h"
#include "common-defs.h"

#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
// pins used for ePaper display
#define EPD_SCL		12
#define EPD_MOSI	11
#define EPD_CS		10
#define EPD_DC		9
#define EPD_RESET	8
#define EPD_BUSY	18

// General GPIO related things
#define GPIO_LVL_HIGH	1
#define GPIO_LVL_LOW	0


// Important commands for ePaper display
#define CMD_SOFT_RESET	0x12
#define CMD_DRV_OUT_CTRL 0x01
#define CMD_BRD_WVFRM	0x3C
#define CMD_TMP_SENSOR	0x18
#define CMD_UPDT_CTRL2	0x22
#define CMD_MASTER_ACT	0x20
#define CMD_WRITE_RAM	0x24
#define CMD_SOFT_START	0x0C
#define CMD_SETRAM_X	0x44
#define CMD_SETRAM_Y	0x45
#define CMD_SETX_ADDR_CT 0x4E
#define CMD_SETY_ADDR_CT 0x4F
#define CMD_DTA_ENTRY_MD 0x11
#define CMD_BOOSTER_CONF 0x06



// 4MHz
#define EPD_CLOCK_SPEED 4 * 1000 * 1000


#define WAIT_BUSY vTaskDelay(pdMS_TO_TICKS(4000))//while (gpio_get_level(EPD_BUSY) == 0) vTaskDelay(pdMS_TO_TICKS(10))

extern spi_device_handle_t	epd_spi;

/**
 * epd_send_cmd() - Send command over SPI in command mode
 * @cmd: command to be sent
 *
 * Return: Non-zero code on error
 */
static inline esp_err_t epd_send_cmd(const uint8_t cmd) {
	int ret;
	
	ret = gpio_set_level(EPD_DC, GPIO_LVL_LOW);
	if (ret) {
		ESP_LOGE("epd_send_cmd", "Error setting SPI to cmd mode: %d", ret);
		return ret;
	}


	spi_transaction_t t = {.length = 8, .tx_buffer = &cmd};
	ret = spi_device_polling_transmit(epd_spi, &t);
	if (ret) {
		ESP_LOGE("epd_send_cmd", "Error sending cmd over SPI: %d", ret);
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
static inline esp_err_t epd_send_data(const uint8_t data) {
	int ret;
	
	ret = gpio_set_level(EPD_DC, GPIO_LVL_HIGH);
	if (ret) {
		ESP_LOGE("epd_send_data", "Error setting SPI to data mode: %d", ret);
		return ret;
	}

	spi_transaction_t t = {.length = 8, .tx_buffer = &data};
	ret = spi_device_polling_transmit(epd_spi, &t);
	if (ret) {
		ESP_LOGE("epd_send_data", "Error sending data over SPI: %d", ret);
		return ret;
	}
	
	return ret;
}

/**
 * epd_send_data_bulk() - Send bulk data over SPI in data mode without flicking
 * CS on and off
 *
 * @data: pointer to data to be sent
 * @len: length of data
 *
 * Return: Non-zero code on error
 */
static inline esp_err_t epd_send_data_bulk(uint8_t *data, size_t len) {
	int ret;
	
	ret = gpio_set_level(EPD_DC, GPIO_LVL_HIGH);
	if (ret) {
		ESP_LOGE("epd_send_data_bulk", "Error setting SPI to data mode: %d", ret);
		return ret;
	}

	spi_transaction_t t = {
		.length = len * 8,
		.tx_buffer = data
	};
	ret = spi_device_polling_transmit(epd_spi, &t);
	if (ret) {
		ESP_LOGE("epd_send_data_bulk", "Error sending data over SPI: %d", ret);
		return ret;
	}
	
	return ret;
}

esp_err_t epd_update_display(uint8_t *buf, size_t size);

/**
 * epd_init_reset() - Performs reset routines necessary for initialization
 *
 * Return: non-zero value in case of error
 */
static inline esp_err_t epd_init_reset() {
	esp_err_t ret;

	ESP_LOGI("epd_init_reset", "Set Reset GPIO pin low");


	// Initialization sequence: Hardware reset
	ret = gpio_set_level(EPD_RESET, GPIO_LVL_LOW);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error pulling EPD_RESET low: %d", ret);
		return ret;
	}
	vTaskDelay(pdMS_TO_TICKS(10));
	ESP_LOGI("epd_init_reset", "Setting Reset pin low successful");

	ESP_LOGI("epd_init_reset", "Setting Reset pin high");
	ret = gpio_set_level(EPD_RESET, GPIO_LVL_HIGH);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error pulling EPD_RESET high: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "Setting Reset pin high successful");

	vTaskDelay(pdMS_TO_TICKS(200));

	// Initialization sequence: Software reset
	ESP_LOGI("epd_init_reset", "Send command CMD_SOFT_RESET");
	ret = epd_send_cmd(CMD_SOFT_RESET);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending cmd 0x12: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "CMD_SOFT_RESET successful");

	// wait 10ms after initial configuration
	vTaskDelay(pdMS_TO_TICKS(10));

	ESP_LOGI("epd_init_reset", "sending command CMD_SOFT_START");
	ret = epd_send_cmd(CMD_SOFT_START);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending softstart command: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "command successful");
	epd_send_data(0x17);
	epd_send_data(0x17);
	epd_send_data(0x17);
	epd_send_data(0x03);

	ESP_LOGI("epd_init_reset", "Send command CMD_DTA_ENTRY_MD");
	ret = epd_send_cmd(CMD_DTA_ENTRY_MD);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending cmd 0x11: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "CMD_DTA_ENTRY_MD successful");


	ESP_LOGI("epd_init_reset", "Send command CMD_DRV_OUT_CTRL");
	// Initialization sequence: Driver output control
	ret = epd_send_cmd(CMD_DRV_OUT_CTRL);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending cmd 0x01: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "CMD_DRV_OUT_CTRL successful");

	ESP_LOGI("epd_init_reset", "Send data 0xC7");
	// Set resolution to 200x200
	ret = epd_send_data(0xC7);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending data for full refresh: %d", ret);
		return ret;
	}
	epd_send_data(0x00);
	epd_send_data(0x00);
	ESP_LOGI("epd_init_reset", "Send data successful");


	ESP_LOGI("epd_init_reset", "send command CMD_BRD_WVFRM");
	// Border Waveform Control
	ret = epd_send_cmd(CMD_BRD_WVFRM);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending Border Waveform Control cmd: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "CMD_BRD_WVFRM successful");

	ESP_LOGI("epd_init_reset", "Send data 0x05 for border behavior config");
	ret = epd_send_data(0x05);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending Border behavior config: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "border behavior config successful");

	ESP_LOGI("epd_init_reset", "send command CMD_TMP_SENSOR");
	// Temperature sensor select
	ret = epd_send_cmd(CMD_TMP_SENSOR);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending Temp Sensor select data: %d", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "CMD_TMP_SENSOR successful");

	ESP_LOGI("epd_init_reset", "send data 0x80 for selecting internal temp sensor");
	ret = epd_send_data(0x80);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error selecting internal Temp Sensor: %d", ret);
		return ret;
	}

	ESP_LOGI("epd_init_reset", "sending CMD_UPDT_CTRL2 to load waveform LUT");
	// load waveform LUT
	ret = epd_send_cmd(CMD_UPDT_CTRL2);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending command CMD_UPDT_CTRL2: %d", ret);
		return ret;
	}

	ESP_LOGI("epd_init_reset", "Send data");;
	ret = epd_send_data(0xB1);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending LUT data: %d", ret);
		return ret;
	}
	
	ESP_LOGI("epd_init_reset", "sending command CMD_MASTER_ACT");

	ret = epd_send_cmd(CMD_MASTER_ACT);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending command CMD_MASTER_ACT: %d", ret);
		return ret;
	}


	WAIT_BUSY;
	ESP_LOGI("epd_init_reset", "loading waveform LUT successful");
		

	ESP_LOGI("epd_init_reset", "sending data successful, reset routine successful");
	return ret;
}

esp_err_t epd_init();

#endif //_EPD_SPI_H

