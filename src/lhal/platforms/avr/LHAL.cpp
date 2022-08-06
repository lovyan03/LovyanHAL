/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#include "../../platform_check.hpp"

#if defined (LHAL_TARGET_PLATFORM) && (LHAL_TARGET_PLATFORM_NUMBER == LHAL_PLATFORM_NUMBER_AVR)

#include <Arduino.h>
//#include <SPI.h>

#include "LHAL.hpp"

namespace lhal
{
  LovyanHAL::GPIO_HAL LovyanHAL::Gpio;

  GPIO_host LovyanHAL::GPIO_HAL_Base::getHost(gpio_port_pin_t pin) { return GPIO_host { pin }; }

/*
  GpioPortHal const PROGMEM LovyanHAL::GPIO_HAL::_port[] = {
#if defined ( PORTA )
      { &PORTA, &PINA, &DDRA },
#else
      { nullptr, nullptr, nullptr },
#endif
      { &PORTB, &PINB, &DDRB },
      { &PORTC, &PINC, &DDRC },
      { &PORTD, &PIND, &DDRD },
#if defined ( PORTE )
      { &PORTE, &PINE, &DDRE },
#if defined ( PORTF )
      { &PORTF, &PINF, &DDRF },
#if defined ( PORTG )
      { &PORTG, &PING, &DDRG },
#if defined ( PORTH )
      { &PORTH, &PINH, &DDRH },
#endif
#endif
#endif
#endif
    };
//*/


  constexpr volatile uint8_t* const PROGMEM LovyanHAL::GPIO_HAL::RAW::_reg_output_table[] =
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

  constexpr volatile uint8_t* const PROGMEM LovyanHAL::GPIO_HAL::RAW::_reg_input_table[] =
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

  constexpr volatile uint8_t* const PROGMEM LovyanHAL::GPIO_HAL::RAW::_reg_mode_table[] =
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


  void LovyanHAL::GPIO_HAL::writePortHigh(gpio::port_num_t port, gpio::pin_mask_t bitmask)
  {
    auto reg = Raw.getOutputReg(port);
    auto sr = LovyanHAL::RAW::disableInterrupt();
    Raw.writeRegHigh(reg, bitmask);
    LovyanHAL::RAW::enableInterrupt(sr);
  }

  void LovyanHAL::GPIO_HAL::writePortLow(gpio::port_num_t port, gpio::pin_mask_t bitmask)
  {
    auto reg = Raw.getOutputReg(port);
    auto sr = LovyanHAL::RAW::disableInterrupt();
    Raw.writeRegLow(reg, bitmask);
    LovyanHAL::RAW::enableInterrupt(sr);
  }

  void LovyanHAL::GPIO_HAL::writePort(gpio::port_num_t port, gpio::pin_mask_t bitmask, bool value)
  {
    auto reg = Raw.getOutputReg(port);
    auto sr = LovyanHAL::RAW::disableInterrupt();
    if (value)
    {
      Raw.writeRegHigh(reg, bitmask);
    }
    else
    {
      Raw.writeRegLow(reg, bitmask);
    }
    LovyanHAL::RAW::enableInterrupt(sr);
  }

  gpio::pin_mask_t LovyanHAL::GPIO_HAL::readPort(gpio::port_num_t port, gpio::pin_mask_t bitmask)
  {
    return *Raw.getInputReg(port) & bitmask;
  }

  void LovyanHAL::GPIO_HAL::setMode(gpio_port_pin_t pp, mode_t mode)
  {
    bool flg_input = !(mode & mode_t::output);
    auto port = pp.port;
    auto reg_mode = Raw.getModeReg(port);
    auto reg_out = Raw.getOutputReg(port);
    auto bit = pp.getMask();

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
  gpio_port_pin_t LovyanHAL::convertArduinoPinNumber(int arduino_pin_number)
  {
    gpio_port_pin_t res;
    if ((uint8_t)arduino_pin_number < NUM_DIGITAL_PINS)
    {
      res.port = digitalPinToPort(arduino_pin_number) - 1;
      res.pin = __builtin_ctz(digitalPinToBitMask(arduino_pin_number));
    }
    return res;
  }
#endif
/*
  bool LovyanHAL::SPI_HAL::_need_wait;

  error_t LovyanHAL::SPI_HAL::beginTransaction(spi_hal_num_t num)
  {
    _need_wait = false;
    auto sr = Raw.disableInterrupt();
    SPCR |= _BV(MSTR);
    SPCR |= _BV(SPE);
    Raw.enableInterrupt(sr);
    return err_ok;
  }

  error_t LovyanHAL::SPI_HAL::endTransaction(spi_hal_num_t num)
  {
    if (_need_wait)
    {
      while (!(SPSR & _BV(SPIF))) ; // wait
    }
    auto sr = Raw.disableInterrupt();
    SPCR &= ~_BV(SPE);
    Raw.enableInterrupt(sr);
    return err_ok;
  }

  error_t LovyanHAL::SPI_HAL::write(spi_hal_num_t num, const uint8_t* src, size_t len)
  {
    if (_need_wait) { while (!(SPSR & _BV(SPIF))); }
    SPDR = *src;
    _need_wait = true;
    if (len > 1)
    {
      size_t i = 0;
      do
      {
        uint8_t tmp = src[++i];
        while (!(SPSR & _BV(SPIF))) ; // wait
        SPDR = tmp;
      } while (i != len);
    }
    return err_ok;
  }

  error_t LovyanHAL::SPI_HAL::write(spi_hal_num_t num, uint8_t data)
  {
    if (_need_wait)
    {
      while (!(SPSR & _BV(SPIF))) ; // wait
    }
    SPDR = data;
    _need_wait = true;
    return err_ok;
  }
//*/
}

#endif
