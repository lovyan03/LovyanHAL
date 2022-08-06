  /*----------------------------------------------------------------------------/
  Lovyan HAL library - Hardware Abstraction Layer library .

Original Source:
 https://github.com/lovyan03/LovyanHAL/

Licence:
 [BSD](https://github.com/lovyan03/LovyanHAL/blob/master/license.txt)

Author:
 [lovyan03](https://twitter.com/lovyan03)
/----------------------------------------------------------------------------*/
#ifndef LOVYANHAL_IBUS_HPP_
#define LOVYANHAL_IBUS_HPP_

#include "init.hpp"

namespace lhal
{
  class LovyanHAL;


  /// 通信バスクラスのインターフェイス
  class IBus
  {
  protected:
#if ( LHAL_TARGET_PLATFORM_NUMBER < LHAL_PLATFORM_NUMBER_PC_MAX )
    // PCで動作させる場合はLHALインスタンスが複数あるため、各バスは紐づけられたLHALのポインタを保持しておく;
    LovyanHAL* _lhal;

    // LovyanHALインスタンスで管理されるバス通し番号 (init時、未設定の場合に附番される)
    uint16_t _bus_number = (uint16_t)~0u;

  public:
    inline LovyanHAL* getHal(void) { return _lhal; }
#else
    // MCUで動作させる場合はLHALインスタンスは単一であるため、LHALのポインタはstaticでよい;
    static LovyanHAL* _lhal;

  public:
    inline LovyanHAL* getHal(void) { return nullptr; }
#endif

  protected:

    // 通信相手の情報、beginTransaction時に渡されるオブジェクト;
    ITransaction* _transaction;

    // 最後に生じたエラー情報(beginTransaction時にクリアされる);
    error_t _last_error;

  public:
    IBus(LovyanHAL* hal = nullptr);

    inline error_t getLastError(void) const { return _last_error; }
    inline bool isError(void) const { return checkError(_last_error); }
    inline bool isSuccess(void) const { return checkSuccess(_last_error); }

    virtual void csControl(bool value, bool read = false)
    {
      waitBusy();
      _transaction->csControl(this, value, read);
    }

    virtual void dcControl(bool value)
    {
      waitBusy();
      _transaction->dcControl(this, value);
    }

    // バスの種類情報を取得する;
    virtual bus_type_t getType(void) const = 0;

    // バスの幅(クロック1サイクルあたりのビット数)を取得する;
    // シリアルバスの場合は1、パラレルバスの場合はバスのビット数を返す;
    virtual uint8_t getBusWidth(void) const { return 1; }

    // バスの設定パラメータの指定を行う;
    // この時点では設定値の準備に留め、バスの準備操作は行わない;
    // バスの準備はinit関数で行う;
    virtual error_t setConfig(const IConfigBus* cfg)
    {
      return error_t::err_not_implement;
    }

    // バスの準備を行う;
    // 事前に setConfig が必要;
    // initを複数回呼び出した場合は、init済みであっても再度準備動作を実行する;
    // GPIOの設定が他の処理に奪われていた場合でも、再設定が行われる;
    virtual error_t init(void);

    // 通信を開始する;
    virtual void beginTransaction(ITransaction* transaction, bool read = false, int dummy_clock_bits = 0)
    {
      _transaction = transaction;
      _last_error = err_ok;
      if (read)
      {
        beginRead(dummy_clock_bits);
      }
      else
      {
        csControl(false);
      }
    }

    // 通信を終了する;
    virtual void endTransaction(void)
    {
      csControl(true);
    }

    // 通信を一度終了して再び開始する;
    virtual void rebeginTransaction(bool read = false, int dummy_clock_bits = 0)
    {
      endTransaction();
      beginTransaction(_transaction, read, dummy_clock_bits);
    }

    // 通信 受信を開始する;
    // dummy_clock_bitsを使ってダミークロックを送信できる;
    virtual void beginRead(int dummy_clock_bits = 0)
    {
      csControl(false, true);
    }

    virtual void endRead(void)
    {
      csControl(true);
      csControl(false);
    }

    // flush buffer data.
    virtual void flush(void) {};

    // バスのビジーチェック。true = 通信中 / false = 無通信;
    virtual bool isBusy(void) { return false; }

    // 通信の終了まで待機する
    virtual void waitBusy(size_t timeout_msec = ~0u) { flush(); while (isBusy()) {}; }

    // write byte array.
    // 16bitパラレルバスの場合、上位・下位の順で2Byteをセットし一度に送信する。なおlenが奇数の場合、端数分は次回の送信時やflush時に送信される;
    // 例:データソースが以下の場合
    //  data[0] = 0xDE
    //  data[1] = 0xAD
    //  data[2] = 0xBE
    //  data[3] = 0xEF
    // 次のように送信される
    //   8bit: 1st:DE / 2nd:AD / 3rd:BE / 4th:EF
    //  16bit: 1st:DEAD / 2nd:BEEF
    virtual void write(const uint8_t* data, size_t len) { _last_error = err_not_implement; }

