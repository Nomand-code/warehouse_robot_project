#include <rclcpp/rclcpp.hpp>
#include <memory>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>

// Твои сообщения
#include <warehouse_controll/msg/panel_state.hpp>
// Заглушка для Webots (стандартное сообщение ROS2 для скоростей)
#include <geometry_msgs/msg/twist.hpp> 

using namespace std::chrono_literals;

class main_brain : public rclcpp::Node
{
private:
    // --- ROS2 Интерфейсы ---
    rclcpp::Subscription<warehouse_controll::msg::PanelState>::SharedPtr panel_state_sub_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr webots_cmd_pub_; // Заглушка под Webots
    rclcpp::TimerBase::SharedPtr control_timer_; // Таймер для отправки команд в Webots

    // --- Потоки ---
    std::thread n8n_thread_;

    // --- Переменные состояния (Защищены потоками) ---
    
    // 1. Atomic для Emergency. Чтение/запись атомарны, мьютекс не нужен.
    std::atomic<bool> is_emergency_{false}; 
    
    // 2. Мьютекс для защиты сложных данных (режим, скорости)
    std::mutex state_mutex_; 
    
    std::string current_mode_{"UNKNOWN"};
    
    // Данные для MANUAL режима (с пульта)
    float manual_linear_vel_{0.0};
    float manual_angular_vel_{0.0};
    
    // Данные для AUTO режима (от n8n)
    float auto_linear_vel_{0.0};
    float auto_angular_vel_{0.0};

    // ==========================================
    // ЛОГИКА ОБРАБОТКИ ДАННЫХ
    // ==========================================

    // 1. Callback, когда приходят данные с пульта
    void panel_state_callback(const warehouse_controll::msg::PanelState::SharedPtr msg)
    {
        // --- БЛОК EMERGENCY ---
        if (msg->mode == "EMERGENCY")
        {
            // Если еще не был включен, логируем
            if (!is_emergency_.load()) {
                RCLCPP_WARN(this->get_logger(), "🚨 EMERGENCY STOP ACTIVATED! Отрубаем всё! 🚨");
            }
            is_emergency_.store(true); // Включаем флаг аварийной остановки
            send_velocity_to_webots(0.0, 0.0); // Мгновенно стопорим Webots
            return; // Выходим, не обрабатываем кнопки/скорости
        }
        else
        {
            // Если Emergency был, а теперь пришел Normal/Manual/Auto -> сбрасываем
            if (is_emergency_.load()) {
                RCLCPP_INFO(this->get_logger(), "✅ Emergency снят. Возобновляем работу.");
            }
            is_emergency_.store(false);
        }

        // --- БЛОК ОБНОВЛЕНИЯ ДАННЫХ ---
        // Берем "замок" (lock_guard), чтобы поток n8n не читал данные, пока мы их меняем
        std::lock_guard<std::mutex> lock(state_mutex_);
        current_mode_ = msg->mode;

        if (msg->mode == "MANUAL")
        {
            // Тут твоя логика преобразования кнопок и потенциометра в скорости
            // Пока сделаю простую заглушку:
            manual_linear_vel_ = msg->btn_forward ? msg->velocity : (msg->btn_backward ? -msg->velocity : 0.0);
            manual_angular_vel_ = msg->btn_left ? 1.0 : (msg->btn_right ? -1.0 : 0.0);
            
            // Примечание: в реальности нужно миксовать (например, если нажаты fwd и left)
        }
           // RCLCPP_INFO(this->get_logger(), "📥 Получен режим: %s, скорость: %.2f", msg->mode.c_str(), msg->velocity);

    }

    // 2. Таймер, который постоянно отправляет команды в Webots (работает в потоке ROS)
    void control_loop_timer_callback()
    {
        // Если Emergency - шлем ноль и выходим
        if (is_emergency_.load())
        {
            send_velocity_to_webots(0.0, 0.0);
            return;
        }

        // Берем "замок", чтобы безопасно прочитать текущие скорости
        std::lock_guard<std::mutex> lock(state_mutex_);

        if (current_mode_ == "MANUAL")
        {
            send_velocity_to_webots(manual_linear_vel_, manual_angular_vel_);
        }
        else if (current_mode_ == "AUTO")
        {
            send_velocity_to_webots(auto_linear_vel_, auto_angular_vel_);
        }
        else
        {
            // Если UNKNOWN или что-то еще - стоп
            send_velocity_to_webots(0.0, 0.0);
        }
    }

    // 3. Поток для общения с n8n (и OpenCV/YOLO в будущем)
    void run_n8n_loop()
    {
        RCLCPP_INFO(this->get_logger(), "Поток n8n запущен. Жду данных от AI...");
        
        // Крутимся, пока ROS2 работает
        while (rclcpp::ok())
        {
            // TODO: ЗДЕСЬ БУДЕТ ТВОЙ HTTP ЗАПРОС К N8N ИЛИ ВЫЗОВ OPENCV/YOLO
            // Пока просто генерируем заглушку:
            float n8n_lin = 0.5; 
            float n8n_ang = 0.0;

            // Кладем данные под замок, чтобы main_brain мог их безопасно забрать
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                auto_linear_vel_ = n8n_lin;
                auto_angular_vel_ = n8n_ang;
            }

            // Спим, чтобы не долбить n8n/OpenCV каждую миллисекунду (например, 5 раз в секунду)
            std::this_thread::sleep_for(200ms); 
        }
    }

    // ==========================================
    // ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
    // ==========================================

    // Заглушка для отправки в Webots
    void send_velocity_to_webots(float linear, float angular)
    {
        geometry_msgs::msg::Twist msg;
        msg.linear.x = linear;
        msg.angular.z = angular;
        webots_cmd_pub_->publish(msg);
        
        RCLCPP_INFO_THROTTLE(
        this->get_logger(), 
        *this->get_clock(), 
        1000, // Интервал в миллисекундах
        "🚀 Webots cmd: lin=%.2f, ang=%.2f", linear, angular
    );
    }

public:
    main_brain() : Node("main_brain")
    {
        RCLCPP_INFO(this->get_logger(), "🧠 Main Brain инициализирован!");

        // Подписка на пульт
        panel_state_sub_ = this->create_subscription<warehouse_controll::msg::PanelState>(
            "/hardware/panel_state", 10,
            std::bind(&main_brain::panel_state_callback, this, std::placeholders::_1));

        // Издатель в Webots (пока просто топик cmd_vel)
        webots_cmd_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("/cmd_vel", 10);

        // Таймер управления (20 Гц = каждые 50 мс)
        control_timer_ = this->create_wall_timer(
            50ms, std::bind(&main_brain::control_loop_timer_callback, this));

        // Запуск потока n8n
        n8n_thread_ = std::thread(&main_brain::run_n8n_loop, this);
    }

    ~main_brain()
    {
        // При закрытии ноды (Ctrl+C) rclcpp::ok() станет false, 
        // цикл в n8n_thread_ сам прервется, и мы сможем его безопасно присоединить.
        if (n8n_thread_.joinable())
        {
            n8n_thread_.join();
        }
        RCLCPP_INFO(this->get_logger(), "Main Brain выключен. Память очищена.");
    }
};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<main_brain>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}