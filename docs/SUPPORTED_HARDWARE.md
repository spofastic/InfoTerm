# Supported Hardware

InfoTerm targets ESP32-based touchscreen devices using the TFT_eSPI display library.

## Tested Configuration

The included example `hardware/User_Setup.h` is prepared for:

- ESP32 board
- ST7789 display driver
- 240x320 resolution
- SPI display connection
- TFT_eSPI library
- Optional XPT2046 touchscreen support

## Display Configuration

Display compatibility depends heavily on the TFT_eSPI setup. If your display differs from the included configuration, adjust:

- Display driver
- Resolution
- SPI pins
- Backlight pin
- Color order
- SPI frequency
- Touch controller settings

## Notes

Many ESP32 touchscreen boards use similar displays but different pin mappings. Always verify your board schematic or vendor documentation before changing the firmware.
