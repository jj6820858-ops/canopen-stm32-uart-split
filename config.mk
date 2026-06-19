BSP_ROOT ?= D:/RT-ThreadStudio/workspace/canopen_stm32-v1.1-uart-split
RTT_ROOT ?= D:/RT-ThreadStudio/workspace/canopen_stm32-v1.1-uart-split/rt-thread

CROSS_COMPILE ?=D:\\RT-ThreadStudio\\workspace\\canopen_stm32-v1.1-uart-split\\arm-none-eabi-

CFLAGS := -D_SIGNAL_H_
AFLAGS :=
LFLAGS :=-T linkscripts//STM32F103RC//link.lds
CXXFLAGS :=

CPPPATHS :=-I$(BSP_ROOT) \
		-I$(BSP_ROOT)\canfestival\include \
		-I$(BSP_ROOT)\canfestival\od_master \
		-I$(BSP_ROOT)\canfestival\port \
		-I$(BSP_ROOT)\drivers \
		-I$(BSP_ROOT)\drivers\include \
		-I$(BSP_ROOT)\drivers\include\config \
		-I$(BSP_ROOT)\libraries\CMSIS\Device\ST\STM32F1xx\Include \
		-I$(BSP_ROOT)\libraries\CMSIS\Include \
		-I$(BSP_ROOT)\libraries\STM32F1xx_HAL_Driver\Inc \
		-I$(BSP_ROOT)\libraries\STM32F1xx_HAL_Driver\Inc\Legacy \
		-I$(RTT_ROOT)\components\drivers\include \
		-I$(RTT_ROOT)\components\finsh \
		-I$(RTT_ROOT)\components\libc\compilers\common \
		-I$(RTT_ROOT)\include \
		-I$(RTT_ROOT)\libcpu\arm\common \
		-I$(RTT_ROOT)\libcpu\arm\cortex-m3 \
		-I$(BSP_ROOT)\packages\freemodbus\modbus\include \
		-I$(BSP_ROOT)\packages\freemodbus\modbus\rtu \
		-I$(BSP_ROOT)\packages\freemodbus\modbus\ascii \
		-I$(BSP_ROOT)\packages\freemodbus\modbus\tcp \
		-I$(BSP_ROOT)\packages\freemodbus\port

DEFINES := -DHAVE_CCONFIG_H -D__RTTHREAD__
