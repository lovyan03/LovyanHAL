/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_LHAL_MCU_HPP_
#define LOVYANHAL_LHAL_MCU_HPP_

#include "../LovyanHAL_Base.hpp"

namespace lhal
{
 namespace v0
 {

  // SPI型 (通信用インスタンス)
  // この型にはソフトウェアSPIを実装し、派生クラスでハードウェアSPIを実装する;
  class Bus_SPI : public IBus
  {
  protected:

    void beginTransaction_1st_impl(void) override;
    error_t beginTransaction_2nd_impl(bool read) override;

  public:
    // IBus:::writeメソッドには複数のオーバーロードが存在するためusingの記述をしておくこと。;
    // using IBus::writeを記述せずにwriteを部分的にオーバーライドした場合、他のwriteメソッドが外部から見えなくなる;
    using IBus::write;

    Bus_SPI(LovyanHAL* hal = nullptr) : IBus { hal } {}

    inline TransactionSPI createTransaction(void) { return TransactionSPI(this); }

    ConfigSPI getConfig(void) const { return _cfg; }

    error_t setConfig(const IConfigBus* cfg) override
    {
      _cfg = *reinterpret_cast<const ConfigSPI*>(cfg);
      return err_ok;
    }

    bus_type_t getType(void) const override { return bus_type_t::bus_spi; }

    error_t init(void) override;

    void dummyClock(uint8_t dummy_clock_bits) override;
    void write(const uint8_t* data, size_t len) override;
    void read(uint8_t* data, size_t len, bool nack = false) override;
    void transfer(const uint8_t* write_data, uint8_t* read_data, size_t len) override;

  protected:
    ConfigSPI _cfg;
    int16_t _nop_wait_w;
    int16_t _nop_wait_r;
  };

  // I2C型 (通信用インスタンス)
  // この型にはソフトウェアI2Cを実装し、派生クラスでハードウェアI2Cを実装する;
  class Bus_I2C : public IBus
  {
  protected:

    void beginTransaction_1st_impl(void) override;
    error_t beginTransaction_2nd_impl(bool read) override;
    error_t endTransaction_impl(void) override;

    error_t sendStartStop(bool stop);

  public:
    // IBus:::writeメソッドには複数のオーバーロードが存在するためusingの記述をしておくこと。;
    // using:: IBus::writeを記述せずにwriteを部分的にオーバーライドした場合、他のwriteメソッドが外部から見えなくなる;
    using IBus::write;

    Bus_I2C(LovyanHAL* hal = nullptr) : IBus { hal } {}

    static inline TransactionI2C createTransaction(void) { return TransactionI2C(); }

    bus_type_t getType(void) const override { return bus_type_t::bus_i2c; }

    ConfigI2C getConfig(void) const { return _cfg; }

    error_t setConfig(const ConfigI2C& cfg)
    {
      _cfg = cfg;
      return err_ok;
    }

    error_t setConfig(const IConfigBus* cfg) override
    {
      _cfg = *reinterpret_cast<const ConfigI2C*>(cfg);
      return err_ok;
    }

    error_t init(const ConfigI2C& cfg)
    {
      auto res = setConfig(cfg);
      return checkError(res) ? res : init();
    }

    error_t init(void) override;

    // void dcControl(bool value) override;
    // void csControl(bool stop, bool read) override;

    void write(const uint8_t* data, size_t len) override;
    void read(uint8_t* data, size_t len, bool nack = false) override;

  protected:
    ConfigI2C _cfg;
    int16_t _nop_wait_w;
    int16_t _nop_wait_r;
    uint16_t _timeout;
  };

  class HardwareSpiHost : public Bus_SPI
  {
  };


  class LovyanHAL_MCU : public LovyanHAL_Base
  {
  public:
    static ConfigSPI createConfigSPI(void) { return ConfigSPI(); }
    static ConfigI2C createConfigI2C(void) { return ConfigI2C(); }
    static Bus_SPI createBusSPI(ConfigSPI &cfg) { Bus_SPI bus; bus.setConfig(&cfg); return bus; }
    static Bus_I2C createBusI2C(ConfigI2C &cfg) { Bus_I2C bus; bus.setConfig(&cfg); return bus; }
  };

 }
}

#endif
