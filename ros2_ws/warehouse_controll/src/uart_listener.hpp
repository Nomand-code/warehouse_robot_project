#include <iostream>
#include <libserial/SerialPort.h>

using namespace LibSerial;

class uart_listener
{
private:
    const char port[64] = "/tmp/ros2_publisher";
    SerialPort arduino_controll;
public:
    uart_listener(/* args */);
   ~uart_listener() = default;
    std::string receive();
    void close_port();
};


