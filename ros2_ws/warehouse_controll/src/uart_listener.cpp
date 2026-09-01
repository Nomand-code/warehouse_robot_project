#include "uart_listener.hpp"

uart_listener::uart_listener(){
    try {
        arduino_controll.Open(port);
        arduino_controll.SetBaudRate(BaudRate::BAUD_9600);
    } catch (const std::exception& e) {
        std::cerr << "Ошибка открытия порта: " << e.what() << std::endl;
    }
}

std::string uart_listener::receive(){
    std::string data_buffer;
    if (!arduino_controll.IsOpen()) return "";

    try {
        if (arduino_controll.IsDataAvailable()) {
            arduino_controll.ReadLine(data_buffer, '\n', 250);
            
            if (!data_buffer.empty() && data_buffer.back() == '\r') {
                data_buffer.pop_back(); 
            }
}
    } catch (const LibSerial::ReadTimeout&) {
        // Таймаут чтения — штатная ситуация, проглатываем
    } catch (const std::exception& e) {
        std::cerr << "Ошибка UART: " << e.what() << std::endl;
    }
    return data_buffer;
}

void uart_listener::close_port() {
    if (arduino_controll.IsOpen()) {
        arduino_controll.Close(); // В LibSerial вызов с заглавной C
    }
}