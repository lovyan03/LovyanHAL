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
 namespace v0
 {

  class LovyanHAL;
  class ITransaction;

  /// 通信バスクラスのインターフェイス
  class IBus
  {
  protected:
    // beginTransaction時にtrueに設定し、endTransaction時にfalseに設定する;
    // bool _in_transaction = false;
#if ( LHAL_TARGET_PLATFORM_NUMBER < LHAL_PLATFORM_NUMBER_HOST_MAX )
    // PCで動作させる場合はLHALインスタンスが複数あるため、各バスは紐づけられたLHALのポインタを保持しておく;
    LovyanHAL* _lhal;

    // LovyanHALインスタンスで管理されるバス通し番号 (init時、未設定の場合に附番される)
    uint8_t _bus_number = (uint8_t)~0u;

  public:
    inline LovyanHAL* getHal(void) { return _lhal; }
#else
    // MCUで動作させる場合はLHALインスタンスは単一であるため、LHALのポインタはstaticでよい;
    static LovyanHAL* _lhal;

  public:
    static inline LovyanHAL* getHal(void) { return _lhal; }
#endif

    // 通信相手の情報。beginTransaction時に渡され、endTransaction時にnullptrに設定する;
    ITransaction* _transaction = nullptr;

    // 最後に生じたエラー情報(beginTransaction時にクリアされる);
    error_t _last_error;

    // 通信開始処理 前半部の実装 (通信準備);
    // 各種ペリフェラルの設定、ピンの初期状態の設定。
    // 通信クロックに関する設定はここでは行わない;
    virtual void beginTransaction_1st_impl(void) {}

    // 通信開始処理 後半部の実装 (パラメータ設定は1stで処理済み)
    // 通信クロックに関する設定をここで行う;
    // I2Cスタートコンディション送信、バス主導権が取れない可能性があるので戻り値で成否を返す;
    // CSアサートはここでは行わない;
    virtual error_t beginTransaction_2nd_impl(bool read)
    {
      return error_t::err_ok;
    }

    // 通信終了処理;
    // 各種ペリフェラルの状態復元、I2Cストップコンディション送信;
    // CSデアサートはここでは行わない;
    virtual error_t endTransaction_impl(void)
    {
      return _last_error;
    }

  public:
    IBus(LovyanHAL* hal = nullptr);
    virtual ~IBus(void) {};

    virtual error_t getLastError(void) { return _last_error; }
    inline bool isError(void) const { return checkError(_last_error); }
    inline bool isSuccess(void) const { return checkSuccess(_last_error); }
/*
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
//*/
    // バスの種類情報を取得する;
    virtual bus_type_t getType(void) const = 0;

    // バスの幅(クロック1サイクルあたりのビット数)を取得する;
    // シリアルバスの場合は1、パラレルバスの場合はバスのビット数 8 や 16 を返す;
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

    virtual error_t release(void) { return err_ok; };

    // 通信を開始する;
    error_t beginTransaction(ITransaction* transaction, bool read = false)
    {
      if (_transaction != transaction)
      {
        if (_transaction != nullptr)
        { // ToDo:ここでロック待機処理を行う?;
          return err_failed;
        }
        _last_error = error_t::err_ok;
        _transaction = transaction;
        beginTransaction_1st_impl();
      }
      return beginTransaction_2nd_impl(read);
    }

    // 通信を終了する;
    error_t endTransaction(ITransaction* transaction)
    {
      if (_transaction != transaction)
      {
        return error_t::err_ok;
      }
      endTransaction_impl();
      _transaction = nullptr;
      return _last_error;
    }

    // ダミークロックを出力する。(1bit単位の半端なクロック数を設定可能)
    //                  (SPI/パラレルバスの場合に実装。SPIではCPOLの極性反転によって実装);
    //                  (ダミークロックはI2CやUARTでは実装しない);
    virtual void dummyClock(uint8_t dummy_clock_bits) {}


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


    // write max 4Byte little endian.
    virtual void write(uint32_t data, uint8_t bytes = 1) { write((const uint8_t*)&data, bytes); };


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
    virtual void write16(uint16_t data) { data = bswap16(data); write((const uint8_t*)&data, 2); }


    // 送受信 (全二重通信で送受信が同時に行われる場合に使う。実質 SPIバス専用)
    // なおwriteとreadに同じポインタを指定してもよい。
    virtual void transfer(const uint8_t* write_data, uint8_t* read_data, size_t len) { _last_error = err_not_implement; }


    // write 16bit data array.
    // lenはデータサイズではなく配列の要素数である点に注意;
    virtual void write16(const uint16_t* data, size_t len) { size_t i = 0; do { write16(data[i]); } while (++i != len); };


    // write 32bit data.
    virtual void write32(uint32_t data) { data = bswap32(data); write((const uint8_t*)&data, 4); };


    // write max 4Byte little endian repeat.
    virtual void writeRepeat(uint32_t data, uint8_t bytes, size_t repeat_count) { do { write((const uint8_t*)&data, bytes); } while (--repeat_count); };


    // I2Cバスで nack がtrueの場合は最後のデータを読み終えた際にNACK応答を返す;
    virtual void read(uint8_t* read_data, size_t len, bool nack = false) {};


    // I2Cバスで nack がtrueの場合は読み終えた際にNACK応答を返す;
    virtual uint8_t read8(bool nack = false) { uint8_t res; read(&res, 1, nack); return res; };
  };



  /// 通信相手に関するパラメータ;
  /// CS、D/C、I2Cアドレスの制御などもここで行う;
  class ITransaction
  {
  protected:
    IBus* _bus;

    virtual void init_impl(void) {};

    /// 通信相手を指定する処理
    /// SPI通信の場合、CSをLOW(assert)に制御する;
    /// I2C通信の場合、通信相手のアドレス((i2c_addr << 1) + read)を送信する;
    virtual void setCsAssert(bool read) {};

    /// 通信相手の指定を解除する処理
    /// SPI通信の場合、CSをHIGH(deassert)に制御する;
    /// I2C通信の場合、通信相手のアドレス((i2c_addr << 1) + read)を送信する;
    virtual void setCsDeassert(void) {};

    virtual void setDcCommand(void) {};

    virtual void setDcData(void) {};

  public:
    ITransaction(IBus* bus = nullptr) : _bus { bus } {};
    virtual ~ITransaction(void) {};

    /// 送信時のクロック周波数
    uint32_t freq_write = 100000;

    /// 受信時のクロック周波数
    uint32_t freq_read = 100000;
/*
    /// CSピンの制御用インターフェイス
    /// value が falseのとき通信を開始、trueのとき通信を終了する;
    /// SPI通信の場合、falseで CSをLOW(active)に制御する;
    /// I2C通信の場合、falseで通信相手のアドレスを送信する;
    virtual void csControl(IBus* bus, bool value, bool read = false) {};

    /// D/Cピンの制御用インターフェイス;
    /// value : false=Command / true=Data
    virtual void dcControl(IBus* bus, bool value) {};
*/

    void init(void)
    {
      init_impl();
    }

    // 通信を開始する;
    error_t beginTransaction(void)
    {
      if (checkError(_bus->beginTransaction(this)))
      {
        return err_failed;
      }

      setCsAssert(false);

      return err_ok;
    }

    // 通信を開始する;
    error_t beginRead(uint8_t dummy_clock_cycle = 0)
    {
      if (checkError(_bus->beginTransaction(this, true)))
      {
        return err_failed;
      }

      setCsAssert(true);

      if (dummy_clock_cycle)
      {
        _bus->dummyClock(dummy_clock_cycle);
      }

      return err_ok;
    }

    // 通信を終了する;
    error_t endTransaction(void)
    {
      setCsDeassert();

      return _bus->endTransaction(this);
    }


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
    void write(const uint8_t* data, size_t len) { _bus->write(data, len); }


    // write max 4Byte little endian.
    void write(uint32_t data, uint8_t bytes = 1) { write((const uint8_t*)&data, bytes); };


    // write 8bit data.
    // 16bitパラレルバスの場合、上位8bitを0埋めして下位8bitのみを送信する;
    // 例:データソースが以下の場合
    //  data = 0xDE
    // 次のように送信される
    //   8bit: 1st:DE
    //  16bit: 1st:00DE
    void write8(uint8_t data) { write(&data, 1); }

    // write 16bit data.
    // 16bitで通信するバスの場合、1回で16bitぶん送信する;
    // 8bitパラレルバスの場合、上位・下位の順で2回に分けて送信する;
    // 例:データソースが以下の場合
    //  data = 0xDEAD (リトルエンディアンのためメモリ上の表現は [0]=DE [1]=AD)
    // 次のように送信される
    //   8bit: 1st:DE / 2nd:AD
    //  16bit: 1st:DEAD
    void write16(uint16_t data) { data = bswap16(data); write((const uint8_t*)&data, 2); }


    // 送受信 (全二重通信で送受信が同時に行われる場合に使う。実質 SPIバス専用)
    // なおwriteとreadに同じポインタを指定してもよい。
    void transfer(const uint8_t* write_data, uint8_t* read_data, size_t len) { _bus->transfer(write_data, read_data, len); }


    // write 16bit data array.
    // lenはデータサイズではなく配列の要素数である点に注意;
    void write16(const uint16_t* data, size_t len) { size_t i = 0; do { write16(data[i]); } while (++i != len); };


    // write 32bit data.
    void write32(uint32_t data) { data = bswap32(data); write((const uint8_t*)&data, 4); };


    // write max 4Byte little endian repeat.
    void writeRepeat(uint32_t data, uint8_t bytes, size_t repeat_count) { _bus->writeRepeat(data, bytes, repeat_count); };


    // I2Cバスで nack がtrueの場合は最後のデータを読み終えた際にNACK応答を返す;
    void read(uint8_t* read_data, size_t len, bool nack = false) { _bus->read(read_data, len, nack); };


    // I2Cバスで nack がtrueの場合は読み終えた際にNACK応答を返す;
    uint8_t read8(bool nack = false) { uint8_t res; read(&res, 1, nack); return res; };

/*
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
//*/
//------- 以下は D/C制御が必要なデバイス用
    // D/C control low(command) and write 1 byte.
    void writeCommand8(uint8_t data) { setDcCommand(); write8(data); };

    // D/C control low(command) and write 2 byte.
    void writeCommand16(uint16_t data) { setDcCommand(); write16(data); };

    // D/C control low(command) and write data array.
    void writeCommand(const uint8_t* data, size_t len) { setDcCommand(); write(data, len); };

    // write data (max 4Byte little endian) repeat.
    void writeCommandRepeat(uint32_t data, uint8_t bytes, size_t repeat_count) { setDcCommand(); writeRepeat(data, bytes, repeat_count); };

    // D/C control high(data) and write 1 byte.
    void writeData8(uint8_t data) { setDcData(); write8(data); };

    // D/C control high(data) and write 2 byte.
    void writeData16(uint16_t data) { setDcData(); write16(data); };

    // D/C control high(data) and write data array.
    void writeData(const uint8_t* data, size_t len) { setDcData(); write(data, len); };

    // write data (max 4Byte little endian).
    void writeData(uint32_t data, uint8_t bytes) { setDcData(); write((const uint8_t*)&data, bytes); };

    // write data (max 4Byte little endian) repeat.
    void writeDataRepeat(uint32_t data, uint8_t bytes, size_t repeat_count) { setDcData(); writeRepeat(data, bytes, repeat_count); };
//*/
  };

 }
}

#endif
