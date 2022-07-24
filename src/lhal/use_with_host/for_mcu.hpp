/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_USE_WITH_HOST_FOR_MCU_HPP_
#define LOVYANHAL_USE_WITH_HOST_FOR_MCU_HPP_

#include "../init.hpp"
#include "common.hpp"

namespace lhal
{
  namespace internal
  {
    void perform_use_with_host(LHAL* hal, ITransportLayer* st);
  }
}

#endif
