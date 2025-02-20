#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

#define HALF_TURN_STEPS 50 // steps for rotation
#define TRIG_PIN_UNO 8
#define ECHO_PIN_UNO 9

#define LED_PIN 11         // Neopixel pin
#define LED_COUNT 5        // LED number

// Neopixel operation
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Detection of ultrasonic of UNO (cm )
long getDistanceUNO() {
  digitalWrite(TRIG_PIN_UNO, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN_UNO, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN_UNO, LOW);
  long duration = pulseIn(ECHO_PIN_UNO, HIGH);
  return duration * 0.034 / 2;
}

//Using the Arduino motor shield
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2); // Use motor number 2

// BLE for Nano1
BLEDevice nano1Device;
BLEByteCharacteristic sensorCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);
BLEByteCharacteristic nano1CommandCharacteristic("19B10004-E8F2-537E-4F6C-D104768A1214", BLEWrite);

// BLE for Nano2
BLEDevice nano2Device;
BLEByteCharacteristic receivedCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLEWrite);

bool motorState = false;

// Signal sent from UNO before
byte prevUnoSignal = 255;
// Last sensor value from Nano1
byte lastSensorValue = 0;

void setup() {
  Serial.begin(9600);
  
  // Reset the sensor pin
  pinMode(TRIG_PIN_UNO, OUTPUT);
  pinMode(ECHO_PIN_UNO, INPUT);
  
  // Reset the Neopixel
  strip.begin();
  strip.show();
  
  if (!BLE.begin()) {
    Serial.println("BLE 초기화 실패");
    while (1);
  }
  
  Serial.println("BLE Central 시작됨 - 1번, 2번 나노 검색 중...");
  AFMS.begin();
  myMotor->setSpeed(100); // Set the motor speed
}

void loop() {
  // Searching for the Nano1.  When disconnected, it automatically retry
  if (!nano1Device || (nano1Device && !nano1Device.connected())) {
    if(nano1Device && !nano1Device.connected()){
      nano1Device.disconnect();
      nano1Device = BLEDevice();
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
  
  // Searching for the Nano2.  When disconnected, it automatically retry
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
  
  // Sensing the distance using Ultrasonic
  long distanceUNO = getDistanceUNO();
  byte unoSignal;
  if (distanceUNO <= 100) {
    unoSignal = 6;
  } else if (distanceUNO >100 && distanceUNO <= 200){
    unoSignal = 5;
  } else {
    unoSignal = 0;
  }
  
  // Send the ultrasonic data
  // Keep sending the data while signal 5, 6 continues, send 0 when detection changes
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
  
  // Motor control
  bool objectDetected = false;
  
  // Update Nano1 sensor value
  if (nano1Device && nano1Device.connected()) {
    BLE.poll();
    if (sensorCharacteristic.valueUpdated()) {
      byte sensorValue;
      sensorCharacteristic.readValue(sensorValue);
      lastSensorValue = sensorValue;
      // Send data to Nano2
      if (nano2Device && nano2Device.connected()) {
        receivedCharacteristic.writeValue(sensorValue);
      }
    }
  }
  
  // Process the object detected when Nano1 send 3, 4 or sensor value of 6 from UNO
  if ((lastSensorValue == 3 || lastSensorValue == 4) || (unoSignal == 6)) {
    objectDetected = true;
  } else {
    objectDetected = false;
  }
  
  // Control the motor
  if (objectDetected && !motorState) {
    delay(1000);
    myMotor->step(HALF_TURN_STEPS, BACKWARD, DOUBLE);
    motorState = true;
  } else if (!objectDetected && motorState) {
    myMotor->step(HALF_TURN_STEPS, FORWARD, DOUBLE);
    motorState = false;
  }
  
  // ----------------------
  // Control LED
  // 1. Blinks blue every 1 second if either Nano1 or Nano2 is not connected
  // 2. Lights red when all BLEs are connected and a sensor signal (3,4 or 6 on UNO) is detected
  // 3. Otherwise, the LED is off
  // ----------------------
  if (!( (nano1Device && nano1Device.connected()) && (nano2Device && nano2Device.connected()) )) {
    // BLE disconnected
    if ((millis() / 1000) % 2 == 0) {
      for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 255));  // Blue
      }
    } else {
      for (int i = 0; i < LED_COUNT; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));    // Off
      }
    }
  }
  else if ((lastSensorValue == 3 || lastSensorValue == 4 || unoSignal == 6)) {
    // Detect signal 3, 4, 6
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(255, 0, 0));  // Red
    }
  } else {
    // LED off
    for (int i = 0; i < LED_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
  }
  strip.show();

  delay(20);
}
