#ifndef __I2C_STICK_ARDUINO_H__
#define __I2C_STICK_ARDUINO_H__

#include <Arduino.h>
#ifdef USE_TINYUSB
  #include <Adafruit_TinyUSB.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


// auto config
// ***********
//
// for a limited set of architectures only.
#ifdef ARDUINO_ARCH_RP2040
  #define HAS_EEPROM_H
  // no need for CORE1 yet!
  // #define HAS_CORE1
  #define BUFFER_COMMAND_ENABLE
  #ifdef USE_TINYUSB
    #define ENABLE_USB_MSC
  #endif // USB_TINYUSB
#endif // ARDUINO_ARCH_RP2040

// Some boards have Wire1 hooked up to the QWIIC connector...
#ifdef ARDUINO_ADAFRUIT_QTPY_RP2040
#define USE_WIRE1
#endif

// board information
#define xstr(s) str(s)
#define str(s) #s

#if defined(__AVR_DEVICE_NAME__) and defined(USB_PRODUCT)
#define BOARD_INFO xstr(__AVR_DEVICE_NAME__) "|" USB_PRODUCT
#elif defined(USB_MANUFACTURER) and defined(USB_PRODUCT)
#define BOARD_INFO USB_MANUFACTURER "|" USB_PRODUCT
#elif defined(USB_PRODUCT)
#define BOARD_INFO USB_PRODUCT
#elif defined(ARDUINO_TEENSY40)
#define BOARD_INFO "Teensy 4.0"
#elif defined(ARDUINO_TEENSY41)
#define BOARD_INFO "Teensy 4.1"
#elif defined(CONFIG_IDF_TARGET) // for ESP32
#define BOARD_INFO CONFIG_IDF_TARGET
#else
#define BOARD_INFO "Unknown"
#endif


// Configure QWIIC I2C Bus:
// ************************
// By default use Wire
// Some boards have Wire1 hooked up to the QWIIC connector...
#include <Wire.h>

#ifdef USE_WIRE1
  extern TwoWire Wire1;
  #define WIRE Wire1
#else
  #define WIRE Wire
#endif

#ifdef WIRE_BUFFER_SIZE
#define READ_BUFFER_SIZE WIRE_BUFFER_SIZE
#elif defined(SERIAL_BUFFER_SIZE)
#define READ_BUFFER_SIZE SERIAL_BUFFER_SIZE
#else
#define READ_BUFFER_SIZE 32
#endif


#ifdef __cplusplus
}
#endif

#endif // __I2C_STICK_HAL_ARDUINO_H__
