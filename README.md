# LovyanHAL

Hardware Abstraction Layer library. (under construction...)

概要 Overview.
----------------
各種マイコンのハードウェア機能の抽象化を目標としたライブラリです。  
主目的はLovyanGFXのハードウェア依存コードを分離し独立したライブラリとすることです。  
現在のところESP32・AVR・Arduino汎用を対象にGPIOのデジタル入出力のみが実装されています。  
SPI / I2C / Parallel 等の通信機能の実装を計画しています。  

Windowsからの利用も可能です。(予めマイコン側に制御用プログラムを書き込んでおく必要があります。)  
 - マイコン側にexamples/UseWithHostを書込む。(ArduinoIDEやVSCodeを使用)  
 - PC側で examples_for_PC/CMake_sample をビルドし実行。(VisualStudio2022を使用)  


ライセンス License
----------------
main : [FreeBSD](license.txt)  


実装予定 Unimplemented request
----------------
  - MCU
    - ESP8266
    - ESP32シリーズ
    - RP2040
    - SAMD21
    - SAMD51
    - SPRESENSE
    - STM32シリーズ

