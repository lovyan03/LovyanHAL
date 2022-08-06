/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#include "../platform_check.hpp"

#if ( LHAL_TARGET_PLATFORM_NUMBER > LHAL_PLATFORM_NUMBER_PC_MAX )

#include "../init.hpp"

namespace lhal
{
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
    return IBus::init();
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

  void Bus_SPI::write(const uint8_t* data, size_t len)
  {
    auto tr = reinterpret_cast<TransactionSPI*>(_transaction);
    bool invert = (tr->spi_mode & 2);
    auto sclk = _lhal->Gpio.getHost(_cfg.pin_sclk);
    auto mosi = _lhal->Gpio.getHost(_cfg.pin_mosi);
    do
    {
      uint_fast8_t mask = 0x80;
      uint_fast8_t d = *data++;
      do
      {
        mosi.write(d & mask);
        sclk.write(!invert);
        sclk.write(invert);
      } while (mask >>= 1);
    } while (--len);
  }

//----------------------------------------------------------------------------
// I2C

  error_t Bus_I2C::init(void)
  {
    if (_cfg.pin_scl.isValid())
    {
      _lhal->Gpio.setMode(_cfg.pin_scl, _lhal->Gpio.output_opendrain);
    }
    if (_cfg.pin_sda.isValid())
    {
      _lhal->Gpio.setMode(_cfg.pin_sda, _lhal->Gpio.output_opendrain);
    }
    return IBus::init();
  }

  void Bus_I2C::dcControl(bool value)
  {
//  waitBusy();
  }
  void Bus_I2C::csControl(bool value, bool read)
  {
    auto scl = _lhal->Gpio.getHost(_cfg.pin_scl);
    auto sda = _lhal->Gpio.getHost(_cfg.pin_sda);
//  waitBusy();

    // ToDo: MCUの速度とfreqの指定に応じて適切なnopウェイト量を求めること;
    _nop_wait = 64;

    auto nw = _nop_wait >> 1;

    int retry = 9;
    if (!sda.read() || value)
    { /// SDAがLOWになっている場合やSTOPコンディションの場合;
      scl.writeLow();
      sda.writeHigh();
      for (int i = nw; i; --i) { __asm__ __volatile__("nop"); }
      // SDAがHIGHになるまでクロック送出しながら待機する。;
      while (!(sda.read()) && (--retry))
      {
        scl.writeHigh();
        for (int i = nw; i; --i) { __asm__ __volatile__("nop"); }
        scl.writeLow();
        for (int i = nw; i; --i) { __asm__ __volatile__("nop"); }
      }
    }

    if (value)
    { // stop condition
      sda.writeLow();
      scl.writeHigh();
      for (int i = nw; i; --i) { __asm__ __volatile__("nop"); }

      sda.writeHigh();
      if (retry == 0)
      { // SDAがhighにならない;
        _last_error = err_failed;
      }
    }
    else
    { // start condition
      scl.writeHigh();
      if (retry)
      {
        for (int i = nw; i; --i) { __asm__ __volatile__("nop"); }
        sda.writeLow();
        for (int i = nw; i; --i) { __asm__ __volatile__("nop"); }

        // send slave address
        auto tr = reinterpret_cast<TransactionI2C*>(_transaction);
        write8(tr->i2c_addr << 1 | (read ? 1 : 0));
      }
      else
      { // SDAがhighにならない;
        _last_error = err_failed;
      }
    }
  }

  void Bus_I2C::write(const uint8_t* data, size_t len)
  {
    if (isError()) return;

    auto scl = _lhal->Gpio.getHost(_cfg.pin_scl);
    auto sda = _lhal->Gpio.getHost(_cfg.pin_sda);

    auto nw0 = _nop_wait >> 1;
    auto nw1 = (_nop_wait+1) >> 1;
    int i;
    do
    {
      uint_fast8_t mask = 0x80;
      uint_fast8_t d = *data++;
      do
      {
        scl.writeLow();
        sda.write((bool)(d & mask));
        mask >>= 1;
        for (i = nw0; i; --i) { __asm__ __volatile__("nop"); }
        scl.writeHigh();
        for (i = nw1; i; --i) { __asm__ __volatile__("nop"); }
        i = 256;
        while (!(scl.read()) && --i) { __asm__ __volatile__("nop"); }
      } while (i && mask);

      scl.writeLow(); // lo
      sda.writeHigh(); // hi
      for (i = nw1; i; --i) { __asm__ __volatile__("nop"); }
      scl.writeHigh(); // hi
      for (i = nw0 - 16; i > 0; --i) { __asm__ __volatile__("nop"); }

      i = 256;
      while (!(scl.read()) && --i) { yield(); }
      if (!i)
      { // クロックストレッチが解除されない
        _last_error = err_timeout;
      }
      else
      if (sda.read())
      { // ACK応答がない
        _last_error = err_no_ack;
      }
    } while (--len);
  }

  void Bus_I2C::read(uint8_t* data, size_t len, bool nack)
  {
    if (isError()) return;

    auto scl = _lhal->Gpio.getHost(_cfg.pin_scl);
    auto sda = _lhal->Gpio.getHost(_cfg.pin_sda);

    auto nw0 = _nop_wait >> 1;
    auto nw1 = (_nop_wait+1) >> 1;

    do
    {
      uint_fast8_t byte = 0;
      int i;
      for (int idx = 0; idx < 8; idx++)
      {
        scl.writeLow();
        sda.writeHigh();
        for (i = nw0; i; --i) { __asm__ __volatile__("nop"); }
        scl.writeHigh();
        for (i = nw1; i; --i) { __asm__ __volatile__("nop"); }

        i = 256;
        while (!scl.read() && --i) { __asm__ __volatile__("nop"); }

        byte = (byte << 1) + (sda.read() ? 1 : 0);
      }
      scl.writeLow();
      /// データが最後でない場合や、NACKが指定されていない場合はACKを返す(SDAをLOWにする);
      if ((--len != 0) || !nack)
      {
        sda.writeLow();
      }
      for (i = nw0; i; --i) { __asm__ __volatile__("nop"); }
      scl.writeHigh();
      for (i = nw1; i; --i) { __asm__ __volatile__("nop"); }
      *data++ = byte;
    } while (len);
  }
};

#endif
