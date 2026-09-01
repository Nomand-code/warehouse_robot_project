#include <rclcpp/rclcpp.hpp>
#include <memory>
#include <chrono>
#include <functional>

#include <cstdio> // Нужно для sscanf

#include <warehouse_controll/msg/panel_state.hpp>
// #include "server.hpp"
#include <thread>

#include "uart_listener.hpp"
using namespace std::chrono_literals;

class controll_panel_publisher : public rclcpp::Node
{
private:
    size_t count_;
    // server my_server{8080};
    uart_listener arduino_controll;
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<warehouse_controll::msg::PanelState>::SharedPtr panel_state_pub_;

    // 1. ПОТОК ТЕПЕРЬ ЖИВЕТ ЗДЕСЬ, внутри класса
    // std::thread server_thread_;

    std::thread uart_thread_;
    void send_msg()
    {
        count_ += 1;
        RCLCPP_INFO(this->get_logger(), "Сообщение от таймера №%zu", count_);
    }

    warehouse_controll::msg::PanelState parse_uart_data(const std::string &data)
    {
        warehouse_controll::msg::PanelState msg;

        int mode, pot, fwd, bwd, left, right;

        // Пытаемся прочитать ровно 6 целых чисел через запятую
        // sscanf автоматически проигнорирует \r и \n в конце!
        int parsed = std::sscanf(data.c_str(), "%d,%d,%d,%d,%d,%d",
                                 &mode, &pot, &fwd, &bwd, &left, &right);

        // Если прочитали все 6 значений, значит строка корректна
        if (parsed == 6)
        {
            if (mode == 0)
                msg.mode = "AUTO";
            else if (mode == 1)
                msg.mode = "MANUAL";
            else if (mode == 2)
                msg.mode = "EMERGENCY";
            else
                msg.mode = "UNKNOWN";

            msg.velocity = static_cast<float>(pot) / 1023.0f;

            msg.btn_forward = (fwd == 1);
            msg.btn_backward = (bwd == 1);
            msg.btn_left = (left == 1);
            msg.btn_right = (right == 1);
        }
        else
        {
            // Если формат битый, можно вывести предупреждение (но не крашить программу)
            RCLCPP_WARN(this->get_logger(), "Битые данные UART: %s", data.c_str());
        }

        return msg;
    }

    /*
    void run_server(){
        std::cout << "Сервер начал слушать...\n";
        while (true)
        {
            std::string msg = my_server.receive(); // Тут поток засыпает и ждет клиента
            if (!msg.empty()) {
            std::cout << ">> ПРИЛЕТЕЛИ ДАННЫЕ: " << msg << "\n";
            }
        }
    }
        */
    //
    void run_uart_listen()
    {
        std::cout << "9600 UART listen" << std::endl;

        while (true)
        {
            std::string msg = arduino_controll.receive(); // Тут поток засыпает и ждет клиента
            if (!msg.empty())
            {
                std::cout << ">> ПРИЛЕТЕЛИ ДАННЫЕ: " << msg << std::endl;
                auto ros_msg = parse_uart_data(msg);

                // Публикуем!
                panel_state_pub_->publish(ros_msg);
            }
        }
    }

public:
    controll_panel_publisher(/* args */) : Node("controll_panel_publisher"), count_(0)
    {
        RCLCPP_INFO(this->get_logger(), "Нода запущена!");

        // 2. ПРАВИЛЬНЫЙ ЗАПУСК ПОТОКА.
        // Передаем адрес метода (&...) и сам объект (this).
        // server_thread_ = std::thread(&controll_panel_publisher::run_server, this);
        panel_state_pub_ = this->create_publisher<warehouse_controll::msg::PanelState>(
            "/hardware/panel_state", 10);

        uart_thread_ = std::thread(&controll_panel_publisher::run_uart_listen, this);
        timer_ = this->create_wall_timer(
            10s, std::bind(&controll_panel_publisher::send_msg, this));
    }

    ~controll_panel_publisher()
    {
        // 3. ВОТ КУДА НУЖЕН JOIN
        // Проверяем, можно ли присоединить поток, и ждем его.
        /*
        if (server_thread_.joinable()) {
            // Примечание: так как у тебя бесконечный цикл (while true) и blocking receive,
            // при штатном закрытии программа тут "зависнет", ожидая клиента.
            // Но так как ты сказал, что закрываешь жестко через Ctrl+C — для тестов это ок,
            // ROS просто убьет процесс.
            server_thread_.join();
        }
            */
        arduino_controll.close_port();
        if (uart_thread_.joinable())
        {
            uart_thread_.join();
        }
    };
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);

    // 1. Создаем ноду (счетчик ссылок = 1)
    auto node = std::make_shared<controll_panel_publisher>();

    // 2. Крутим ноду бесконечно, пока не нажмут Ctrl+C
    rclcpp::spin(node);

    // 3. Выключаем ROS2 (без аргументов!)
    rclcpp::shutdown();

    // 4. Умный указатель node выходит из области видимости main().
    // Счетчик падает до 0 -> сам вызывается деструктор -> ВЫЗЫВАЕТСЯ НАШ JOIN -> память чистится.
    return 0;
}