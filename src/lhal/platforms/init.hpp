/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_PLATFORM_INIT_HPP_
#define LOVYANHAL_PLATFORM_INIT_HPP_

#if defined ( ARDUINO )
 #include <Arduino.h>
#endif

#include <stdint.h>
#include <stddef.h>

#define LHAL_PLATFORM_NUMBER_WINWOWS 10
#define LHAL_PLATFORM_NUMBER_LINUX   11
#define LHAL_PLATFORM_NUMBER_ARDUINO 20
#define LHAL_PLATFORM_NUMBER_AVR     30
#define LHAL_PLATFORM_NUMBER_ESP8266 40
#define LHAL_PLATFORM_NUMBER_ESP32   41
#define LHAL_PLATFORM_NUMBER_ESP32S2 42
#define LHAL_PLATFORM_NUMBER_ESP32S3 43
#define LHAL_PLATFORM_NUMBER_ESP32C3 44
#define LHAL_PLATFORM_NUMBER_SAMD21  50
#define LHAL_PLATFORM_NUMBER_SAMD51  51
#define LHAL_PLATFORM_NUMBER_STM32   60

#if defined ( _WIN64 ) || defined ( _WIN32 )

 #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_WINDOWS
 #define LHAL_TARGET_PLATFORM windows

#elif defined ( ESP_PLATFORM )

 #if __has_include (<sdkconfig.h>)
  #include <sdkconfig.h>
 #endif
/*
 #if defined ( CONFIG_IDF_TARGET_ESP32C3 )

  #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_ESP32C3
  #define LHAL_TARGET_PLATFORM esp32c3

 #elif defined ( CONFIG_IDF_TARGET_ESP32S3 )

  #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_ESP32S3
  #define LHAL_TARGET_PLATFORM esp32s3

 #elif defined ( CONFIG_IDF_TARGET_ESP32S2 )

  #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_ESP32S2
  #define LHAL_TARGET_PLATFORM esp32s2

 #elif defined ( CONFIG_IDF_TARGET_ESP32 ) || !defined ( CONFIG_IDF_TARGET )
*/
  #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_ESP32
  #define LHAL_TARGET_PLATFORM esp32

// #endif

#elif defined ( __AVR__ )

 #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_AVR
 #define LHAL_TARGET_PLATFORM avr

#elif defined ( ARDUINO )

 #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_ARDUINO
 #define LHAL_TARGET_PLATFORM arduino

#else

 #warning "unsupported platform..."

#endif

#define LHAL_LOCAL_INCLUDE(x) #x
#define LHAL_CONCAT(x, y) LHAL_LOCAL_INCLUDE(x/y)

#include LHAL_CONCAT(LHAL_TARGET_PLATFORM, init.hpp)
#include "../common.hpp"
#include LHAL_CONCAT(LHAL_TARGET_PLATFORM, LHAL.hpp)

#undef LHAL_CONCAT
#undef LHAL_LOCAL_INCLUDE

#endif
