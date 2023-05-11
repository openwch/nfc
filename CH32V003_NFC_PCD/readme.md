# CH32V003 NFC工程

本工程对应原理图为[**CH32V003-NFC-EVT**](../Sch&PCB/CH32V003-NFC-EVT)

## 本工程相对于原版EVT工程修改点

### HSE_VALUE

由于使用了13.56Mhz晶振，所以在`ch32V00x.h`中的`HSE_VALUE`由原来的`24000000`改为`13560000`。

### system_ch32v00x.c

在`system_ch32v00x.c`中，时钟初始化已经根据`13.56Mhz`外接晶振更改，使用nfc时系统时钟只可以使用`27.12Mhz`。

## 本工程相对其他NFC代码变化

1. CH32V003只有一个放大器且输出不可选，所以在`NFC_Reader_bsp.h`中，删除了`NFC_OPA`和`NFC_OPA_MODE`的宏定义，并且在`NFC_Reader.c`中，原先的`OPA_Cmd(NFC_OPA, ENABLE);`改为`OPA_Cmd(ENABLE);`。
2. CH32V003只有两个定时器`TIM1`和`TIM2`，所以在`NFC_Reader_bsp.c`中，删除了一些宏判断。用户如需要定时器功能，可以使用`Systick`。
3. CH32V003引脚有复用，默认例程中TIM1引脚映射配置为全映射。

## [返回主readme](../readme.md)
