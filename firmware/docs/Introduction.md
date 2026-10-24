# misfits Firmware Documentation - Introduction

"misfits" is an ESP32-S3-based, open-source, freely programmable e-paper
smartwatch. This repository contains the official firmware for the device,
providing accurate timekeeping, Bluetooth Low Energy (BLE) synchronization,
an ultra-low-power e-paper display subsystem, and multi-language support.

## Project Overview

The misfits smartwatch is designed around the Espressif ESP32-S3 microcontroller
and a monochrome electrophoretic display (E-Ink). The primary design goals of the
firmware are:

- **Energy Efficiency**: Minimizing power consumption through efficient display
  refresh cycles and support for deep sleep with timer and GPIO wakeups.
- **Accurate Timekeeping**: Hardware RTC timekeeping complemented by BLE Current
  Time Service (CTS) automatic synchronization with smartphones or host devices.
- **Display Longevity & Quality**: Managed partial and full refresh strategies to
  prevent display ghosting while providing fast update times.
- **Extensible & Clean Codebase**: Modular architecture cleanly separating
  Bluetooth connectivity, display driving, typography, localization, and system
  lifecycle management.

## Hardware Specifications

| Component | Specification | Description / Pinout |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32-S3-MINI-1 | Dual-core Xtensa LX7, 2.4 GHz Wi-Fi & BLE 5.0 |
| **Flash Memory** | 2 MB Quad/Dual SPI Flash | Configured in DIO mode at 80 MHz |
| **Display Panel** | GDEY0154D67 | 1.54-inch monochrome E-Paper display (200x200 pixels) |
| **Display Interface** | 4-Wire SPI (SPI2_HOST) | SCK: GPIO 12, MOSI: GPIO 11, DC: GPIO 9, CS: GPIO 10 |
| **Display Control** | Dedicated GPIO | BUSY: GPIO 18, RESET: GPIO 8 |
| **Wakeup Input** | External Button | GPIO 0 (active-low with internal pull-up) |

## Firmware Module Structure

The firmware source code resides in the `main/` directory and is structured into
discrete, loosely-coupled modules:

```text
main/
├── firmware.c         # Application entry point, main loop, and state machine
├── ble.h / ble.c      # Bluetooth Low Energy, NimBLE stack, and CTS time synchronization
├── eink.h / eink.c    # E-Paper display initialization, framebuffer, and rendering routines
├── glyph.h / glyph.c  # UTF-8 text decoding, font binary search, and glyph blitting
├── freemono_fonts.h   # Pre-rendered FreeMono bitmap font data (small and large variants)
├── localisation.h     # Compile-time language selection and localized date string tables
├── idf_component.yml  # ESP-IDF component manifest (gdey0154d67 driver dependency)
└── CMakeLists.txt     # Component build script defining source files and dependencies
```

### Module Responsibilities

1. **System & Lifecycle (`firmware.c`)**:
   Initializes hardware, configures POSIX time zones, checks time validity,
   controls display refresh intervals, manages persistent RTC states, and coordinates
   the active application loop.
2. **Display Subsystem (`eink.h`, `eink.c`)**:
   Initializes the SPI bus and the GDEY0154D67 panel controller. Maintains a
   5,000-byte 1-bit monochrome framebuffer and handles full vs. partial refresh
   cycles to balance update speed and image quality.
3. **Typography Engine (`glyph.h`, `glyph.c`, `freemono_fonts.h`)**:
   Provides UTF-8 string parsing, O(log N) binary search glyph lookup, bounding
   box calculation, and pixel-exact bitmap blitting into the monochrome
   framebuffer.
4. **Bluetooth Low Energy Subsystem (`ble.h`, `ble.c`)**:
   Integrates the Apache NimBLE stack to advertise the device as "misfits", host
   the GATT Current Time Service (CTS, UUID 0x1805), and allow connected devices
   to read and write the system clock.
5. **Localization (`localisation.h`)**:
   Provides localized names for weekdays and months with support for
   English and German.

## Documentation Index

The complete firmware documentation is structured as follows:

1. [Table of Contents](Table_of_Contents.md)
2. [Introduction](Introduction.md)
3. [Build and Flash Guide](Build_and_flash.md)
4. [Firmware Architecture and Lifecycle](Firmware_Architecture.md)
5. [Display Subsystem](Display_Subsystem.md)
6. [Typography and Font Engine](Typography_and_Fonts.md)
7. [Bluetooth Low Energy (BLE)](Bluetooth_Low_Energy.md)
8. [Localization](Localization.md)
