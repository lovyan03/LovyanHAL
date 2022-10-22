/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#include "../../platform_check.hpp"

#if defined (LHAL_TARGET_PLATFORM) && (LHAL_TARGET_PLATFORM_NUMBER == LHAL_PLATFORM_NUMBER_ARDUINO)

#include "../mcu_impl.inl"

#include "LHAL.hpp"

namespace lhal
{
 namespace v0
 {

  LovyanHAL::GPIO_HAL LovyanHAL::Gpio;

  GPIO_host LovyanHAL::GPIO_HAL_Base::getHost(gpio_port_pin_t pin) { return GPIO_host { pin }; }

  void LovyanHAL::GPIO_HAL::setMode(gpio_port_pin_t pin, mode_t mode)
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
      if (mode & mode_t::output_opendrain)
      {
        m |= OPEN_DRAIN;
      }
#endif
    }

    pinMode(pin, m);
  }

 }
}

#endif
