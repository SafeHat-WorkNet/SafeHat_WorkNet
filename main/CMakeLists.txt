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
)

