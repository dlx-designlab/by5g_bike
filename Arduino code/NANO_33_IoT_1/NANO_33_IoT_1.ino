#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

// 핀 정의
#define TRIG_PIN1 9    // 초음파 센서1 Trig
#define ECHO_PIN1 10   // 초음파 센서1 Echo
#define TRIG_PIN2 11   // 초음파 센서2 Trig
#define ECHO_PIN2 12   // 초음파 센서2 Echo

#define LED_PIN1 6     // LED 스트립1 (좌 -> 우)
#define LED_PIN2 7     // LED 스트립2 (우 -> 좌)
#define NUM_LEDS 22    // 각 스트립의 LED 개수

#define DISTANCE_THRESHOLD_1 200  // 70cm 이하 동작 조건
#define DISTANCE_THRESHOLD_2 100  // 30cm 이하 동작 조건

// BLE 서비스 및 특성 UUID
BLEService sensorService("19B10000-E8F2-537E-4F6C-D104768A1214");
// 기존 센서 알림 특성 (Nano2로도 데이터 전송)
BLEByteCharacteristic sensorCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);
// 추가: UNO 명령 수신용 특성 (BLEWrite)
BLEByteCharacteristic commandCharacteristic("19B10004-E8F2-537E-4F6C-D104768A1214", BLEWrite);

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUM_LEDS, LED_PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUM_LEDS, LED_PIN2, NEO_GRB + NEO_KHZ800);

bool sensor1Triggered_70 = false;  // Sensor1 70~30cm 감지 상태
bool sensor2Triggered_70 = false;  // Sensor2 70~30cm 감지 상태
bool sensor1Triggered_50 = false;  // Sensor1 30cm 미만 감지 상태
bool sensor2Triggered_50 = false;  // Sensor2 30cm 미만 감지 상태
bool wasConnected = false;         // 중앙(UNO)과 연결 여부

// 함수 선언
int getDistance(int trigPin, int echoPin);
void activateLED_1(Adafruit_NeoPixel &strip, bool leftToRight);
void activateLED_2(Adafruit_NeoPixel &strip, bool leftToRight);
void executeCommand(byte cmd);

void setup() {
  Serial.begin(9600);
  
  // 초음파 센서 핀 설정
  pinMode(TRIG_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);
  pinMode(TRIG_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  // LED 스트립 초기화
  strip1.begin();
  strip1.show();
  strip2.begin();
  strip2.show();

  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }
  
  // BLE 기본 설정
  BLE.setLocalName("NanoUltrasonic");
  BLE.setAdvertisedService(sensorService);
  sensorService.addCharacteristic(sensorCharacteristic);
  sensorService.addCharacteristic(commandCharacteristic);  // 추가한 명령 특성 등록
  BLE.addService(sensorService);
  BLE.advertise();
  Serial.println("BLE device active, waiting for connections...");
}

