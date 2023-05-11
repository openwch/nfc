# CH32-NFC-EVT

NFC-EVT是沁恒微电子在CH32通用系列单片机上实现的无需额外芯片的NFC读写方案。

## 支持芯片

[**CH32V003**](./CH32V003_NFC_PCD/readme.md)

[**CH32V20x**](./CH32V20x_NFC_PCD/readme.md)

[**CH32V30x**](./CH32V30x_NFC_PCD/readme.md)

[**CH32F20x**](./CH32F20x_NFC_PCD/readme.md)

## CH32 NFC原理概述

NFC操作代码位于工程目录中`Drivers/NFC_Reader`目录下。

NFC硬件底层使用了一个高级定时器，一个通用定时器，一个DMA通道。

高级定时器做为主定时器，通用定时器做为从定时器。在主定时器每个周期PWM波发出后，从定时器计数值+1，达到从定时器设定的周期值后，进入从定时器中断。在通用定时器中断中，通过修改寄存器`CHCTLRx`，对高级定时器产生的PWM输出进行开关。

DMA通道为该通用定时器更新事件所对应的DMA通道。

在波形发送完成后，通用定时器切换到接收模式，进行接收。

## LPCD低功耗检卡

LPCD低功耗检卡参考[CH32V208 NFC双晶振低功耗检卡操作例程](./CH32V208_NFC_LPCD_2HSE_EXAMPLE/readme.md)。

## 硬件相关注意事项

硬件相关注意事项请查看[硬件设计](./docs/hardware.md)

## 软件相关注意事项

软件相关注意事项请查看[软件设计](./docs/software.md)

## 操作示例

`NFC_Reader_M1.c`中为NFC M1卡操作库。

`NFC_Reader.c`中为NFC代码操作示例。

在`NFC_Reader.c`中演示了常用的读卡号、选中卡、读、写、扣款、充值等操作。

## 通讯测试

针对本方案所进行的一些测试请查看[测试汇总](./docs/test-report.md)
