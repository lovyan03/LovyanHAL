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
#include "../../LHAL_Base.hpp"

namespace lhal
{
  class LHAL : public LHAL_Base
  {
  public:
    class GPIO : public GPIO_Base
    {
    public:
      static void setMode(gpio::gpio_pin_t pin, mode_t mode);
      static inline void writeHigh(gpio::gpio_pin_t pin) { digitalWrite(pin, 1); }
      static inline void writeLow(gpio::gpio_pin_t pin) { digitalWrite(pin, 0); }
      static inline void write(gpio::gpio_pin_t pin, bool value) { digitalWrite(pin, value); }
      static inline bool read(gpio::gpio_pin_t pin) { return digitalRead(pin); }
    };

    LHAL(void) : LHAL_Base() {}

    GPIO Gpio;

    static inline void delay(size_t msec) { ::delay(msec); }
    static constexpr gpio::gpio_pin_t convertArduinoPinNumber(int arduino_pin_number) { return arduino_pin_number; }
  };
}
