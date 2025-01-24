idf_component_register(
  SRCS
    "i2c.c"
    "error_handler.c"
  INCLUDE_DIRS
    "include"
  PRIV_REQUIRES
    driver
)

