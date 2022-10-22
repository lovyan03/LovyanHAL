/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#if defined ( LHAL_TARGET_PLATFORM )

#include "../common.inl"
#include "../linkmode/for_mcu.inl"

namespace lhal
{
 namespace v0
 {

  error_t Bus_SPI::init(void)
  {

    if (_cfg.pin_sclk.isValid())
    {
      _lhal->Gpio.setMode(_cfg.pin_sclk, gpio::mode_t::output);
    }
    if (_cfg.pin_mosi.isValid())
    {
      _lhal->Gpio.setMode(_cfg.pin_mosi, gpio::mode_t::output);
    }
    if (_cfg.pin_miso.isValid())
    {
      _lhal->Gpio.setMode(_cfg.pin_miso, gpio::mode_t::input_pulldown);
    }
    return IBus::init();
  }

  void Bus_SPI::beginTransaction_1st_impl(void)
  {
    auto tr = reinterpret_cast<TransactionSPI*>(_transaction);
    // SPI_MODEのCPOLに応じた極性設定を行う
    _lhal->Gpio.write(_cfg.pin_sclk, (tr->spi_mode & 2));

    // MCUの速度とfreqの指定に応じてnopウェイト量を求める;
    uint_fast32_t freq = (LovyanHAL::getCpuFrequency()<<1) / internal::nop_delay_cycle_x2;
    _nop_wait_w = freq / tr->freq_write;
    _nop_wait_r = freq / tr->freq_read ;
  }

  error_t Bus_SPI::beginTransaction_2nd_impl(bool read)
  {
    auto tr = reinterpret_cast<TransactionSPI*>(_transaction);
    if (tr->read_by_mosi)
    {
      _lhal->Gpio.setMode
      (
        _cfg.pin_mosi,
        read ? lhal::gpio::mode_t::input
             : lhal::gpio::mode_t::output
      );
    }
    return error_t::err_ok;
  }

  void Bus_SPI::dummyClock(uint8_t dummy_clock_bits)
  {
    auto tr = reinterpret_cast<TransactionSPI*>(_transaction);
    bool cpol = (tr->spi_mode & 2);
    auto sclk = _lhal->Gpio.getHost(_cfg.pin_sclk);

    int nw = _nop_wait_r;
    auto nw0 =  nw    >> 1;
    auto nw1 = (nw+1) >> 1;

    do
    {
      sclk.write(!cpol);
      for (int i = nw1; i >= 0; --i) { __asm__ __volatile__("nop"); }
      sclk.write(cpol);
      for (int i = nw0; i >= 0; --i) { __asm__ __volatile__("nop"); }
    } while (--dummy_clock_bits);
  }

