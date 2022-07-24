/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#include "for_mcu.hpp"

namespace lhal
{
  namespace internal
  {
    static void send_response(ITransportLayer* st, uint8_t* data, uint8_t length)
    {
      uint8_t checksum = lhal::internal::control_code::etx;
      for (int i = 0; i < length - 2; ++i)
      {
        checksum ^= data[i];
      }
      data[length - 2] = checksum;
      st->write(data, length);
    }

    void perform_use_with_host(LHAL* hal, ITransportLayer* st)
    {
      static uint8_t recv_buf[256];
      static uint_fast8_t recv_idx = 0;
      static uint_fast8_t sendlen = 0;
      static uint_fast8_t checksum = 0;

      int val;
      while ((val = st->read()) >= 0)
      {
        recv_buf[recv_idx] = val;
        checksum ^= val;
        if (++recv_idx > 2)
        {
          if (--sendlen == 0)
          {
            /// ETX + CheckSum ?
            if (recv_buf[recv_idx - 1] == lhal::internal::control_code::etx && checksum == 0)
            {
              switch (recv_buf[2])
              {
              case lhal::internal::command::check_lhal_device:
                send_response(st, recv_buf, recv_idx);
                break;

              case lhal::internal::command::convert_arduino_pin_num:
                for (uint_fast8_t i = 3, i_end = recv_idx - 2; i < i_end; ++i )
                {
                  recv_buf[i] = hal->convertArduinoPinNumber( recv_buf[i] );
                }
                send_response(st, recv_buf, recv_idx);
                break;

              case lhal::internal::command::gpio_set_mode:
                for (uint_fast8_t i = 3, i_end = recv_idx - 2; i < i_end; i += 2 )
                {
                  hal->Gpio.setMode( recv_buf[i] , (LHAL::GPIO::mode_t)recv_buf[i + 1] );
                }
                break;

              case lhal::internal::command::gpio_read:
                for (uint_fast8_t i = 3, i_end = recv_idx - 2; i < i_end; ++i )
                {
                  recv_buf[i] = hal->Gpio.read( recv_buf[i] );
                }
                send_response(st, recv_buf, recv_idx);
                break;

              case lhal::internal::command::gpio_write_high:
                for (uint_fast8_t i = 3, i_end = recv_idx - 2; i < i_end; ++i )
                {
                  hal->Gpio.writeHigh( recv_buf[i] );
                }
                break;

              case lhal::internal::command::gpio_write_low:
                for (uint_fast8_t i = 3, i_end = recv_idx - 2; i < i_end; ++i )
                {
                  hal->Gpio.writeLow( recv_buf[i] );
                }
                break;

              default:
                break;
              }
            }
            recv_idx = 0;
            checksum = 0;
          }
        }
        else
        if (recv_idx == 1)
        {
          if (val != lhal::internal::control_code::stx)
          {  // STX?
            recv_idx = 0;
            checksum = 0;
          }
        }
        else if (recv_idx == 2)
        {
          sendlen = val;
        }
      }
    }
  }
}
