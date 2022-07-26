#include <LovyanHAL.hpp>

// 予めマイコン側に examples/UseWithHost を書込んでおくこと;

// LHALインスタンスを作成する。COM2～COM99を順にオープンして応答があったものを使用する;
LHAL hal;

// COM番号が判明している場合は接続先を引数で指定できる;
// LHAL hal("COM4");

/// LAN経由で接続している場合は接続先をIPアドレスやドメイン名で指定できる;
// LHAL hal("192.168.1.123");

int main(int, char**)
{
  // 最初に インスタンスの init を実行してマイコンと接続する;
  if (hal.init() != lhal::err_ok)
  {
    for (;;)
    {
      hal.delay(1000);
    }
  }

  // ※ LHALで指定するピン番号はArduinoのピン番号ではなくMCUのピン番号である点に注意すること;
  // ( ただしLHALが正式対応していないMCUをArduino依存で動作させている場合は、Arduinoのピン番号で指定する )
  // ( ESP32の場合はArduinoのピン番号とMCUのピン番号が一致しているので気にしなくても良い )
  // Arduino UNOの場合;
  // digital 0 ~ 7 = PortD 0~7 = 0x18~0x1F = 24~31
  // digital 8 ~13 = PortB 0~5 = 0x08~0x0D =  8~13
  // analog  0 ~ 5 = PortC 0=5 = 0x10~0x15 = 16~21

  // ※ Arduinoのピン番号を元にMCUピン番号を得るconvertArduinoPinNumber関数が用意されており、これを使用してもよい;
  // Arduino環境の4番ピンのMCU番号を得る (ArduinoUnoでは 0x1C = 28が得られる);
  auto pin4 = hal.convertArduinoPinNumber(4);

  // Arduino環境の4番ピンをinput_pullupモードにする;
  hal.Gpio.setMode(pin4, hal.Gpio.input_pullup);

  // 4番ピンを読み取る;
  bool pin4value = hal.Gpio.read(pin4);

  // Arduino環境の5番ピンのMCU番号を得る (ArduinoUnoでは 0x1E = 30が得られる);
  auto pin5 = hal.convertArduinoPinNumber(5);

  // Arduino環境の5番ピンをoutputモードにする;
  hal.Gpio.setMode(pin5, hal.Gpio.output);

  // 5番ピンの出力を4番ピンと同じにする;
  hal.Gpio.write(pin5, pin4value);


  // Gpio.getHostで特定のピンに対する操作オブジェクトを取得できる;

  // Arduino環境の12番ピンの操作オブジェクトを取得する ( ボタンが接続されている想定 )
  auto btn = hal.Gpio.getHost(hal.convertArduinoPinNumber(12));

  // Arduino環境の13番ピンの操作オブジェクトを取得する ( LEDが接続されている想定 )
  auto led = hal.Gpio.getHost(hal.convertArduinoPinNumber(13));

  // ボタンをinputモードにする;
  btn.setMode( hal.Gpio.input );

  // LEDをoutputモードにする;
  led.setMode( hal.Gpio.output );

  for (;;)
  {
    // ボタンの状態を読取り、LEDに反映する;
    led.write( btn.read() );

    // 16ミリ秒待機
    hal.delay(16);
  }
}
