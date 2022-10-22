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

#include "env.hpp"
#include "../LovyanHAL_MCU.hpp"

namespace lhal
{
 namespace v0
 {

  /*
    class GpioPortHal
    {
    protected:
      volatile uint8_t* const _reg_output;
      volatile uint8_t* const _reg_input;
      volatile uint8_t* const _reg_mode;
    public:
      constexpr GpioPortHal(volatile uint8_t* reg_o, volatile uint8_t* reg_i, volatile uint8_t* reg_m) :
        _reg_output { reg_o },
        _reg_input  { reg_i },
        _reg_mode   { reg_m }
      {}

      inline volatile uint8_t* getOutputReg(void) const { return reinterpret_cast<volatile uint8_t*>(pgm_read_word(_reg_output)); }
      inline volatile uint8_t* getInputReg(void) const { return reinterpret_cast<volatile uint8_t*>(pgm_read_word(_reg_input)); }
      inline volatile uint8_t* getModeReg(void) const { return reinterpret_cast<volatile uint8_t*>(pgm_read_word(_reg_mode)); }

      inline void writeHigh(gpio::pin_mask_t bitmask) const { *getOutputReg() |= bitmask; };
      inline void writeLow(gpio::pin_mask_t bitmask) const { *getOutputReg() &= ~bitmask; };
    };
//*/
  class LovyanHAL : public LovyanHAL_MCU
  {
  public:

    class GPIO_HAL : public GPIO_HAL_Base
    {
      // static GpioPortHal const PROGMEM _port[];
    public:
      /// RAW : environment-dependent API (環境依存API);
      /// avrでRAWクラスを使ってレジスタに直接値を書き込む場合は事前にdisableInterruptを呼び出してSREGの内容を保持しておき、;
      /// レジスタの操作を終えたらenableInterruptを呼び出して 保持しておいたSREGの値を復元すること。;
      /// こうして割込みを禁止することで、ポート値の操作中の値の一貫性を保証できる。;
      class RAW
      {
        static volatile uint8_t* const PROGMEM _reg_output_table[];
        static volatile uint8_t* const PROGMEM _reg_input_table[];
        static volatile uint8_t* const PROGMEM _reg_mode_table[];

      public:
        /// GPIO出力用レジスタを取得する;
        /// @param port port number  0=PA / 1=PB / 2=PC / 3=PD / ...
        static inline volatile uint8_t* getOutputReg(gpio::port_num_t port) { return reinterpret_cast<volatile uint8_t*>(pgm_read_word(_reg_output_table + port)); }

        /// GPIO入力用レジスタを取得する;
        /// @param port port number  0=PA / 1=PB / 2=PC / 3=PD / ...
        static inline volatile uint8_t* getInputReg(gpio::port_num_t port) { return reinterpret_cast<volatile uint8_t*>(pgm_read_word(_reg_input_table + port)); }

        /// GPIO設定用レジスタを取得する;
        /// @param port port number  0=PA / 1=PB / 2=PC / 3=PD / ...
        static inline volatile uint8_t* getModeReg(gpio::port_num_t port) { return reinterpret_cast<volatile uint8_t*>(pgm_read_word(_reg_mode_table + port)); }

        static inline void writeRegHigh(volatile uint8_t* reg, gpio::pin_mask_t bitmask) { *reg |= bitmask; };
        static inline void writeRegLow(volatile uint8_t* reg, gpio::pin_mask_t bitmask) { *reg &= ~bitmask; };
      };

      static RAW Raw;

      static void writePortHigh(gpio::port_num_t port, gpio::pin_mask_t bitmask);
      static void writePortLow(gpio::port_num_t port, gpio::pin_mask_t bitmask);
      static void writePort(gpio::port_num_t port, gpio::pin_mask_t bitmask, bool value);
      static gpio::pin_mask_t readPort(gpio::port_num_t port, gpio::pin_mask_t bitmask);

      static void setMode(gpio_port_pin_t pin, gpio::mode_t mode);

      // static inline void writeHigh(gpio_hal_num_t pin) { writePortHigh(getPortNum(pin), getPinMask(pin)); }
      // static inline void writeHigh(gpio_port_pin_t pp) { _port[pp.port].writeHigh(1 << pp.pin); }
      static inline void writeHigh(gpio_port_pin_t pp) { writePortHigh(pp.port, pp.getMask()); }
      // static inline void writeLow(gpio_hal_num_t pin) { writePortLow(getPortNum(pin), getPinMask(pin)); }
      static inline void writeLow(gpio_port_pin_t pp) { writePortLow(pp.port, pp.getMask()); }
      // static inline void writeLow(gpio_port_pin_t pp) { _port[pp.port].writeLow(1 << pp.pin); }
      static inline void write(gpio_port_pin_t pp, bool value) { writePort(pp.port, pp.getMask(), value); }
      static inline bool read(gpio_port_pin_t pp) { return readPort(pp.port, pp.getMask()); }
    };

