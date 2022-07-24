/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#include "../init.hpp"

#if (LHAL_TARGET_PLATFORM_NUMBER == LHAL_PLATFORM_NUMBER_WINDOWS) \
 || (LHAL_TARGET_PLATFORM_NUMBER == LHAL_PLATFORM_NUMBER_LINUX)

#include "for_host.hpp"

#include <string.h>

namespace lhal
{
  bool LHAL_Host::setTransportLayer(internal::ITransportLayer* transport_layer)
  {
    _transport = transport_layer;
    _sendbuf[cmd_idx + 0] = internal::command::check_lhal_device;
    _sendbuf[cmd_idx + 1] = 0xAA;
    _sendbuf[cmd_idx + 2] = 0x55;
    int retry = 4;
    do
    {
      if (_sendCommand(3))
      {
        auto recvlen = _recvCommand();
        if (recvlen == 3 + 4)
        {
          if (memcmp(_sendbuf, _recvbuf, recvlen) == 0)
          {
            return true;
          }
        }
      }
    } while (--retry);
    return false;
  }

  void LHAL_Host::init(void)
  {
    _sendbuf[cmd_idx] = internal::command::convert_arduino_pin_num;
    int pin = 0;
    for (int i = 0; i < 2; ++i)
    {
      for (int j = 1; j <= 128; ++j, ++pin)
      {
        _arduino_pin_table[pin] = pin;
        _sendbuf[cmd_idx + j] = pin;
      }
      if (_sendCommand(129))
      {
        auto len = _recvCommand();
        if (len == 133 && _recvbuf[cmd_idx] == internal::command::convert_arduino_pin_num)
        {
          memcpy(&_arduino_pin_table[i*128], &_recvbuf[3], 128);
        }
      }
    }
  }

  static auto _start_time = std::chrono::system_clock::now();

  uint32_t LHAL_Host::millis(void)
  {
    auto end = std::chrono::system_clock::now();
    auto dur = end - _start_time;
    return std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
  }

  uint32_t LHAL_Host::micros(void)
  {
    auto end = std::chrono::system_clock::now();
    auto dur = end - _start_time;
    return std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
  }

  bool LHAL_Host::_sendCommand(size_t len)
  {
    if (!_transport) { return false; }
    if (len > 252) { return false; }
    _sendbuf[0] = internal::control_code::stx;
    _sendbuf[1] = len + 2;
    _sendbuf[len + 3] = internal::control_code::etx;
    uint_fast8_t cc = internal::control_code::etx;
    for (int i = 0; i < len + 2; ++i)
    {
      cc ^= _sendbuf[i];
    }
    _sendbuf[len + 2] = cc;
    len += 4;


    int retry = 16;
    while (!_transport->write(_sendbuf, len) && --retry)
    {
      _transport->disconnect();
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
      _transport->connect();
    }
    return retry;
  }

  size_t LHAL_Host::_recvCommand(void)
  {
    uint_fast8_t recv_idx = 0;
    uint_fast8_t sendlen = 0;
    uint_fast8_t checksum = 0;
    int retry = 4;
    int val;
    while (--retry)
    {
      val = _transport->read();
      if (val < 0)
      {
        continue;
      }
      retry = 4;

      _recvbuf[recv_idx] = val;
      checksum ^= val;
      if (++recv_idx == 1)
      {
        if (val != internal::control_code::stx)
        {  // STX?
          recv_idx = 0;
          checksum = 0;
        }
      }
      else if (recv_idx == 2)
      {
        sendlen = val;
      }
      else if (--sendlen == 0)
      {
        /// ETX + CheckSum ?
        if (_recvbuf[recv_idx - 1] == internal::control_code::etx && checksum == 0)
        {
          return recv_idx;
        }
        recv_idx = 0;
        checksum = 0;
      }
    }
    return 0;
  }

  void LHAL::GPIO::setMode(gpio::gpio_pin_t pin, mode_t mode)
  {
    if (pin == (gpio::gpio_pin_t)~0u) { return; }
    _lhal->_sendbuf[_lhal->cmd_idx + 0] = internal::command::gpio_set_mode;
    _lhal->_sendbuf[_lhal->cmd_idx + 1] = pin;
    _lhal->_sendbuf[_lhal->cmd_idx + 2] = mode;
    _lhal->_sendCommand(3);
  }

  void LHAL::GPIO::writePortHigh(gpio::port_num_t port, gpio::pin_mask_t bitmask)
  {
    int pin = port << gpio::port_shift;
    _lhal->_sendbuf[_lhal->cmd_idx + 0] = internal::command::gpio_write_high;
    size_t idx = 1;
    for (int i = 0; i < 8; ++i)
    {
      if (bitmask & (1 << i))
      {
        _lhal->_sendbuf[_lhal->cmd_idx + idx] = pin | i;
        ++idx;
      }
    }
    _lhal->_sendCommand(idx);
  }

  void LHAL::GPIO::writePortLow(gpio::port_num_t port, gpio::pin_mask_t bitmask)
  {
    int pin = port << gpio::port_shift;
    _lhal->_sendbuf[_lhal->cmd_idx + 0] = internal::command::gpio_write_high;
    size_t idx = 1;
    for (int i = 0; i < 8; ++i)
    {
      if (bitmask & (1 << i))
      {
        _lhal->_sendbuf[_lhal->cmd_idx + idx] = pin | i;
        ++idx;
      }
    }
    _lhal->_sendCommand(idx);
  }

  void LHAL::GPIO::writeHigh(gpio::gpio_pin_t pin)
  {
    if (pin == (gpio::gpio_pin_t)~0u) { return; }
    _lhal->_sendbuf[_lhal->cmd_idx + 0] = internal::command::gpio_write_high;
    _lhal->_sendbuf[_lhal->cmd_idx + 1] = pin;
    _lhal->_sendCommand(2);
  }

  void LHAL::GPIO::writeLow(gpio::gpio_pin_t pin)
  {
    if (pin == (gpio::gpio_pin_t)~0u) { return; }
    _lhal->_sendbuf[_lhal->cmd_idx + 0] = internal::command::gpio_write_low;
    _lhal->_sendbuf[_lhal->cmd_idx + 1] = pin;
    _lhal->_sendCommand(2);
  }

  bool LHAL::GPIO::read(gpio::gpio_pin_t pin)
  {
    if (pin == (gpio::gpio_pin_t)~0u) { return false; }
    _lhal->_sendbuf[_lhal->cmd_idx + 0] = internal::command::gpio_read;
    _lhal->_sendbuf[_lhal->cmd_idx + 1] = pin;
    if (_lhal->_sendCommand(2))
    {
      auto len = _lhal->_recvCommand();
      if (len == 6 && _lhal->_recvbuf[_lhal->cmd_idx] == internal::command::gpio_read) {
        return _lhal->_recvbuf[_lhal->cmd_idx + 1];
      }
    }
    return false;
  }
}

#endif
