#include <ArduinoBLE.h>

#define PIEZO1_PIN 5  // First Piezo
#define PIEZO2_PIN 6  // Second Piezo

BLEService sensorService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEByteCharacteristic receivedCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);

byte lastReceivedValue = 0; // Store the last received data

void setup() {
    Serial.begin(9600);
    pinMode(PIEZO1_PIN, OUTPUT);
    pinMode(PIEZO2_PIN, OUTPUT);

    digitalWrite(PIEZO1_PIN, LOW); // defalt(sound off)
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

}

// Check the new detection
void checkForNewSignal() {
    BLE.poll();
    if (receivedCharacteristic.written()) {
        byte tempValue;
        receivedCharacteristic.readValue(tempValue);
        lastReceivedValue = tempValue;

        Serial.println(lastReceivedValue);

        if (lastReceivedValue == 0) {
            stopPiezo();
        }
    }
}

// Stop the Piezo
void stopPiezo() {
    digitalWrite(PIEZO1_PIN, LOW);
    digitalWrite(PIEZO2_PIN, LOW);
}

void loop() {
    BLEDevice central = BLE.central();

    if (central) {
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
                    stopPiezo(); // Stop Piezo
                }
            }

            // Operate Piezo based on the signal
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
        stopPiezo(); // Stop Piezo when disconnected
    }
}

// Piezo operation(smooth sound)
void activatePiezo_1(int pin) {
    while (lastReceivedValue == 1 || lastReceivedValue == 2) { // keep the function while signal 1 and 2
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

// Piezo operation(rapid sound)
void activatePiezo_2(int pin) {
    while (lastReceivedValue == 3 || lastReceivedValue == 4) { // keep the function while signal 3 and 4
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
    while (lastReceivedValue == 5) { // keep the function while signal 5
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
    while (lastReceivedValue == 6) { // keep the function while signal 5
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
