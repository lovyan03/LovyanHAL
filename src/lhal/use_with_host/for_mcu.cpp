/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#include "../platform_check.hpp"

#if defined ( LHAL_TARGET_PLATFORM )
 #if (LHAL_TARGET_PLATFORM_NUMBER != LHAL_PLATFORM_NUMBER_WINDOWS) \
  && (LHAL_TARGET_PLATFORM_NUMBER != LHAL_PLATFORM_NUMBER_LINUX)

#include "for_mcu.hpp"

namespace lhal
{
  static IBus* bus_table[256] = {};
  static TransactionSPI tr_spi;
  static TransactionI2C tr_i2c;

  namespace internal
  {
    static void send_response(ITransportLayer* st, uint8_t* data, uint8_t length)
    {
      uint8_t checksum = control_code::etx;
      data[cmd_datalen_idx] = length - cmd_payload_idx;
      data[length - 1] = control_code::etx;
      for (int i = 0; i < length - 2; ++i)
      {
        checksum ^= data[i];
      }
      data[length - 2] = checksum;
      st->write(data, length);
    }

    void perform_use_with_host(LovyanHAL* hal, ITransportLayer* st)
    {
      static uint8_t recv_buf[256];
      static uint8_t recv_idx = 0;
      static uint8_t sendlen = 0;
      static uint8_t checksum = 0;
      // static uint8_t prev_seqnum = 0; // 前回コマンド受信時のシーケンス番号
      // static bool seq_ok = false;

      int val;
      while ((val = st->read()) >= 0)
      {
        recv_buf[recv_idx] = val;
        checksum ^= val;
        switch (recv_idx ++)
        {
        case internal::cmd_stx_idx:
          if (val != lhal::internal::control_code::stx)
          {  // STX?
            recv_idx = 0;
            checksum = 0;
          }
          break;

        case internal::cmd_seqnum_idx:
          // seq_ok = ((uint8_t)(val - prev_seqnum)) == 1;
          // if (seq_ok) { ++prev_seqnum; }
          break;

        case internal::cmd_datalen_idx:
          sendlen = val;
          break;

        default:
          if (--sendlen == 0)
          {
            /// ETX + CheckSum ?
            if (recv_buf[recv_idx - 1] == lhal::internal::control_code::etx && checksum == 0)
            {
              uint8_t bus_num = 0;
              IBus* bus = nullptr;
              bool last_error_res = false;

              size_t res_len = cmd_payload_idx + 1 + cmd_suffix_len;
              switch (recv_buf[cmd_payload_idx])
              {
              case lhal::internal::command::check_lhal_device:
                // prev_seqnum = recv_buf[cmd_seqnum_idx];
                // 受信した内容をそのまま送信する;
                res_len = recv_idx;
                break;

              case lhal::internal::command::convert_arduino_pin_num:
                // 受信したArduinoピン番号をMCUピン番号に変換して送信する;
                for (uint_fast8_t i = cmd_payload_idx + 1, i_end = recv_idx - cmd_suffix_len; i < i_end; ++i )
                {
                  recv_buf[i] = hal->convertArduinoPinNumber( recv_buf[i] );
                }
                res_len = recv_idx;
                break;

              case lhal::internal::command::gpio_set_mode:
                // 受信したMCUピン番号に対してGPIOモード設定を行う;
                for (uint_fast8_t i = cmd_payload_idx + 1, i_end = recv_idx - cmd_suffix_len; i < i_end; i += 2 )
                {
                  hal->Gpio.setMode( recv_buf[i] , (LovyanHAL::GPIO_HAL::mode_t)recv_buf[i + 1] );
                }
                break;

              case lhal::internal::command::gpio_read:
                for (uint_fast8_t i = cmd_payload_idx + 1, i_end = recv_idx - cmd_suffix_len; i < i_end; ++i )
                {
                  recv_buf[i] = hal->Gpio.read( recv_buf[i] );
                }
                res_len = recv_idx;
                break;

              case lhal::internal::command::gpio_write_high:
                for (uint_fast8_t i = cmd_payload_idx + 1, i_end = recv_idx - cmd_suffix_len; i < i_end; ++i )
                {
                  hal->Gpio.writeHigh( recv_buf[i] );
                }
                break;

              case lhal::internal::command::gpio_write_low:
                for (uint_fast8_t i = cmd_payload_idx + 1, i_end = recv_idx - cmd_suffix_len; i < i_end; ++i )
                {
                  hal->Gpio.writeLow( recv_buf[i] );
                }
                break;

              case lhal::internal::command::bus_init:
                last_error_res = true;
                bus_num = recv_buf[internal::cmd_payload_idx + 1];
                switch (recv_buf[internal::cmd_payload_idx + 2])
                {
                case bus_type_t::bus_spi:
                  {
                    auto bus_spi = new Bus_SPI();
                    bus = bus_spi;
                    auto cfg = bus_spi->getConfig();
                    cfg.pin_sclk = recv_buf[internal::cmd_payload_idx + 3];
                    cfg.pin_mosi = recv_buf[internal::cmd_payload_idx + 4];
                    cfg.pin_miso = recv_buf[internal::cmd_payload_idx + 5];
                    cfg.pin_dc   = recv_buf[internal::cmd_payload_idx + 6];
                  //cfg.hw_port_num = recv_buf[internal::cmd_payload_idx + 7];
                    bus_spi->setConfig(&cfg);
                  }
                  break;

                case bus_type_t::bus_i2c:
                  {
                    auto bus_i2c = new Bus_I2C();
                    bus = bus_i2c;
                    auto cfg = bus_i2c->getConfig();
                    cfg.pin_scl = recv_buf[internal::cmd_payload_idx + 3];
                    cfg.pin_sda = recv_buf[internal::cmd_payload_idx + 4];
                  //cfg.hw_port_num = recv_buf[internal::cmd_payload_idx + 5];
                    bus_i2c->setConfig(&cfg);
                  }
                  break;

                default:
                  break;
                }
                if (bus)
                {
                  bus_table[bus_num] = bus;
                  bus->init();
                }
                break;

              case lhal::internal::command::bus_begin_transaction:
                last_error_res = true;
                bus_num = recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  ITransaction* tr = nullptr;
                  bool read = recv_buf[internal::cmd_payload_idx + 3];
                  uint8_t dummy_clock = 0;
                  switch (bus->getType())
                  {
                  case bus_type_t::bus_spi:
                    tr_spi.pin_cs = recv_buf[internal::cmd_payload_idx + 2];
                    dummy_clock = recv_buf[internal::cmd_payload_idx + 4];
                    tr_spi.spi_mode = recv_buf[internal::cmd_payload_idx + 5];
                    tr_spi.read_by_mosi = recv_buf[internal::cmd_payload_idx + 6];
                    if (tr_spi.pin_cs.isValid())
                    {
                      hal->Gpio.setMode(tr_spi.pin_cs, hal->Gpio.output_high);
                    }
                    tr = &tr_spi;
                    break;

                  case bus_type_t::bus_i2c:
                    tr_i2c.i2c_addr = recv_buf[internal::cmd_payload_idx + 2];
                    tr = &tr_i2c;
                    break;
                  }
                  if (tr != nullptr)
                  {
                    bus->beginTransaction(tr, read, dummy_clock);
                  }
                }
                break;

              case lhal::internal::command::bus_end_transaction:
                last_error_res = true;
                bus_num = recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  bus->endTransaction();
                }
                break;

              case lhal::internal::command::bus_write:
                last_error_res = true;
                bus_num = recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  bus->write( &recv_buf[cmd_payload_idx + 2], recv_idx - (cmd_suffix_len + cmd_payload_idx + 2) );
                }
                break;

              case lhal::internal::command::bus_write_command:
                last_error_res = true;
                bus_num = recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  bus->writeCommand( &recv_buf[cmd_payload_idx + 2], recv_idx - (cmd_suffix_len + cmd_payload_idx + 2) );
                }
                break;

              case lhal::internal::command::bus_write_data:
                last_error_res = true;
                bus_num = recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  bus->writeData( &recv_buf[cmd_payload_idx + 2], recv_idx - (cmd_suffix_len + cmd_payload_idx + 2) );
                }
                break;

              case lhal::internal::command::bus_write_repeat:
                last_error_res = true;
                bus_num = recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  uint32_t repeat_count = recv_buf[cmd_payload_idx + 2] << 24
                                        | recv_buf[cmd_payload_idx + 3] << 16
                                        | recv_buf[cmd_payload_idx + 4] <<  8
                                        | recv_buf[cmd_payload_idx + 5]
                                        ;
                  uint8_t bytes = recv_buf[cmd_payload_idx + 6];
                  uint32_t data = recv_buf[cmd_payload_idx + 7];
                  if (bytes > 1)
                  {
                    data |= recv_buf[cmd_payload_idx + 8] << 8; 
                    if (bytes > 2)
                    {
                      data |= recv_buf[cmd_payload_idx + 9] << 16;
                      if (bytes > 3)
                      {
                        data |= recv_buf[cmd_payload_idx + 10] << 24;
                      }
                    }
                  }
                  bus->writeRepeat( data, bytes, repeat_count );
                }
                break;

