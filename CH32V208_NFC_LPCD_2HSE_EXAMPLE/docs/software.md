# CH32V208 NFC双晶振低功耗检卡软件设计

## 工程打印配置

本工程调试打印使用第三方printf库，路径为`Drivers/Debug/printf`，其中`printf_config.h`为配置文件，`putchar_`接口函数和浮点打印等配置可直接在该文件中配置。打印函数为`printf_`，在工程中可以使用`PRINT`代替，请勿使用标准的`printf`。

总打印开关在`Drivers/Debug/debug.h`中，注释掉所有`DEBUG`的宏定义即可。

其他有些文件有单独的打印开关：

```c
/* 每个文件单独debug打印的开关，置0可以禁止本文件内部打印 */
#define DEBUG_PRINT_IN_THIS_FILE    1
#if DEBUG_PRINT_IN_THIS_FILE
    #define PRINTF(...) PRINT(__VA_ARGS__)
#else
    #define PRINTF(...) do {} while (0)
#endif
```

在文件中增加打印时可以使用`PRINTF`进行，方便调试。

## 晶振切换

本工程对应使用CH445P进行切换晶振，使用PC13控制：高电平切换为27.12Mhz、低电平切换为32Mhz晶振。

### 芯片启动时晶振配置

由于CH445P需要电平来选择启用的通道，所有本工程在`system_ch32v20x.c`中初始化PLL时钟时，就对外设时钟进行了初始化。从而将PC13设置为低电平，默认使用32Mhz。
另外，CH32V208内部集成的晶振负载电容配置为16pF。

```c
    OSC->HSE_CAL_CTRL &= ~(0x07<<28);
    OSC->HSE_CAL_CTRL |= 0x04<<28;
```

由于在外部初始化了晶振电容设置，所以在`Drivers/BLE/MCU.c`中的`WCHBLE_Init`函数中，需要将晶振配置注释掉

```c
    /*
    // 32M晶振匹配电容和电流
    OSC->HSE_CAL_CTRL &= ~(0x07<<28);
    OSC->HSE_CAL_CTRL |= 0x03<<28;
    OSC->HSE_CAL_CTRL |= 3<<24;
    */
```

### 检测有卡后切换27.12Mhz晶振

通过`NFC_Reader.c`中的`changeHSETo27_12Mhz`切换为27.12Mhz晶振，在切换后，系统运行频率为108480000hz。

### 读写卡操作后切换32Mhz晶振

通过`NFC_Reader.c`中的`changeHSETo32Mhz`切换为32Mhz晶振，在切换后，系统运行频率为96000000hz。

## NFC程序配置

NFC底层可通过```NFC_Reader_bsp.h```中的宏进行配置，工程系统时钟有变化或者定时器有更换则只需要更改如下宏定义即可，如果有不能搭配的，会有宏进行报错。

```c
#define NFC_SYS_FREQUENCY           108480000       /* 系统运行时钟，单位HZ */

#define MAIN_PWM_TIM                1               /* 发送的nfc基本波形的高级定时器，1即为TIM1 */
#define MAIN_PWM_TIM_CCx            1               /* 发送的nfc基本波形的高级定时器的通道 */
#define MAIN_PWM_TIM_CHx_GPIOx      GPIOA           /* 发送的nfc基本波形的高级定时器的通道对应的GPIO组 */
#define MAIN_PWM_TIM_CHx_PIN        GPIO_Pin_8      /* 发送的nfc基本波形的高级定时器的通道对应的GPIO PIN */
#define MAIN_PWM_TIM_CHxN_GPIOx     GPIOB           /* 发送的nfc基本波形的高级定时器的互补通道对应的GPIO组 */
#define MAIN_PWM_TIM_CHxN_PIN       GPIO_Pin_13     /* 发送的nfc基本波形的高级定时器的互补通道对应的GPIO PIN */

#define CTRL_REC_TIM                2               /* 发送和接收载波的定时器，2即为TIM2 */
#define TIM_NFC_REC_CCx             4               /* 接收载波的定时器捕获通道 */
#define CTRL_REC_TIM_CHx_GPIOx      GPIOA           /* 接收的nfc波形的定时器的通道对应的GPIO组 */
#define CTRL_REC_TIM_CHx_PIN        GPIO_Pin_3      /* 接收的nfc波形的定时器的通道对应的GPIO PIN */

#define NFC_DMA                     1               /* 发送使用的DMA，即CTRL_REC_TIM的UPDATE更新事件所对应的DMA通道，1即为DMA1_Channely */
#define NFC_DMA_CHANNEL             2               /* 发送使用的DMA下的通道，2即为DMAx_Channel2 */

#define NFC_OPA                     OPA1            /* 使用的放大器 */
#define NFC_OPA_PSEL                CHP1            /* 选择的放大器的正向输入通道 */
#define NFC_OPA_NSEL                CHN1            /* 选择的放大器的负向输入通道 */
#define NFC_OPA_MODE                OUT_IO_OUT0     /* 选择的放大器的输出通道 */
#define NFC_OPA_CHP_GPIOx           GPIOB           /* 芯片集成放大器正向输入端引脚对应的GPIO组 */
#define NFC_OPA_CHP_PIN             GPIO_Pin_0      /* 芯片集成放大器正向输入端引脚对应的GPIO PIN */
#define NFC_OPA_CHN_GPIOx           GPIOA           /* 芯片集成放大器负向输入端引脚对应的GPIO组 */
#define NFC_OPA_CHN_PIN             GPIO_Pin_6      /* 芯片集成放大器负向输入端引脚对应的GPIO PIN */
```

