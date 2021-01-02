# 基于 ESP8266 片外 SPI-Flash 的演示

启动一个freertos任务运行flashDB的例程，然后倒计时1000秒后重启。运行log，请看当前目录下的ESP8266_flashDB_log.rtf 日志

# ESP8266开发环境

- Ubuntu 16.04 64位
- 用乐鑫IDF:ESP8266_RTOS_SDK(版本:ESP-IDF v3.2-252-g73bf28b-dirty),下载:https://github.com/espressif/ESP8266_RTOS_SDK.git
- 安装交叉编译工具链，下载:https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz

# ESP8266开发指南

- 详细的安装开发教程，请参考:https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html

# 编译和下载

- cd进入esp8266_spi_flash目录
- make all
- 使用串口线把8266开发板连接到PC
- 使能串口，执行命令：sudo chmod 777 /dev/ttyUSB*
- make flash
- make monitor

