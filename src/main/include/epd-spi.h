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


// 4MHz
#define EPD_CLOCK_SPEED 4 * 1000 * 1000


#define WAIT_BUSY while (!gpio_get_level(EPD_BUSY)) vTaskDelay(pdMS_TO_TICKS(10))

extern spi_device_handle_t	epd_spi;

static esp_err_t epd_send_cmd(const uint8_t cmd);
static esp_err_t epd_send_data(const uint8_t data);
esp_err_t epd_update_display(uint8_t *buf, size_t size);
static esp_err_t epd_init_reset();
esp_err_t epd_init();

#endif //_EPD_SPI_H