修改定时器时注意，需要去修改相关引脚定义。

芯片片上放大器的输出引脚必须是选定的接收定时器的通道脚，无需额外连接即可直接读取信号。

## NFC初始化

初始化参考`NFC_Reader.c`中的`nfc_signal_bsp_init`函数，需要调用如下几个函数

`NFC_PWMOutGPIO_Init()`：该函数初始化发13.56Mhz的PWM的定时器的GPIO引脚。

`NFC_PWMOut_Init(6, 0, 4)`：该函数初始化发13.56Mhz的PWM定时器。
**参数的计算方式**：
`蓝牙系统时钟96Mhz / (6 + 1) = 13.714Mhz`，所以第一个参数为6，第三个参数为`(6 + 1) / 2 = 3.5`，取3和4都可以。此处之所以取4，是因为切换到27.12Mhz晶振时，系统时钟为108.48Mhz，参数为`108.48Mhz / (7 + 1) = 13.56Mhz`。第三个参数为`(7 + 1) / 2 = 4`。取4可以避免切换晶振后多改一个定时器参数。

`NFC_REC_NVIC_GPIO_Init()`：该函数初始化了接收定时器的GPIO和中断配置。

`NFC_CTRL_REC_Prepare_Init()`：该函数初始化了发送接收定时器在发送模式和接收模式公用的一些寄存器。

`NFC_DMA_Prepare()`：该函数初始化了发送接收定时器更新事件对应的DMA通道。

`NFC_OPA_Init()`：该函数初始化了NFC解码所使用的芯片内部放大器。

`nfc_pcd_cypto_rand_register(tmos_rand)`：该函数用来注册一个给加密协议提供随机数的函数，不注册的话加密协议将使用固定数。

之所以将函数都分开，是为了防止在项目中，有可能遇到的临时使用某些IO的其他功能，方便独立进行初始化。

## NFC启动

NFC初始化完成后，并没有立刻启动，而且NFC的启动分以下两种情况

### LPCD检卡启动

LPCD检卡时，天线需要启动，即主定时器需要发送13.56Mhz的PWM波形。但是芯片内部放大器不工作，而是通过ADC采集平均能量值，来判断有无卡。

打开天线通过调用函数：`nfc_signal_antenna_on()`
关闭天线通过调用函数：`nfc_signal_antenna_off()`

### 正常读写卡启动

正常读写卡时，启用放大器，不会启用ADC。放大器和天线应一起启动，并且延迟一定时间后（1ms以上）来保证电路达到最佳工作状态和卡接收到了足够的能量。

打开放大器通过调用函数：`OPA_Cmd(NFC_OPA, ENABLE)`
关闭放大器通过调用函数：`OPA_Cmd(NFC_OPA, DISABLE)`

## 低功耗检卡函数

例程中，低功耗检卡函数是通过蓝牙协议栈调用的，在`Src\APP\peripheral.c`中，通过函数`LL_AdvertiseEventRegister(nfc_period);`将`nfc_period`注册为每次广播事件后调用的函数来进行检卡。

在`nfc_period`中调用`NFC_Process`函数进行检卡。

`NFC_Process`函数中，使用`adcValueRead`函数检测放大器正向的平均电压AD值，当平均电压AD值超过设定的阈值`NFC_LPCD_THRESHOLD_VALUE`后，关闭天线，开始切换晶振，准备进一步的读写操作。

需要注意，如果更改了OPA_P正向引脚，则`adcValueRead`函数中ADC通道需要做出对应的改变。

## 读写例程

`NFC_Reader.c`在低功耗检卡成功后，会进行读卡写卡操作。可以参考该例程进行读写操作。
每次操作后，`g_nfc_pcd_signal_data.decode_buf.v8`中是解码得到的数据。

读写M1卡和RC522等NFC芯片一致，需要按照以下步骤进行

1. `PcdRequest`去唤醒和寻卡
2. `PcdAnticoll`去获取卡号
3. `PcdSelect`去选卡，选卡成功后必须执行`g_nfc_pcd_signal_data.is_encrypted = 0;`
4. `PcdAuthState`去验证密钥，只有密钥通过后才可以读写。
5. 使用`PcdRead`和`PcdWrite`可以读写数据。

## 注意事项

读写卡期间，除去NFC所用的两个中断，其他中断不可以打开，NFC对时间要求极高，处理其他中断会导致通讯失败。

## [返回主readme](../readme.md)
