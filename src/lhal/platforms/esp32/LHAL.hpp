/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LHAL_PLATFORMS_LHAL_HPP
#define LHAL_PLATFORMS_LHAL_HPP

#include "init.hpp"
#include "../LovyanHAL_MCU.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <soc/gpio_reg.h>

namespace lhal
{
  class LovyanHAL : public LovyanHAL_MCU
  {
  public:
    class GPIO_HAL : public GPIO_HAL_Base
    {
    public:
      class RAW
      {
        static volatile uint32_t* const _write_reg[][2];
        static volatile uint32_t* const _read_reg[];

      public:
        static inline volatile uint32_t* const* getWriteReg(gpio::port_num_t port = 0) { return _write_reg[port]; };
        static inline volatile uint32_t* getReadReg(gpio::port_num_t port = 0) { return _read_reg[port]; };
        static inline volatile uint32_t* getHighReg(gpio::port_num_t port = 0) { return _write_reg[port][1]; };
        static inline volatile uint32_t* getLowReg(gpio::port_num_t port = 0) { return _write_reg[port][0]; };
      };
      static RAW Raw;
      static inline void writeMaskHigh(gpio::port_num_t port, gpio::pin_mask_t bitmask) { *RAW::getHighReg(port) = bitmask; };
      static inline void writeMaskLow(gpio::port_num_t port, gpio::pin_mask_t bitmask) { *RAW::getLowReg(port) = bitmask; };
      static inline void writeMask(gpio::port_num_t port, gpio::pin_mask_t bitmask, bool value) { *RAW::getWriteReg(port)[value] = bitmask; }
      static inline gpio::pin_mask_t readMask(gpio::port_num_t port, gpio::pin_mask_t bitmask) { return *RAW::getReadReg(port) & bitmask; };

      static inline void writeHigh(gpio_port_pin_t pp) { writeMaskHigh(pp.port, pp.getMask()); }
      static inline void writeLow(gpio_port_pin_t pp) { writeMaskLow(pp.port, pp.getMask()); }
      static inline void write(gpio_port_pin_t pp, bool value) { writeMask(pp.port, pp.getMask(), value); }
      static inline bool read(gpio_port_pin_t pp) { return readMask(pp.port, pp.getMask()); }

      static void setMode(gpio_port_pin_t pp, mode_t mode);
    };

    LovyanHAL(void) : LovyanHAL_MCU() {}

    static GPIO_HAL Gpio;

    static constexpr error_t init(void) { return error_t::err_ok; }
    static void delay(uint32_t msec) { vTaskDelay(msec / portTICK_PERIOD_MS); }

    /// Arduino環境のピン番号からMCUのポート+ピン番号に変換する…ESP32はMCUピン番号がそのままArduinoのピン番号となっているので変換が不要;
    static constexpr gpio_port_pin_t convertArduinoPinNumber(int arduino_pin_number)
    {
      return (gpio_port_pin_t)(GPIO_NUM_MAX > (uint32_t)arduino_pin_number ? arduino_pin_number : ~0u);
    }
  };

  class GPIO_host
  {
#if defined ( GPIO_OUT1_REG )
    volatile uint32_t* _reg_output[2]; // [0]=write low reg / [1]=write high reg
    volatile uint32_t* _reg_input;
#endif
    gpio::pin_mask_t _pin_mask;
    gpio_port_pin_t _gpio_pin;

  public:

    GPIO_host(gpio_port_pin_t pp) :
#if defined ( GPIO_OUT1_REG )
      _reg_output { LovyanHAL::GPIO_HAL::RAW::getLowReg(pp.port), LovyanHAL::GPIO_HAL::RAW::getHighReg(pp.port) },
      _reg_input { LovyanHAL::GPIO_HAL::RAW::getReadReg(pp.port) },
#endif
      _pin_mask { pp.getMask() },
      _gpio_pin { pp }
    {};

    void setMode(LovyanHAL::GPIO_HAL::mode_t mode) { LovyanHAL::Gpio.setMode(_gpio_pin, mode); }

#if defined ( GPIO_OUT1_REG )
    void writeHigh(void) { *_reg_output[1] = _pin_mask; }
    void writeLow(void) { *_reg_output[0] = _pin_mask; }
    void write(bool value) { *_reg_output[value] = _pin_mask; }
    bool read(void) { return *_reg_input & _pin_mask; }
#else
    void writeHigh(void) { *LovyanHAL::GPIO_HAL::RAW::getHighReg() = _pin_mask; }
    void writeLow(void) { *LovyanHAL::GPIO_HAL::RAW::getLowReg() = _pin_mask; }
    void write(bool value) { *(value ? LovyanHAL::GPIO_HAL::RAW::getHighReg() : LovyanHAL::GPIO_HAL::RAW::getLowReg()) = _pin_mask; }
    bool read(void) { return *LovyanHAL::GPIO_HAL::RAW::getReadReg() & _pin_mask; }
#endif
  };
}

#endif