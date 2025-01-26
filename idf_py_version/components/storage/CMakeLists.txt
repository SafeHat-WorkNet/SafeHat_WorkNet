idf_component_register(
  SRCS
    "sd_card_hal/sd_card_hal.c"
  INCLUDE_DIRS
    "sd_card_hal/include"
  PRIV_REQUIRES
    driver
    fatfs
    common
)

