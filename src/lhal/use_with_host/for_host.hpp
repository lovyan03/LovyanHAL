/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_USE_WITH_HOST_FOR_HOST_HPP_
#define LOVYANHAL_USE_WITH_HOST_FOR_HOST_HPP_

#include <thread>

#include "../LovyanHAL_Base.hpp"
#include "common.hpp"

namespace lhal
{
  class LovyanHAL : public LovyanHAL_Base
  {
  public:

    class GPIO_t : public GPIO_Base
    {
      LovyanHAL* _lhal;
    public:
      GPIO_t(LovyanHAL* hal) : _lhal { hal } {}

      GPIO_host getHost(gpio_pin_t pin);

      void setMode(gpio_pin_t pin, mode_t mode);

      void writePortHigh(port_num_t port, pin_mask_t bitmask);
      void writePortLow(port_num_t port, pin_mask_t bitmask);

      void writeHigh(gpio_pin_t pin);
      void writeLow(gpio_pin_t pin);
      void write(gpio_pin_t pin, bool value) { if (value) { writeHigh(pin); } else { writeLow(pin); } };
      bool read(gpio_pin_t pin);
    };

    LovyanHAL(void);
    LovyanHAL(internal::ITransportLayer* transport_layer) : LovyanHAL{} { setTransportLayer(transport_layer); }

    GPIO_t Gpio { this };

    gpio::gpio_pin_t convertArduinoPinNumber(int arduino_pin_number) { return _arduino_pin_table[arduino_pin_number]; }

    virtual error_t init(void);

    static uint32_t millis(void);
    static uint32_t micros(void);
    static void delay(size_t msec);
    static void delayMicroseconds(size_t usec);

  protected:
    error_t setTransportLayer(internal::ITransportLayer* transport_layer);
    internal::ITransportLayer* _transport = nullptr;

  private:
    static constexpr size_t timeout_msec = 512;

    gpio::gpio_pin_t _arduino_pin_table[256]; // arduinoピン番号からMCUピン番号への変換テーブル;

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
      uint8_t sendbuf[256]; // 送信バッファ;
      uint8_t recvbuf[256]; // 送信バッファ;
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
    LovyanHAL* _lhal;
    LovyanHAL::GPIO_t::gpio_pin_t _gpio_pin;
  public:
    GPIO_host(LovyanHAL::GPIO_t::gpio_pin_t pin, LovyanHAL* lhal) : _lhal { lhal }, _gpio_pin { pin } {};

    void setMode(LovyanHAL::GPIO_t::mode_t mode) { _lhal->Gpio.setMode(_gpio_pin, mode); }
    void writeHigh(void) { _lhal->Gpio.writeHigh(_gpio_pin); }
    void writeLow(void) { _lhal->Gpio.writeLow(_gpio_pin); }
    void write(bool value) { _lhal->Gpio.write(_gpio_pin, value); }
    bool read(void) { return _lhal->Gpio.read(_gpio_pin); }
  };
}

#endif