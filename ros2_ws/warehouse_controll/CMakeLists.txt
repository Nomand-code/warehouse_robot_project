cmake_minimum_required(VERSION 3.8)
project(warehouse_controll)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. Поиск основных зависимостей
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSERIAL REQUIRED libserial)

# 2. Поиск генераторов сообщений
find_package(rosidl_default_generators REQUIRED)
find_package(builtin_interfaces REQUIRED)

# 3. Генерация кастомных сообщений
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/PanelState.msg"
  DEPENDENCIES builtin_interfaces std_msgs
)

# 4. Создание исполняемого файла
add_executable(controll_panel_publisher 
  src/controll_panel_publisher.cpp 
  src/uart_listener.cpp
)

# 5. Подключение заголовочных файлов libserial
target_include_directories(controll_panel_publisher PRIVATE
  ${LIBSERIAL_INCLUDE_DIRS}
)

# 6. САМОЕ ГЛАВНОЕ для ROS 2 Humble/Jazzy/Lyrical:
# Получаем специальный таргет для типов поддержки C++ вместо устаревшей команды
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

# 7. Линковка библиотек (включая наш кастомный msg таргет!)
target_link_libraries(controll_panel_publisher PRIVATE
  rclcpp::rclcpp
  std_msgs::std_msgs
  ${LIBSERIAL_LIBRARIES}
  ${cpp_typesupport_target} # <-- Вот так теперь правильно линкуется кастомный .msg
)

# 8. Установка
install(TARGETS
  controll_panel_publisher
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()