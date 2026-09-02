#include <Wire.h>
#include <Arduino.h>
#include <Keypad.h>

// --- Настройки LCD и кнопок ---
const uint8_t LCD_ADDR = 0x3E;
const uint8_t btnMode = 3;
const uint8_t emergencyStopBtn = 2;
volatile bool emergencyStop = false;

bool isConnected = true; // Примечание: в этом коде эта переменная нигде не меняется на false
uint8_t btnState = HIGH;

enum class Cmd : uint8_t {
    Mode8Bit = 0x38,
    DisplayOn = 0x0C,
    Clear = 0x01
};

uint8_t mode = 0; // 0 - AUTO, 1 - MANUAL, 2 - EMERGENCY, 3 - NO LINK

// --- Настройки Keypad и Потенциометра ---
const byte ROWS = 2;
const byte COLS = 2;
char keys[ROWS][COLS] = {
  {'u', 'd'}, 
  {'l', 'r'}  
};
byte rowPins[ROWS] = {9, 8}; 
byte colPins[COLS] = {7, 6};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const uint8_t POT_PIN = A1;

bool btnU = false;
bool btnD = false;
bool btnL = false;
bool btnR = false;

// --- Функции LCD ---
void sendCMD(Cmd cmd) {
    Wire.beginTransmission(LCD_ADDR);
    Wire.write(0x00); 
    Wire.write(static_cast<uint8_t>(cmd));
    Wire.endTransmission();
}

void sendData(char symbol) {
    Wire.beginTransmission(LCD_ADDR);
    Wire.write(0x40); 
    Wire.write(symbol);
    Wire.endTransmission();
}

void sendText(const char *txt) {
    while (*txt != '\0') {
        sendData(*txt);
        txt++;
    }
}

void emerGencyStop() {
    emergencyStop = true;
}

void setup() {
    Serial.begin(9600); 
    Wire.begin();
    delay(50);

    sendCMD(Cmd::Mode8Bit);
    delay(5);
    sendCMD(Cmd::DisplayOn);
    delay(5);
    sendCMD(Cmd::Clear);
    delay(5);

    pinMode(emergencyStopBtn, INPUT_PULLUP); 
    pinMode(btnMode, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(emergencyStopBtn), emerGencyStop, FALLING);
}

void sendUartData() {
    int potValue = analogRead(POT_PIN);
    
    Serial.print(mode);
    Serial.print(",");
    Serial.print(potValue);
    Serial.print(",");
    Serial.print(btnU ? "1" : "0");
    Serial.print(",");
    Serial.print(btnD ? "1" : "0");
    Serial.print(",");
    Serial.print(btnL ? "1" : "0");
    Serial.print(",");
    Serial.println(btnR ? "1" : "0");
}

void loop() {
    static uint8_t oldMode = 255;
    static unsigned long lastSendTime = 0;
    const unsigned long SEND_INTERVAL = 100;

    // 1. НЕТ СВЯЗИ - не актуально пока что т.к uart
    if (!isConnected) {
        if (oldMode != 3) {
            sendCMD(Cmd::Clear);
            delay(5);
            sendText("NO LINK...");
            oldMode = 3;
        }
        
    }

    // 2. АВАРИЙНАЯ КНОПКА
    if (emergencyStop) {
        mode = 2; // Принудительно режим EMERGENCY
        if (oldMode != 2) {
            sendCMD(Cmd::Clear);
            delay(5);
            sendText("EMERGENCY STOP!");
            oldMode = 2;
        }
        // Сбрасываем флаг, если физическая кнопка отпущена
        if (digitalRead(emergencyStopBtn) == HIGH) {
            emergencyStop = false;
            // mode восстановится на следующем цикле из btnMode
        }
    } 
    else {
        // 3. ОБЫЧНЫЙ РЕЖИМ (Чтение тумблера AUTO/MANUAL)
        // Выполняется только если НЕТ аварии
        uint8_t currentPinState = digitalRead(btnMode);
        mode = (currentPinState == LOW) ? 1 : 0;

        if (mode != oldMode) {
            sendCMD(Cmd::Clear);
            delay(5);
            sendText("Mode: ");
            sendText(mode == 0 ? "AUTO" : "MANUAL");
            Serial.print("Mode changed: ");
            Serial.println(mode == 0 ? "AUTO" : "MANUAL");

            btnU = false; btnD = false; btnL = false; btnR = false;
            oldMode = mode;
        }
    }

     // 4. ОПРОС КЛАВИАТУРЫ (ПРАВИЛЬНАЯ ЛОГИКА ЧЕРЕЗ getKeys)
if (mode == 1) { // Опрос только в MANUAL
    keypad.getKeys(); // Эта команда обновляет массив состояний ВСЕХ кнопок
    
    // Проходимся по всем возможным клавишам (LIST_MAX обычно 10)
    for (int i = 0; i < LIST_MAX; i++) {
        if (keypad.key[i].kchar == 'u') {
            btnU = (keypad.key[i].kstate == PRESSED || keypad.key[i].kstate == HOLD);
        } 
        else if (keypad.key[i].kchar == 'd') {
            btnD = (keypad.key[i].kstate == PRESSED || keypad.key[i].kstate == HOLD);
        } 
        else if (keypad.key[i].kchar == 'l') {
            btnL = (keypad.key[i].kstate == PRESSED || keypad.key[i].kstate == HOLD);
        } 
        else if (keypad.key[i].kchar == 'r') {
            btnR = (keypad.key[i].kstate == PRESSED || keypad.key[i].kstate == HOLD);
        }
    }
        } else {
            // Если режим AUTO или EMERGENCY, принудительно гасим все кнопки
            btnU = false; btnD = false; btnL = false; btnR = false;
        }
    
    // 5. ОТПРАВКА ДАННЫХ ПО UART (Раз в 100 мс)
   
    if (millis() - lastSendTime >= SEND_INTERVAL) {
        sendUartData();
        lastSendTime = millis();
    }
}