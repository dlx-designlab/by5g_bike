#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

#define HALF_TURN_STEPS 50 // 반 바퀴 회전에 필요한 스텝 수
#define TRIG_PIN_UNO 8
#define ECHO_PIN_UNO 9

#define LED_PIN 11         // Neopixel 신호핀
#define LED_COUNT 5        // LED 개수

// Neopixel 스트립 객체 생성 (GRB 순서, 800kHz)
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// UNO 초음파 센서로부터 거리 측정 (cm 단위)
// 타임아웃을 5000µs로 낮춰 빠른 응답을 유도함
long getDistanceUNO() {
  digitalWrite(TRIG_PIN_UNO, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN_UNO, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN_UNO, LOW);
  long duration = pulseIn(ECHO_PIN_UNO, HIGH);
  return duration * 0.034 / 2;
}

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2); // 1번 모터 사용, 200 스텝

// 1번 나노 관련 BLE
BLEDevice nano1Device;
BLEByteCharacteristic sensorCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);
BLEByteCharacteristic nano1CommandCharacteristic("19B10004-E8F2-537E-4F6C-D104768A1214", BLEWrite);

// 2번 나노 관련 BLE
BLEDevice nano2Device;
BLEByteCharacteristic receivedCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLEWrite);

bool motorState = false;

// 이전에 전송한 UNO 신호 (0, 5, 6, 7 중 하나)
byte prevUnoSignal = 255;
// Nano1 센서에서 받은 마지막 값 (0, 1, 2, 3, 4)
byte lastSensorValue = 0;

void setup() {
  Serial.begin(9600);
  
  // 초음파 센서 핀 초기화
  pinMode(TRIG_PIN_UNO, OUTPUT);
  pinMode(ECHO_PIN_UNO, INPUT);
  
  // Neopixel 초기화
  strip.begin();
  strip.show(); // 모든 LED를 off 상태로 초기화
  
  if (!BLE.begin()) {
    Serial.println("BLE 초기화 실패");
    while (1);
  }
  
  Serial.println("BLE Central 시작됨 - 1번, 2번 나노 검색 중...");
  AFMS.begin();
  myMotor->setSpeed(100); // 모터 속도 설정
}

