# misfits Firmware Documentation - Table of Contents

Welcome to the technical documentation for the misfits e-paper smartwatch
firmware.

## Navigation

1. **[Introduction](Introduction.md)**
   - Project overview, design goals, hardware specifications, and repository
     structure.

2. **[Build and Flash Guide](Build_and_flash.md)**
   - Prerequisites, ESP-IDF component management, target configuration,
     compilation, flashing, and serial monitoring.

3. **[Firmware Architecture and Lifecycle](Firmware_Architecture.md)**
   - System lifecycle, application entry (`app_main`), timekeeping initialization,
     persistent state management, main loop event handling, and power management.

4. **[Display Subsystem](Display_Subsystem.md)**
   - Good Display GDEY0154D67 e-paper controller, SPI configuration, 1bpp
     monochrome framebuffer, partial vs. full refresh cycles, ghosting mitigation,
     and UI layouts.

5. **[Typography and Font Engine](Typography_and_Fonts.md)**
   - Custom raster typography architecture, UTF-8 streaming parser, O(log N)
     binary search glyph lookup, text measurement, and pixel-exact bitmap blitting.

6. **[Bluetooth Low Energy (BLE)](Bluetooth_Low_Energy.md)**
   - Apache NimBLE stack integration, GAP advertising, GATT Current Time Service
     (CTS 0x1805), 10-byte time payload format, connection callbacks, and
     security configuration.

7. **[Localization](Localization.md)**
   - Compile-time language selection macros, localized day and month string
     tables, UTF-8 multi-byte support, and POSIX timezone configuration.
