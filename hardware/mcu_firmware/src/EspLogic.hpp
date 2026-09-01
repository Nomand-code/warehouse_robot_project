#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>

class Esp8266Comm {
private:
    SoftwareSerial esp_serial_;
    bool is_connected_;
    unsigned long last_send_time_;
    const int send_interval_;
    bool flag_emergency_msg_;

public:
    Esp8266Comm(uint8_t rx_pin, uint8_t tx_pin);
    ~Esp8266Comm();

    // Запрещаем копирование
    Esp8266Comm(const Esp8266Comm&) = delete;
    Esp8266Comm& operator=(const Esp8266Comm&) = delete;

    void begin(unsigned long baud = 9600);
    bool wait_for_response(const char* target, unsigned long timeout);

    bool is_connected() const;
    void set_connected(bool state);

    unsigned long last_send_time() const;
    void update_send_time();
    int send_interval() const;

    bool has_emergency_msg() const;
    void set_emergency_msg(bool state);
};