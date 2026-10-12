# misfits Firmware Documentation - Display Subsystem

This document describes the e-paper display hardware interface, framebuffer
management, refresh cycle strategies, and user interface rendering implemented
in [`main/eink.h`](../main/eink.h) and [`main/eink.c`](../main/eink.c).

## Hardware Interface & Driver

The misfits watch uses a Good Display **GDEY0154D67** 1.54-inch electrophoretic
display (E-Paper) with a resolution of 200x200 pixels. The display is driven using
the ESP-IDF `esp_lcd` framework via the `kk-kk-k/gdey0154d67` component.

### Pinout and SPI Configuration

The panel connects to the ESP32-S3 over a 4-wire Serial Peripheral Interface
(SPI) using `SPI2_HOST`:

| Pin / Signal | GPIO | Description |
| :--- | :--- | :--- |
| **SCK** (SCLK) | GPIO 12 | SPI serial clock line (1 MHz clock rate) |
| **SDI** (MOSI) | GPIO 11 | SPI Master Out / Slave In data line |
| **DC** | GPIO 9 | Data / Command control line (Low = Command, High = Data) |
| **CS** | GPIO 10 | SPI chip select line (Active-low) |
| **BUSY** | GPIO 18 | Busy status output from display controller (Active-high) |
| **RES** (RESET)| GPIO 8 | Hardware reset line (Active-low) |

### Panel Initialization (`get_eink`)

The function [`get_eink()`](../main/eink.c) initializes both the SPI bus and the
panel device driver:

```c
const spi_bus_config_t buscfg = {
    .sclk_io_num = disp_cfg->sck,
    .mosi_io_num = disp_cfg->sdi,
    .miso_io_num = -1,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE
};
ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
```

The panel IO handle is created with 8-bit command and parameter widths in SPI
mode 0 (clock idle low, data captured on leading edge) at 1 MHz
(`pclk_hz = 1000000`).

The vendor configuration passes the `busy_pin` and sets `retain_ram = true` to
preserve internal display RAM between operations:

```c
esp_lcd_gdey0154d67_vendor_config_t esp_lcd_gdey0154d67_cfg = {
    .busy_pin = disp_cfg->busy,
    .retain_ram = disp_cfg->retain_ram,
};
```

## Framebuffer Memory Architecture

The display subsystem maintains an in-memory monochrome framebuffer in internal RAM:

```c
#define DISPLAY_W 200
#define DISPLAY_H 200

uint8_t framebuffer[DISPLAY_W * DISPLAY_H / 8]; // 5,000 bytes
```

### Pixel Layout and Color Polarity

- **Dimensions**: 200 pixels width × 200 pixels height = 40,000 pixels.
- **Bit Depth**: 1 bit per pixel (1 bpp), requiring exactly 5,000 bytes.
- **Byte Packing**: 8 horizontal pixels per byte.
- **Bit Order**: Most Significant Bit (MSB) first. The leftmost pixel of each
  byte corresponds to `0x80 >> (x % 8)`.
- **Display Polarity**:
  - `1` (Bit set): **White** (reflective state).
  - `0` (Bit cleared): **Black** (absorptive state).

### Buffer Clear Routine

The function [`framebuffer_clear()`](../main/eink.c) resets all bits to `1`
(0xFF), rendering a blank white canvas:

```c
void framebuffer_clear(void) {
    memset(framebuffer, 0xFF, sizeof(framebuffer));
}
```

## Refresh Cycle Strategies & Ghosting Mitigation

E-paper displays rely on physical electrophoretic particles suspended in
microcapsules. Repeated rapid updates without a full refresh cause residual
pigment particles to linger on screen, an artifact known as **ghosting**.

The misfits firmware manages refresh cycles via
[`update_display()`](../main/eink.c):

