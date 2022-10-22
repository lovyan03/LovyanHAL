/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#include "../common.inl"

#include <string.h>
#include <algorithm>

namespace lhal
{
 namespace v0
 {

  GPIO_host LovyanHAL_PC::GPIO_HAL::getHost(gpio_port_pin_t pin) { return GPIO_host { pin, _lhal }; }

  uint8_t LovyanHAL_PC::getBusSequenceNumber(IBus* bus)
  {
    uint8_t res = _bus_count++;
    _bus_list[res] = bus;
    return res;
  }

  LovyanHAL_PC::LovyanHAL_PC(void) : LovyanHAL_Base{}
  {}

  error_t LovyanHAL_PC::setTransportLayer(internal::ITransportLayer* transport_layer)
  {
    _transport = transport_layer;

    int retry = 4;
    do
    {
      _idx_next_queue = 0;
      _idx_current_queue = (uint8_t)-1;
      _idx_send_queue = 0;
      _idx_recv_queue = 0;

      _sendbuf[internal::cmd_payload_idx + 0] = internal::command::check_lhal_device;
      _sendbuf[internal::cmd_payload_idx + 1] = 0xAA;
      _sendbuf[internal::cmd_payload_idx + 2] = 0x55;
      if (_sendCommand(3) == error_t::err_ok)
      {
        auto recvlen = _recvCommand();
        if (recvlen == 3 + internal::cmd_prefix_len + internal::cmd_suffix_len)
        {
          //if (memcmp((void*)_sendbuf, (const void*)_recvbuf, recvlen) == 0)
          if (memcmp(_queue[_idx_current_queue].sendbuf, _queue[_idx_current_queue].recvbuf, recvlen) == 0)
          {
            return error_t::err_ok;
          }
        }
      }
      _transport->disconnect();
      delay(16);
      _transport->connect();
      memset(_sendbuf, 0, 256);
      _transport->write(_sendbuf, 256);
      delay(16);
      while (_transport->read() >= 0);
    } while (--retry);
    return error_t::err_failed;
  }

  error_t LovyanHAL_PC::init(void)
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

  uint64_t LovyanHAL_PC::millis(void)
  {
    auto end = std::chrono::system_clock::now();
    auto dur = end - _start_time;
    return std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
  }

  uint64_t LovyanHAL_PC::micros(void)
  {
    auto end = std::chrono::system_clock::now();
    auto dur = end - _start_time;
    return std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
  }

  void LovyanHAL_PC::delay(size_t msec)
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

  void LovyanHAL_PC::delayMicroseconds(size_t usec)
  {
    auto us = micros();
    do
    {
      std::this_thread::yield();
    } while (micros() - us < usec);
  }


  error_t LovyanHAL_PC::_sendCommand(size_t len)
  {
    if (!_transport) { return error_t::err_failed; }
    if (len > internal::cmd_payload_maxlen) { return error_t::err_failed; }
    _sendbuf[internal::cmd_stx_idx] = internal::control_code::stx;
    _sendbuf[internal::cmd_seqnum_idx] = _idx_next_queue;
    _sendbuf[internal::cmd_datalen_idx] = static_cast<uint8_t>(len + internal::cmd_suffix_len);
    _sendbuf[internal::cmd_datalen_idx + len + 2] = internal::control_code::etx;
    len += internal::cmd_prefix_len;
    uint_fast8_t cc = internal::control_code::etx;
    for (int i = 0; i < len; ++i)
    {
      cc ^= _sendbuf[i];
    }
    _sendbuf[len] = cc;
    len += internal::cmd_suffix_len;


//    while (_queued_bytes >= 192 && (proc_queue() == error_t::err_ok));


    _queue[_idx_next_queue].recvlen = 0;
    _queue[_idx_next_queue].sendlen = static_cast<uint8_t>(len);
    _queue[_idx_next_queue].state = queue_data_t::state_wait_send;
    auto result = proc_queue();
    if (result != error_t::err_ok) { return result; }

    _idx_current_queue = _idx_next_queue++;
    while (_queue[_idx_next_queue].state != queue_data_t::state_free)
    {
      // ToDo: exception throw して処理を終える？
      printf("queue jam.\n");
      return error_t::err_failed;
    }
    _sendbuf = _queue[_idx_next_queue].sendbuf;
    _recvbuf = _queue[_idx_next_queue].recvbuf;
    return error_t::err_ok;
  }

