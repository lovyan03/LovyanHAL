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

namespace lhal
{
  class LHAL : public LovyanHAL_Base
  {
  public:
    class GPIO_t : public GPIO_Base
    {
    public:
      static void setMode(gpio_pin_t pin, mode_t mode);
      static inline void writeHigh(gpio_pin_t pin) { digitalWrite(pin, 1); }
      static inline void writeLow(gpio_pin_t pin) { digitalWrite(pin, 0); }
      static inline void write(gpio_pin_t pin, bool value) { digitalWrite(pin, value); }
      static inline bool read(gpio_pin_t pin) { return digitalRead(pin); }
    };

    LHAL(void) : LovyanHAL_Base() {}

    static error_t init(void) { return error_t::err_ok; }

    static GPIO_t Gpio;

    static inline void delay(size_t msec) { ::delay(msec); }
    static constexpr LHAL::GPIO_t::gpio_pin_t convertArduinoPinNumber(int arduino_pin_number) { return arduino_pin_number; }
  };

  class GPIO_host
  {
    LHAL::GPIO_t::gpio_pin_t _gpio_pin;
  public:
    GPIO_host(LHAL::GPIO_t::gpio_pin_t pin) : _gpio_pin { pin } {};

    void setMode(LHAL::GPIO_t::mode_t mode) { LHAL::Gpio.setMode(_gpio_pin, mode); }
    void writeHigh(void) { LHAL::Gpio.writeHigh(_gpio_pin); }
    void writeLow(void) { LHAL::Gpio.writeLow(_gpio_pin); }
    void write(bool value) { LHAL::Gpio.write(_gpio_pin, value); }
    bool read(void) { return LHAL::Gpio.read(_gpio_pin); }
  };
}