void loop() {
  BLEDevice central = BLE.central();
  
  if (central) {
    if (!wasConnected) {
      Serial.print("Connected to: ");
      Serial.println(central.address());
      wasConnected = true;
    }
    
    while (central.connected()) {
      BLE.poll(); // BLE 이벤트 처리 호출
      delay(500);
      // UNO에서 전달된 명령 수신 처리
      if (commandCharacteristic.written()) {
        byte receivedCmd;
        commandCharacteristic.readValue(receivedCmd);
        Serial.print("UNO로부터 받은 명령: ");
        Serial.println(receivedCmd);
        executeCommand(receivedCmd);
      }

      // 두 초음파 센서를 통해 거리 측정
      int distance1 = getDistance(TRIG_PIN1, ECHO_PIN1);
      int distance2 = getDistance(TRIG_PIN2, ECHO_PIN2);
      
      // Sensor1: 200cm ~ 100cm 범위 감지 (신호 1)
      if (distance1 > DISTANCE_THRESHOLD_2 && distance1 <= DISTANCE_THRESHOLD_1) {
        if (!sensor1Triggered_70) {
          sensorCharacteristic.writeValue(1);
          Serial.println("Sent: 1 (Sensor1 200-100cm Triggered)");
          sensor1Triggered_70 = true;
        }
        activateLED_1(strip1, true);
      } else {
        if (sensor1Triggered_70) {
          sensorCharacteristic.writeValue(0);
          Serial.println("Sent: 0 (Sensor1 Cleared)");
          sensor1Triggered_70 = false;
        }
        strip1.clear();
        strip1.show();
      }
      
      // Sensor2: 200cm ~ 100cm 범위 감지 (신호 2)
      if (distance2 > DISTANCE_THRESHOLD_2 && distance2 <= DISTANCE_THRESHOLD_1) {
        if (!sensor2Triggered_70) {
          sensorCharacteristic.writeValue(2);
          Serial.println("Sent: 2 (Sensor2 200-100cm Triggered)");
          sensor2Triggered_70 = true;
        }
        activateLED_1(strip2, true);
      } else {
        if (sensor2Triggered_70) {
          sensorCharacteristic.writeValue(0);
          Serial.println("Sent: 0 (Sensor2 Cleared)");
          sensor2Triggered_70 = false;
        }
        strip2.clear();
        strip2.show();
      }
      
      // Sensor1: 100cm 미만 감지 (신호 3)
      if (distance1 > 0 && distance1 < DISTANCE_THRESHOLD_2) {
        if (!sensor1Triggered_50) {
          sensorCharacteristic.writeValue(3);
          Serial.println("Sent: 3 (Sensor1 <100cm Triggered)");
          sensor1Triggered_50 = true;
        }
        activateLED_2(strip1, true);
      } else {
        if (sensor1Triggered_50) {
          sensorCharacteristic.writeValue(0);
          Serial.println("Sent: 0 (Sensor1 Cleared)");
          sensor1Triggered_50 = false;
        }
        strip1.clear();
        strip1.show();
      }
      
      // Sensor2: 100cm 미만 감지 (신호 4)
      if (distance2 > 0 && distance2 < DISTANCE_THRESHOLD_2) {
        if (!sensor2Triggered_50) {
          sensorCharacteristic.writeValue(4);
          Serial.println("Sent: 4 (Sensor2 <100cm Triggered)");
          sensor2Triggered_50 = true;
        }
        activateLED_2(strip2, true);
      } else {
        if (sensor2Triggered_50) {
          sensorCharacteristic.writeValue(0);
          Serial.println("Sent: 0 (Sensor2 Cleared)");
          sensor2Triggered_50 = false;
        }
        strip2.clear();
        strip2.show();
      }
      
      delay(100);
    }



  }else{
    if(wasConnected){
      Serial.println("Disconnected. Adversite again");
      BLE.advertise();
      wasConnected = false;
    }
  }
}

// 초음파 센서로부터 거리 측정 (cm 단위)
int getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0) {
    return -1;
  }
  return duration * 0.034 / 2;
}

// LED 스트립을 좌→우 또는 우→좌로 점등시키는 함수
void activateLED_1(Adafruit_NeoPixel &strip, bool leftToRight) {
  strip.clear();
  if (leftToRight) {
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
      strip.setPixelColor(i, strip.Color(255, 140, 0));
      strip.show();
      delay(30);
      BLE.poll(); // LED 애니메이션 중에도 BLE 이벤트 처리
    }
  }
}

// LED 스트립을 좀 더 빠르게 점등시키는 함수 (예: 100cm 미만 감지 시)
void activateLED_2(Adafruit_NeoPixel &strip, bool leftToRight) {
  strip.clear();
  if (leftToRight) {
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
      strip.setPixelColor(i, strip.Color(255, 0, 0));
      strip.show();
      delay(10);
      BLE.poll(); // LED 애니메이션 중에도 BLE 이벤트 처리
    }
  }
}

// UNO에서 전달된 명령 값에 따른 동작 실행 함수
void executeCommand(byte cmd) {
  if (cmd == 5) {
    Serial.println("UNO 초음파 센서 신호 수신: LED 스트립 주황색 깜빡임 동작 실행");
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < NUM_LEDS; j++) {
        strip1.setPixelColor(j, strip1.Color(255, 140, 0));
        strip2.setPixelColor(j, strip2.Color(255, 140, 0));
      }
      strip1.show();
      strip2.show();
      delay(500);
      BLE.poll();
      strip1.clear();
      strip2.clear();
      strip1.show();
      strip2.show();
      delay(500);
      BLE.poll();
    }
  }
  
  if (cmd == 6) {
    Serial.println("UNO 초음파 센서 신호 수신: LED 스트립 빨간색 깜빡임 동작 실행");
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < NUM_LEDS; j++) {
        strip1.setPixelColor(j, strip1.Color(255, 0, 0));
        strip2.setPixelColor(j, strip2.Color(255, 0, 0));
      }
      strip1.show();
      strip2.show();
      delay(100);
      BLE.poll();
      strip1.clear();
      strip2.clear();
      strip1.show();
      strip2.show();
      delay(100);
      BLE.poll();
    }
  }
  // 필요에 따라 다른 명령(cmd 값)에 대한 동작 추가 가능
}