              case lhal::internal::command::bus_write_data_repeat:
                last_error_res = true;
                bus_num = recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  uint32_t repeat_count = recv_buf[cmd_payload_idx + 2] << 24
                                        | recv_buf[cmd_payload_idx + 3] << 16
                                        | recv_buf[cmd_payload_idx + 4] <<  8
                                        | recv_buf[cmd_payload_idx + 5]
                                        ;
                  uint8_t bytes = recv_buf[cmd_payload_idx + 6];
                  uint32_t data = recv_buf[cmd_payload_idx + 7];
                  if (bytes > 1)
                  {
                    data |= recv_buf[cmd_payload_idx + 8] << 8; 
                    if (bytes > 2)
                    {
                      data |= recv_buf[cmd_payload_idx + 9] << 16;
                      if (bytes > 3)
                      {
                        data |= recv_buf[cmd_payload_idx + 10] << 24;
                      }
                    }
                  }
                  bus->writeDataRepeat( data, bytes, repeat_count );
                }
                break;

              case lhal::internal::command::bus_read:
                last_error_res = true;
                bus_num = recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  uint8_t len = recv_buf[internal::cmd_payload_idx + 2];
                  bool nack = recv_buf[internal::cmd_payload_idx + 3];
                  bus->read( &recv_buf[internal::cmd_payload_idx + 1], len, nack );
                  res_len = cmd_payload_idx + 1 + len + cmd_suffix_len;
                }
                break;

              default:
                break;
              }

              if (last_error_res)
              {
                auto idx = res_len - cmd_suffix_len;
                auto result = (bus == nullptr)
                            ? error_t::err_not_implement
                            : bus->getLastError();
                recv_buf[idx++] = (uint8_t)(result >> 8);
                recv_buf[idx++] = (uint8_t)result;
                res_len += 2;
              }
              send_response(st, recv_buf, res_len);
            }
            recv_idx = 0;
            checksum = 0;
          }
          break;
        }
      }
    }
  }
}

 #endif
#endif