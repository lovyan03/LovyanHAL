/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#include "../init.hpp"

#if LHAL_TARGET_PLATFORM_NUMBER == LHAL_PLATFORM_NUMBER_AVR

#include "LHAL.hpp"

namespace lhal
{
  constexpr volatile uint8_t* const PROGMEM LHAL::GPIO::RAW::reg_output_table[] =
  {
#if defined ( PORTA )
    &PORTA,
#else
    nullptr,
#endif
    &PORTB,
    &PORTC,
    &PORTD,
#if defined ( PORTE )
    &PORTE,
#if defined ( PORTF )
    &PORTF,
#if defined ( PORTG )
    &PORTG,
#if defined ( PORTH )
    &PORTH,
#endif
#endif
#endif
#endif
  };

  constexpr volatile uint8_t* const PROGMEM LHAL::GPIO::RAW::reg_input_table[] =
  {
#if defined ( PINA )
    &PINA,
#else
    nullptr,
#endif
    &PINB,
    &PINC,
    &PIND,
#if defined ( PINE )
    &PINE,
#if defined ( PINF )
    &PINF,
#if defined ( PING )
    &PING,
#if defined ( PINH )
    &PINH,
#endif
#endif
#endif
#endif
  };

  constexpr volatile uint8_t* const PROGMEM LHAL::GPIO::RAW::reg_mode_table[] =
  {
#if defined ( DDRA )
    &DDRA,
#else
    nullptr,
#endif
    &DDRB,
    &DDRC,
    &DDRD,
#if defined ( DDRE )
    &DDRE,
#if defined ( DDRF )
    &DDRF,
#if defined ( DDRG )
    &DDRG,
#if defined ( DDRH )
    &DDRH,
#endif
#endif
#endif
#endif
  };


  void LHAL::GPIO::writePortHigh(gpio::port_num_t port, gpio::pin_mask_t bitmask)
  {
    auto reg = Raw.getOutputReg(port);
    auto sr = LHAL::RAW::disableInterrupt();
    Raw.writeRegHigh(reg, bitmask);
    LHAL::RAW::enableInterrupt(sr);
  }

  void LHAL::GPIO::writePortLow(gpio::port_num_t port, gpio::pin_mask_t bitmask)
  {
    auto reg = Raw.getOutputReg(port);
    auto sr = LHAL::RAW::disableInterrupt();
    Raw.writeRegLow(reg, bitmask);
    LHAL::RAW::enableInterrupt(sr);
  }

  void LHAL::GPIO::writePort(gpio::port_num_t port, gpio::pin_mask_t bitmask, bool value)
  {
    auto reg = Raw.getOutputReg(port);
    auto sr = LHAL::RAW::disableInterrupt();
    if (value)
    {
      Raw.writeRegHigh(reg, bitmask);
    }
    else
    {
      Raw.writeRegLow(reg, bitmask);
    }
    LHAL::RAW::enableInterrupt(sr);
  }

  gpio::pin_mask_t LHAL::GPIO::readPort(gpio::port_num_t port, gpio::pin_mask_t bitmask)
  {
    return *Raw.getInputReg(port) & bitmask;
  }

  void LHAL::GPIO::setMode(gpio::gpio_pin_t pp, mode_t mode)
  {
    bool flg_input = !(mode & mode_t::output);
    auto port = getPortNum(pp);
    auto reg_mode = Raw.getModeReg(port);
    auto reg_out = Raw.getOutputReg(port);
    auto bit = getPinMask(pp);

    uint8_t oldSREG = SREG;
    cli();
    *reg_mode = flg_input ? (*reg_mode & ~bit) : (*reg_mode | bit);
    if (mode & mode_t::input_pullup)
    {
      *reg_out |= bit;
    }
    else
    if (mode & mode_t::input_pulldown)
    {
      *reg_out &= ~bit;
    }
    SREG = oldSREG;
  }

#if defined ( digitalPinToPort )
  gpio::gpio_pin_t LHAL::convertArduinoPinNumber(int arduino_pin_number)
  {
    if (arduino_pin_number >= NUM_DIGITAL_PINS)
    {
      return (gpio::gpio_pin_t)~0u;
    }
    gpio::port_num_t port = digitalPinToPort(arduino_pin_number) - 1;
    gpio::pin_num_t pin = __builtin_ctz(digitalPinToBitMask(arduino_pin_number));
    return pin | port << gpio::port_shift;
  }
#endif
}

#endif
