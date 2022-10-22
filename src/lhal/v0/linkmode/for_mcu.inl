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
 #if (LHAL_TARGET_PLATFORM_NUMBER > LHAL_PLATFORM_NUMBER_HOST_MAX)

#include "for_mcu.hpp"

#define LHAL_LOG(str)
// #define LHAL_LOG(str) printf(str)

namespace lhal
{
 namespace v0
 {
  namespace internal
  {
    static constexpr const uint8_t bus_num_count = 32;
    static constexpr const uint8_t bus_num_mask = bus_num_count - 1;
    static IBus* bus_table[bus_num_count] = { nullptr };
    static ITransaction* transaction_table[bus_num_count] = { nullptr };

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
LHAL_LOG("bus_init:");
                last_error_res = true;
                bus_num = bus_num_mask & recv_buf[internal::cmd_payload_idx + 1];
                switch (recv_buf[internal::cmd_payload_idx + 2])
                {
                case bus_type_t::bus_spi:
LHAL_LOG("bus_spi:");
                  {
                    auto bus_spi = new Bus_SPI();
                    bus = bus_spi;
                    auto cfg = bus_spi->getConfig();
                    cfg.pin_sclk = recv_buf[internal::cmd_payload_idx + 3];
                    cfg.pin_mosi = recv_buf[internal::cmd_payload_idx + 4];
                    cfg.pin_miso = recv_buf[internal::cmd_payload_idx + 5];
                  //cfg.hw_port_num = recv_buf[internal::cmd_payload_idx + 6];
                    bus_spi->setConfig(&cfg);
                  }
                  break;

                case bus_type_t::bus_i2c:
LHAL_LOG("bus_i2c:");
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
                  if (bus_table[bus_num] != nullptr) { delete bus_table[bus_num]; }
                  bus_table[bus_num] = bus;
                  bus->init();
LHAL_LOG("bus->init() exec\n");
                }
                break;

              case lhal::internal::command::bus_begin_transaction:
LHAL_LOG("bus_begin_transaction:");
                last_error_res = true;
                bus_num = bus_num_mask & recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  ITransaction* tr = transaction_table[bus_num];
                  bool read = recv_buf[internal::cmd_payload_idx + 10];

                  uint16_t freq_write_h = ((uint16_t)recv_buf[cmd_payload_idx + 2] << 8) | recv_buf[cmd_payload_idx + 3];
                  uint16_t freq_write_l = ((uint16_t)recv_buf[cmd_payload_idx + 4] << 8) | recv_buf[cmd_payload_idx + 5];
                  uint16_t freq_read_h  = ((uint16_t)recv_buf[cmd_payload_idx + 6] << 8) | recv_buf[cmd_payload_idx + 7];
                  uint16_t freq_read_l  = ((uint16_t)recv_buf[cmd_payload_idx + 8] << 8) | recv_buf[cmd_payload_idx + 9];

                  switch (bus->getType())
                  {
                  default:
                    break;

                  case bus_type_t::bus_spi:
LHAL_LOG("bus_spi:");
                    {
                      auto tr_spi = reinterpret_cast<TransactionSPI*>(tr);
                      if (tr_spi == nullptr)
                      {
                        tr_spi = new TransactionSPI(bus);
                        tr = tr_spi;
                      }
                      tr_spi->freq_write = (uint32_t)freq_write_h << 16 | freq_write_l;
                      tr_spi->freq_read  = (uint32_t)freq_read_h << 16 | freq_read_l;
                      tr_spi->spi_mode = recv_buf[internal::cmd_payload_idx + 11];
                      tr_spi->read_by_mosi = recv_buf[internal::cmd_payload_idx + 12];
                    }
                    break;

                  case bus_type_t::bus_i2c:
LHAL_LOG("bus_i2c:");
                    {
                      auto tr_i2c = reinterpret_cast<TransactionI2C*>(tr);
                      if (tr == nullptr)
                      {
                        tr_i2c = new TransactionI2C(bus);
                        tr = tr_i2c;
                      }
                      tr_i2c->freq_write = (uint32_t)freq_write_h << 16 | freq_write_l;
                      tr_i2c->freq_read  = (uint32_t)freq_read_h << 16 | freq_read_l;
                      tr_i2c->timeout_msec = ((uint16_t)recv_buf[internal::cmd_payload_idx + 11] << 8)
                                           | (recv_buf[internal::cmd_payload_idx + 12]);
                    }
                    break;
                  }
                  if (tr != nullptr)
                  {
                    transaction_table[bus_num] = tr;
                    bus->beginTransaction(tr, read);
LHAL_LOG("bus->beginTransaction() exec\n");
                  }
                }
                break;

