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

#include "../init.hpp"

namespace lhal
{
  namespace gpio
  {
    typedef uint8_t gpio_pin_t;   // ポート番号+ピン番号の型;
    typedef uint8_t port_num_t;   // ポート番号用の型;
    typedef uint8_t pin_num_t;    // ポート内ピン番号の型;
    typedef uint32_t pin_mask_t;  // 同一ポート内でのピンビットマスク値;
    static constexpr const uint8_t port_shift = 5; // 5bit = 0-31
  }
}
