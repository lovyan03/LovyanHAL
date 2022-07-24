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

#include "../LHAL_Base.hpp"
#include "common.hpp"

namespace lhal
{
  class LHAL_Host;

  class LHAL_Host : public LHAL_Base
  {
  public:
    class GPIO : public GPIO_Base
    {
      LHAL_Host* _lhal;
    public:
      GPIO(LHAL_Host* hal) : _lhal { hal } {}
      void setMode(gpio::gpio_pin_t pin, mode_t mode);

      void writePortHigh(gpio::port_num_t port, gpio::pin_mask_t bitmask);
      void writePortLow(gpio::port_num_t port, gpio::pin_mask_t bitmask);

      void writeHigh(gpio::gpio_pin_t pin);
      void writeLow(gpio::gpio_pin_t pin);
      void write(gpio::gpio_pin_t pin, bool value) { if (value) { writeHigh(pin); } else { writeLow(pin); } };
      bool read(gpio::gpio_pin_t pin);
    };

    LHAL_Host(void) : LHAL_Base{} {}
    LHAL_Host(lhal::internal::ITransportLayer* transport_layer) : LHAL_Base{} { setTransportLayer(transport_layer); init(); }

    GPIO Gpio { this };

    gpio::gpio_pin_t convertArduinoPinNumber(int arduino_pin_number) { return _arduino_pin_table[arduino_pin_number]; }

    static uint32_t millis(void);
    static uint32_t micros(void);
    static inline void delay(size_t msec) { std::this_thread::sleep_for(std::chrono::milliseconds(msec)); }
    static inline void delayMicroseconds(size_t usec) { std::this_thread::sleep_for(std::chrono::microseconds(usec)); }

  protected:
    bool setTransportLayer(internal::ITransportLayer* transport_layer);
    void init(void);
    gpio::gpio_pin_t _arduino_pin_table[256]; // arduinoピン番号からMCUピン番号への変換テーブル;

    static constexpr size_t cmd_idx = 2; // _sendbuf内でのコマンド部の位置;
    uint8_t _sendbuf[256]; // 送信バッファ;
    uint8_t _recvbuf[256]; // 受信バッファ;

    bool _sendCommand(size_t len);
    size_t _recvCommand(void);
    internal::ITransportLayer* _transport;
  };
}

#endif