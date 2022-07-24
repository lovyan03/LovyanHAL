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

#include <driver/gpio.h>

namespace lhal
{
  class LHAL : public LHAL_Base
  {
  public:
    class GPIO : public GPIO_Base
    {
    protected:
#if defined ( CONFIG_IDF_TARGET_ESP32C3 )
      static inline volatile uint32_t* getInputReg(gpio::port_num_t port) { return &::GPIO.in; };
      static inline volatile uint32_t* getHighReg(gpio::port_num_t port) { return &::GPIO.out_w1ts; };
      static inline volatile uint32_t* getLowReg(gpio::port_num_t port) { return &::GPIO.out_w1tc; };
#else
      static inline volatile uint32_t* getInputReg(gpio::port_num_t port) { return (port & 1) ? (volatile uint32_t*)&::GPIO.in1 : &::GPIO.in; };
      static inline volatile uint32_t* getHighReg(gpio::port_num_t port) { return (port & 1) ? &::GPIO.out1_w1ts.val : &::GPIO.out_w1ts; };
      static inline volatile uint32_t* getLowReg(gpio::port_num_t port) { return (port & 1) ? &::GPIO.out1_w1tc.val : &::GPIO.out_w1tc; };
#endif
    public:
      static void setMode(gpio::gpio_pin_t pin, mode_t mode);

      static inline void writePortHigh(gpio::port_num_t port, gpio::pin_mask_t bitmask) { *getHighReg(port) = bitmask; };
      static inline void writePortLow(gpio::port_num_t port, gpio::pin_mask_t bitmask) { *getLowReg(port) = bitmask; };
      static inline gpio::pin_mask_t readPort(gpio::port_num_t port, gpio::pin_mask_t bitmask) { return *getInputReg(port) & bitmask; };

      static inline void writeHigh(gpio::gpio_pin_t pin) { writePortHigh(getPortNum(pin), getPinMask(pin)); }
      static inline void writeLow(gpio::gpio_pin_t pin) { writePortLow(getPortNum(pin), getPinMask(pin)); }
      static inline void write(gpio::gpio_pin_t pin, bool value) { auto port = getPortNum(pin); *(value ? getHighReg(port) : getLowReg(port)) = getPinMask(pin); }
      static inline bool read(gpio::gpio_pin_t pin) { return readPort(getPortNum(pin), getPinMask(pin)); }
    };

    LHAL(void) : LHAL_Base() {}

    GPIO Gpio;

    /// Arduino環境のピン番号からMCUのポート+ピン番号に変換する…ESP32はMCUピン番号がそのままArduinoのピン番号となっているので変換が不要;
    static constexpr gpio::gpio_pin_t convertArduinoPinNumber(int arduino_pin_number)
    {
      return (gpio::gpio_pin_t)(GPIO_NUM_MAX > arduino_pin_number ? arduino_pin_number : ~0u);
    }
  };
}
