#include <ArduinoBLE.h>

#define PIEZO1_PIN 5  // 첫 번째 피에조 센서 핀
#define PIEZO2_PIN 6  // 두 번째 피에조 센서 핀

BLEService sensorService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEByteCharacteristic receivedCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

byte lastReceivedValue = 0; // 마지막으로 받은 신호 값을 저장

void setup() {
    Serial.begin(9600);
    pinMode(PIEZO1_PIN, OUTPUT);
    pinMode(PIEZO2_PIN, OUTPUT);

    digitalWrite(PIEZO1_PIN, LOW); // 기본 상태 (소리 OFF)
    digitalWrite(PIEZO2_PIN, LOW);

    if (!BLE.begin()) {
        Serial.println("Starting BLE failed!");
        while (1);
    }

    BLE.setLocalName("Nano2Ultrasonic");
    BLE.setAdvertisedService(sensorService);
    sensorService.addCharacteristic(receivedCharacteristic);
    BLE.addService(sensorService);
    BLE.advertise();

    Serial.println("2번 나노 BLE 활성화됨, 연결 대기 중...");
}

// 새로운 신호를 확인하는 함수 (즉시 전환 가능)
void checkForNewSignal() {
    BLE.poll();
    if (receivedCharacteristic.written()) {
        byte tempValue;
        receivedCharacteristic.readValue(tempValue);
        lastReceivedValue = tempValue;

        Serial.print("우노로부터 받은 값 업데이트: ");
        Serial.println(lastReceivedValue);

        if (lastReceivedValue == 0) {
            stopPiezo();
        }
    }
}

// 피에조 센서 정지 함수
void stopPiezo() {
    digitalWrite(PIEZO1_PIN, LOW);
    digitalWrite(PIEZO2_PIN, LOW);
    Serial.println("피에조 센서 정지");
}

void loop() {
    BLEDevice central = BLE.central();

    if (central) {
        Serial.print("우노와 연결됨: ");
        Serial.println(central.address());

        while (central.connected()) {
            BLE.poll();

            if (receivedCharacteristic.written()) {
                byte tempValue;
                receivedCharacteristic.readValue(tempValue);
                lastReceivedValue = tempValue;

                Serial.print("우노로부터 받은 값: ");
                Serial.println(lastReceivedValue);

                if (lastReceivedValue == 0) {
                    stopPiezo(); // 두 피에조 센서 정지
                }
            }

            // 신호에 따라 각각 실행 (즉시 전환 가능)
            switch (lastReceivedValue) {
                case 1:
                    activatePiezo_1(PIEZO1_PIN);
                    break;
                case 2:
                    activatePiezo_1(PIEZO2_PIN);
                    break;
                case 3:
                    activatePiezo_2(PIEZO1_PIN);
                    break;
                case 4:
                    activatePiezo_2(PIEZO2_PIN);
                    break;
                case 5:
                    activatePiezo_3();
                    break;
                case 6:
                    activatePiezo_4();
                    break;
            }

            delay(100);
        }

        Serial.println("우노와 연결 해제됨");
        stopPiezo(); // 연결이 끊어지면 피에조 센서 정지
    }
}

// 피에조 센서 작동 함수 1 (부드럽게 진동)
void activatePiezo_1(int pin) {
    while (lastReceivedValue == 1 || lastReceivedValue == 2) { // 1 또는 2 신호 유지 시 계속 실행
        for (int beepnumber = 0; beepnumber < 2; beepnumber++) {
            for (int pwmValue = 0; pwmValue <= 5; pwmValue += 1) {  
                analogWrite(pin, pwmValue);
                Serial.print("PWM 값 (증가): ");
                Serial.println(pwmValue);
                delay(20);
                checkForNewSignal();
            }
            for (int pwmValue = 5; pwmValue >= 0; pwmValue -= 1) {  
                analogWrite(pin, pwmValue);
                Serial.print("PWM 값 (감소): ");
                Serial.println(pwmValue);
                delay(20);
                checkForNewSignal();
            }
            delay(600);
        }
    }
}

// 피에조 센서 작동 함수 2 (빠르게 반복)
void activatePiezo_2(int pin) {
    while (lastReceivedValue == 3 || lastReceivedValue == 4) { // 3 또는 4 신호 유지 시 계속 실행
        for (int beepnumber = 0; beepnumber < 2; beepnumber++) {
            for (int pwmValue = 0; pwmValue <= 5; pwmValue += 1) {  
                analogWrite(pin, pwmValue);
                Serial.print("PWM 값 (증가): ");
                Serial.println(pwmValue);
                delay(20);
                checkForNewSignal();
            }
            for (int pwmValue = 5; pwmValue >= 0; pwmValue -= 1) {  
                analogWrite(pin, pwmValue);
                Serial.print("PWM 값 (감소): ");
                Serial.println(pwmValue);
                delay(20);
                checkForNewSignal();
            }
            delay(100);
        }
    }
}

void activatePiezo_3() {
    while (lastReceivedValue == 5) { // 5 신호 유지 시 계속 실행
        for (int beepnumber = 0; beepnumber < 2; beepnumber++) {
            for (int pwmValue = 0; pwmValue <= 5; pwmValue += 1) {  
                analogWrite(PIEZO1_PIN, pwmValue);
                analogWrite(PIEZO2_PIN, pwmValue);
                Serial.print("PWM 값 (증가): ");
                Serial.println(pwmValue);
                delay(20);
                checkForNewSignal();
            }
            for (int pwmValue = 5; pwmValue >= 0; pwmValue -= 1) {  
                analogWrite(PIEZO1_PIN, pwmValue);
                analogWrite(PIEZO2_PIN, pwmValue);
                Serial.print("PWM 값 (감소): ");
                Serial.println(pwmValue);
                delay(20);
                checkForNewSignal();
            }
            delay(600);
        }
    }
}

void activatePiezo_4() {
    while (lastReceivedValue == 6) { // 6 신호 유지 시 계속 실행
        for (int beepnumber = 0; beepnumber < 2; beepnumber++) {
            for (int pwmValue = 0; pwmValue <= 5; pwmValue += 1) {  
                analogWrite(PIEZO1_PIN, pwmValue);
                analogWrite(PIEZO2_PIN, pwmValue);
                Serial.print("PWM 값 (증가): ");
                Serial.println(pwmValue);
                delay(20);
                checkForNewSignal();
            }
            for (int pwmValue = 5; pwmValue >= 0; pwmValue -= 1) {  
                analogWrite(PIEZO1_PIN, pwmValue);
                analogWrite(PIEZO2_PIN, pwmValue);
                Serial.print("PWM 값 (감소): ");
                Serial.println(pwmValue);
                delay(20);
                checkForNewSignal();
            }
            delay(100);
        }
    }
}
