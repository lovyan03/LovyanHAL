/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_PLATFORM_CHECK_HPP_
#define LOVYANHAL_PLATFORM_CHECK_HPP_

#define LHAL_PLATFORM_NUMBER_HOST_WINDOWS     1
#define LHAL_PLATFORM_NUMBER_HOST_LINUX       2

#define LHAL_PLATFORM_NUMBER_HOST_MAX        99

#define LHAL_PLATFORM_NUMBER_ARDUINO        100
#define LHAL_PLATFORM_NUMBER_AVR            200
#define LHAL_PLATFORM_NUMBER_ESP8266        300
#define LHAL_PLATFORM_NUMBER_ESP32_UNKNOWN  310
#define LHAL_PLATFORM_NUMBER_ESP32          311
#define LHAL_PLATFORM_NUMBER_ESP32_S2       312
#define LHAL_PLATFORM_NUMBER_ESP32_S3       313
#define LHAL_PLATFORM_NUMBER_ESP32_C3       314
#define LHAL_PLATFORM_NUMBER_SAMD21         400
#define LHAL_PLATFORM_NUMBER_SAMD51         410
#define LHAL_PLATFORM_NUMBER_RP2040         500
#define LHAL_PLATFORM_NUMBER_SPRESENSE      600
#define LHAL_PLATFORM_NUMBER_STM32          700


#if defined ( _WIN64 ) || defined ( _WIN32 )

 #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_HOST_WINDOWS
 #define LHAL_TARGET_PLATFORM platforms_host/windows

#elif defined ( ESP_PLATFORM )

 #if __has_include (<sdkconfig.h>)
  #include <sdkconfig.h>
 #endif

 #if defined ( CONFIG_IDF_TARGET_ESP32C3 )

  #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_ESP32_C3
  #define LHAL_TARGET_PLATFORM platforms/esp32/esp32c3

 #elif defined ( CONFIG_IDF_TARGET_ESP32S3 )

  #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_ESP32_S3
  #define LHAL_TARGET_PLATFORM platforms/esp32/esp32s3

 #elif defined ( CONFIG_IDF_TARGET_ESP32S2 )

  #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_ESP32_S2
  #define LHAL_TARGET_PLATFORM platforms/esp32/esp32s2

 #elif defined ( CONFIG_IDF_TARGET_ESP32 ) || !defined ( CONFIG_IDF_TARGET )

  #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_ESP32
  #define LHAL_TARGET_PLATFORM platforms/esp32/esp32

 #else
  // It may be a new model that is not yet supported.
  #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_ESP32_UNKNOWN
  #define LHAL_TARGET_PLATFORM platforms/esp32

 #endif

#elif defined ( __AVR__ )

 #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_AVR
 #define LHAL_TARGET_PLATFORM platforms/avr

#elif defined ( ARDUINO )

 #define LHAL_TARGET_PLATFORM_NUMBER LHAL_PLATFORM_NUMBER_ARDUINO
 #define LHAL_TARGET_PLATFORM platforms/arduino

#endif

#endif
