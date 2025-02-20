#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

// 핀 정의
#define TRIG_PIN1 9    // Sonicsensor1 Trig
#define ECHO_PIN1 10   // Sonicsensor1 Echo
#define TRIG_PIN2 11   // Sonicsensor2 Trig
#define ECHO_PIN2 12   // Sonicsensor2 Echo

#define LED_PIN1 6     // LED strip1
#define LED_PIN2 7     // LED strip2
#define NUM_LEDS 22    // LED number

#define DISTANCE_THRESHOLD_1 200
#define DISTANCE_THRESHOLD_2 100

// BLE service & UUID
BLEService sensorService("19B10000-E8F2-537E-4F6C-D104768A1214");
// Sensor feature (Send to Nano2)
BLEByteCharacteristic sensorCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);
// For BLEWrite
BLEByteCharacteristic commandCharacteristic("19B10004-E8F2-537E-4F6C-D104768A1214", BLEWrite);

Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(NUM_LEDS, LED_PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUM_LEDS, LED_PIN2, NEO_GRB + NEO_KHZ800);

bool sensor1Triggered_70 = false;  // Sensor1 detect 200~100cm
bool sensor2Triggered_70 = false;  // Sensor2 detect 200~100cm
bool sensor1Triggered_50 = false;  // Sensor1 detect under 100cm
bool sensor2Triggered_50 = false;  // Sensor2 detect under 100cm
bool wasConnected = false;         // Connection with UNO

int getDistance(int trigPin, int echoPin);
void activateLED_1(Adafruit_NeoPixel &strip, bool leftToRight);
void activateLED_2(Adafruit_NeoPixel &strip, bool leftToRight);
void executeCommand(byte cmd);

void setup() {
  Serial.begin(9600);
  
  pinMode(TRIG_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);
  pinMode(TRIG_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);

  strip1.begin();
  strip1.show();
  strip2.begin();
  strip2.show();

  if (!BLE.begin()) {
    Serial.println("Starting BLE failed!");
    while (1);
  }
  
  // BLE setting
  BLE.setLocalName("NanoUltrasonic");
  BLE.setAdvertisedService(sensorService);
  sensorService.addCharacteristic(sensorCharacteristic);
  sensorService.addCharacteristic(commandCharacteristic);
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
      BLE.poll();
      delay(500);
      // Processing Commands from UNOs
      if (commandCharacteristic.written()) {
        byte receivedCmd;
        commandCharacteristic.readValue(receivedCmd);
        Serial.print("UNO로부터 받은 명령: ");
        Serial.println(receivedCmd);
        executeCommand(receivedCmd);
      }

      // Detect the distance
      int distance1 = getDistance(TRIG_PIN1, ECHO_PIN1);
      int distance2 = getDistance(TRIG_PIN2, ECHO_PIN2);
      
      // Sensor1: 200cm ~ 100cm
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
      
      // Sensor2: 200cm ~ 100cm
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
      
      // Sensor1: 100cm
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
      
      // Sensor2: 100cm
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

// Detect the distance (cm)
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

// LED animation
void activateLED_1(Adafruit_NeoPixel &strip, bool leftToRight) {
  strip.clear();
  if (leftToRight) {
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
      strip.setPixelColor(i, strip.Color(255, 140, 0));
      strip.show();
      delay(30);
      BLE.poll();
    }
  }
}

// LED animation(fater)
void activateLED_2(Adafruit_NeoPixel &strip, bool leftToRight) {
  strip.clear();
  if (leftToRight) {
    for (int i = NUM_LEDS - 1; i >= 0; i--) {
      strip.setPixelColor(i, strip.Color(255, 0, 0));
      strip.show();
      delay(10);
      BLE.poll();
    }
  }
}

// LED animation(blinking)
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
 
}
