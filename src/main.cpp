#include <Arduino.h>

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include <S11059.h>

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// デバイス名
//
// デバイス名が5文字より多いとアドバタイズパケットにServiceのUUIDが載らないので注意。
// 詳細は未調査。下記参考。
//
// service advertising not working · Issue #163 · nkolban/esp32-snippets
// https://github.com/nkolban/esp32-snippets/issues/163
#define DEVICE_NAME "0001"

// ServiceのUUID
#define SERVICE_UUID "9D86A3DA-467C-4224-B96C-36D5F85C1725"

// CharacteristicのUUID
#define CHARACTERISTIC_UUID "BEB5483E-36E1-4688-B7F5-EA07361B26A8"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) { deviceConnected = true; };

  void onDisconnect(BLEServer *pServer) { deviceConnected = false; }
};

const int sdaPin = 25;
const int sclPin = 26;
const int ledPin = 13;
bool ledToggle = false;

S11059 colorSensor;

void setupBLE() {
  // GAP Serviceにおけるデバイス名を指定して初期化
  BLEDevice::init(DEVICE_NAME);

  // Serverを生成ろー
  pServer = BLEDevice::createServer();
  // Serverにコールバック関数を設定
  pServer->setCallbacks(new MyServerCallbacks());

  // Serviceを生成
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Characteristicを生成
  // プロパティはNotifyのみ
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);

  // CharacteristicにClient Configuration Descriptorを追加
  // クライアントから0x0001が書き込まれたらNotification開始
  pCharacteristic->addDescriptor(new BLE2902());

  // Serviceを開始
  Serial.println("start service");
  pService->start();

  // アドバタイズパケットにServiceの情報を含める
  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);

  // アドバタイズを開始
  Serial.println("start adivertising");
  pServer->getAdvertising()->start();
}

void setupColorSensor() {
  // SDAとSCLのピン番号を設定
  Wire.begin(sdaPin, sclPin);

  // 積分モードを固定時間モードに設定
  colorSensor.setMode(S11059_MODE_FIXED);

  // ゲイン選択
  // * S11059_GAIN_HIGH: Highゲイン
  // * S11059_GAIN_LOW: Lowゲイン
  colorSensor.setGain(S11059_GAIN_HIGH);

  // 1色あたりの積分時間設定(下記は指定可能な定数ごとの固定時間モードの場合の積分時間)
  // * S11059_TINT0: 87.5 us
  // * S11059_TINT1: 1.4 ms
  // * S11059_TINT2: 22.4 ms
  // * S11059_TINT3: 179.2 ms
  colorSensor.setTint(S11059_TINT2);

  // ADCリセット、スリープ解除
  if (!colorSensor.reset()) {
    Serial.println("reset failed");
  }

  // ADCリセット解除、バスリリース
  if (!colorSensor.start()) {
    Serial.println("start failed");
  }
}

bool readRGB(uint8_t (&dest)[3]) {
  if (colorSensor.update()) {
    // カラーセンサの各色のカウント値を格納
    float rgb[3] = {(float)colorSensor.getRed(), (float)colorSensor.getGreen(),
                    (float)colorSensor.getBlue()};

    // 最大値の色を選択
    int maxIndex = 0;
    for (int i = 1; i < 3; i++) {
      if (rgb[i] > rgb[maxIndex]) {
        maxIndex = i;
      }
    }

    // 最大値を255として各色を0-255に変換
    for (int i = 0; i < 3; i++) {
      dest[i] = (uint8_t)(255.0 * (rgb[i] / rgb[maxIndex]));
    }

    return true;
  } else {
    Serial.println("update failed");
    return false;
  }
}

void setup() {
  Serial.begin(9600);
  setupColorSensor();
  setupBLE();

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
}

void loop() {
  // 接続中
  if (deviceConnected) {
    delay(1000);

    // 積分時間よりも長く待機
    colorSensor.delay();

    uint8_t values[3];
    if (readRGB(values)) {
      pCharacteristic->setValue(values, 3);
      pCharacteristic->notify();
    }

    if (ledToggle) {
      digitalWrite(ledPin, HIGH);
    } else {
      digitalWrite(ledPin, LOW);
    }
    ledToggle = !ledToggle;
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    // ESP32のBluetoothスタックの処理が完了するまで待機
    // 500ミリ秒という値はサンプルコードからの流用で根拠は不明
    delay(500);
    Serial.println("devcie disconnected");

    // アドバタイズを再開
    pServer->startAdvertising();
    Serial.println("start advertising");

    oldDeviceConnected = deviceConnected;
    digitalWrite(ledPin, HIGH);
  }

  // 接続確立時
  if (deviceConnected && !oldDeviceConnected) {
    Serial.println("devcie connected");
    oldDeviceConnected = deviceConnected;
  }
}
