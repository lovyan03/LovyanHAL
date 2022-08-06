/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_INIT_HPP_
#define LOVYANHAL_INIT_HPP_

#include "gitTagVersion.h"
#include "platform_check.hpp"


#if defined (LHAL_TARGET_PLATFORM)

  #if defined ( ARDUINO )
   #include <Arduino.h>
  #endif


  #define LHAL_LOCAL_INCLUDE(x) #x
  #define LHAL_CONCAT(x, y) LHAL_LOCAL_INCLUDE(x/y)

  #include LHAL_CONCAT(LHAL_TARGET_PLATFORM, init.hpp)
  #include "common.hpp"
  #include LHAL_CONCAT(LHAL_TARGET_PLATFORM, LHAL.hpp)

  #undef LHAL_CONCAT
  #undef LHAL_LOCAL_INCLUDE

  #include "IBus.hpp"

  namespace lhal
  {
    using namespace gpio;
  }

#else

 #warning "unsupported platform..."

#endif

#endif
