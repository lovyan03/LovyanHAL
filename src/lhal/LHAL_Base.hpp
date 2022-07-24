/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_LHAL_BASE_HPP_
#define LOVYANHAL_LHAL_BASE_HPP_

#include "init.hpp"

namespace lhal
{
  class LHAL_Base
  {
  /// ここには環境依存のない共通のコードを実装していく。;
  public:
    class GPIO_Base
    {
    public:
      enum mode_t : uint8_t
      {
        //                   +-----pull_down ( or output low  )
        //                   |+----pull_up   ( or output high )
        //                   ||+---open_drain
        //                   |||+--output
        //                   ||||
        input            = 0b0000,
        output           = 0b0001,
  //    opendrain        = 0b0010,
        output_opendrain = 0b0011,
        input_pullup     = 0b0100,
        output_high      = 0b0101,
        input_pulldown   = 0b1000,
        output_low       = 0b1001,
      };
      using gpio_pin_t = gpio::gpio_pin_t;
      using port_num_t = gpio::port_num_t;
      using pin_num_t = gpio::pin_num_t;
      using pin_mask_t = gpio::pin_mask_t;
      static constexpr uint8_t port_shift = gpio::port_shift;

      static constexpr port_num_t getPortNum(gpio_pin_t port_pin_number) { return port_pin_number >> port_shift; }
      static constexpr pin_mask_t getPinMask(gpio_pin_t port_pin_number) { return 1 << (port_pin_number & ((1 << port_shift) - 1)); }
    };

    LHAL_Base(void) {}
    virtual ~LHAL_Base(void) {}
  };
}

#endif