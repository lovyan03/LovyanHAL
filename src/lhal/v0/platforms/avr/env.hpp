/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#pragma once

#include <stdint.h>

namespace lhal
{
 namespace v0
 {

  namespace gpio
  {
    typedef uint8_t port_num_t;   // ポート番号用の型;
    typedef uint8_t pin_num_t;    // ポート内ピン番号の型;
    typedef uint8_t pin_mask_t;   // 同一ポート内でのピンビットマスク値;
    static constexpr const uint8_t port_shift = 3; // 3bit = 0-7
  }

  namespace internal
  {
    static constexpr const uint8_t nop_delay_cycle_x2 = 14;
  }

 }
}
