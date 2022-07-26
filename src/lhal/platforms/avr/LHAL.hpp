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
      /// RAW : environment-dependent API (環境依存API);
      /// avrでRAWクラスを使ってレジスタに直接値を書き込む場合は事前にdisableInterruptを呼び出してSREGの内容を保持しておき、;
      /// レジスタの操作を終えたらenableInterruptを呼び出して 保持しておいたSREGの値を復元すること。;
      /// こうして割込みを禁止することで、ポート値の操作中の値の一貫性を保証できる。;
      class RAW
      {
        static volatile uint8_t* const PROGMEM reg_output_table[];
        static volatile uint8_t* const PROGMEM reg_input_table[];
        static volatile uint8_t* const PROGMEM reg_mode_table[];

      public:
        /// GPIO出力用レジスタを取得する;
        /// @param port port number  0=PA / 1=PB / 2=PC / 3=PD / ...
        static inline volatile uint8_t* getOutputReg(gpio::port_num_t port) { return reinterpret_cast<volatile uint8_t*>(pgm_read_word(reg_output_table + port)); }

        /// GPIO入力用レジスタを取得する;
        /// @param port port number  0=PA / 1=PB / 2=PC / 3=PD / ...
        static inline volatile uint8_t* getInputReg(gpio::port_num_t port) { return reinterpret_cast<volatile uint8_t*>(pgm_read_word(reg_input_table + port)); }

        /// GPIO設定用レジスタを取得する;
        /// @param port port number  0=PA / 1=PB / 2=PC / 3=PD / ...
        static inline volatile uint8_t* getModeReg(gpio::port_num_t port) { return reinterpret_cast<volatile uint8_t*>(pgm_read_word(reg_mode_table + port)); }

        static inline void writeRegHigh(volatile uint8_t* reg, gpio::pin_mask_t bitmask) { *reg |= bitmask; };
        static inline void writeRegLow(volatile uint8_t* reg, gpio::pin_mask_t bitmask) { *reg &= ~bitmask; };
      };

      static RAW Raw;

      static void writePortHigh(gpio::port_num_t port, gpio::pin_mask_t bitmask);
      static void writePortLow(gpio::port_num_t port, gpio::pin_mask_t bitmask);
      static void writePort(gpio::port_num_t port, gpio::pin_mask_t bitmask, bool value);
      static gpio::pin_mask_t readPort(gpio::port_num_t port, gpio::pin_mask_t bitmask);

      static void setMode(gpio::gpio_pin_t pin, mode_t mode);

      static inline void writeHigh(gpio::gpio_pin_t pin) { writePortHigh(getPortNum(pin), getPinMask(pin)); }
      static inline void writeLow(gpio::gpio_pin_t pin) { writePortLow(getPortNum(pin), getPinMask(pin)); }
      static inline void write(gpio::gpio_pin_t pin, bool value) { writePort(getPortNum(pin), getPinMask(pin), value); }
      static inline bool read(gpio::gpio_pin_t pin) { return readPort(getPortNum(pin), getPinMask(pin)); }
    };

    class RAW
    {
    public:
      static inline uint_fast8_t disableInterrupt(void) { auto sr = SREG; cli(); return sr; };
      static inline void enableInterrupt(uint_fast8_t sr) { SREG = sr; };
    };

    LHAL(void) : LovyanHAL_Base() {}

    static error_t init(void) { return error_t::err_ok; }

    static RAW Raw;
    static GPIO_t Gpio;

#if defined ( digitalPinToPort )

    /// Converts from Arduino pin number to MCU port + pin number;
    /// @return MCU port<<port_shift | pin number.
    /// @attention Returns ~0u if conversion is not possible.
    /// @attention 変換できない場合 ~0uを返す;
    static gpio::gpio_pin_t convertArduinoPinNumber(int arduino_pin_number);
#else
    static constexpr gpio::gpio_pin_t convertArduinoPinNumber(int arduino_pin_number) { return arduino_pin_number; }
#endif

  };

  class GPIO_host
  {
    volatile uint8_t* _reg_output;
    volatile uint8_t* _reg_input;
    gpio::pin_mask_t _pin_mask;
    LHAL::GPIO_t::gpio_pin_t _gpio_pin;
  public:
    GPIO_host(LHAL::GPIO_t::gpio_pin_t pin) :
      _reg_output { LHAL::GPIO_t::RAW::getOutputReg(LHAL::GPIO_t::getPortNum(_gpio_pin)) },
      _reg_input { LHAL::GPIO_t::RAW::getInputReg(LHAL::GPIO_t::getPortNum(_gpio_pin)) },
      _pin_mask { LHAL::GPIO_t::getPinMask(_gpio_pin) },
      _gpio_pin { pin }
    {};

    void setMode(LHAL::GPIO_t::mode_t mode) { LHAL::Gpio.setMode(_gpio_pin, mode); }
    void writeHigh(void) { auto sr = LHAL::RAW::disableInterrupt(); *_reg_output |= _pin_mask; LHAL::RAW::enableInterrupt(sr); }
    void writeLow(void) { auto sr = LHAL::RAW::disableInterrupt(); *_reg_output &= ~_pin_mask; LHAL::RAW::enableInterrupt(sr); }
    void write(bool value) { if (value) { writeHigh(); } else { writeLow(); } }
    bool read(void) { return *_reg_input & _pin_mask; }
  };
}
