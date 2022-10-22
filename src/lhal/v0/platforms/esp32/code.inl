/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#if defined ( LHAL_TARGET_PLATFORM )
#include "../mcu_impl.inl"

#include <driver/rtc_io.h>
#include <soc/rtc.h>

namespace lhal
{
 namespace v0
 {

  LovyanHAL::GPIO_HAL LovyanHAL::Gpio;

  GPIO_host LovyanHAL::GPIO_HAL_Base::getHost(gpio_port_pin_t pin) { return GPIO_host { pin }; }

  volatile uint32_t* const LovyanHAL::GPIO_HAL::RAW::_write_reg[][2] =
  {
    { reinterpret_cast<uint32_t*>(GPIO_OUT_W1TC_REG), reinterpret_cast<uint32_t*>(GPIO_OUT_W1TS_REG) },
#if defined ( GPIO_OUT1_REG )
    { reinterpret_cast<uint32_t*>(GPIO_OUT1_W1TC_REG), reinterpret_cast<uint32_t*>(GPIO_OUT1_W1TS_REG) },
#endif
  };

  volatile uint32_t* const LovyanHAL::GPIO_HAL::RAW::_read_reg[] =
  {
    reinterpret_cast<uint32_t*>(GPIO_IN_REG),
#if defined ( GPIO_OUT1_REG )
    reinterpret_cast<uint32_t*>(GPIO_IN1_REG),
#endif
  };

  void LovyanHAL::GPIO_HAL::setMode(gpio_port_pin_t pin, mode_t mode)
  {
    if (pin >= GPIO_NUM_MAX) { return; }
    auto num = (gpio_num_t)pin;
    gpio_set_direction(num, GPIO_MODE_DISABLE);

#if SOC_RTCIO_INPUT_OUTPUT_SUPPORTED
    if (rtc_gpio_is_valid_gpio(num)) { rtc_gpio_deinit(num); }
#endif
    gpio_mode_t m = GPIO_MODE_INPUT;
    if (mode & mode_t::output)
    {
      m = (gpio_mode_t)(m | GPIO_MODE_OUTPUT);
      if ((mode & mode_t::output_opendrain) == mode_t::output_opendrain)
      {
        m = (gpio_mode_t)(m | GPIO_MODE_OUTPUT_OD);
      }
      // if ((mode & mode_t::output_high) == mode_t::output_high)
      // {
      //   writeHigh(pin);
      // }
      // else
      // if ((mode & mode_t::output_low ) == mode_t::output_low )
      // {
      //   writeLow(pin);
      // }
    }

    gpio_config_t io_conf;
    io_conf.mode = m;
    io_conf.pull_up_en   = (mode == mode_t::input_pullup  ) ? GPIO_PULLUP_ENABLE   : GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = (mode == mode_t::input_pulldown) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (uint64_t)1 << num;
    gpio_config(&io_conf);
    if (mode & mode_t::output)
    {
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
  }


  void delayMicroseconds(uint32_t usec)
  {
    uint64_t m = (uint64_t)esp_timer_get_time();
    while((uint64_t)esp_timer_get_time() - m < usec)
    {
      NOP();
    }
  }

 }
}

#endif