  void Bus_SPI::write(const uint8_t* write_data, size_t len)
  {
    auto tr = reinterpret_cast<TransactionSPI*>(_transaction);
    bool cpol = (tr->spi_mode & 2);
    bool cpha = (tr->spi_mode & 1);
    bool flip = cpol ^ cpha;
    auto sclk = _lhal->Gpio.getHost(_cfg.pin_sclk);
    auto mosi = _lhal->Gpio.getHost(_cfg.pin_mosi);

    int nw = _nop_wait_w - 6;
    auto nw0 =  nw    >> 1;
    auto nw1 = (nw+1) >> 1;

    do
    {
      uint_fast8_t mask = 0x80;
      uint_fast8_t d = *write_data++;
      do
      {
        mosi.write(d & mask);
        sclk.write(flip);
        for (int i = nw1; i >= 0; --i) { __asm__ __volatile__("nop"); }

        sclk.write(!flip);
        for (int i = nw0; i >= 0; --i) { __asm__ __volatile__("nop"); }
      } while (mask >>= 1);
    } while (--len);

    sclk.write(cpol);
  }

/*              timing      timing
    begin   s0    s1    s0    s1    end
MODE0 ↓　　↓　　↑　　↓　　↑　　↓
　　　　　　　　　┏━━┓　　┏━━┓
━━━━━━━━━┛　　┗━━┛　　┗━━

    begin   s0    s1    s0    s1    end
MODE1 ↓　　↑　　↓　　↑　　↓　　↓
　　　　　　┏━━┓　　┏━━┓
━━━━━━┛　　┗━━┛　　┗━━━━━

    begin   s0    s1    s0    s1    end
MODE2 ↑　　↑　　↓　　↑　　↓　　↑
━━━━━━━━━┓　　┏━━┓　　┏━━
　　　　　　　　　┗━━┛　　┗━━┛

    begin   s0    s1    s0    s1    end
MODE3 ↑　　↓　　↑　　↓　　↑　　↑
━━━━━━┓　　┏━━┓　　┏━━━━━
　　　　　　┗━━┛　　┗━━┛
*/
  void Bus_SPI::transfer(const uint8_t* write_data, uint8_t* read_data, size_t len)
  {
    // 注: read_by_mosi が有効の場合であっても MISOで読取りを実施する;

    auto tr = reinterpret_cast<TransactionSPI*>(_transaction);
    bool cpol = (tr->spi_mode & 2);
    bool cpha = (tr->spi_mode & 1);
    bool flip = cpol ^ cpha;
    auto sclk = _lhal->Gpio.getHost(_cfg.pin_sclk);
    auto mosi = _lhal->Gpio.getHost(_cfg.pin_mosi);
    auto miso = _lhal->Gpio.getHost(_cfg.pin_miso);

    int nw = _nop_wait_r - 12;
    auto nw0 =  nw    >> 1;
    auto nw1 = (nw+1) >> 1;

    uint_fast8_t r = 0;
    do
    {
      uint_fast8_t mask = 0x80;
      uint_fast8_t d = *write_data++;
      do
      {
        mosi.write(d & mask);
        sclk.write(flip);
#if defined (ESP_PLATFORM)
        sclk.write(flip);  // for ESP32、GPIOの反映が稀にワンテンポ遅れて読取りデータが化けるためsclkを2回更新する;
#endif
        for (int i = nw0; i >= 0; --i) { __asm__ __volatile__("nop"); }
        r = (r << 1) + (miso.read() ? 1 : 0);
        sclk.write(!flip);
        for (int i = nw1; i >= 0; --i) { __asm__ __volatile__("nop"); }
      } while (mask >>= 1);

      *read_data++ = (uint8_t)r;
    } while (--len);
    sclk.write(cpol);
  }

  void Bus_SPI::read(uint8_t* read_data, size_t len, bool nack)
  {
    auto tr = reinterpret_cast<TransactionSPI*>(_transaction);
    bool cpol = (tr->spi_mode & 2);
    bool cpha = (tr->spi_mode & 1);
    bool flip = cpol ^ cpha;
    auto sclk = _lhal->Gpio.getHost(_cfg.pin_sclk);

    int nw = _nop_wait_r - 10;
    auto nw0 =  nw    >> 1;
    auto nw1 = (nw+1) >> 1;

    bool mosi_read = tr->read_by_mosi;
    auto miso = _lhal->Gpio.getHost(mosi_read ? _cfg.pin_mosi : _cfg.pin_miso);
    if (!mosi_read)
    {
      _lhal->Gpio.writeHigh(_cfg.pin_mosi);
    }

    uint_fast8_t r = 0;
    do
    {
      uint_fast8_t mask = 0x80;
      do
      {
        sclk.write(flip);
#if defined (ESP_PLATFORM)
        sclk.write(flip);  // for ESP32、GPIOの反映が稀にワンテンポ遅れて読取りデータが化けるためsclkを2回更新する;
#endif
        for (int i = nw0; i >= 0; --i) { __asm__ __volatile__("nop"); }
        r = (r << 1) + (miso.read() ? 1 : 0);
        sclk.write(!flip);
        for (int i = nw1; i >= 0; --i) { __asm__ __volatile__("nop"); }
      } while (mask >>= 1);

      *read_data++ = (uint8_t)r;
    } while (--len);
    sclk.write(cpol);
  }

//----------------------------------------------------------------------------
// I2C

  error_t Bus_I2C::init(void)
  {
    if (_cfg.pin_scl.isValid())
    {
      // _lhal->Gpio.setMode(_cfg.pin_scl, _lhal->Gpio.output_opendrain);
      _lhal->Gpio.getHost(_cfg.pin_scl).writeI2CHigh();
    }
    if (_cfg.pin_sda.isValid())
    {
      // _lhal->Gpio.setMode(_cfg.pin_sda, _lhal->Gpio.output_opendrain);
      _lhal->Gpio.getHost(_cfg.pin_sda).writeI2CHigh();
    }
    return IBus::init();
  }


