#include <LovyanHAL.hpp>

/// LHALインスタンスを作成する。
LHAL hal;

void setup(void)
{
  // ※ LHALで指定するピン番号はArduinoのピン番号ではなくMCUのピン番号である点に注意すること。;
  // 　( ESP32の場合はArduinoのピン番号とMCUのピン番号は共通なので変換が不要。)
  // Arduino UNOの場合;
  // digital 0 ~ 7 = PortD 0~7 = 0x18~0x1F = 24~31
  // digital 8 ~13 = PortB 0~5 = 0x08~0x0D =  8~13
  // analog  0 ~ 5 = PortC 0=5 = 0x10~0x15 = 16~21

  // ※ Arduinoのピン番号を元にMCUピン番号を得るconvertArduinoPinNumber関数が用意されており、これを使用してもよい;
  // Arduino環境の5番ピンのMCU番号を得る (ArduinoUnoでは 0x1D = 29が得られる);
  auto pin5 = hal.convertArduinoPinNumber(5);

  // Arduinoの5番ピンをoutputモードにする;
  hal.Gpio.setMode(pin5, hal.Gpio.output);

  // Arduinoの5番ピンをhighにする;
  hal.Gpio.writeHigh(pin5);


  // MCUの13番ピンをinput_pullupモードにする。 ( Arduino UNOの場合 digital 13番が該当する;
  hal.Gpio.setMode(13, hal.Gpio.input_pullup);

  // MCUの12番ピンをoutputモードにする。 ( Arduino UNOの場合 digital 12番が該当する;
  hal.Gpio.setMode(12, hal.Gpio.output);

  // 13番ピンを読み取る;
  bool pin13value = hal.Gpio.read(13);

  // 12番ピンの出力を13番ピンと同じにする;
  hal.Gpio.write(12, pin13value);
}

void loop(void)
{
  // 13番ピンを読み取る;
  bool pin13value = hal.Gpio.read(13);

  // 12番ピンの出力を13番ピンと同じにする;
  hal.Gpio.write(12, pin13value);
}
