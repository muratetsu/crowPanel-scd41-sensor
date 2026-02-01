# CrowPanel SCD41 CO2 Monitor

CrowPanel ESP32 HMI 2.4インチディスプレイとSCD41センサーを使用した、CO2・温度・湿度モニタリングシステムです。
測定値をリアルタイムで表示するだけでなく、履歴データのグラフ表示やNTPによる時刻同期機能も備えています。

## 機能
*   **環境計測**: Sensirion SCD41センサーを使用し、CO2濃度、温度、湿度を5秒ごとに計測。
*   **グラフィカル表示**: 計測値を視認性の高いUIで表示し、トレンドをグラフで可視化。
*   **WiFi接続**: 起動時にWiFiへ接続し、NTPサーバーから正確な時刻を取得・同期。

## ハードウェア構成
*   **メインボード**: [CrowPanel ESP32 HMI 2.4-inch Display](https://www.elecrow.com/esp32-display-2-4-inch-intelligent-spi-tft-lcd-touch-screen-hmi-display.html)
*   **センサー**: Sensirion SCD41 (I2C接続)

## 必要なライブラリ
以下のライブラリをArduino IDEのライブラリマネージャー等からインストールしてください。

1.  **TFT_eSPI** (by Bodmer) - バージョン 2.5.43 推奨
2.  **Sensirion I2C SCD4x** (by Sensirion) - バージョン 1.1.0 推奨

## セットアップ方法

### 1. WiFi設定 (`Secrets.h`)
プロジェクトフォルダ内に `Secrets.h` ファイルを作成し、ご自身の環境のWiFi接続情報を記述してください。

```cpp
#ifndef SECRETS_H
#define SECRETS_H

#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#endif
```

### 2. TFT_eSPI の設定
CrowPanel 2.4インチ版を使用する場合、`TFT_eSPI` ライブラリの設定ファイル（`User_Setup.h`）を適切に構成する必要があります。
主なピン割り当ては以下の通りです：

*   **TFT_MISO**: 12
*   **TFT_MOSI**: 13
*   **TFT_SCLK**: 14
*   **TFT_CS**: 15
*   **TFT_DC**: 2
*   **TFT_BL**: 27

## 技術的詳細：LCDとWiFiの競合問題について
本プロジェクトでは、**LCDの表示不良を防ぐために特殊な初期化シーケンス**を採用しています。

### 問題の背景
CrowPanel 2.4インチモデルでは、LCDの Data/Command (DC) 切替ピンに **GPIO 2** が割り当てられています。
ESP32において GPIO 2 はブートストラップピンであり、かつWiFi機能が使用するADC2回路の一部でもあります。そのため、通常の `WiFi.begin()` を実行すると GPIO 2 の設定がリセットされ、直前に設定したLCDのピン設定が破壊されて画面が表示されなくなります。

### 解決策
この問題を回避するため、`setup()` 関数内では以下の順序で初期化を行っています。

1.  **WiFiハードウェアの初期化 (`NWManager::init()`)**: 
    *   接続は開始せず、WiFi物理層（PHY）のみを起動します。このタイミングでLCDのピン設定は一時的に破壊されます。
2.  **LCDの初期化 (`lcd.begin()`)**:
    *   破壊されたピン設定を正しい状態（LCD用）に再設定・修復します。
3.  **WiFi接続の開始 (`NWManager::connectWiFi()`)**:
    *   ハードウェアは既に起動しているため、ピン設定を破壊することなく接続処理のみを開始します。

> **注意**: コードを修正する際は、この初期化順序（WiFi Init -> LCD Init -> WiFi Connect）を崩さないようにしてください。

## ライセンス
MIT License
