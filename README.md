# esp32-ble-color-sensor

カラーセンサー「S11059-02DT」の値を24ビットのRGB値に変換してBLE通信で送信するESP-WROOM-32用のプログラムです。

## 動作確認部品

- ESP-WROOM-32ボード: [ESPr® Developer 32 \- スイッチサイエンス](https://www.switch-science.com/catalog/3210/)
- S11059-02DT実装基板: [Ｉ２Ｃ対応デジタルカラーセンサモジュール　Ｓ１１０５９－０２ＤＴ: センサ一般 秋月電子通商\-電子部品・ネット通販](http://akizukidenshi.com/catalog/g/gK-08316/)
- LED x 1

## 開発環境

[PlatformIO IDE for VSCode](http://docs.platformio.org/en/latest/ide/vscode.html)を使用しています。

## 動作確認用プログラム

下記のAndroidアプリで受信できます。

- [jakalada/android\-color\-sensor\-monitor](https://github.com/jakalada/android-color-sensor-monitor)