  void Bus_I2C::beginTransaction_1st_impl(void)
  {
    auto sda = _lhal->Gpio.getHost(_cfg.pin_sda);
    auto scl = _lhal->Gpio.getHost(_cfg.pin_scl);
    sda.setMode(lhal::gpio::mode_t::output_opendrain);
    scl.setMode(lhal::gpio::mode_t::output_opendrain);
    scl.writeI2CHigh();
    sda.writeI2CHigh();

    auto tr = reinterpret_cast<TransactionI2C*>(_transaction);
    _timeout = tr->timeout_msec;
    // MCUの速度とfreqの指定に応じてnopウェイト量を求める;
    uint_fast32_t freq = (LovyanHAL::getCpuFrequency()<<1) / internal::nop_delay_cycle_x2;
    _nop_wait_w = freq / tr->freq_write;
    _nop_wait_r = freq / tr->freq_read ;
  }

  error_t Bus_I2C::beginTransaction_2nd_impl(bool read)
  {
    return sendStartStop(false);
  }

  error_t Bus_I2C::endTransaction_impl(void)
  {
    return sendStartStop(true);
  }

  error_t Bus_I2C::sendStartStop(bool stop)
  {
    auto sda = _lhal->Gpio.getHost(_cfg.pin_sda);
    auto scl = _lhal->Gpio.getHost(_cfg.pin_scl);

    int nw = _nop_wait_w >> 1;

    int retry = 9;
    // if (!sda.read() || stop)
    if (stop)
    { /// SDAがLOWになっている場合やSTOPコンディションの場合;
      scl.writeI2CLow();
      sda.writeI2CHigh();
      for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }
      // SDAがHIGHになるまでクロック送出しながら待機する。;
      while (!(sda.read()) && (--retry))
      {
        scl.writeI2CHigh();
        for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }
        scl.writeI2CLow();
        for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }
      }
    }

    if (stop)
    { // stop condition
      sda.writeI2CLow();
      scl.writeI2CHigh();
      for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }

      sda.writeI2CHigh();
      if (retry == 0)
      { // SDAがhighにならない;
        _last_error = err_failed;
      }
    }
    else
    { // start condition
      scl.writeI2CHigh();
      if (retry)
      {
        for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }
        sda.writeI2CLow();
        for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }
        scl.writeI2CLow();
      }
      else
      { // SDAがhighにならない;
        _last_error = err_failed;
      }
    }

    return error_t::err_ok;
  }
