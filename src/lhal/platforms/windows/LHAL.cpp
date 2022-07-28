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

#if LHAL_TARGET_PLATFORM_NUMBER == LHAL_PLATFORM_NUMBER_WINDOWS

#include "LHAL.hpp"

#pragma comment(lib, "Ws2_32.lib")

#include <stdio.h>

namespace lhal
{
  static SOCKET createSockHandle(const char* hostname)
  {
    static constexpr size_t URL_MAXLENGTH = 2083;
    static WSADATA wsaData;
    static bool _wsa_startup = false;
    if (_wsa_startup == false)
    {
      _wsa_startup = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
      if (_wsa_startup == false) { return 0; }
    }
    if (strlen(hostname) > URL_MAXLENGTH) { return 0; }

    hostent* Host = gethostbyname(hostname);
    SOCKADDR_IN SockAddr;
    SockAddr.sin_port = htons(lhal::internal::default_tcp_port);
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_addr.s_addr = *((unsigned long*)Host->h_addr);

    auto result_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int flag = -1;

    if (SOCKET_ERROR == setsockopt(result_socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&flag), 1))
    {
      printf("setsockopt failed.");
      closesocket(result_socket);
      return 0;
    }

    auto is_connected = (connect(result_socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) == 0);
    if (is_connected)
    {
      return result_socket;
    }
    closesocket(result_socket);
    return 0;
  }

  // --------------------------------------------------------------------------------

  static HANDLE createComHandle(const char* name)
  {
    char filename[16];
    snprintf(filename, sizeof(filename), "\\\\.\\%s", name);
    HANDLE hCom = CreateFile(filename,
      GENERIC_READ | GENERIC_WRITE,
      0,
      NULL,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      NULL
    );

    if (hCom == INVALID_HANDLE_VALUE)
    { // Handle the error. 
      //printf("%s : CreateFile failed with error %d.\n", name, GetLastError());
      return INVALID_HANDLE_VALUE;
    }

    int check = SetupComm(hCom, 4096, 256);
    if (check == FALSE) {
      printf("%s : SetupComm failed with error %d.\n", name, GetLastError());
      CloseHandle(hCom);
      return INVALID_HANDLE_VALUE;
    }

    DCB dcb;
    GetCommState(hCom, &dcb);
    // 以下の設定は変更するとボードによっては通信不能になるので、環境から取得した値のままでよい。変更しないこと。;
    // dcb.fOutxCtsFlow ;
    // dcb.fOutxDsrFlow ;
    // dcb.fDtrControl ;
    // dcb.fRtsControl ;
    // dcb.fDsrSensitivity ;

    dcb.DCBlength = sizeof(DCB);
    dcb.BaudRate = lhal::internal::default_serial_baudrate;
    dcb.ByteSize = 8;
    dcb.fBinary = TRUE;
    dcb.fParity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;

    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fTXContinueOnXoff = TRUE;
    dcb.XonLim = 128;
    dcb.XoffLim = 128;
    dcb.XonChar = 0x11;
    dcb.XoffChar = 0x13;

    dcb.fNull = FALSE;
    dcb.fAbortOnError = TRUE;
    dcb.fErrorChar = FALSE;
    dcb.ErrorChar = 0x00;
    dcb.EofChar = 0x03;
    dcb.EvtChar = 0x02;

    check = SetCommState(hCom, &dcb);
    if (check == FALSE) {
      printf("%s : SetCommState failed with error %d.\n", name, GetLastError());
      CloseHandle(hCom);
      return INVALID_HANDLE_VALUE;
    }

    COMMTIMEOUTS TimeOut;
    GetCommTimeouts(hCom, &TimeOut);

    TimeOut.ReadTotalTimeoutMultiplier = 0;
    TimeOut.ReadTotalTimeoutConstant = 0;
    TimeOut.WriteTotalTimeoutMultiplier = 1;
    TimeOut.WriteTotalTimeoutConstant = 256;
    TimeOut.ReadIntervalTimeout = MAXDWORD;

    check = SetCommTimeouts(hCom, &TimeOut);
    if (check == FALSE) {
      printf("%s : SetCommTimeouts failed with error %d.\n", name, GetLastError());
      CloseHandle(hCom);
      return INVALID_HANDLE_VALUE;
    }

    check = PurgeComm(
      hCom,
      PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR
    );
    if (check == FALSE) {
      printf("%s : PurgeComm failed with error %d.\n", name, GetLastError());
      CloseHandle(hCom);
      return INVALID_HANDLE_VALUE;
    }

    return hCom;
  }

  // --------------------------------------------------------------------------------

  error_t LHAL::TransportCom::init(const char* target)
  {
    if (target == nullptr || target[0] == 0)
    {
      _target = nullptr;
      disconnect();
      return error_t::err_failed;
    }

    _com = createComHandle(target);
    if (_com != INVALID_HANDLE_VALUE)
    {
      _target = target;
      printf("%s : Open succeed. \n", target);
      return error_t::err_ok;
    }
    return error_t::err_failed;
  }

  int LHAL::TransportCom::write(const uint8_t* data, size_t len)
  {
    DWORD writelen = 0;
    WriteFile(_com, data, len, &writelen, nullptr);
    return writelen;
  }

  void LHAL::TransportCom::disconnect(void)
  {
    if (_com != INVALID_HANDLE_VALUE)
    {
      CloseHandle(_com);
      _com = INVALID_HANDLE_VALUE;
    }
  }

  error_t LHAL::TransportCom::connect(void)
  {
    disconnect();
    if (_target != nullptr && _target[0] != 0)
    {
      _com = createComHandle(_target);
    }
    return (_com == INVALID_HANDLE_VALUE) ? error_t::err_failed : error_t::err_ok;
  }

  int LHAL::TransportCom::read(void)
  {
    DWORD len = 0;
    uint8_t buf[4];
    if (ReadFile(_com, buf, 1, &len, nullptr) && len)
    {
      return buf[0];
    }
    return -1;
  }

