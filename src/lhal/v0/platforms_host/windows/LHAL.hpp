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

#include "init.hpp"
#include "../LovyanHAL_HOST.hpp"

#include <WinSock2.h>

#ifndef LHAL_DEFAULT_CONNECTION_NAME
#define LHAL_DEFAULT_CONNECTION_NAME nullptr
#endif

namespace lhal
{
 namespace v0
 {

  class LovyanHAL : public LovyanHAL_PC
  {
    class TransportCom : public internal::ITransportLayer
    {
      HANDLE _com = INVALID_HANDLE_VALUE;
      const char* _target;
    public:

      error_t init(const char* target);
      error_t connect(void) override;
      void disconnect(void) override;

      int read(void) override;
      int write(const uint8_t* data, size_t len) override;
    };

    class TransportSock : public internal::ITransportLayer
    {
      SOCKET _sock = 0;
    public:

      error_t init(const char* target);
      int read(void) override;
      int write(const uint8_t* data, size_t len) override;
    };

    char _target[64] = "";

    TransportCom _tl_com;
    TransportSock _tl_sock;

  public:

    LovyanHAL(const char* target = LHAL_DEFAULT_CONNECTION_NAME);
    LovyanHAL(internal::ITransportLayer* transport_layer);

    error_t init(void) override;
  };

 }
}
