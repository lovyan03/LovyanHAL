/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_COMMON_HPP_
#define LOVYANHAL_COMMON_HPP_

#include "platforms/init.hpp"

namespace lhal
{
  enum error_t
  {
    err_ok = 0,
    err_failed,
  };

  class IBus
  {
  public:
    /// peripheral setup.
    virtual error_t begin(void);

    /// peripheral release.
    virtual error_t end(void);

    /// write data.
    virtual error_t write(const uint8_t*, size_t length) = 0;

    /// read data.
    virtual error_t read(uint8_t*, size_t length) = 0;
  };
}

#endif
