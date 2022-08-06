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
  namespace gpio
  {
    /*
      port_num_t  : ポート番号用の型;
      pin_num_t   : 各ポートにおけるピン番号の型 (通し番号ではない);
      pin_mask_t  : ピン番号のビット表現。 1 << pin_num の範囲を表現できる型;
      port_shift  : port_num表現に必要なビット数。

      設定例 : 1ポートに最大32本のピンがあり、ポートがAからDの4つある場合;
        port_num_t  0~3 の範囲を表現できる型 (PortA = 0, PortB = 1, PortC = 2, PortD = 3)
        pin_num_t   0~31 の範囲を表現できる型 (各ポートにおけるピンの番号)
        pin_mask_t  1 << 31 を表現できる型;
        port_shift  0~31を表現可能なビット数は5bitであるため、port_shift = 5
    */
    typedef uint8_t port_num_t;   // ポート番号用の型;
    typedef uint8_t pin_num_t;    // ポート内ピン番号の型;
    typedef uint32_t pin_mask_t;    // 同一ポート内でのピンビットマスク値;
    static constexpr const uint8_t port_shift = 5; // 5bit = 0~31
  }
}
