idf_component_register(SRCS "firmware.c"
                            "ble.c"
                            "eink.c"
                            "glyph.c"
                       INCLUDE_DIRS "."
                       REQUIRES gdey0154d67 esp_lcd driver bt nvs_flash)