```c
#define FULL_UPDATE_INTERVAL 5

void update_display(esp_lcd_panel_handle_t eink) {
    if (partial_updates >= FULL_UPDATE_INTERVAL) {
        ESP_ERROR_CHECK(esp_lcd_gdey0154d67_set_update_mode(eink, esp_lcd_gdey0154d67_full_update));
        partial_updates = 0;
    } else {
        ESP_ERROR_CHECK(esp_lcd_gdey0154d67_set_update_mode(eink, esp_lcd_gdey0154d67_partial_update));
        partial_updates++;
    }

    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(eink, 0, 0, DISPLAY_W, DISPLAY_H, framebuffer));
}
```

### Refresh Mode Comparison

| Mode | Refresh Duration | Artifacts / Ghosting | Use Case |
| :--- | :--- | :--- | :--- |
| **Partial Update** | ~300 ms | Gradual ghosting accumulation | Regular minute-by-minute clock ticks |
| **Full Update** | ~1,500 - 2,000 ms | Completely eliminates ghosting | Initial boot, BLE sync event, every 5th minute |

### Refresh Schedule

1. **Every 5 Partial Refreshes**: The counter `partial_updates` increments on
   each minute tick. When it reaches `FULL_UPDATE_INTERVAL` (5), the next minute
   update triggers a full refresh cycle.
2. **Immediate Full Refresh on BLE Sync**: When a host central updates the device
   clock, `partial_updates` is reset to `FULL_UPDATE_INTERVAL`, ensuring the new
   time is immediately rendered with a full refresh to eliminate any remnants of
   the pairing screen.

## Screen Layouts and UI Rendering

### 1. Main Watch Face (`render_screen`)

The main clock face displays the current time in a large font and the localized
date in a small font. Both elements are centered horizontally on the 200x200
canvas.

```c
void render_screen(const struct tm *now) {
    char time_text[8];
    char date_text[32];

    snprintf(time_text, sizeof(time_text), "%02d:%02d",
             now->tm_hour, now->tm_min);
    snprintf(date_text, sizeof(date_text), "%s, %d. %s",
             weekday_names[now->tm_wday], now->tm_mday,
             month_names[now->tm_mon]);

    framebuffer_clear();

    int time_x = (DISPLAY_W - text_width(&freemono_large, time_text)) / 2;
    int date_x = (DISPLAY_W - text_width(&freemono_small, date_text)) / 2;

    draw_text(&freemono_large, time_x, 88, time_text);
    draw_text(&freemono_small, date_x, 140, date_text);
}
```

- **Time Text**: Baseline Y = 88, centered using
  [`freemono_large`](../main/freemono_fonts.h).
- **Date Text**: Baseline Y = 140, centered using
  [`freemono_small`](../main/freemono_fonts.h).

### 2. Pairing Screen (`render_pairing_screen`)

When the watch boots without a verified real-time clock timestamp, it renders a
multi-line prompt guiding the user to pair via BLE:

```c
void render_pairing_screen(void) {
    const char *line1 = "Willkommen bei";
    const char *line2 = "misfits";
    const char *line3 = "Bitte per Bluetooth";
    const char *line4 = "mit dem Smartphone";
    const char *line5 = "verbinden";

    framebuffer_clear();

    draw_text(&freemono_small, (DISPLAY_W - text_width(&freemono_small, line1)) / 2, 52, line1);
    draw_text(&freemono_small, (DISPLAY_W - text_width(&freemono_small, line2)) / 2, 76, line2);
    draw_text(&freemono_small, (DISPLAY_W - text_width(&freemono_small, line3)) / 2, 120, line3);
    draw_text(&freemono_small, (DISPLAY_W - text_width(&freemono_small, line4)) / 2, 144, line4);
    draw_text(&freemono_small, (DISPLAY_W - text_width(&freemono_small, line5)) / 2, 168, line5);
}
```

Each line is rendered using `freemono_small` and centered horizontally, with
baselines evenly distributed vertically (Y = 52, 76, 120, 144, 168).
