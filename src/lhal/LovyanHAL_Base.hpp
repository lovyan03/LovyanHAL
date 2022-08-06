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
  class GPIO_host;
  class LovyanHAL;

  extern LovyanHAL* lhalDefaultInstance;
  static inline LovyanHAL* getDefaultInstance(void) { return lhalDefaultInstance; }
  static inline void setDefaultInstance(LovyanHAL* hal) { lhalDefaultInstance = hal; }

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
      enum mode_t : uint8_t
      {
        //                   +-----pull_down ( or output low  )
        //                   |+----pull_up   ( or output high )
        //                   ||+---open_drain
        //                   |||+--output
        //                   ||||
        input            = 0b0000,
        output           = 0b0001,
        output_opendrain = 0b0011,
        input_pullup     = 0b0100,
        output_high      = 0b0101,
        input_pulldown   = 0b1000,
        output_low       = 0b1001,
      };
      using port_num_t = gpio::port_num_t;
      using pin_num_t = gpio::pin_num_t;
      using pin_mask_t = gpio::pin_mask_t;
      static constexpr uint8_t port_shift = gpio::port_shift;

      GPIO_host getHost(gpio_port_pin_t pin);
    };

  };

  class TransactionSPI : public ITransaction
  {
  public:
    /// 送信時のクロック周波数
    uint32_t freq_write;
    uint32_t freq_read;
    gpio_port_pin_t pin_cs;
    uint8_t spi_mode = 0;       // SPI Mode 0~3
    bool read_by_mosi = false;  // 3wire read mode (read from mosi, no use miso read)
    void csControl(IBus* bus, bool value, bool read) override;
    // void dcControl(IBus* bus, bool value) override { if (pin_dc.isValid()) { LovyanHAL::GPIO_HAL::write(pin_dc, value); } }
  };

  class TransactionI2C : public ITransaction
  {
    uint8_t _prev_dc;
  public:
    uint32_t freq_write;
    uint32_t freq_read;
    uint8_t dc_prefix[2] = { 0, 0 };
    uint8_t i2c_addr;
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
          bus->rebeginTransaction();
        }
        bus->write8(dc_prefix[value]);
      }
    }
  };
}

#endif