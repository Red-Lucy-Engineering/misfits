# misfits Firmware Documentation - Build and Flash Guide

This document outlines the prerequisites, dependency management, configuration,
building, flashing, and monitoring procedures for the misfits firmware.

## Prerequisites

Before building the firmware, ensure that your development machine has the
following tools installed and configured:

1. **ESP-IDF**:
   - Supported versions: ESP-IDF v5.0 or newer (the component manager and target
     architecture support ESP-IDF v5.x and v6.x).
   - Ensure your environment variables are configured in your shell:
     ```bash
     . $HOME/esp/esp-idf/export.sh
     ```
2. **Build Tools**:
   - CMake (v3.22 or higher)
   - Ninja build system
   - Python 3.8+ with standard ESP-IDF virtual environment dependencies
3. **Hardware Connection**:
   - USB cable connecting the ESP32-S3 board to your computer.
   - Appropriate device permissions (e.g. adding your user to the `dialout` or
     `uucp` group on Linux):
     ```bash
     sudo usermod -aG dialout $USER
     ```

## Dependency Management

The project uses the **ESP-IDF Component Manager** to manage external
dependencies. 

The configuration file is located at
[`main/idf_component.yml`](../main/idf_component.yml):

```yaml
dependencies:
  idf:
    version: '>=4.1.0'
  kk-kk-k/gdey0154d67: ^1.1.0
```

The component manager automatically resolves and downloads the required E-Paper
panel driver (`kk-kk-k/gdey0154d67`) into the `managed_components/` directory
during the first build invocation. Lock file state is stored in
[`dependencies.lock`](../dependencies.lock).

## Configuration

The misfits firmware is preconfigured for the ESP32-S3 target architecture with
a 2 MB flash footprint.

### Target Selection
When building for the first time or after a full clean, set the target to ESP32-S3:

```bash
idf.py set-target esp32s3
```

### Menuconfig (untested)
To review or modify system parameters:

```bash
idf.py menuconfig
```

Key configuration parameters include:
- **Serial Flasher Config**:
  - Flash SPI mode: `DIO`
  - Flash SPI speed: `80 MHz`
  - Flash size: `2 MB`
- **Component config -> Bluetooth**:
  - Bluetooth Host: `NimBLE - BLE only`
- **Compiler Options**:
  - Preprocessor definitions can be added here or in CMakeLists for language
    selection (e.g., `-DCFG_LANG_DE` or `-DCFG_LANG_EN`).

## Building the Firmware

To compile the firmware and generate the flash binaries:

```bash
idf.py build
```

This command runs CMake, invokes Ninja, fetches any missing components via the
ESP-IDF component manager, compiles all source files (`firmware.c`, `ble.c`,
`eink.c`, `glyph.c`), and outputs the final binaries to the `build/` directory:
- `bootloader/bootloader.bin`
- `partition_table/partition-table.bin`
- `firmware.bin`

## Flashing to the Hardware

Identify the serial port assigned to your ESP32-S3 device (typically
`/dev/ttyUSB0` or `/dev/ttyACM0` on Linux, or `COMx` on Windows).

To flash the compiled binaries to the microcontroller:

```bash
idf.py -p /dev/ttyACM0 flash
```

Replace `/dev/ttyACM0` with your actual serial port.

### Entering Bootloader Mode Manually
If the device fails to enter download mode automatically:
1. Press and hold the **BOOT** button (connected to GPIO 0).
2. Press and release the **RESET** button (or power cycle the device).
3. Release the **BOOT** button.
4. Execute `idf.py -p <PORT> flash` again.

## Monitoring Serial Output

To inspect serial log outputs in real time:

```bash
idf.py -p /dev/ttyUSB0 monitor
```

To flash and immediately start monitoring in a single command:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

### Key Serial Log Tags
When monitoring, look for log entries tagged with:
- `misfits`: General application flow, time synchronization status, and screen
  render events.
- `misfits_ble`: NimBLE stack events, GAP connection state, and CTS time updates.

## Clean Build and Troubleshooting

### Erasing Flash Memory
If the non-volatile storage (NVS) contains invalid data or bonding tables need
to be cleared:

```bash
idf.py -p /dev/ttyUSB0 erase-flash
```

After erasing the flash, re-flash the device using `idf.py flash`.

### Full Project Clean
To delete all generated build artifacts and re-trigger CMake configuration:

```bash
idf.py fullclean
```
