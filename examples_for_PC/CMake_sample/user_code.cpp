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
  // ※ LHALで指定するピン番号はArduinoのピン番号ではなくMCUのピン番号である点に注意すること。;
  // 　( ESP32の場合はArduinoのピン番号とMCUのピン番号が一致しているので気にしなくても良い )
  // Arduino UNOの場合;
  // digital 0 ~ 7 = PortD 0~7 = 0x18~0x1F = 24~31
  // digital 8 ~13 = PortB 0~5 = 0x08~0x0D =  8~13
  // analog  0 ~ 5 = PortC 0=5 = 0x10~0x15 = 16~21

  // MCUの13番ピンをinput_pullupモードにする。 ( Arduino UNOの場合 digital 13番が該当する;
  hal.Gpio.setMode(13, hal.Gpio.input_pullup);

  // MCUの12番ピンをoutputモードにする。 ( Arduino UNOの場合 digital 12番が該当する;
  hal.Gpio.setMode(12, hal.Gpio.output);

  // 13番ピンを読み取る;
  bool pin13value = hal.Gpio.read(13);

  // 12番ピンの出力を13番ピンと同じにする;
  hal.Gpio.write(12, pin13value);

  // ※ Arduinoのピン番号を元にMCUピン番号を得るconvertArduinoPinNumber関数が用意されており、これを使用してもよい;
  // Arduino環境の5番ピンのMCU番号を得る (ArduinoUnoでは 0x1D = 29が得られる);
  auto pin5 = hal.convertArduinoPinNumber(5);

  // Arduinoの5番ピンをoutputモードにする;
  hal.Gpio.setMode(pin5, hal.Gpio.output);

  for (;;)
  {
    // Arduinoの5番ピンをhighにする;
    hal.Gpio.writeHigh(pin5);

    // Arduinoの5番ピンをlowにする;
    hal.Gpio.writeLow(pin5);
  }
}
