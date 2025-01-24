cmake_minimum_required(VERSION 3.5)

# add_compile_definitions(USE_OV7670_XCLK_GPIO_27)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Topographic-Robot)

# Find and include the dotenv package
find_package(Dotenv REQUIRED)

idf_component_register(
  SRCS
    "main.c"
    "include/tasks/motor_tasks.c"
    "include/tasks/wifi_tasks.c"
    "include/tasks/webserver_tasks.c"
    "include/tasks/sensor_tasks.c"
    "include/tasks/system_tasks.c"
    "include/managers/time_manager.c"
    "include/managers/file_write_manager.c"
  INCLUDE_DIRS
    "include"
    "include/tasks/include"
    "include/managers/include"
  PRIV_REQUIRES
    dotenv
)