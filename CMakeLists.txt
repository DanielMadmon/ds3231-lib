set(COMPONENT_SRCS "ds3231_lib_util.c" "ds3231_lib_private.c" "ds3231_lib.c" "ds3231_lib_util.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")

set(COMPONENT_REQUIRES esp_driver_i2c esp_driver_gpio)

register_component()