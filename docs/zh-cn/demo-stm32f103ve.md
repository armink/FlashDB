# 基于 STM32F10X 片上 flash 的演示

## 介绍

基于 STM32F10X 芯片片上 Flash 的 KVDB  及 TSDB 演示

- MCU: STM32F103VE
- Flash Driver
  - STM32 on-chip flash
- IO
  - UART
    - TXD: PA9
    - RXD: PA10

## 使用

### 步骤1：连接串口助手或终端

将开发板的串口连接至 PC，并打开串口助手或终端工具

### 步骤2：打开演示工程

支持下面两种 IDE

- Keil MDK：打开 `RVMDK\FlashDB.uvprojx`
- [RT-Studio](https://www.rt-thread.org/page/studio.html): 通过工程向导进行导入

### 步骤3：编译与下载

编译演示工程，如果编译成功，下载固件至开发板

### 步骤4：检查日志结果

演示工程的运行结果，将会输出至 PC 的串口工具上。

>  PS: 演示工程代码及日志分析，详见：入门 -> [演示说明文档](zh-cn/demo-details.md)