idf_component_register(
  SRCS
    "ov7670_hal/ov7670_hal.c"
  INCLUDE_DIRS
    "ov7670_hal/include"
  PRIV_REQUIRES
    driver
    common
)

