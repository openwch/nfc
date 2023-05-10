# CH32V30x NFC工程

## 本工程相对于原版EVT工程修改点

### HSE_VALUE

由于使用了13.56Mhz晶振，所以在`ch32v30x.h`中的`HSE_VALUE`由原来的`8000000`改为`13560000`。

### system_ch32v30x.c

在`system_ch32v30x.c`中，时钟初始化已经根据13.56Mhz晶振更改，用户根据需要选择自己的时钟配置。

在使用本工程时如果需要同时使用USB，使用USB的时钟来源可以选择为`HSI`。

## [返回主readme](../readme.md)
