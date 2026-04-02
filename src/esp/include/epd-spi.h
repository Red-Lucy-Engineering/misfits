#ifndef _EPD_SPI_H
#define _EPD_SPI_H

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_err.h"
#include "common-defs.h"

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


// 4MHz
#define EPD_CLOCK_SPEED 4 * 1000 * 1000


#define WAIT_BUSY while (gpio_get_level(EPD_BUSY)) vTaskDelay(pdMS_TO_TICKS(10))

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
		ESP_LOGE("epd_send_cmd", "Error setting SPI to cmd mode: %d\n", ret);
		return ret;
	}


	spi_transaction_t t = {.length = 8, .tx_buffer = &cmd};
	ret = spi_device_polling_transmit(epd_spi, &t);
	if (ret) {
		ESP_LOGE("epd_send_cmd", "Error sending cmd over SPI: %d\n", ret);
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
		ESP_LOGE("epd_send_data", "Error setting SPI to data mode: %d\n", ret);
		return ret;
	}

	spi_transaction_t t = {.length = 8, .tx_buffer = &data};
	ret = spi_device_polling_transmit(epd_spi, &t);
	if (ret) {
		ESP_LOGE("epd_send_data", "Error sending data over SPI: %d\n", ret);
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

	ESP_LOGI("epd_init_reset", "Set Reset GPIO pin low\n");

	// Initialization sequence: Hardware reset
	ret = gpio_set_level(EPD_RESET, GPIO_LVL_LOW);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error pulling EPD_RESET low: %d\n", ret);
		return ret;
	}
	vTaskDelay(pdMS_TO_TICKS(10));
	ESP_LOGI("epd_init_reset", "Setting Reset pin low successful\n");

	ESP_LOGI("epd_init_reset", "Setting Reset pin high\n");
	ret = gpio_set_level(EPD_RESET, GPIO_LVL_HIGH);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error pulling EPD_RESET high: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "Setting Reset pin high successful\n");

	// wait while busy
	WAIT_BUSY;

	ESP_LOGI("epd_init_reset", "Send command CMD_SOFT_RESET\n");
	// Initialization sequence: Software reset
	ret = epd_send_cmd(CMD_SOFT_RESET);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending cmd 0x12: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "CMD_SOFT_RESET successful\n");

	// wait while busy
	WAIT_BUSY;

	ESP_LOGI("epd_init_reset", "Send command CMD_DRV_OUT_CTRL\n");
	// Initialization sequence: Driver output control
	ret = epd_send_cmd(CMD_DRV_OUT_CTRL);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending cmd 0x01: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "CMD_DRV_OUT_CTRL successful\n");

	ESP_LOGI("epd_init_reset", "Send data 0xC7\n");
	// Set resolution to 200x200
	ret = epd_send_data(0xC7);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending data for full refresh: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "Send data successful\n");
	ESP_LOGI("epd_init_reset", "send zero byte\n");
	epd_send_data(0x00);
	ESP_LOGI("epd_init_reset", "send zero byte\n");
	epd_send_data(0x00);
	ESP_LOGI("epd_init_reset", "zero bytes successful\n");


	ESP_LOGI("epd_init_reset", "send command CMD_BRD_WVFRM\n");
	// Border Waveform Control
	ret = epd_send_cmd(CMD_BRD_WVFRM);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending Border Waveform Control cmd: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "CMD_BRD_WVFRM successful\n");

	ESP_LOGI("epd_init_reset", "Send data 0x05 for border behavior config\n");
	ret = epd_send_data(0x05);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending Border behavior config: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "border behavior config successful\n");

	ESP_LOGI("epd_init_reset", "send command CMD_TMP_SENSOR\n");
	// Temperature sensor select
	ret = epd_send_cmd(CMD_TMP_SENSOR);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error sending Temp Sensor select data: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "CMD_TMP_SENSOR successful\n");

	ESP_LOGI("epd_init_reset", "send data 0x80 for selecting internal temp sensor\n");
	ret = epd_send_data(0x80);
	if (ret) {
		ESP_LOGE("epd_init_reset", "Error selecting internal Temp Sensor: %d\n", ret);
		return ret;
	}
	ESP_LOGI("epd_init_reset", "sending data successful, reset routine successful\n");

	return ret;
}

esp_err_t epd_init();

#endif //_EPD_SPI_H

