/*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_USE_WITH_PC_COMMON_HPP_
#define LOVYANHAL_USE_WITH_PC_COMMON_HPP_

#include <stdint.h>
#include <stddef.h>
#include "../init.hpp"

namespace lhal
{
  namespace internal
  {
    // ホストとの通信に用いるインターフェイス;
    class ITransportLayer
    {
    public:
      virtual ~ITransportLayer(void) {}

      /// disconnect.
      virtual void disconnect(void) {}

      /// connect (or reconnect).
      virtual error_t connect(void) { return error_t::err_failed; }

      /// read 1byte.
      /// @return data or -1(read failed)
      virtual int read(void) { return -1; }

      /// write data.
      /// @param data write data.
      /// @param len write length.
      virtual int write(const uint8_t* data, size_t len) { return 0; }

      /// write 1byte.
      /// @param data write data.
      virtual int write(uint8_t data) { return write(&data, 1); }
    };

    // ホストとの接続にTCP通信を用いる場合のポート番号;
    static constexpr size_t default_tcp_port = 63243;
    static constexpr uint32_t default_serial_baudrate = 115200;


    static constexpr size_t cmd_stx_idx = 0;     // _sendbuf内のSTXヘッダの位置 ;
    static constexpr size_t cmd_seqnum_idx = 1;  // _sendbuf内のシーケンス番号位置 ;
    static constexpr size_t cmd_datalen_idx = 2; // _sendbuf内のデータサイズ位置 ;
    static constexpr size_t cmd_payload_idx = 3; // _sendbuf内のデータ部位置 ;
    static constexpr size_t cmd_prefix_len = 3;  // _sendbuf先頭のプリフィクスサイズ 3Byte (STX,シーケンス番号,データ長) ;
    static constexpr size_t cmd_suffix_len = 2;  // _sendbuf末尾のサフィックスサイズ 2Byte (チェックサム,ETX) ;
    static constexpr size_t cmd_payload_maxlen = 256 - (cmd_prefix_len + cmd_suffix_len);  // データ部の最大長

    enum control_code : uint8_t
    {
      stx = 0x02,
      etx = 0x03,
    };

    enum command : uint8_t
    {
      check_lhal_device = 0x00,
      convert_arduino_pin_num = 0x01,
      gpio_set_mode = 0x10,
      gpio_read = 0x11,
      gpio_read_port = 0x11,
      gpio_write_low = 0x14,
      gpio_write_high = 0x15,
    };
  }
}

#endif
