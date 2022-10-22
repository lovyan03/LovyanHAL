/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#include "init.hpp"

#if !defined (LHAL_TARGET_PLATFORM)

#warning "unsupported platform..."

#else

#define LHAL_LOCAL_INCLUDE(x) #x
#define LHAL_CONCAT(x, y) LHAL_LOCAL_INCLUDE(x/y)

#include LHAL_CONCAT(LHAL_TARGET_PLATFORM, code.inl)

#undef LHAL_CONCAT
#undef LHAL_LOCAL_INCLUDE

#endif
