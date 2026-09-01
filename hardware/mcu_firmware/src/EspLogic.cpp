#include "EspLogic.hpp"

Esp8266Comm::Esp8266Comm(uint8_t rx_pin, uint8_t tx_pin)
    : esp_serial_(rx_pin, tx_pin),
      is_connected_(true),
      last_send_time_(0),
      send_interval_(100),
      flag_emergency_msg_(false) {
}

Esp8266Comm::~Esp8266Comm() {
}

void Esp8266Comm::begin(unsigned long baud) {
    esp_serial_.begin(baud);
}

bool Esp8266Comm::wait_for_response(const char* target, unsigned long timeout) {
    unsigned long start_time = millis();
    uint8_t target_len = strlen(target);
    uint8_t matched = 0;

    while (millis() - start_time < timeout) {
        while (esp_serial_.available()) {
            char c = esp_serial_.read();
            Serial.write(c); // Дублируем в Монитор порта для отладки

            if (c == target[matched]) {
                matched++;
                if (matched == target_len) {
                    return true;
                }
            } else {
                // Если цепочка разорвалась, проверяем, не является ли символ началом слова заново
                if (c == target[0]) {
                    matched = 1;
                } else {
                    matched = 0;
                }
            }
        }
    }
    return false; // Вышли по таймауту — ответ не найден
}

bool Esp8266Comm::is_connected() const {
    return is_connected_;
}

void Esp8266Comm::set_connected(bool state) {
    is_connected_ = state;
}

unsigned long Esp8266Comm::last_send_time() const {
    return last_send_time_;
}

void Esp8266Comm::update_send_time() {
    last_send_time_ = millis();
}

int Esp8266Comm::send_interval() const {
    return send_interval_;
}

bool Esp8266Comm::has_emergency_msg() const {
    return flag_emergency_msg_;
}

void Esp8266Comm::set_emergency_msg(bool state) {
    flag_emergency_msg_ = state;
}