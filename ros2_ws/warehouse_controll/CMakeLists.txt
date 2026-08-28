cmake_minimum_required(VERSION 3.20)
project(warehouse_controll)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

add_executable(controll_panel_publisher src/controll_panel_publisher.cpp src/server.cpp)

target_link_libraries(controll_panel_publisher PRIVATE
    rclcpp::rclcpp
)

# === ВОТ ЭТОГО НЕ ХВАТАЛО ===
# Устанавливаем скомпилированную ноду туда, где её найдет ROS 2
install(TARGETS
  controll_panel_publisher
  DESTINATION lib/${PROJECT_NAME}
)
# ===========================

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()