/*
  void Bus_I2C::dcControl(bool value)
  {
    if (!_in_transaction || isError()) return;
//  waitBusy();
  }

  void Bus_I2C::csControl(bool stop, bool read)
  {
    if (!_in_transaction || isError()) return;

    auto tr = reinterpret_cast<TransactionI2C*>(_transaction);
    if (tr == nullptr) { return; }

    auto scl = _lhal->Gpio.getHost(_cfg.pin_scl);
    auto sda = _lhal->Gpio.getHost(_cfg.pin_sda);

    // MCUの速度とfreqの指定に応じてnopウェイト量を求める;
    auto freq = (read ? tr->freq_read : tr->freq_write);
    _nop_wait = LovyanHAL::getCpuFrequency() / (freq * internal::nop_delay_cycle);
    int nw = _nop_wait >> 1;

    int retry = 9;
    if (!sda.read() || stop)
    { /// SDAがLOWになっている場合やSTOPコンディションの場合;
      scl.writeI2CLow();
      sda.writeI2CHigh();
      for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }
      // SDAがHIGHになるまでクロック送出しながら待機する。;
      while (!(sda.read()) && (--retry))
      {
        scl.writeI2CHigh();
        for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }
        scl.writeI2CLow();
        for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }
      }
    }

    if (stop)
    { // stop condition
      sda.writeI2CLow();
      scl.writeI2CHigh();
      for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }

      sda.writeI2CHigh();
      if (retry == 0)
      { // SDAがhighにならない;
        _last_error = err_failed;
      }
    }
    else
    { // start condition
      scl.writeI2CHigh();
      if (retry)
      {
        for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }
        sda.writeI2CLow();
        for (int i = nw; i >= 0; --i) { __asm__ __volatile__("nop"); }
        scl.writeI2CLow();

        // send slave address
        write8((tr->i2c_addr << 1) + (read ? 1 : 0));
      }
      else
      { // SDAがhighにならない;
        _last_error = err_failed;
      }
    }
  }
//*/
  static bool wait_clock_stretch(GPIO_host& scl, uint_fast16_t to)
  {
    auto ms = millis();
    do
    {
      yield();
      if (scl.read())
      {
        return true;
      }
    } while ((millis() - ms) < to);
    return false;
  }

  void Bus_I2C::write(const uint8_t* data, size_t len)
  {
    if (_transaction == nullptr || isError()) return;

    auto scl = _lhal->Gpio.getHost(_cfg.pin_scl);
    auto sda = _lhal->Gpio.getHost(_cfg.pin_sda);

    auto nw = _nop_wait_w - 10;
    int_fast16_t nw0 = (nw    ) >> 1;
    int_fast16_t nw1 = (nw + 1) >> 1;
    do
    {
      uint_fast8_t mask = 0x80;
      uint_fast8_t d = *data++;

      // 最初の1bitの送信
      sda.writeI2C((bool)(d & mask));
      for (int i = nw0; i >= 0; --i) { __asm__ __volatile__("nop"); }
      scl.writeI2CHigh();
      for (int i = nw1; i >= 0; --i) { __asm__ __volatile__("nop"); }
      // クロックストレッチの判定と待機;
      if (!scl.read())
      { // タイムアウト時間までクロックストレッチが解除されるのを待つ;
        if (!wait_clock_stretch(scl, _timeout))
        {
          _last_error = err_timeout;
          // endTransaction();
          break;
        }
      }

      mask >>= 1;
      do
      { // 2bit~8bit目の送信;
        scl.writeI2CLow();
        sda.writeI2C((bool)(d & mask));
        for (int i = nw0; i >= 0; --i) { __asm__ __volatile__("nop"); }
        scl.writeI2CHigh();
        for (int i = nw1; i >= 0; --i) { __asm__ __volatile__("nop"); }
        mask >>= 1;
      } while (mask);

      // ACK応答チェック;
      scl.writeI2CLow();  // SCL lo
      sda.writeI2CHigh(); // SDA hi
      for (int i = nw1; i >= 0; --i) { __asm__ __volatile__("nop"); }
      scl.writeI2CHigh(); // hi
      for (int i = nw0; i >= 0; --i) { __asm__ __volatile__("nop"); }

      // クロックストレッチの判定と待機;
      if (!scl.read())
      { // タイムアウト時間までクロックストレッチが解除されるのを待つ;
        if (!wait_clock_stretch(scl, _timeout))
        {
          _last_error = err_timeout;
          // endTransaction();
          break;
        }
      }
      if (sda.read())
      { // ToDo:ACK応答がない場合の処理;
        _last_error = err_no_ack;
      }
      scl.writeI2CLow();
    } while (--len && isSuccess());
  }

  void Bus_I2C::read(uint8_t* data, size_t len, bool nack)
  {
    if (_transaction == nullptr || isError()) return;

    auto scl = _lhal->Gpio.getHost(_cfg.pin_scl);
    auto sda = _lhal->Gpio.getHost(_cfg.pin_sda);

    auto nw = _nop_wait_w - 4;
    auto nw0 =  nw      >> 1;
    auto nw1 = (nw + 1) >> 1;

    do
    {
      // 最初の1bitの受信
      sda.writeI2CHigh();

      for (int i = nw0; i >= 0; --i) { __asm__ __volatile__("nop"); }
      scl.writeI2CHigh();
      for (int i = nw1; i >= 0; --i) { __asm__ __volatile__("nop"); }

      // クロックストレッチの判定と待機;
      if (!scl.read())
      { // タイムアウト時間までクロックストレッチが解除されるのを待つ;
        if (!wait_clock_stretch(scl, _timeout))
        {
          _last_error = err_timeout;
          // endTransaction();
          break;
        }
      }

      uint_fast8_t mask = 0x80;
      uint_fast8_t byte = sda.read() ? mask : 0;
      mask >>= 1;

      do
      {
        scl.writeI2CLow();
        for (int i = nw0; i >= 0; --i) { __asm__ __volatile__("nop"); }
        scl.writeI2CHigh();
        for (int i = nw1; i >= 0; --i) { __asm__ __volatile__("nop"); }

        if (sda.read())
        {
          byte |= mask;
        }
        mask >>= 1;
      } while (mask);
      scl.writeI2CLow();
      /// ACKを返す (ただし、データ末尾かつNACK指定がある場合はACKを返さない);
      if ((--len != 0) || !nack)
      {
        sda.writeI2CLow();
      }
      for (int i = nw0; i >= 0; --i) { __asm__ __volatile__("nop"); }
      scl.writeI2CHigh();
      for (int i = nw1; i >= 0; --i) { __asm__ __volatile__("nop"); }
      scl.writeI2CLow();
      *data++ = byte;
    } while (len);
  }

 }
}

#endif
