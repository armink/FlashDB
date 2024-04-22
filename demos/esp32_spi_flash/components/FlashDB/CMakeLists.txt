idf_component_register(SRCS "porting/fal_flash_esp32_port.c"
                    INCLUDE_DIRS "inc" "../../../../port/fal/inc"
                    REQUIRES "spi_flash" "esp_partition" "esp_rom")
