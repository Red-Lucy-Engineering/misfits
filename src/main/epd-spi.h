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

// 4MHz
#define EPD_CLOCK_SPEED 4 * 1000 * 1000

spi_device_handle_t	epd_spi;

/**
 * epd_gpio_init() - Initialization function for the ePaper display
 *
 * Return: Non-zero code on error
 */
esp_err_t epd_gpio_init() {
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
	io_conf.pull_up_pen	= GPIO_PULLUP_ENABLE;
	ret = gpio_config(&io_conf);
	if (ret) {
		ESP_LOGE(TAG, "Error initializing GPIO input pin: %d\n", ret);
		return ret;
	}

	// Initialize SPI bus
	spi_bus_config_t buscfg = {
		.miso_io_num	= -1,
		.mosi_io_num	= EPD_MOSI,
		.sclk_to_num	= EPD_SCL,
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

	return ret;
}

/**
 * epd_send_cmd() - Send command over SPI in command mode
 * @cmd: command to be sent
 *
 * Return: Non-zero code on error
 */
esp_err_t epd_send_cmd(const uint8_t cmd) {
	int ret;
	
	ret = gpio_set_level(EPD_DC, 0);
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
esp_err_t epd_send_data(const uint8_t data) {
	int ret;
	
	ret = gpio_set_level(EPD_DC, 1);
	if (ret) {
		ESP_LOGE(TAG, "Error setting SPI to data mode: %d\n", ret);
		return ret;
	}

	spi_transaction_t t = {.length = 8, .tx_buffer = data};
	ret = spi_device_polling_transmit(epd_spi, &t);
	if (ret) {
		ESP_LOGE(TAG, "Error sending data over SPI: %d\n", ret);
		return ret;
	}
	
	return ret;
}

