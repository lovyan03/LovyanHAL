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

#if LHAL_TARGET_PLATFORM_NUMBER == LHAL_PLATFORM_NUMBER_ESP32

#include "LHAL.hpp"

 #if __has_include (<sdkconfig.h>)
  #include <sdkconfig.h>
 #endif

 #include <driver/rtc_io.h>
 #include <soc/rtc.h>

namespace lhal
{
  LHAL::GPIO_t LHAL::Gpio;

  GPIO_host LHAL::GPIO_Base::getHost(gpio::gpio_pin_t pin) { return GPIO_host { pin }; }

  void LHAL::GPIO_t::setMode(gpio::gpio_pin_t pin, mode_t mode)
  {
    if (pin >= GPIO_NUM_MAX) { return; }

    gpio_set_direction((::gpio_num_t)pin, GPIO_MODE_DISABLE);

#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
    if (rtc_gpio_is_valid_gpio((gpio_num_t)pin)) { rtc_gpio_deinit((gpio_num_t)pin); }
#endif
    gpio_mode_t m = GPIO_MODE_INPUT;
    if (mode & mode_t::output)
    {
      m = (gpio_mode_t)(m | GPIO_MODE_OUTPUT);
      if ((mode & mode_t::output_opendrain) == mode_t::output_opendrain)
      {
        m = (gpio_mode_t)(m | GPIO_MODE_OUTPUT_OD);
      }

      if ((mode & mode_t::output_high) == mode_t::output_high)
      {
        writeHigh(pin);
      }
      else
      if ((mode & mode_t::output_low ) == mode_t::output_low )
      {
        writeLow(pin);
      }
    }

    gpio_config_t io_conf;
    io_conf.mode = m;
    io_conf.pull_up_en   = (mode == mode_t::input_pullup  ) ? GPIO_PULLUP_ENABLE   : GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = (mode == mode_t::input_pulldown) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (uint64_t)1 << pin;
    gpio_config(&io_conf);
  }
}

#endif
