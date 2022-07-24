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

#if LHAL_TARGET_PLATFORM_NUMBER == LHAL_PLATFORM_NUMBER_ARDUINO

#include "LHAL.hpp"

namespace lhal
{
  void LHAL::GPIO::setMode(pin_num_t pin, mode_t mode)
  {
    uint_fast8_t m = (mode & mode_t::output) ? OUTPUT : INPUT;
    if (m == INPUT)
    {
#if defined(INPUT_PULLUP)
      if (mode & mode_t::input_pullup)
      {
        m = INPUT_PULLUP;
      }
#endif
#if defined(INPUT_PULLDOWN)
      if (mode & mode_t::input_pulldown)
      {
        m = INPUT_PULLDOWN;
      }
#endif
    }
    else
    {
#if defined(OPEN_DRAIN)
      if (mode & mode_t::opendrain)
      {
        m |= OPEN_DRAIN;
      }
#endif
    }

    pinMode(pin, m);
  }
}

#endif
