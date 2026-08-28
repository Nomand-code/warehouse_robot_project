#include <Wire.h>
#include <Arduino.h>

const uint8_t LCD_ADDR = 0x3E;
const uint8_t btnMode = 3;
volatile bool emergencyStop = false;


uint8_t btnState = HIGH;

bool FlagEmergencyMSG = false;

enum class Cmd : uint8_t {
    Mode8Bit  = 0x38,
    DisplayOn = 0x0C,
    Clear     = 0x01
};

// 0 -auto
// 1 - manual
// 2 - emergency stop
uint8_t mode  = 0;
void sendCMD(Cmd cmd){
    Wire.beginTransmission(LCD_ADDR);
    Wire.write(0x00); // Режим команды
    Wire.write(static_cast<uint8_t>(cmd));
    Wire.endTransmission();
}

// ДОБАВЛЯЕМ: Функция для отправки символов текста
void sendData(char symbol) {
    Wire.beginTransmission(LCD_ADDR);
    Wire.write(0x40); // Режим данных (Data)
    Wire.write(symbol);
    Wire.endTransmission();
}

void emerGencyStop(){
    emergencyStop = true;
}

void sendText(const char *txt){
    while (*txt != '\0')
    {
        sendData(*txt);
        txt++;
    }
    
}
void setup() {
    Wire.begin();
    delay(50); 
    
    sendCMD(Cmd::Mode8Bit);
    delay(5);  
    sendCMD(Cmd::DisplayOn);
    delay(5);
    sendCMD(Cmd::Clear);
    delay(5);
    pinMode(2, INPUT); // Настраиваем пин 2 на вход на схеме уже стоят подтягивающие резисторы 
    pinMode(btnMode,INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(2),emerGencyStop,FALLING); // т.к у нас резистор от v5 идет то у нас 1 всегда на пине Falling - ловит от 1 к 0 
    // ТЕСТ: Отправляем символ 'A' на экран
}

void loop() {
     // Создаем статическую переменную, которая помнит свое значение между витками loop
// Ставим 255, чтобы при самом первом запуске платы условие (mode != oldMode) точно сработало!
static uint8_t oldMode = 255; 

if (emergencyStop == false)
{
    uint8_t currentPinState = digitalRead(btnMode);

    if (FlagEmergencyMSG == true) {
        oldMode = 255; // Если вышли из аварии, принудительно заставим экран обновиться
        FlagEmergencyMSG = false;
    }

    // Привязываем режим к тумблеру
    mode = (currentPinState == LOW) ? 1 : 0;
    
    // Если режим изменился (или это первый запуск платы)
    if (mode != oldMode) 
    {
        sendCMD(Cmd::Clear);
        delay(5); 
        
        sendText("Mode: "); 
        
        // СТРОГО МАССИВ ИЗ 4 СИМВОЛОВ, чтобы itoa не ломала память контроллера!
        if (mode == 0) {
            sendText("AUTO");
        } 
        else if (mode == 1) {
            sendText("MANUAL");
        }
        
        // Запоминаем текущий режим для следующего цикла
        oldMode = mode; 
        
        delay(50); // Антидребезг тумблера
    }
}
else {
    if (FlagEmergencyMSG == false)
    {
        mode = 2; 
        sendCMD(Cmd::Clear);
        delay(5);
        sendText("EMERGENCY STOP!");
        FlagEmergencyMSG = true; 
    }
}

}

     