              case lhal::internal::command::bus_end_transaction:
                last_error_res = true;
                bus_num = bus_num_mask & recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  bus->endTransaction(transaction_table[bus_num]);
                }
                break;

              case lhal::internal::command::bus_dummy_clock:
                last_error_res = true;
                bus_num = bus_num_mask & recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  bus->dummyClock(recv_buf[cmd_payload_idx + 2]);
                }
                break;

              case lhal::internal::command::bus_write:
                last_error_res = true;
                bus_num = bus_num_mask & recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  bus->write( &recv_buf[cmd_payload_idx + 2], recv_idx - (cmd_suffix_len + cmd_payload_idx + 2) );
                }
                break;

              case lhal::internal::command::bus_write_repeat:
                last_error_res = true;
                bus_num = bus_num_mask & recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  uint16_t repeat_count_h = (recv_buf[cmd_payload_idx + 2] << 8) + recv_buf[cmd_payload_idx + 3];
                  uint16_t repeat_count_l = (recv_buf[cmd_payload_idx + 4] << 8) + recv_buf[cmd_payload_idx + 5];
                  uint8_t bytes = recv_buf[cmd_payload_idx + 6];

                  uint32_t data = recv_buf[cmd_payload_idx + 7];
                  if (bytes > 1)
                  {
                    data |= recv_buf[cmd_payload_idx + 8] << 8; 
                    if (bytes > 2)
                    {
                      data |= (uint32_t)recv_buf[cmd_payload_idx + 9] << 16;
                      if (bytes > 3)
                      {
                        data |= (uint32_t)recv_buf[cmd_payload_idx + 10] << 24;
                      }
                    }
                  }
                  bus->writeRepeat( data, bytes, ((uint32_t)repeat_count_h << 16) + repeat_count_l );
                }
                break;

              case lhal::internal::command::bus_transfer:
// LovyanHAL::GPIO_HAL::setMode(25, lhal::gpio::output_high);
// LovyanHAL::GPIO_HAL::writeHigh(25);
                last_error_res = true;
                bus_num = bus_num_mask & recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  uint_fast8_t len = recv_idx - (internal::cmd_suffix_len + internal::cmd_payload_idx + 2);
                  bus->transfer( &recv_buf[internal::cmd_payload_idx + 2], &recv_buf[internal::cmd_payload_idx + 1], len );
// for (int i = 1; i < len; ++i) {
//   recv_buf[internal::cmd_payload_idx + i] = i;
// }
                  res_len = internal::cmd_payload_idx + 1 + len + internal::cmd_suffix_len;
                }
// LovyanHAL::GPIO_HAL::writeLow(25);
                break;

              case lhal::internal::command::bus_read:
                last_error_res = true;
                bus_num = bus_num_mask & recv_buf[internal::cmd_payload_idx + 1];
                bus = bus_table[bus_num];
                if (bus)
                {
                  uint8_t len = recv_buf[internal::cmd_payload_idx + 2];
                  bool nack = recv_buf[internal::cmd_payload_idx + 3];
                  bus->read( &recv_buf[internal::cmd_payload_idx + 1], len, nack );
                  res_len = internal::cmd_payload_idx + 1 + len + internal::cmd_suffix_len;
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
}

 #endif
#endif