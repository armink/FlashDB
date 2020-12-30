# stm32f103ve on-chip flash demo

## What

KVDB  and TSDB demo on STM32F10X chip

- MCU: STM32F103VE
- Library: STM32F10x_StdPeriph_Driver
- Flash Driver
  - STM32 on-chip flash
- IO
  - UART
    - TXD: PA9
    - RXD: PA10

## How

### Step1: connect the serial terminal

Connect the board to PC by serial port and open the PC serial terminal.

### Step2: open demo project

support 2 kinds of IDEs

- Keil MDK: open `Project\RVMDK\FlashDB.uvprojx`

  (MDK5.33 or later)

- IAR FOR ARM:  open `Project\EWARM\FlashDB.uvprojx`

  (IAR For ARM 8.40 or later )

### Step3: build and download

Download the firmware to your board when build successful.

### Step4: check the log

This demo's log will output to PC serial or SeggerRTT.

