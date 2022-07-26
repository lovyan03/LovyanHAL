/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#pragma once

#include "init.hpp"
#include "../../LovyanHAL_Base.hpp"

#include <driver/gpio.h>

#if defined ( CONFIG_IDF_TARGET_ESP32C3 )
 #define LHAL_GPIO_NONEED_PORTCHECK
#endif

namespace lhal
{
  class LHAL : public LovyanHAL_Base
  {
  public:
    class GPIO_t : public GPIO_Base
    {
    public:
      class RAW
      {
      public:
#if defined ( LHAL_GPIO_NONEED_PORTCHECK )
        static constexpr inline volatile uint32_t* getInputReg(gpio::port_num_t = 0) { return &::GPIO.in; };
        static constexpr inline volatile uint32_t* getHighReg(gpio::port_num_t = 0) { return &::GPIO.out_w1ts; };
        static constexpr inline volatile uint32_t* getLowReg(gpio::port_num_t = 0) { return &::GPIO.out_w1tc; };
#else
        static inline volatile uint32_t* getInputReg(gpio::port_num_t port) { return (port & 1) ? (volatile uint32_t*)&::GPIO.in1 : &::GPIO.in; };
        static inline volatile uint32_t* getHighReg(gpio::port_num_t port) { return (port & 1) ? &::GPIO.out1_w1ts.val : &::GPIO.out_w1ts; };
        static inline volatile uint32_t* getLowReg(gpio::port_num_t port) { return (port & 1) ? &::GPIO.out1_w1tc.val : &::GPIO.out_w1tc; };
#endif
      };

      static RAW Raw;

      static inline void writePortHigh(gpio::port_num_t port, gpio::pin_mask_t bitmask) { *RAW::getHighReg(port) = bitmask; };
      static inline void writePortLow(gpio::port_num_t port, gpio::pin_mask_t bitmask) { *RAW::getLowReg(port) = bitmask; };
      static inline gpio::pin_mask_t readPort(gpio::port_num_t port, gpio::pin_mask_t bitmask) { return *RAW::getInputReg(port) & bitmask; };

      static void setMode(gpio::gpio_pin_t pin, mode_t mode);

      static inline void writeHigh(gpio::gpio_pin_t pin) { writePortHigh(getPortNum(pin), getPinMask(pin)); }
      static inline void writeLow(gpio::gpio_pin_t pin) { writePortLow(getPortNum(pin), getPinMask(pin)); }
      static inline void write(gpio::gpio_pin_t pin, bool value) { auto port = getPortNum(pin); *(value ? RAW::getHighReg(port) : RAW::getLowReg(port)) = getPinMask(pin); }
      static inline bool read(gpio::gpio_pin_t pin) { return readPort(getPortNum(pin), getPinMask(pin)); }
    };

    LHAL(void) : LovyanHAL_Base() {}

    static error_t init(void) { return error_t::err_ok; }

    static GPIO_t Gpio;

    /// Arduino環境のピン番号からMCUのポート+ピン番号に変換する…ESP32はMCUピン番号がそのままArduinoのピン番号となっているので変換が不要;
    static constexpr gpio::gpio_pin_t convertArduinoPinNumber(int arduino_pin_number)
    {
      return (gpio::gpio_pin_t)(GPIO_NUM_MAX > arduino_pin_number ? arduino_pin_number : ~0u);
    }

    static void delay(uint32_t msec) { vTaskDelay(msec / portTICK_PERIOD_MS); }
  };

  class GPIO_host
  {
#if !defined ( LHAL_GPIO_NONEED_PORTCHECK )
    volatile uint32_t* _reg_output[2]; // [0]=low reg / [1]=high reg
    volatile uint32_t* _reg_input;
#endif
    gpio::pin_mask_t _pin_mask;
    LHAL::GPIO_t::gpio_pin_t _gpio_pin;

  public:

    GPIO_host(LHAL::GPIO_t::gpio_pin_t pin) :
#if !defined ( LHAL_GPIO_NONEED_PORTCHECK )
      _reg_output { LHAL::GPIO_t::RAW::getLowReg(LHAL::GPIO_t::getPortNum(pin)), LHAL::GPIO_t::RAW::getHighReg(LHAL::GPIO_t::getPortNum(pin)) },
      _reg_input { LHAL::GPIO_t::RAW::getInputReg(LHAL::GPIO_t::getPortNum(pin)) },
#endif
      _pin_mask { LHAL::GPIO_t::getPinMask(pin) },
      _gpio_pin { pin }
    {};

    void setMode(LHAL::GPIO_t::mode_t mode) { LHAL::Gpio.setMode(_gpio_pin, mode); }

#if defined ( LHAL_GPIO_NONEED_PORTCHECK )
    void writeHigh(void) { *LHAL::GPIO_t::RAW::getHighReg() = _pin_mask; }
    void writeLow(void) { *LHAL::GPIO_t::RAW::getLowReg() = _pin_mask; }
    void write(bool value) { *(value ? LHAL::GPIO_t::RAW::getHighReg() : LHAL::GPIO_t::RAW::getLowReg()) = _pin_mask; }
    bool read(void) { return *LHAL::GPIO_t::RAW::getInputReg() & _pin_mask; }
#else
    void writeHigh(void) { *_reg_output[1] = _pin_mask; }
    void writeLow(void) { *_reg_output[0] = _pin_mask; }
    void write(bool value) { *_reg_output[value] = _pin_mask; }
    bool read(void) { return *_reg_input & _pin_mask; }
#endif
  };
}

#ifdef LHAL_GPIO_NONEED_PORTCHECK
#undef LHAL_GPIO_NONEED_PORTCHECK
#endif
