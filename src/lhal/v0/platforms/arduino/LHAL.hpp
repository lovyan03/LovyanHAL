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
#include "../LovyanHAL_MCU.hpp"

namespace lhal
{
 namespace v0
 {

  class LovyanHAL : public LovyanHAL_MCU
  {
  public:
    class GPIO_HAL : public GPIO_HAL_Base
    {
    public:
      static void setMode(gpio_port_pin_t pp, mode_t mode);
      static inline void writeHigh(gpio_port_pin_t pp) { digitalWrite(pp, 1); }
      static inline void writeLow(gpio_port_pin_t pp) { digitalWrite(pp, 0); }
      static inline void write(gpio_port_pin_t pp, bool value) { digitalWrite(pp, value); }
      static inline bool read(gpio_port_pin_t pp) { return digitalRead(pp); }
    };

    LovyanHAL(void) : LovyanHAL_MCU() {}

    static error_t init(void) { return error_t::err_ok; }

    static GPIO_HAL Gpio;

    static inline void delay(size_t msec) { ::delay(msec); }
    static constexpr gpio_port_pin_t convertArduinoPinNumber(int arduino_pin_number) { return arduino_pin_number; }
  };

  class GPIO_host
  {
    gpio_port_pin_t _gpio_pin;
  public:
    GPIO_host(gpio_port_pin_t pin) : _gpio_pin { pin } {};

    void setMode(LovyanHAL::GPIO_HAL::mode_t mode) { LovyanHAL::Gpio.setMode(_gpio_pin, mode); }
    void writeHigh(void) { LovyanHAL::Gpio.writeHigh(_gpio_pin); }
    void writeLow(void) { LovyanHAL::Gpio.writeLow(_gpio_pin); }
    void write(bool value) { LovyanHAL::Gpio.write(_gpio_pin, value); }
    bool read(void) { return LovyanHAL::Gpio.read(_gpio_pin); }

    inline void writeI2CHigh(void) { writeHigh(); }
    inline void writeI2CLow(void) { writeLow(); }
    inline void writeI2C(bool value) { write(value); }
  };

 }
}
