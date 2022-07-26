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
  GPIO_host LovyanHAL::GPIO_t::getHost(gpio_pin_t pin) { return GPIO_host { pin, _lhal }; }

  LovyanHAL::LovyanHAL(void) : LovyanHAL_Base{}
  {}

  error_t LovyanHAL::setTransportLayer(internal::ITransportLayer* transport_layer)
  {
    _transport = transport_layer;
    _sendbuf[internal::cmd_payload_idx + 0] = internal::command::check_lhal_device;
    _sendbuf[internal::cmd_payload_idx + 1] = 0xAA;
    _sendbuf[internal::cmd_payload_idx + 2] = 0x55;
    int retry = 4;
    do
    {
      if (_sendCommand(3) == error_t::err_ok)
      {
        auto recvlen = _recvCommand();
        if (recvlen == 3 + internal::cmd_prefix_len + internal::cmd_suffix_len)
        {
          //if (memcmp((void*)_sendbuf, (const void*)_recvbuf, recvlen) == 0)
          if (memcmp((void*)_queue[_idx_recv_queue].sendbuf, (const void*)_queue[_idx_recv_queue].recvbuf, recvlen) == 0)
          {
            return error_t::err_ok;
          }
        }
      }
    } while (--retry);
    return error_t::err_failed;
  }

  error_t LovyanHAL::init(void)
  {
    int pin = 0;
    for (int i = 0; i < 2; ++i)
    {
      int retry = 4;
      do
      {
        _sendbuf[internal::cmd_payload_idx] = internal::command::convert_arduino_pin_num;
        pin = i * 128;
        for (int j = 1; j <= 128; ++j, ++pin)
        {
          _arduino_pin_table[pin] = pin;
          _sendbuf[internal::cmd_payload_idx + j] = pin;
        }

        if (_sendCommand(129) == error_t::err_ok)
        {
          auto len = _recvCommand();
          if (len == (129 + internal::cmd_prefix_len + internal::cmd_suffix_len) && _recvbuf[internal::cmd_payload_idx] == internal::command::convert_arduino_pin_num)
          {
            memcpy(&_arduino_pin_table[i*128], (const void*) & _recvbuf[internal::cmd_payload_idx + 1], 128);
            break;
          }
        }
        delay(8);
      } while (--retry);
      if (!retry)
      {
        return error_t::err_failed;
      }
    }
    return error_t::err_ok;
  }

  static auto _start_time = std::chrono::system_clock::now();

  uint32_t LovyanHAL::millis(void)
  {
    auto end = std::chrono::system_clock::now();
    auto dur = end - _start_time;
    return std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
  }

  uint32_t LovyanHAL::micros(void)
  {
    auto end = std::chrono::system_clock::now();
    auto dur = end - _start_time;
    return std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
  }

  void LovyanHAL::delay(size_t msec)
  {
    if (msec < 32)
    {
      delayMicroseconds(msec * 1000);
    }
    else
    {
      auto ms = millis();
      std::this_thread::sleep_for(std::chrono::milliseconds(msec - 8));
      do
      {
        std::this_thread::yield();
      } while (millis() - ms < msec);
    }
  }

  void LovyanHAL::delayMicroseconds(size_t usec)
  {
    auto us = micros();
    do
    {
      std::this_thread::yield();
    } while (micros() - us < usec);
  }


  error_t LovyanHAL::_sendCommand(size_t len)
  {
    if (!_transport) { return error_t::err_failed; }
    if (len > internal::cmd_payload_maxlen) { return error_t::err_failed; }
    _sendbuf[internal::cmd_stx_idx] = internal::control_code::stx;
    _sendbuf[internal::cmd_seqnum_idx] = _idx_send_queue;
    _sendbuf[internal::cmd_datalen_idx] = len + internal::cmd_suffix_len;
    _sendbuf[internal::cmd_datalen_idx + len + 2] = internal::control_code::etx;
    len += internal::cmd_prefix_len;
    uint_fast8_t cc = internal::control_code::etx;
    for (int i = 0; i < len; ++i)
    {
      cc ^= _sendbuf[i];
    }
    _sendbuf[len] = cc;
    len += internal::cmd_suffix_len;


    while (_queued_bytes >= 192 && (proc_queue() == error_t::err_ok));


    _queue[_idx_send_queue].recvlen = 0;
    _queue[_idx_send_queue].sendlen = len;
    _queue[_idx_send_queue].state = queue_data_t::state_wait_send;
    proc_queue();

    _idx_recv_queue = _idx_send_queue++;
    while (_queue[_idx_send_queue].state != queue_data_t::state_free)
    {
      // ToDo: exception throw して処理を終える？
      printf("queue jam.");
      return error_t::err_failed;
    }
    _sendbuf = _queue[_idx_send_queue].sendbuf;
    _recvbuf = _queue[_idx_send_queue].recvbuf;
    return error_t::err_ok;
  }

  error_t LovyanHAL::proc_queue(void)
  {
    auto q = &_queue[idx_read_queue];
    if (q->state == queue_data_t::state_wait_recv)
    {
      int val;
      for (int i = 0; i < 16; ++i)
      {
        val = _transport->read();
        if (val < 0 || val == internal::control_code::stx) { break; }
      }
      if (val == internal::control_code::stx)
      {
        q->recvbuf[0] = val;
        uint_fast8_t checksum = val;
        uint_fast8_t recv_idx = 1;
        uint_fast8_t remain_len = 0;
        int retry = 65536;
        bool succeed = false;
        while (!succeed && --retry)
        {
          val = _transport->read();
          if (val < 0)
          {
            std::this_thread::yield();
            continue;
          }
          //_transport->write(0);
          retry = 65536;

          q->recvbuf[recv_idx] = val;
          checksum ^= val;
          switch (recv_idx++)
          {
          case internal::cmd_seqnum_idx:
            if (val != q->sendbuf[internal::cmd_seqnum_idx])
            {
              recv_idx = 0;
              checksum = 0;
            }
            break;

          case internal::cmd_stx_idx:
            if (val != internal::control_code::stx)
            {  // STX?
              recv_idx = 0;
              checksum = 0;
            }
            break;

          case internal::cmd_datalen_idx:
            remain_len = val;
            break;

          default:
            if (--remain_len == 0)
            {
              q->recvlen = recv_idx;
              /// ETX + CheckSum ?
              if (q->recvbuf[recv_idx - 1] == internal::control_code::etx && checksum == 0)
              {
                succeed = true;
                _queued_bytes -= q->sendlen;
                q->state = queue_data_t::state_free;
                ++idx_read_queue;
                break;
              }
              recv_idx = 0;
              checksum = 0;
            }
            break;
          }
        }
        // retryが0なら受信失敗、retryが残っているなら受信成功;
        if (!retry)
        {
          q->state = queue_data_t::state_error_recv;
          return error_t::err_failed;
        }
      }
    }

    {
      auto q = &_queue[idx_write_queue];
      if (q->state == queue_data_t::state_wait_send)
      {
        // データ送信処理;
        int retry = 16;
        _queued_bytes += q->sendlen;
        while (!_transport->write((const uint8_t*)q->sendbuf, q->sendlen) && --retry)
        {
          _transport->disconnect();
          std::this_thread::sleep_for(std::chrono::milliseconds(16));
          _transport->connect();
        }
        // retryが0なら送信失敗、retryが残っているなら送信成功;
        if (!retry)
        {
          q->state = queue_data_t::state_error_send;
          return error_t::err_failed;
        }
        q->state = queue_data_t::state_wait_recv;
        idx_write_queue++;
      }
    }
    return error_t::err_ok;
  }

  size_t LovyanHAL::_recvCommand(void)
  {
    _recvbuf = _queue[_idx_recv_queue].recvbuf;
    while (_queue[_idx_recv_queue].state != queue_data_t::state_free)
    {
      proc_queue();
//    std::this_thread::yield();
      if (_queue[_idx_recv_queue].state == queue_data_t::state_error_send)
      {
        return 0;
      }
      if (_queue[_idx_recv_queue].state == queue_data_t::state_error_recv)
      {
        return 0;
      }
    }
    return _queue[_idx_recv_queue].recvlen;

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

  void LHAL::GPIO_t::setMode(gpio::gpio_pin_t pin, mode_t mode)
  {
    if (pin == (gpio::gpio_pin_t)~0u) { return; }
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::gpio_set_mode;
    _lhal->_sendbuf[internal::cmd_payload_idx + 1] = pin;
    _lhal->_sendbuf[internal::cmd_payload_idx + 2] = mode;
    _lhal->_sendCommand(3);
  }

  void LHAL::GPIO_t::writePortHigh(gpio::port_num_t port, gpio::pin_mask_t bitmask)
  {
    int pin = port << gpio::port_shift;
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::gpio_write_high;
    size_t idx = 1;
    for (int i = 0; i < 8; ++i)
    {
      if (bitmask & (1 << i))
      {
        _lhal->_sendbuf[internal::cmd_payload_idx + idx] = pin | i;
        ++idx;
      }
    }
    _lhal->_sendCommand(idx);
  }

  void LHAL::GPIO_t::writePortLow(gpio::port_num_t port, gpio::pin_mask_t bitmask)
  {
    int pin = port << gpio::port_shift;
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::gpio_write_high;
    size_t idx = 1;
    for (int i = 0; i < 8; ++i)
    {
      if (bitmask & (1 << i))
      {
        _lhal->_sendbuf[internal::cmd_payload_idx + idx] = pin | i;
        ++idx;
      }
    }
    _lhal->_sendCommand(idx);
  }

  void LHAL::GPIO_t::writeHigh(gpio::gpio_pin_t pin)
  {
    if (pin == (gpio::gpio_pin_t)~0u) { return; }
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::gpio_write_high;
    _lhal->_sendbuf[internal::cmd_payload_idx + 1] = pin;
    _lhal->_sendCommand(2);
  }

  void LHAL::GPIO_t::writeLow(gpio::gpio_pin_t pin)
  {
    if (pin == (gpio::gpio_pin_t)~0u) { return; }
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::gpio_write_low;
    _lhal->_sendbuf[internal::cmd_payload_idx + 1] = pin;
    _lhal->_sendCommand(2);
  }

  bool LHAL::GPIO_t::read(gpio::gpio_pin_t pin)
  {
    if (pin == (gpio::gpio_pin_t)~0u) { return false; }
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::gpio_read;
    _lhal->_sendbuf[internal::cmd_payload_idx + 1] = pin;
    if (_lhal->_sendCommand(2) == error_t::err_ok)
    {
      auto len = _lhal->_recvCommand();
      if (len == (2 + internal::cmd_prefix_len + internal::cmd_suffix_len) && _lhal->_recvbuf[internal::cmd_payload_idx] == internal::command::gpio_read) {
        return _lhal->_recvbuf[internal::cmd_payload_idx + 1];
      }
    }
    return false;
  }
}

#endif