    // write 8bit data.
    // 16bitパラレルバスの場合、上位8bitを0埋めして下位8bitのみを送信する;
    // 例:データソースが以下の場合
    //  data = 0xDE
    // 次のように送信される
    //   8bit: 1st:DE
    //  16bit: 1st:00DE
    virtual void write8(uint8_t data) { write(&data, 1); }

    // write 16bit data.
    // 16bitで通信するバスの場合、1回で16bitぶん送信する;
    // 8bitパラレルバスの場合、上位・下位の順で2回に分けて送信する;
    // 例:データソースが以下の場合
    //  data = 0xDEAD (リトルエンディアンのためメモリ上の表現は [0]=DE [1]=AD)
    // 次のように送信される
    //   8bit: 1st:DE / 2nd:AD
    //  16bit: 1st:DEAD
    virtual void write16(uint16_t data) { data = (uint16_t)((data << 8) | (data >> 8)); write((const uint8_t*)&data, 2); }

    // データの代わりに関数ポインタを受け取り、関数に生成させたデータを送信する;
    // データ生成関数の型は以下の通り
    //  引数1 void*: writeWithFunctorの第2引数のポインタがそのまま渡される。ユーザー定義オブジェクトに使用する
    //  引数2 uint8_t*: 生成されたデータを受ける配列のポインタ
    //  引数3 uint32_t: 要求するデータの最大長
    //  戻り値 uint32_t:生成されたデータ長
    virtual uint32_t writeWithGenerator(uint32_t (*fp)(void*, uint8_t*, uint32_t), void* obj, uint32_t length)
    {
      uint8_t data[16];
      uint32_t result = 0;
      do
      {
        uint32_t l = length < 16 ? length : 16;
        if (0 == (l = fp(obj, data, 16)))
        {
          break;
        }
        write(data, l);
        result += l;
        length -= l;
      } while (length);
      return result;
    }


    // 送受信 (全二重通信で送受信が同時に行われる場合に使う。実質 SPIバス専用)
    // なおwriteとreadに同じポインタを指定してもよい。
    virtual void transmission(const uint8_t* write_data, uint8_t* read_data, size_t len) { _last_error = err_not_implement; }


    // write 16bit data array.
    // lenはデータサイズではなく配列の要素数である点に注意;
    virtual void write16(const uint16_t* data, size_t len) { size_t i = 0; do { write16(data[i]); } while (++i != len); };

    // write max 4Byte little endian.
    virtual void write(uint32_t data, uint8_t bytes = 1) { write((const uint8_t*)&data, bytes); };

    // write max 4Byte little endian repeat.
    virtual void writeRepeat(uint32_t data, uint8_t bytes, size_t repeat_count) { do { write(data, bytes); } while (--repeat_count); };


    // endReadがtrueの場合は読み終えた時点で通信を終了する;
    virtual void read(uint8_t* read_data, size_t len, bool nack = false) {};


//------- 以下は D/C制御が必要なデバイス用

    // D/C control low(command) and write 1 byte.
    virtual void writeCommand8(uint8_t data) { dcControl(false); write8(data); };

    // D/C control low(command) and write 2 byte.
    virtual void writeCommand16(uint16_t data) { dcControl(false); write16(data); };

    // D/C control low(command) and write data array.
    virtual void writeCommand(const uint8_t* data, size_t len) { dcControl(false); write(data, len); };

    // write data (max 4Byte little endian) repeat.
    virtual void writeCommandRepeat(uint32_t data, uint8_t bytes, size_t repeat_count) { dcControl(false); writeRepeat(data, bytes, repeat_count); };

    // D/C control high(data) and write 1 byte.
    virtual void writeData8(uint8_t data) { dcControl(true); write8(data); };

    // D/C control high(data) and write 2 byte.
    virtual void writeData16(uint16_t data) { dcControl(true); write16(data); };

    // D/C control high(data) and write data array.
    virtual void writeData(const uint8_t* data, size_t len) { dcControl(true); write(data, len); };

    // write data (max 4Byte little endian).
    virtual void writeData(uint32_t data, uint8_t bytes) { dcControl(true); write((const uint8_t*)&data, bytes); };

    // write data (max 4Byte little endian) repeat.
    virtual void writeDataRepeat(uint32_t data, uint8_t bytes, size_t repeat_count) { dcControl(true); writeRepeat(data, bytes, repeat_count); };
  };
}

#endif
