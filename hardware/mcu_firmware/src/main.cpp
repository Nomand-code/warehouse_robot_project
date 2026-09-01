#include <Wire.h>
#include <Arduino.h>
#include <Keypad.h> // Подключаем библиотеку клавиатуры

// --- Твои старые настройки LCD и кнопок ---
const uint8_t LCD_ADDR = 0x3E;
const uint8_t btnMode = 3;
const uint8_t emergencyStopBtn = 2;
volatile bool emergencyStop = false;

bool isConnected = true; 
uint8_t btnState = HIGH;

enum class Cmd : uint8_t {
    Mode8Bit = 0x38,
    DisplayOn = 0x0C,
    Clear = 0x01
};

uint8_t mode = 0; // 0 - AUTO, 1 - MANUAL, 2 - EMERGENCY, 3 - NO LINK

// --- НОВЫЕ НАСТРОЙКИ: Keypad и Потенциометр ---
const byte ROWS = 2;
const byte COLS = 2;
char keys[ROWS][COLS] = {
  {'u', 'd'}, // u = up (вперед), d = down (назад)
  {'l', 'r'}  // l = left, r = right
};
// Пины: Row1, Row2, Col1, Col2
byte rowPins[ROWS] = {9, 8}; 
byte colPins[COLS] = {7, 6};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const uint8_t POT_PIN = A1; // Потенциометр скорости

// Переменные для хранения состояния кнопок (чтобы отправлять их постоянно)
bool btnU = false;
bool btnD = false;
bool btnL = false;
bool btnR = false;

// --- Функции LCD (твои старые) ---
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

// --- НОВАЯ ФУНКЦИЯ: Отправка данных в ROS2 ---
void sendUartData() {
    int potValue = analogRead(POT_PIN); // Читаем потенциометр (0-1023)
    
    // Формат: MODE,POT,U,D,L,R
    // MODE: 0=AUTO, 1=MANUAL, 2=EMERGENCY
    // POT: значение потенциометра
    // U,D,L,R: 1 если нажата, 0 если нет
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
    Serial.println(btnR ? "1" : "0"); // println добавит '\r\n' в конце
}

void loop() {
    static uint8_t oldMode = 255;
    static unsigned long lastSendTime = 0;
    const unsigned long SEND_INTERVAL = 100; // Отправка раз в 100 мс

    // 1. НЕТ СВЯЗИ
    if (!isConnected) {
        if (oldMode != 3) {
            sendCMD(Cmd::Clear);
            delay(5);
            sendText("NO LINK...");
            oldMode = 3;
        }
        return;
    }

    // 2. АВАРИЙНАЯ КНОПКА
    if (emergencyStop) {
        mode = 2; // Устанавливаем режим EMERGENCY для отправки в ROS
        if (oldMode != 2) {
            sendCMD(Cmd::Clear);
            delay(5);
            sendText("EMERGENCY STOP!");
            oldMode = 2;
        }
        if (digitalRead(emergencyStopBtn) == HIGH) {
            emergencyStop = false;
        }
        return;
    }

    // 3. ОБЫЧНЫЙ РЕЖИМ (Чтение тумблера AUTO/MANUAL)
    uint8_t currentPinState = digitalRead(btnMode);
    mode = (currentPinState == LOW) ? 1 : 0;

    if (mode != oldMode) {
        sendCMD(Cmd::Clear);
        delay(5);
        sendText("Mode: ");
        sendText(mode == 0 ? "AUTO" : "MANUAL");
        Serial.print("Mode changed: ");
        Serial.println(mode == 0 ? "AUTO" : "MANUAL");

        btnU = false; 
        btnD = false; 
        btnL = false; 
        btnR = false;
        oldMode = mode;
    }

    // 4. ОПРОС КЛАВИАТУРЫ (Keypad)
    // Сбрасываем все кнопки перед опросом
    btnU = false; btnD = false; btnL = false; btnR = false;
    
        // 4. ОПРОС КЛАВИАТУРЫ (Исправленная логика)
    if (mode == 1) { // Опрос только в MANUAL
        char key = keypad.getKey();
        KeyState state = keypad.getState();

        if (state == PRESSED || state == HOLD) {
            if (key == 'u') btnU = true;
            if (key == 'd') btnD = true;
            if (key == 'l') btnL = true;
            if (key == 'r') btnR = true;
        } 
        else if (state == RELEASED) {
            if (key == 'u') btnU = false;
            if (key == 'd') btnD = false;
            if (key == 'l') btnL = false;
            if (key == 'r') btnR = false;
        }
    }
    
    

    // 5. ОТПРАВКА ДАННЫХ ПО UART (Раз в 100 мс)
    if (millis() - lastSendTime >= SEND_INTERVAL) {
        sendUartData();
        lastSendTime = millis();
    }
}