    class RAW
    {
    public:
      static inline uint_fast8_t disableInterrupt(void) { auto sr = SREG; cli(); return sr; };
      static inline void enableInterrupt(uint_fast8_t sr) { SREG = sr; };
    };

    LovyanHAL(void) : LovyanHAL_MCU() {}

    static RAW Raw;
    static GPIO_HAL Gpio;
    // static SPI_HAL Spi;

    static constexpr error_t init(void) { return error_t::err_ok; }

    static constexpr uint32_t getCpuFrequency(void) { return F_CPU; }

/// ピン番号の変換用define関数があれば使用する;
#if defined ( digitalPinToPort )
    /// Converts from Arduino pin number to MCU port + pin number;
    /// @return MCU port<<port_shift | pin number.
    /// @attention Returns ~0u if conversion is not possible.
    /// @attention 変換できない場合 ~0uを返す;
    static gpio_port_pin_t convertArduinoPinNumber(int arduino_pin_number);
#else
    static constexpr gpio_port_pin_t convertArduinoPinNumber(int arduino_pin_number) { return arduino_pin_number; }
#endif
  };

  class GPIO_porthandler
  {
    volatile uint8_t* _reg_output;
    // volatile uint8_t* _reg_input = &internal::dummy_reg_data;
    // volatile uint8_t* _reg_mode = &internal::dummy_reg_data;
  public:
    GPIO_porthandler(gpio::port_num_t port) :
      _reg_output { LovyanHAL::GPIO_HAL::RAW::getOutputReg(port) }
      // _reg_input { LovyanHAL::GPIO_HAL::RAW::getInputReg(port) },
      // _reg_mode { LovyanHAL::GPIO_HAL::RAW::getModeReg(port) }
    {};

    inline gpio::pin_mask_t getRawValue(void) { return (gpio::pin_mask_t)*_reg_output; }
    inline void setRawMask(gpio::pin_mask_t value) { *_reg_output = value; }
  };

  class GPIO_host
  {
    volatile uint8_t* _reg_output = &internal::dummy_reg_data;
    volatile uint8_t* _reg_input = &internal::dummy_reg_data;
    volatile uint8_t* _reg_mode = &internal::dummy_reg_data;
    gpio::pin_mask_t _pin_mask;
    gpio_port_pin_t _gpio_pin;
  public:
    GPIO_host(gpio_port_pin_t pp) :
      _reg_output { LovyanHAL::GPIO_HAL::RAW::getOutputReg(pp.port) },
      _reg_input { LovyanHAL::GPIO_HAL::RAW::getInputReg(pp.port) },
      _reg_mode { LovyanHAL::GPIO_HAL::RAW::getModeReg(pp.port) },
      _pin_mask { pp.getMask() },
      _gpio_pin { pp }
    {};

    inline void setMode(lhal::v0::gpio::mode_t mode) { LovyanHAL::Gpio.setMode(_gpio_pin, mode); }
    inline void writeHigh(void) { auto sr = LovyanHAL::RAW::disableInterrupt(); *_reg_output |= _pin_mask; LovyanHAL::RAW::enableInterrupt(sr); }
    inline void writeLow(void) { auto sr = LovyanHAL::RAW::disableInterrupt(); *_reg_output &= ~_pin_mask; LovyanHAL::RAW::enableInterrupt(sr); }
    inline void write(bool value) { if (value) { writeHigh(); } else { writeLow(); } }

    inline void writeHigh_WithoutInterrupt(void) { *_reg_output |= _pin_mask; }
    inline void writeLow_WithoutInterrupt(void) { *_reg_output &= ~_pin_mask; }
    inline void write_WithoutInterrupt(bool value) { if (value) { writeHigh_WithoutInterrupt(); } else { writeLow_WithoutInterrupt(); } }

    void writeI2CHigh(void);
    void writeI2CLow(void);// { LovyanHAL::Gpio.setMode(_gpio_pin, lhal::gpio::mode_t::output_low); }
    void writeI2C(bool value);

    bool read(void) { return *_reg_input & _pin_mask; }
  };

#if defined ( ARDUINO )
  static inline uint32_t millis(void) { return ::millis(); }
  static inline uint32_t micros(void) { return ::micros(); }
  static inline void yield(void) { ::yield(); }
  static inline void delay(uint32_t msec) { ::delay(msec); }
  static inline void delayMicroseconds(uint32_t usec) { ::delayMicroseconds(usec); };
#else

#endif

 }
}
