# LovyanHAL

Hardware Abstraction Layer library. (under construction...)

概要 Overview.
----------------
各種マイコンのハードウェア機能の抽象化を目標としたライブラリです。  
主目的はLovyanGFXのハードウェア依存コードを分離し独立したライブラリとすることです。  
現在のところESP32・AVR・Arduino汎用を対象にGPIOのデジタル入出力のみが実装されています。  
SPI / I2C / Parallel 等の通信機能の実装を計画しています。  

Windowsからの利用も可能です。  
 - マイコン側にexamples/UseWithHostを書込む。  
 - VisualStudioにcmakeオプションを追加し、でexamples_for_PC/CMake_sample をビルドし実行。  


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