void loop() {
  // 1번 나노 검색 및 연결 (연결이 끊겼거나 서비스 연결 실패 시 재시도)
  if (!nano1Device || (nano1Device && !nano1Device.connected())) {
    if(nano1Device && !nano1Device.connected()){
      nano1Device.disconnect();  // 혹은 명시적으로 disconnect 호출
      nano1Device = BLEDevice();   // 변수 초기화
    }
    BLE.scanForName("NanoUltrasonic");
    nano1Device = BLE.available();
    if (nano1Device) {
      Serial.println("1번 나노 발견, 연결 시도...");
      delay(500);
      if (nano1Device.connect()) {
        Serial.println("1번 나노 연결 성공");
        delay(500);
        if (nano1Device.discoverService("19B10000-E8F2-537E-4F6C-D104768A1214")) {
          BLECharacteristic tempCharacteristic = nano1Device.characteristic("19B10001-E8F2-537E-4F6C-D104768A1214");
          if (tempCharacteristic) {
            sensorCharacteristic = static_cast<BLEByteCharacteristic&>(tempCharacteristic);
            sensorCharacteristic.subscribe();
            Serial.println("1번 나노 센서 특성 구독 완료");
          } else {
            Serial.println("1번 나노 센서 특성 발견 실패, 기존 연결 끊고 재연결 시도");
            nano1Device.disconnect();
            nano1Device = BLEDevice();
          }
          BLECharacteristic tempCommandCharacteristic = nano1Device.characteristic("19B10004-E8F2-537E-4F6C-D104768A1214");
          if (tempCommandCharacteristic) {
            nano1CommandCharacteristic = static_cast<BLEByteCharacteristic&>(tempCommandCharacteristic);
            Serial.println("1번 나노 명령 특성 설정 완료");
          } else {
            Serial.println("1번 나노 명령 특성 발견 실패, 기존 연결 끊고 재연결 시도");
            nano1Device.disconnect();
            nano1Device = BLEDevice();
          }
        } else {
          Serial.println("1번 나노 서비스 발견 실패, 기존 연결 끊고 재연결 시도");
          nano1Device.disconnect();
          nano1Device = BLEDevice();
        }
      } else {
        Serial.println("1번 나노 연결 실패, 재연결 시도");
      }
    }
  }
  
  // 2번 나노 검색 및 연결 (연결이 끊겼거나 서비스 연결 실패 시 재시도)
  if (!nano2Device || (nano2Device && !nano2Device.connected())) {
    if(nano2Device && !nano2Device.connected()){
      nano2Device.disconnect();
      nano2Device = BLEDevice();
    }
    BLE.scanForName("Nano2Ultrasonic");
    nano2Device = BLE.available();
    if (nano2Device) {
      Serial.println("2번 나노 발견, 연결 시도...");
      if (nano2Device.connect()) {
        Serial.println("2번 나노 연결 성공");
        delay(1000);
        if (nano2Device.discoverService("19B10000-E8F2-537E-4F6C-D104768A1214")) {
          BLECharacteristic tempCharacteristic = nano2Device.characteristic("19B10002-E8F2-537E-4F6C-D104768A1214");
          if (tempCharacteristic) {
            receivedCharacteristic = static_cast<BLEByteCharacteristic&>(tempCharacteristic);
            Serial.println("2번 나노 특성 설정 완료");
          } else {
            Serial.println("2번 나노 특성 발견 실패, 기존 연결 끊고 재연결 시도");
            nano2Device.disconnect();
            nano2Device = BLEDevice();
          }
        } else {
          Serial.println("2번 나노 서비스 발견 실패, 기존 연결 끊고 재연결 시도");
          nano2Device.disconnect();
          nano2Device = BLEDevice();
        }
      } else {
        Serial.println("2번 나노 연결 실패, 재연결 시도");
      }
    }
  }
  
  // UNO 초음파 센서로부터 거리 측정 및 unoSignal 결정
  long distanceUNO = getDistanceUNO();
  byte unoSignal;
  if (distanceUNO <= 100) {
    unoSignal = 6;
  } else if (distanceUNO >100 && distanceUNO <= 200){
    unoSignal = 5;
  } else {
    unoSignal = 0;
  }
  // Serial.println(distanceUNO);
  
  // UNO 초음파 센서 신호 전송
  // 5,6 신호는 감지되는 동안 계속 전송, 0은 상태 변화 시만 전송
  if (unoSignal == 5 || unoSignal == 6) {
    if (nano1Device && nano1Device.connected()) {
      nano1CommandCharacteristic.writeValue(unoSignal);
    }
    if (nano2Device && nano2Device.connected()) {
      receivedCharacteristic.writeValue(unoSignal);
    }
    prevUnoSignal = unoSignal;
  } else {
    if (unoSignal != prevUnoSignal) {
      if (nano1Device && nano1Device.connected()) {
        nano1CommandCharacteristic.writeValue(unoSignal);
      }
      if (nano2Device && nano2Device.connected()) {
        receivedCharacteristic.writeValue(unoSignal);
      }
      prevUnoSignal = unoSignal;
    }
  }
  
  // 모터 제어: Nano1 센서 값(1~4)와 UNO 센서 값(5,6)을 결합하여 물체 감지 여부 판단
  bool objectDetected = false;
  
  // Nano1 센서 값 업데이트 (값이 0이면 미감지로 간주)
  if (nano1Device && nano1Device.connected()) {
    BLE.poll();
    if (sensorCharacteristic.valueUpdated()) {
      byte sensorValue;
      sensorCharacteristic.readValue(sensorValue);
      lastSensorValue = sensorValue;
      // Nano2로 데이터 전달
      if (nano2Device && nano2Device.connected()) {
        receivedCharacteristic.writeValue(sensorValue);
      }
    }
  }
  
  // 물체 감지 조건: Nano1 센서 값이 3 또는 4, 또는 UNO 센서 값이 6이면 감지된 것으로 처리
  if ((lastSensorValue == 3 || lastSensorValue == 4) || (unoSignal == 6)) {
    objectDetected = true;
  } else {
    objectDetected = false;
  }
  
  // 모터 제어 (감지 시 전진 회전, 미감지 시 후진 회전)
  if (objectDetected && !motorState) {
    delay(1000);
    myMotor->step(HALF_TURN_STEPS, BACKWARD, DOUBLE);
    motorState = true;
  } else if (!objectDetected && motorState) {
    myMotor->step(HALF_TURN_STEPS, FORWARD, DOUBLE);
    motorState = false;
  }
  
  // ----------------------
  // LED 스트립 제어 로직 추가
  // 1. Nano1, Nano2 중 하나라도 연결이 안되어 있으면 1초 간격으로 파란색 깜빡임
  // 2. BLE가 모두 연결되어 있고, 센서 신호(3,4 또는 UNO의 6)가 감지되면 빨간색으로 켜짐
  // 3. 그 외의 경우 LED는 꺼짐
  // ----------------------
  if (!( (nano1Device && nano1Device.connected()) && (nano2Device && nano2Device.connected()) )) {
    // 블루투스 연결 미완료: 1초 간격으로 깜빡임
    if ((millis() / 1000) % 2 == 0) {
      for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 255));  // 파란색
      }
    } else {
      for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));    // 꺼짐
      }
    }
  }
  else if ((lastSensorValue == 3 || lastSensorValue == 4 || unoSignal == 6)) {
    // 신호 3, 4, 6이 감지됨: 빨간색 고정
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(255, 0, 0));  // 빨간색
    }
  } else {
    // 그 외: LED 끔
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
  }
  strip.show();
  
  // 루프 딜레이를 20ms로 줄여 빠른 반복을 유도
  delay(20);
}
