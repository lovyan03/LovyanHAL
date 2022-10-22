/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_LHAL_BASE_HPP_
#define LOVYANHAL_LHAL_BASE_HPP_

#include "init.hpp"
#include "IBus.hpp"

#include <stdint.h>
#include <stddef.h>

namespace lhal
{
 namespace v0
 {

  class GPIO_porthandler;
  class GPIO_host;
  class LovyanHAL;

  namespace internal
  {
    extern uint_fast8_t dummy_reg_data;
    extern LovyanHAL* lhalDefaultInstance;
  }

  static inline LovyanHAL* getDefaultInstance(void) { return internal::lhalDefaultInstance; }
  static inline void setDefaultInstance(LovyanHAL* hal) { internal::lhalDefaultInstance = hal; }

  class LovyanHAL_Base
  {
  /// ここには環境依存のない共通のコードを実装していく。;
  /// 各MCU環境依存のLHALクラスは、このLovyanHAL_Baseを継承して作成する。;
  public:
    LovyanHAL_Base(void)
    {
      if (getDefaultInstance() == nullptr)
      {
        setDefaultInstance(reinterpret_cast<LovyanHAL*>(this));
      }
    }
    virtual ~LovyanHAL_Base(void) {}

    class GPIO_HAL_Base
    {
    public:
      // enum mode_t : uint8_t
      // {
      //   //                   +-----pull_down ( or output low  )
      //   //                   |+----pull_up   ( or output high )
      //   //                   ||+---open_drain
      //   //                   |||+--output
      //   //                   ||||
      //   input            = 0b0000,
      //   output           = 0b0001,
      //   output_opendrain = 0b0011,
      //   input_pullup     = 0b0100,
      //   output_high      = 0b0101,
      //   input_pulldown   = 0b1000,
      //   output_low       = 0b1001,
      // };
      typedef gpio::mode_t mode_t;
      using port_num_t = gpio::port_num_t;
      using pin_num_t = gpio::pin_num_t;
      using pin_mask_t = gpio::pin_mask_t;
      static constexpr uint8_t port_shift = gpio::port_shift;

      GPIO_porthandler getPortHandler(port_num_t port);
      GPIO_host getHost(gpio_port_pin_t pin);
    };

  };

  class TransactionSPI : public ITransaction
  {
  protected:
    void init_impl(void) override;
    void setCsAssert(bool read) override;
    void setCsDeassert(void) override;
    void setDcCommand(void) override;
    void setDcData(void) override;

  public:
    TransactionSPI(IBus* bus = nullptr) : ITransaction { bus } {};
    gpio_port_pin_t pin_cs;
    gpio_port_pin_t pin_dc;
    uint8_t spi_mode = 0;       // SPI Mode 0~3
    bool read_by_mosi = false;  // 3wire read mode (read from mosi, no use miso read)
    // void csControl(IBus* bus, bool value, bool read) override;
    // void dcControl(IBus* bus, bool value) override { if (pin_dc.isValid()) { LovyanHAL::GPIO_HAL::write(pin_dc, value); } }
  };

  class TransactionI2C : public ITransaction
  {
  protected:
    void setCsAssert(bool read) override;
    // void setCsDeassert(void) override;

  public:
    TransactionI2C(IBus* bus = nullptr) : ITransaction { bus } {};
    // I2Cタイムアウト時間(クロックストレッチ待機等);
    uint16_t timeout_msec = 128;
    uint8_t i2c_addr;
    uint8_t dc_prefix[2] = { 0, 0 };
/*
    void csControl(IBus* bus, bool value, bool read) override
    {
      _prev_dc = (uint8_t)~0u;
      if (value == false)
      {
        bus->write8((i2c_addr << 1) | (read ? 1 : 0));
      }
    }
    void dcControl(IBus* bus, bool value) override
    { 
      if (_prev_dc !=(uint8_t)value)
      {
        if (_prev_dc != (uint8_t)~0u)
        {
          bus->endTransaction();
          bus->beginTransaction();
        }
        bus->write8(dc_prefix[value]);
      }
    }

    uint8_t readRegister8(IBus* bus, uint8_t reg)
    {
      bus->beginTransaction(this);
      bus->write8(reg);
      if (bus->isSuccess())
      {
        bus->beginTransaction(true);
        reg = bus->read8(true);
      }
      bus->endTransaction();
      return reg;
    }

    void writeRegister8(IBus* bus, uint8_t reg, uint8_t val)
    {
      uint8_t buf[2] = { reg, val };
      bus->beginTransaction(this);
      bus->write(buf, 2);
      bus->endTransaction();
    }
//*/
  protected:
    uint8_t _prev_dc;
  };

 }
}

#endif