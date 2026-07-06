# Modbus RTU 实时分析上位机

这个工具用于在 Windows 上实时读取串口数据，按 Modbus RTU 帧解析，并把关键寄存器和值显示出来。

## 运行

```bat
cd /d D:\RT-ThreadStudio\workspace\canopen_stm32-v1.1-uart-split\tools\modbus_host
python -m pip install -r requirements.txt
python modbus_host.py
```

也可以双击 `run_modbus_host.bat` 启动。

## 默认参数

- 波特率：`115200`
- 数据位：`8`
- 校验位：`None`
- 停止位：`1`
- 分帧间隔：`20ms`

如果一帧被拆成多行，把“分帧间隔ms”调大；如果多帧粘在一起，把它调小。

## 功能

- 自动刷新和选择 COM 口。
- 实时读取 RX 数据并显示原始 HEX。
- 解析 Modbus RTU 功能码 `0x03`、`0x04`、`0x06`、`0x10`。
- 校验 CRC。
- 对常用寄存器显示中文名称和关键动作含义。
- 支持手动发送 HEX，可自动追加 CRC。
- 支持保存抓包结果为 CSV。

## 注意

Windows 串口通常只能被一个程序打开。如果成品上位机已经占用同一个 COM 口，本工具无法同时读取，需要关闭成品上位机，或使用虚拟串口/串口分线做镜像抓包。
