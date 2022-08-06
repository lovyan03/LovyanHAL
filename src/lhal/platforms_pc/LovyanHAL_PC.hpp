/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_PLATFORMS_PC_LOVYANHAL_PC_HPP_
#define LOVYANHAL_PLATFORMS_PC_LOVYANHAL_PC_HPP_

#include <thread>

#include "../init.hpp"
#include "../LovyanHAL_Base.hpp"
#include "common.hpp"

namespace lhal
{
  class IBus_PC;
  class Bus_SPI;
  class Bus_I2C;
  class LovyanHAL_PC : public LovyanHAL_Base
  {
    friend IBus;
    friend IBus_PC;
    friend Bus_SPI;
    friend Bus_I2C;
  public:

    class GPIO_HAL : public GPIO_HAL_Base
    {
      LovyanHAL_PC* _lhal;
    public:
      GPIO_HAL(LovyanHAL_PC* hal) : _lhal { hal } {}

      GPIO_host getHost(gpio_port_pin_t pin);

      void setMode(gpio_port_pin_t pin, mode_t mode);

      void writePortHigh(port_num_t port, pin_mask_t bitmask);
      void writePortLow(port_num_t port, pin_mask_t bitmask);

      void writeHigh(gpio_port_pin_t pin);
      void writeLow(gpio_port_pin_t pin);
      void write(gpio_port_pin_t pin, bool value) { if (value) { writeHigh(pin); } else { writeLow(pin); } };
      bool read(gpio_port_pin_t pin);
    };

    LovyanHAL_PC(void);
    LovyanHAL_PC(internal::ITransportLayer* transport_layer) : LovyanHAL_PC{} { setTransportLayer(transport_layer); }

    GPIO_HAL Gpio { this };

    gpio_port_pin_t convertArduinoPinNumber(int arduino_pin_number) { return _arduino_pin_table[arduino_pin_number]; }

    uint8_t getBusSequenceNumber(IBus* bus);

    virtual error_t init(void);

    static uint32_t millis(void);
    static uint32_t micros(void);
    static void delay(size_t msec);
    static void delayMicroseconds(size_t usec);

  protected:
    error_t setTransportLayer(internal::ITransportLayer* transport_layer);
    internal::ITransportLayer* _transport = nullptr;
    IBus* _bus_list[256] = { 0 };
    uint8_t _bus_count = 0;

  private:
    static constexpr size_t timeout_msec = 2048;

    gpio_port_pin_t _arduino_pin_table[256]; // arduinoピン番号からMCUピン番号への変換テーブル;

    error_t proc_queue(void);
    error_t _proc_receive(void);

    struct queue_data_t
    {
      enum state_t
      {
        state_free,
        state_wait_send,
        state_wait_recv,
        state_error_send,
        state_error_recv,
      };
      uint8_t sendbuf[264]; // 送信バッファ;
      uint8_t recvbuf[264]; // 送信バッファ;
      uint8_t sendlen = 0;
      uint8_t recvlen = 0;
      state_t state = state_free;
    };
    queue_data_t _queue[256];
    uint8_t _idx_next_queue = 0;
    uint8_t _idx_current_queue = (uint8_t)-1;
    uint8_t _idx_send_queue = 0;
    uint8_t _idx_recv_queue = 0;

    uint8_t* _sendbuf = _queue[0].sendbuf; // 送信バッファ;
    uint8_t* _recvbuf = _queue[0].recvbuf; // 受信バッファ;
    size_t _queued_bytes = 0;

    error_t _sendCommand(size_t len); 
    size_t _recvCommand(void);
  };

  class GPIO_host
  {
    LovyanHAL_PC* _lhal = nullptr;
    gpio_port_pin_t _gpio_pin;
  public:
    GPIO_host(void) = default;
    GPIO_host(gpio_port_pin_t pin, LovyanHAL_PC* lhal) : _lhal { lhal }, _gpio_pin { pin } {};

    void setMode(LovyanHAL_PC::GPIO_HAL::mode_t mode) { _lhal->Gpio.setMode(_gpio_pin, mode); }
    void writeHigh(void) { _lhal->Gpio.writeHigh(_gpio_pin); }
    void writeLow(void) { _lhal->Gpio.writeLow(_gpio_pin); }
    void write(bool value) { _lhal->Gpio.write(_gpio_pin, value); }
    bool read(void) { return _lhal->Gpio.read(_gpio_pin); }
  };


  class IBus_PC : public IBus
  {
  protected:
    size_t _update_last_error(void);
    void _write_internal(const uint8_t* data, size_t len, uint8_t cmd);
    void _write_repeat_internal(uint32_t data, uint8_t bytes, size_t repeat_count, uint8_t cmd);

  public:
    using IBus::write;

    IBus_PC(LovyanHAL* hal = nullptr) : IBus(hal) {}

//    void write8(uint8_t data) override { _write_internal(&data, 1, internal::command::bus_write); }

//    void write16(uint16_t data) override;
    void write(const uint8_t* data, size_t len) override { _write_internal(data, len, internal::command::bus_write); }
    void writeRepeat(uint32_t data, uint8_t bytes, size_t repeat_count) override { _write_repeat_internal(data, bytes, repeat_count, internal::command::bus_write_repeat); }

    void writeData8(uint8_t data) override { _write_internal(&data, 1, internal::command::bus_write_data); }
    void writeData(uint32_t data, uint8_t bytes) override { writeData((uint8_t*)&data, bytes); };
    void writeData(const uint8_t* data, size_t len) override { _write_internal(data, len, internal::command::bus_write_data); }
    void writeCommand8(uint8_t data) override { _write_internal(&data, 1, internal::command::bus_write_command); }
    void writeDataRepeat(uint32_t data, uint8_t bytes, size_t repeat_count) override { _write_repeat_internal(data, bytes, repeat_count, internal::command::bus_write_data_repeat); }

    void read(uint8_t* data, size_t len, bool nack = false) override;
  };

  // SPI型 (通信用インスタンス)
  class Bus_SPI : public IBus_PC
  {

  public:
    Bus_SPI(LovyanHAL* hal = nullptr) : IBus_PC(hal) {}
    TransactionSPI createTransaction(void) const { return TransactionSPI(); }

    bus_type_t getType(void) const override { return bus_type_t::bus_spi; }

    ConfigSPI getConfig(void) const { return _cfg; }

    error_t setConfig(const IConfigBus* cfg) override
    {
      _cfg = *reinterpret_cast<const ConfigSPI*>(cfg);
      return err_ok;
    }

    error_t init(void) override;

    void beginTransaction(ITransaction* transaction, bool read = false, int dummy_clock_bits = 0) override;
    void endTransaction(void) override;

    void dcControl(bool value) override;
    void csControl(bool value, bool read = false) override;

  protected:
    ConfigSPI _cfg;
  };

  // I2C型 (通信用インスタンス)
  class Bus_I2C : public IBus_PC
  {

  public:
    Bus_I2C(LovyanHAL* hal = nullptr) : IBus_PC(hal) {}
    TransactionI2C createTransaction(void) const { return TransactionI2C(); }

    bus_type_t getType(void) const override { return bus_type_t::bus_i2c; }

    ConfigI2C getConfig(void) const { return _cfg; }

    error_t setConfig(const IConfigBus* cfg) override
    {
      _cfg = *reinterpret_cast<const ConfigI2C*>(cfg);
      return err_ok;
    }

    error_t init(void) override;

    void beginTransaction(ITransaction* transaction, bool read = false, int dummy_clock_bits = 0) override;
    void endTransaction(void) override;

  protected:
    ConfigI2C _cfg;
  };
}

#endif