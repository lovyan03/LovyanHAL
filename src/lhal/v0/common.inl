/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#include "IBus.hpp"

namespace lhal
{
 namespace v0
 {

  namespace internal
  {
    LovyanHAL* lhalDefaultInstance = nullptr;
    uint_fast8_t dummy_reg_data = 0;
  }

  IBus::IBus(LovyanHAL* hal) { _lhal = (hal == nullptr) ? lhal::getDefaultInstance() : hal; }


#if ( LHAL_TARGET_PLATFORM_NUMBER < LHAL_PLATFORM_NUMBER_HOST_MAX )
  error_t IBus::init(void)
  {
    if (_bus_number == 255)
    {
      _bus_number = _lhal->getBusSequenceNumber(this);
    }
    _last_error = error_t::err_ok;
    return _last_error;
  }
#else
  LovyanHAL* IBus::_lhal = nullptr;

  error_t IBus::init(void)
  {
    _last_error = error_t::err_ok;
    return _last_error;
  }
#endif

  void TransactionSPI::init_impl(void)
  {
    auto gpio = _bus->getHal()->Gpio;
    if (pin_cs.isValid())
    {
      gpio.setMode(pin_cs, gpio::output_high);
    }
    if (pin_dc.isValid())
    {
      gpio.setMode(pin_dc, gpio::output_high);
    }
  }

  void TransactionSPI::setCsAssert(bool read)
  {
    _bus->waitBusy();
    if (pin_cs.isValid())
    {
      _bus->getHal()->Gpio.write(pin_cs, false);
    }
  }

  void TransactionSPI::setCsDeassert(void)
  {
    _bus->waitBusy();
    if (pin_cs.isValid())
    {
      _bus->getHal()->Gpio.write(pin_cs, true);
    }
  }

  void TransactionSPI::setDcCommand(void)
  {
    if (pin_dc.isValid())
    {
      _bus->waitBusy();
      _bus->getHal()->Gpio.write(pin_dc, false);
    }
  }

  void TransactionSPI::setDcData(void)
  {
    if (pin_dc.isValid())
    {
      _bus->waitBusy();
      _bus->getHal()->Gpio.write(pin_dc, true);
    }
  }

/*
  error_t Bus_SPI::init(void)
  {
    if (_cfg.pin_sclk.isValid())
    {
      _lhal->Gpio.setMode(_cfg.pin_sclk, _lhal->Gpio.output);
    }
    if (_cfg.pin_mosi.isValid())
    {
      _lhal->Gpio.setMode(_cfg.pin_mosi, _lhal->Gpio.output);
    }
    if (_cfg.pin_miso.isValid())
    {
      _lhal->Gpio.setMode(_cfg.pin_miso, _lhal->Gpio.input_pullup);
    }
    if (_cfg.pin_dc.isValid())
    {
      _lhal->Gpio.setMode(_cfg.pin_dc, _lhal->Gpio.output);
    }
    return err_ok;
  }

  void Bus_SPI::write8(uint8_t data)
  {
    auto tr = reinterpret_cast<TransactionSPI*>(_transaction);
    bool invert = (tr->spi_mode & 2);
    auto sclk = _lhal->Gpio.getHost(_cfg.pin_sclk);
    auto mosi = _lhal->Gpio.getHost(_cfg.pin_mosi);
    uint_fast8_t mask = 0x80;
    do
    {
      mosi.write(data & mask);
      sclk.write(!invert);
      sclk.write(invert);
    } while (mask >>= 1);
  }

  void Bus_SPI::dcControl(bool value)
  {
    waitBusy();
    if (_cfg.pin_dc.isValid())
    {
      _lhal->Gpio.write(_cfg.pin_dc, value);
    }
  }
  void Bus_SPI::csControl(bool value, bool read)
  {
    waitBusy();
    auto tr = reinterpret_cast<TransactionSPI*>(_transaction);
    _lhal->Gpio.write(_cfg.pin_sclk, (tr->spi_mode & 2));
    tr->csControl(this, value, read);
  }
//*/

  void TransactionI2C::setCsAssert(bool read)
  {
    _bus->write8((i2c_addr << 1) + read);
  }

 }
}
