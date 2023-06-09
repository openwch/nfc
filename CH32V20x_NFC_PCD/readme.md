# CH32V20x NFC工程

本工程对应原理图为[**CH32FVx0xR-NFC-EVT**](../Sch&PCB/CH32FVx0xR-NFC-EVT)

## 本工程相对于原版EVT工程修改点

### HSE_VALUE

由于使用了13.56Mhz/27.12Mhz晶振，所以在`ch32v20x.h`中的`HSE_VALUE`由原来的`8000000`改为`13560000`/`27120000`。

### system_ch32v20x.c

在`system_ch32v20x.c`中，时钟初始化已经根据`13.56Mhz`或者`27.12Mhz`晶振更改，用户根据需要选择自己的时钟配置。

### 注意事项

CH32V208、CH32V203R是必须使用`27.12Mhz`晶振的，而且不支持`135.6Mhz`运行频率。
其他型号必须使用`13.56Mhz`晶振，支持`135.6Mhz`运行频率。

在使用本工程时将不能同时使用USB，使用USB可以将系统时钟切换到`HSI`。

## [返回主readme](../readme.md)
