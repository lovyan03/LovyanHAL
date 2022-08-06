/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_COMMON_HPP_
#define LOVYANHAL_COMMON_HPP_

#include "init.hpp"

namespace lhal
{
  enum bus_type_t : uint8_t
  {
    bus_unknown,
    bus_spi,
    bus_i2c,
    bus_parallel8,
    bus_parallel16,
    bus_uart,
  };

  /// マイナス値をエラー扱いとする;
  /// 0を含むプラス値を成功扱いとする;
  /// エラーか否かの判定にはcheckError / checkSuccess関数を使用できる;
  enum error_t : int16_t
  {
    /// エラーなし;
    err_ok = 0,

    /// 処理失敗;
    err_failed = -1,

    /// 設定や引数の値に不備;
    err_invalid_param = -2,

    /// 通信相手からの応答がない;
    err_no_ack = -3,

    /// 処理時間切れ;
    /// 待ち時間内にACK応答がない場合のエラーにはtimeoutではなくno_ackを使う;
    err_timeout = -4,

    /// 未実装機能…;
    err_not_implement = -5,
  };
  static inline constexpr bool checkError(error_t err) { return err < 0; }
  static inline constexpr bool checkSuccess(error_t err) { return err >= 0; }

/*
  struct result_t
  {
    int val;
    constexpr result_t (int _val = 0) : val { _val } {}
    constexpr operator int(void) const { return val; }
    result_t operator=(int _val) { return val = _val; }
    result_t operator=(const result_t& _val) { return val = _val.val; }
    constexpr bool isSuccess(void) const { return val >= 0; }
    constexpr bool isError(void) const { return val < 0; }
  };
//*/

  // ポート番号+ピン番号の型;
  struct gpio_port_pin_t
  {
    union
    {
      uint8_t num;
      struct
      {
        uint8_t pin:    gpio::port_shift;
        uint8_t port: 8-gpio::port_shift;
      };
    };
    constexpr gpio::pin_mask_t getMask(void) const { return 1u << pin; }
    constexpr gpio_port_pin_t (uint8_t _val = (uint8_t)~0u) : num { _val } {}
    constexpr gpio_port_pin_t (uint8_t _port, uint8_t _pin) : pin { _pin }, port { _port } {}
    constexpr bool isValid(void) const { return num != (uint8_t)~0u; }
    constexpr bool isInvalid(void) const { return num == (uint8_t)~0u; }
    constexpr operator uint_fast8_t(void) const { return num; }

    gpio_port_pin_t operator=(uint8_t value) { return num = value; }
    gpio_port_pin_t operator=(const gpio_port_pin_t& value) { return num = value.num; }
    void set(uint8_t _num) { num = _num; }
    void set(uint8_t _port, uint8_t _pin) { num = _pin; port = _port; }

#if defined ( ESP_PLATFORM )
    constexpr explicit operator ::gpio_num_t(void) const { return (::gpio_num_t)num; }
#endif
  };

  // typedef uint8_t spi_hal_num_t;


  class IBus;

  /// 通信相手に関するパラメータ;
  /// CS、D/C、I2Cアドレスの制御などもここで行う;
  class ITransaction
  {
  public:
    /// CSピンの制御用インターフェイス
    /// value が falseのとき通信を開始、trueのとき通信を終了する;
    /// SPI通信の場合、falseで CSをLOW(active)に制御する;
    /// I2C通信の場合、falseで通信相手のアドレスを送信する;
    virtual void csControl(IBus* bus, bool value, bool read = false) {};

    /// D/Cピンの制御用インターフェイス;
    /// value : false=Command / true=Data
    virtual void dcControl(IBus* bus, bool value) {};
  };

  // IBus設定用クラス これを継承して各種ピン定義などを盛り込む;
  // ConfigSPI,ConfigI2C,ConfigParallel8等の型を用意する
  class IConfigBus
  {
  public:
    virtual bus_type_t getType(void) const = 0;
    uint8_t port_num = (uint8_t)~0u;  // ハードウェアペリフェラル番号(SERCOM番号等);
  };

  class ConfigSPI : public IConfigBus
  {
  public:
    bus_type_t getType(void) const override { return bus_type_t::bus_spi; }

    // clock pin
    gpio_port_pin_t pin_sclk;

    // write data pin (copi)
    gpio_port_pin_t pin_mosi;

    // read data pin (cipo)
    gpio_port_pin_t pin_miso;

    // data/command pin (RS)
    gpio_port_pin_t pin_dc;
  };

  class ConfigI2C : public IConfigBus
  {
  public:
    bus_type_t getType(void) const override { return bus_type_t::bus_i2c; }

    // clock pin
    gpio_port_pin_t pin_scl;

    // data pin
    gpio_port_pin_t pin_sda;
  };

  class ConfigParallel8 : public IConfigBus
  {
  public:
    bus_type_t getType(void) const override { return bus_type_t::bus_parallel8; }

    // write clock pin
    gpio_port_pin_t pin_wr;

    // read clock pin
    gpio_port_pin_t pin_rd;

    // data/command pin (RS)
    gpio_port_pin_t pin_dc;

    // data pin
    gpio_port_pin_t pin_d0;
    gpio_port_pin_t pin_d1;
    gpio_port_pin_t pin_d2;
    gpio_port_pin_t pin_d3;
    gpio_port_pin_t pin_d4;
    gpio_port_pin_t pin_d5;
    gpio_port_pin_t pin_d6;
    gpio_port_pin_t pin_d7;
  };

  class ConfigParallel16 : public IConfigBus
  {
  public:
    bus_type_t getType(void) const override { return bus_type_t::bus_parallel16; }

    // data pin
    gpio_port_pin_t pin_d8;
    gpio_port_pin_t pin_d9;
    gpio_port_pin_t pin_d10;
    gpio_port_pin_t pin_d11;
    gpio_port_pin_t pin_d12;
    gpio_port_pin_t pin_d13;
    gpio_port_pin_t pin_d14;
    gpio_port_pin_t pin_d15;
  };

}

#endif