// --------------------------------------------------------------------------------

  error_t LHAL::TransportSock::init(const char* target)
  {
    _sock = createSockHandle(target);

    if (_sock != 0)
    {
      printf("%s : Connected. \n", target);
      return error_t::err_ok;
    }
    return error_t::err_failed;
  }

  int LHAL::TransportSock::write(const uint8_t* data, size_t len)
  {
    auto res = send(_sock, (const char*)data, len, 0);
    if (res != len)
    {
      res = 0;
    }
    return res;
  }

  int LHAL::TransportSock::read(void)
  {
    uint8_t buf[4];
    int len = recv(_sock, (char*)buf, 1, 0);
    return len > 0 ? buf[0] : -1;
  }

// --------------------------------------------------------------------------------

  LHAL::LHAL(internal::ITransportLayer* transport_layer)
    : LovyanHAL { transport_layer }
  {
  }

  LHAL::LHAL(const char* target)
   : LovyanHAL {}
  {
    if (target)
    {
      strcpy_s(_target, sizeof(_target), target);
    }
    else
    {
      _target[0] = 0;
    }
  }

  error_t LHAL::init(void)
  {
    if (_transport == nullptr)
    {
      error_t result = error_t::err_failed;
      if (_target[0] == 0)
      {
        for (int i = 2; (error_t::err_ok != result) && i < 100; ++i)
        {
          snprintf(_target, sizeof(_target), "COM%d", i);
          if (error_t::err_ok == _tl_com.init(_target))
          {
            result = setTransportLayer(&_tl_com);
            if (result != error_t::err_ok)
            {
              printf("%s : Response nothing...\n", _target);
              _target[0] = 0;
              _tl_com.init(nullptr);
            }
          }
        }
      }
      else
      {
        if (tolower(_target[0]) == 'c' && tolower(_target[1]) == 'o' && tolower(_target[2]) == 'm' && _target[3] >= '0' && _target[3] <= '9')
        {
          if (error_t::err_ok == _tl_com.init(_target))
          {
            result = setTransportLayer(&_tl_com);
          }
          else
          {
            printf("com open failed.\n");
          }
        }
        else
        {
          if (error_t::err_ok != _tl_sock.init(_target))
          {
            printf("socket open failed.\n");
          }
          else
          {
            result = setTransportLayer(&_tl_sock);
          }
        }
      }
      if (result != error_t::err_ok)
      {
        printf("connection not found.\n");
        return result;
      }
    }
    printf("connect !\n");
    return LovyanHAL::init();
  }
}

#endif
