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
  class LovyanHAL_MCU : public LovyanHAL_Base
  {
  };

  // SPI型 (通信用インスタンス)
  // この型にはソフトウェアSPIを実装し、派生クラスでハードウェアSPIを実装する;
  class Bus_SPI : public IBus
  {
  public:
    Bus_SPI(LovyanHAL* hal = nullptr) : IBus { hal } {}

    bus_type_t getType(void) const override { return bus_type_t::bus_spi; }

    ConfigSPI getConfig(void) const { return _cfg; }

    error_t setConfig(const IConfigBus* cfg) override
    {
      _cfg = *reinterpret_cast<const ConfigSPI*>(cfg);
      return err_ok;
    }

    error_t init(void) override;

    void dcControl(bool value) override;
    void csControl(bool value, bool read) override;

    void write(const uint8_t* data, size_t len) override;

  protected:
    ConfigSPI _cfg;
  };

  // I2C型 (通信用インスタンス)
  // この型にはソフトウェアI2Cを実装し、派生クラスでハードウェアI2Cを実装する;
  class Bus_I2C : public IBus
  {
  public:
    Bus_I2C(LovyanHAL* hal = nullptr) : IBus { hal } {}

    bus_type_t getType(void) const override { return bus_type_t::bus_i2c; }

    ConfigI2C getConfig(void) const { return _cfg; }

    error_t setConfig(const IConfigBus* cfg) override
    {
      _cfg = *reinterpret_cast<const ConfigI2C*>(cfg);
      return err_ok;
    }

    error_t init(void) override;

    void dcControl(bool value) override;
    void csControl(bool value, bool read) override;

    void write(const uint8_t* data, size_t len) override;
    void read(uint8_t* data, size_t len, bool nack = false) override;

  protected:
    ConfigI2C _cfg;
    uint16_t _nop_wait;
  };

  class HardwareSpiHost : public Bus_SPI
  {
  };
};

#endif