  error_t LovyanHAL_PC::_proc_receive(void)
  {
    auto q = &_queue[_idx_recv_queue];
    if (q->state != queue_data_t::state_wait_recv)
    {
      return error_t::err_ok;
    }

    int val;
    while ((val = _transport->read()) >= 0)
    {
      if ((q->recvlen == internal::cmd_stx_idx) && (val != internal::control_code::stx))
      {
        // ToDo:データ異常対策の実装?;
        printf("error: receive unknown data: %02x  %c  \n", val, val);
        continue;
      }

      q->recvbuf[q->recvlen] = val;
      switch (q->recvlen++)
      {
      case internal::cmd_stx_idx:
        break;

      case internal::cmd_seqnum_idx:
        if (val != _idx_recv_queue)
        { // ToDo:データ異常対策の実装;
          printf("error: receive sequence mismatch. send:%02x  recv:%02x\n", _idx_recv_queue, val);
          return error_t::err_failed;
        }
        break;

      case internal::cmd_datalen_idx:
        break;

      default:
        if ((q->recvbuf[internal::cmd_datalen_idx] + internal::cmd_payload_idx) == q->recvlen)
        {
          if (val != internal::control_code::etx)
          { // ToDo:データ異常対策の実装;
            printf("error: receive etx mismatch. seq:%02x\n", _idx_recv_queue);
            q->state = q->state_error_recv;
            return error_t::err_failed;
          }
          else
          {
            uint_fast8_t checksum = 0;
            for (int i = 0; i < q->recvlen; ++i)
            {
              checksum ^= q->recvbuf[i];
            }
            if (checksum != 0)
            { // ToDo:データ異常対策の実装;
              printf("error: receive checksum mismatch. seq:%02x\n", _idx_recv_queue);
              q->state = q->state_error_recv;
              return error_t::err_failed;
            }
            _queued_bytes -= q->sendlen;
            q->state = q->state_free;
            q = &_queue[++_idx_recv_queue];
            return error_t::err_ok;
          }
        }
        break;
      }
    }
    return error_t::err_ok;
  }

  error_t LovyanHAL_PC::proc_queue(void)
  {
    error_t result = _proc_receive();
    if (result != error_t::err_ok)
    {
      return result;
    }

    if (_queued_bytes >= 192)
    {
      auto ms = millis();
      do
      {
        error_t result = _proc_receive();
        if (result != error_t::err_ok)
        {
          return result;
        }
        if (millis() - ms > timeout_msec)
        {
          printf("error: receive timeout.\n");
          return error_t::err_failed;
        }
      } while (_queued_bytes >= 192);
    }

    {
      auto q = &_queue[_idx_send_queue];
      if (q->state == queue_data_t::state_wait_send)
      {
        // データ送信処理;
        int retry = 16;
        _queued_bytes += q->sendlen;
        while (!_transport->write((const uint8_t*)q->sendbuf, q->sendlen) && --retry)
        {
          _transport->disconnect();
          delay(16);
          _transport->connect();
        }
        // retryが0なら送信失敗、retryが残っているなら送信成功;
        if (!retry)
        {
          q->state = queue_data_t::state_error_send;
          return error_t::err_failed;
        }
        q->state = queue_data_t::state_wait_recv;
        _idx_send_queue++;
      }
    }
    return error_t::err_ok;
  }

