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
      printf("CreateFile failed with error %d.\n", GetLastError());
      return INVALID_HANDLE_VALUE;
    }

    int check = SetupComm(hCom, 1024, 1024);
    if (check == FALSE) {
      printf("SetupComm failed with error %d.\n", GetLastError());
      CloseHandle(hCom);
      return INVALID_HANDLE_VALUE;
    }

    DCB dcb;
    GetCommState(hCom, &dcb);
    dcb.DCBlength = sizeof(DCB);
    dcb.BaudRate = 115200;
    dcb.ByteSize = 8;
    dcb.fBinary = TRUE;
    dcb.fParity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
    dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    dcb.fDsrSensitivity = FALSE;

    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fTXContinueOnXoff = TRUE;
    dcb.XonLim = 512;
    dcb.XoffLim = 512;
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
      printf("SetCommState failed with error %d.\n", GetLastError());
      CloseHandle(hCom);
      return INVALID_HANDLE_VALUE;
    }

    COMMTIMEOUTS TimeOut;
    GetCommTimeouts(hCom, &TimeOut);

    TimeOut.ReadTotalTimeoutMultiplier = 0;
    TimeOut.ReadTotalTimeoutConstant = 128;
    TimeOut.WriteTotalTimeoutMultiplier = 0;
    TimeOut.WriteTotalTimeoutConstant = 128;

    check = SetCommTimeouts(hCom, &TimeOut);
    if (check == FALSE) {
      printf("SetCommTimeouts failed with error %d.\n", GetLastError());
      CloseHandle(hCom);
      return INVALID_HANDLE_VALUE;
    }

    check = PurgeComm(
      hCom,
      PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR
    );
    if (check == FALSE) {
      printf("PurgeComm failed with error %d.\n", GetLastError());
      CloseHandle(hCom);
      return INVALID_HANDLE_VALUE;
    }

    return hCom;
  }

  // --------------------------------------------------------------------------------

  error_t LHAL::TransportCom::init(const char* target)
  {
    strcpy_s(_target, sizeof(_target), target);
    _com = createComHandle(target);
    if (_com == INVALID_HANDLE_VALUE)
    {
      return error_t::err_failed;
    }

    return error_t::err_ok;
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
    _com = createComHandle(_target);
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
    return (_sock == 0) ? error_t::err_failed : error_t::err_ok;
  }

  int LHAL::TransportSock::write(const uint8_t* data, size_t len)
  {
    static size_t send_count = 0;
    static uint32_t prev_usec;
    std::this_thread::yield();
    uint32_t usec = LHAL::millis();
    uint32_t diff = usec - prev_usec;
    if (diff < 4)
    {
      send_count += len + 256;
      if (send_count > 1024)
      {
        send_count = 0;
        delay(1);
        std::this_thread::yield();
      }
    }
    else
    {
      send_count = len + 64;
    }
    prev_usec = usec;
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

  LHAL::LHAL(const char* target)
  {
    bool connect_mcu = false;;
    if (target == nullptr)
    {
      char filename[8];
      for (int i = 2; !connect_mcu && i < 99; ++i)
      {
        snprintf(filename, sizeof(filename), "COM%d", i);
        if (error_t::err_ok == _tl_com.init(filename))
        {
          connect_mcu = setTransportLayer(&_tl_com);
        }
      }
    }
    else
    {
      if (tolower(target[0]) == 'c' && tolower(target[1]) == 'o' && tolower(target[2]) == 'm' && target[3] >= '0' && target[3] <= '9')
      {
        if (error_t::err_ok != _tl_com.init(target))
        {
          printf("com open failed.");
        }
        else
        {
          connect_mcu = setTransportLayer(&_tl_com);
        }
      }
      else
      {
        if (error_t::err_ok != _tl_sock.init(target))
        {
          printf("socket open failed.");
        }
        else
        {
          connect_mcu = setTransportLayer(&_tl_sock);
        }
      }
    }
    if (connect_mcu)
    {
      init();
    }
  }
}

#endif
