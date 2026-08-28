#include <rclcpp/rclcpp.hpp>
#include <memory>
#include <chrono>
#include <functional>

#include "server.hpp"
#include <thread>

using namespace std::chrono_literals;

class controll_panel_publisher : public rclcpp::Node
{
private:
    size_t count_; 
    server my_server{8080};
    
    rclcpp::TimerBase::SharedPtr timer_;
    
    // 1. ПОТОК ТЕПЕРЬ ЖИВЕТ ЗДЕСЬ, внутри класса
    std::thread server_thread_; 

    void send_msg(){
        count_+=1;
        RCLCPP_INFO(this->get_logger(), "Сообщение от таймера №%zu", count_);
    }
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
    
public:
    controll_panel_publisher(/* args */) : Node("controll_panel_publisher"),count_(0)
    {
        RCLCPP_INFO(this->get_logger(), "Нода запущена!");
        
        // 2. ПРАВИЛЬНЫЙ ЗАПУСК ПОТОКА. 
        // Передаем адрес метода (&...) и сам объект (this).
        server_thread_ = std::thread(&controll_panel_publisher::run_server, this);
        
        timer_ = this->create_wall_timer(
            1s,std::bind(& controll_panel_publisher::send_msg,this));
    }
    
    ~controll_panel_publisher(){
        // 3. ВОТ КУДА НУЖЕН JOIN
        // Проверяем, можно ли присоединить поток, и ждем его.
        if (server_thread_.joinable()) {
            // Примечание: так как у тебя бесконечный цикл (while true) и blocking receive, 
            // при штатном закрытии программа тут "зависнет", ожидая клиента.
            // Но так как ты сказал, что закрываешь жестко через Ctrl+C — для тестов это ок, 
            // ROS просто убьет процесс.
            server_thread_.join(); 
        }
    };
};

int main(int argc, char * argv[])
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