  size_t LovyanHAL_PC::_recvCommand(void)
  {
    auto ms = millis();
    _recvbuf = _queue[_idx_current_queue].recvbuf;
    while (_queue[_idx_current_queue].state != queue_data_t::state_free)
    {
      if (millis() - ms > timeout_msec)
      {
        printf("error: receive timeout.\n");
        return error_t::err_failed;
      }
      proc_queue();
//    std::this_thread::yield();
      if (_queue[_idx_current_queue].state == queue_data_t::state_error_send)
      {
        return 0;
      }
      if (_queue[_idx_current_queue].state == queue_data_t::state_error_recv)
      {
        return 0;
      }
    }
    return _queue[_idx_current_queue].recvlen;

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

  void LovyanHAL::GPIO_HAL::setMode(gpio_port_pin_t pin, mode_t mode)
  {
    if (pin.isInvalid()) { return; }
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::gpio_set_mode;
    _lhal->_sendbuf[internal::cmd_payload_idx + 1] = pin;
    _lhal->_sendbuf[internal::cmd_payload_idx + 2] = mode;
    _lhal->_sendCommand(3);
  }

  void LovyanHAL::GPIO_HAL::writePortHigh(gpio::port_num_t port, gpio::pin_mask_t bitmask)
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

  void LovyanHAL::GPIO_HAL::writePortLow(gpio::port_num_t port, gpio::pin_mask_t bitmask)
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

  void LovyanHAL::GPIO_HAL::writeHigh(gpio_port_pin_t pin)
  {
    if (pin.isInvalid()) { return; }
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::gpio_write_high;
    _lhal->_sendbuf[internal::cmd_payload_idx + 1] = pin;
    _lhal->_sendCommand(2);
  }

  void LovyanHAL::GPIO_HAL::writeLow(gpio_port_pin_t pin)
  {
    if (pin.isInvalid()) { return; }
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::gpio_write_low;
    _lhal->_sendbuf[internal::cmd_payload_idx + 1] = pin;
    _lhal->_sendCommand(2);
  }

  bool LovyanHAL::GPIO_HAL::read(gpio_port_pin_t pin)
  {
    if (pin.isInvalid()) { return false; }
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


//--------------------------------------------------------------------------------

  size_t IBus_PC::_update_last_error(void)
  {
    _last_error = error_t::err_failed;
    auto len = _lhal->_recvCommand();
    if (len > (2 + internal::cmd_prefix_len + internal::cmd_suffix_len))
    {
      _last_error = (error_t)
        ( _lhal->_recvbuf[len - internal::cmd_suffix_len - 2] << 8
        | _lhal->_recvbuf[len - internal::cmd_suffix_len - 1] );
    }
    return len;
  }

  error_t IBus_PC::getLastError(void)
  {
    _update_last_error();
    return _last_error;
  }

  void IBus_PC::dummyClock(uint8_t dummy_clock_bits)
  {
    if (dummy_clock_bits == 0) { return; }
    int idx = internal::cmd_payload_idx + 0;
    auto buf = _lhal->_sendbuf;
    buf[idx++] = internal::command::bus_dummy_clock;
    buf[idx++] = _bus_number;
    buf[idx++] = dummy_clock_bits;
    _lhal->_sendCommand(idx);
  }

  void IBus_PC::_write_internal(const uint8_t* data, size_t len, uint8_t cmd)
  {
    do
    {
      _lhal->_sendbuf[internal::cmd_payload_idx + 0] = cmd;
      _lhal->_sendbuf[internal::cmd_payload_idx + 1] = _bus_number;
      size_t l = std::min<size_t>(len, internal::cmd_payload_maxlen - 2);
      memcpy(&_lhal->_sendbuf[internal::cmd_payload_idx + 2], data, l);
      _lhal->_sendCommand(l + 2);
      data += l;
      len -= l;
    } while (len);
  }

  void IBus_PC::_write_repeat_internal(uint32_t data, uint8_t bytes, size_t repeat_count, uint8_t cmd)
  {
    if ((uint8_t)(bytes - 1) > 3u) { return; }
    int idx = internal::cmd_payload_idx + 0;
    auto buf = _lhal->_sendbuf;
    buf[idx++] = cmd;
    buf[idx++] = _bus_number;
    buf[idx++] = (uint8_t)(repeat_count >> 24);
    buf[idx++] = (uint8_t)(repeat_count >> 16);
    buf[idx++] = (uint8_t)(repeat_count >> 8);
    buf[idx++] = (uint8_t)(repeat_count);
    buf[idx++] = bytes;
    do
    {
      buf[idx++] = (uint8_t)data;
      data >>= 8;
    } while (--bytes);
    _lhal->_sendCommand(idx);
  }
  /*
  void IBus_PC::write16(uint16_t data)
  {
    uint8_t buf[2] = { (uint8_t)(data >> 8), (uint8_t)data };
    _write_internal(buf, 2, internal::command::bus_write);

    _update_last_error();
  }
//*/

  void IBus_PC::transfer(const uint8_t* write_data, uint8_t* read_data, size_t len)
  {
    do
    {
      uint8_t l = static_cast<uint8_t>(std::min<size_t>(len, internal::cmd_payload_maxlen - 2));
      _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::bus_transfer;
      _lhal->_sendbuf[internal::cmd_payload_idx + 1] = _bus_number;
      memcpy(&_lhal->_sendbuf[internal::cmd_payload_idx + 2], write_data, l);
      _lhal->_sendCommand(l + 2);
      write_data += l;
      size_t readlen = _update_last_error();
      if (isError())
      {
        return;
      }
      memcpy(read_data, &_lhal->_recvbuf[internal::cmd_payload_idx + 1], l);
      read_data += l;

      len -= l;
    } while (len);
  }

  void IBus_PC::read(uint8_t* data, size_t len, bool nack)
  {
    do
    {
      uint8_t l = static_cast<uint8_t>(std::min<size_t>(len, internal::cmd_payload_maxlen - 2));
      len -= l;
      _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::bus_read;
      _lhal->_sendbuf[internal::cmd_payload_idx + 1] = _bus_number;
      _lhal->_sendbuf[internal::cmd_payload_idx + 2] = l;
      _lhal->_sendbuf[internal::cmd_payload_idx + 3] = (len == 0) && nack;
      _lhal->_sendCommand(4);

      size_t readlen = _update_last_error();
      if (isError())
      {
        return;
      }
      memcpy(data, &_lhal->_recvbuf[internal::cmd_payload_idx + 1], l);
      data += l;
    } while (len);
  }

//--------------------------------------------------------------------------------

  error_t Bus_SPI::init(void)
  {
    auto res = IBus_PC::init();
    if (checkSuccess(res))
    {
      _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::bus_init;
      _lhal->_sendbuf[internal::cmd_payload_idx + 1] = _bus_number;
      _lhal->_sendbuf[internal::cmd_payload_idx + 2] = bus_type_t::bus_spi;
      _lhal->_sendbuf[internal::cmd_payload_idx + 3] = _cfg.pin_sclk.num;
      _lhal->_sendbuf[internal::cmd_payload_idx + 4] = _cfg.pin_mosi.num;
      _lhal->_sendbuf[internal::cmd_payload_idx + 5] = _cfg.pin_miso.num;
      _lhal->_sendbuf[internal::cmd_payload_idx + 6] = 255; // hw_periph_num
      _lhal->_sendCommand(7);
    }
    return getLastError();
  }

  error_t Bus_SPI::beginTransaction_2nd_impl(bool read)
  {
    _last_error = err_ok;
    auto tr = reinterpret_cast<TransactionSPI*>(_transaction);
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::bus_begin_transaction;
    _lhal->_sendbuf[internal::cmd_payload_idx + 1] = _bus_number;
    _lhal->_sendbuf[internal::cmd_payload_idx + 2] = (uint8_t)(tr->freq_write >> 24);
    _lhal->_sendbuf[internal::cmd_payload_idx + 3] = (uint8_t)(tr->freq_write >> 16);
    _lhal->_sendbuf[internal::cmd_payload_idx + 4] = (uint8_t)(tr->freq_write >> 8);
    _lhal->_sendbuf[internal::cmd_payload_idx + 5] = (uint8_t)(tr->freq_write >> 0);
    _lhal->_sendbuf[internal::cmd_payload_idx + 6] = (uint8_t)(tr->freq_read >> 24);
    _lhal->_sendbuf[internal::cmd_payload_idx + 7] = (uint8_t)(tr->freq_read >> 16);
    _lhal->_sendbuf[internal::cmd_payload_idx + 8] = (uint8_t)(tr->freq_read >> 8);
    _lhal->_sendbuf[internal::cmd_payload_idx + 9] = (uint8_t)(tr->freq_read >> 0);
    _lhal->_sendbuf[internal::cmd_payload_idx + 10] = read;
    _lhal->_sendbuf[internal::cmd_payload_idx + 11] = tr->spi_mode;
    _lhal->_sendbuf[internal::cmd_payload_idx + 12] = tr->read_by_mosi;
    _lhal->_sendCommand(13);
    return getLastError();
  }

  // 通信を終了する;
  error_t Bus_SPI::endTransaction_impl(void)
  {
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::bus_end_transaction;
    _lhal->_sendbuf[internal::cmd_payload_idx + 1] = _bus_number;
    _lhal->_sendCommand(2);
    return getLastError();
  }
/*
  void Bus_SPI::dcControl(bool value)
  {
    waitBusy();
    if (_cfg.pin_dc.isValid())
    {
      _lhal->Gpio.write(_cfg.pin_dc, value);
    }
  }
  void Bus_SPI::csControl(bool value, bool read)
  {
    waitBusy();
    auto tr = reinterpret_cast<TransactionSPI*>(_transaction);
    _lhal->Gpio.write(_cfg.pin_sclk, (tr->spi_mode & 2));
    tr->csControl(this, value, read);
  }
//*/
//--------------------------------------------------------------------------------

  error_t Bus_I2C::init(void)
  {
    auto res = IBus_PC::init();
    if (checkSuccess(res))
    {
      _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::bus_init;
      _lhal->_sendbuf[internal::cmd_payload_idx + 1] = _bus_number;
      _lhal->_sendbuf[internal::cmd_payload_idx + 2] = bus_type_t::bus_i2c;
      _lhal->_sendbuf[internal::cmd_payload_idx + 3] = _cfg.pin_scl.num;
      _lhal->_sendbuf[internal::cmd_payload_idx + 4] = _cfg.pin_sda.num;
      _lhal->_sendbuf[internal::cmd_payload_idx + 5] = 255; // hw_periph_num
      _lhal->_sendCommand(6);
    }
    return getLastError();
  }

  error_t Bus_I2C::beginTransaction_2nd_impl(bool read)
  {
    auto tr = reinterpret_cast<TransactionI2C*>(_transaction);
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::bus_begin_transaction;
    _lhal->_sendbuf[internal::cmd_payload_idx + 1] = _bus_number;
    _lhal->_sendbuf[internal::cmd_payload_idx + 2] = (uint8_t)(tr->freq_write >> 24);
    _lhal->_sendbuf[internal::cmd_payload_idx + 3] = (uint8_t)(tr->freq_write >> 16);
    _lhal->_sendbuf[internal::cmd_payload_idx + 4] = (uint8_t)(tr->freq_write >>  8);
    _lhal->_sendbuf[internal::cmd_payload_idx + 5] = (uint8_t)(tr->freq_write >>  0);
    _lhal->_sendbuf[internal::cmd_payload_idx + 6] = (uint8_t)(tr->freq_read >> 24);
    _lhal->_sendbuf[internal::cmd_payload_idx + 7] = (uint8_t)(tr->freq_read >> 16);
    _lhal->_sendbuf[internal::cmd_payload_idx + 8] = (uint8_t)(tr->freq_read >> 8);
    _lhal->_sendbuf[internal::cmd_payload_idx + 9] = (uint8_t)(tr->freq_read >> 0);
    _lhal->_sendbuf[internal::cmd_payload_idx + 10] = read;
    _lhal->_sendbuf[internal::cmd_payload_idx + 11] = (uint8_t)(tr->timeout_msec >> 8);
    _lhal->_sendbuf[internal::cmd_payload_idx + 12] = (uint8_t)(tr->timeout_msec);
    _lhal->_sendCommand(13);
    return getLastError();
  }

  // 通信を終了する;
  error_t Bus_I2C::endTransaction_impl(void)
  {
    _lhal->_sendbuf[internal::cmd_payload_idx + 0] = internal::command::bus_end_transaction;
    _lhal->_sendbuf[internal::cmd_payload_idx + 1] = _bus_number;
    _lhal->_sendCommand(2);
    return getLastError();
  }

 }
}
