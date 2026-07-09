/******************************************************************************
 * InfoTerm
 * Version : 0.0.4
 * Module  : TFT_eSPI user setup
 *
 * This file contains a known-working TFT_eSPI configuration for a 240x320
 * ST7789-based ESP32 touchscreen board used during InfoTerm development.
 *
 * Usage:
 *   Copy this file to the TFT_eSPI library folder and replace the existing
 *   User_Setup.h, or include it from your own TFT_eSPI configuration setup.
 *
 * Important:
 *   Display modules vary. If your screen stays black, colors are wrong or
 *   touch calibration does not match the display, verify the driver, pin
 *   mapping, color order and rotation for your specific hardware.
 ******************************************************************************/

#pragma once

// Display driver
// #define ILI9341_DRIVER
#define ST7789_DRIVER

// Display resolution
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Color order. Many ST7789 modules require BGR instead of RGB.
#define TFT_RGB_ORDER TFT_BGR

// SPI display pin mapping
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1

// Backlight control
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

// Font support
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

// SPI frequencies
#define SPI_FREQUENCY       40000000
#define SPI_TOUCH_FREQUENCY 2500000
