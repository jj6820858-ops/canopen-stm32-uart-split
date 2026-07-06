from __future__ import annotations

from dataclasses import dataclass
import re
from typing import Iterable


@dataclass(frozen=True)
class RegisterInfo:
    start: int
    end: int
    name: str
    access: str
    unit: str = ""


@dataclass(frozen=True)
class DecodedFrame:
    raw_hex: str
    slave: str
    function: str
    function_name: str
    operation: str
    register: str
    values: str
    crc: str
    note: str


FUNCTION_NAMES = {
    0x01: "读线圈",
    0x02: "读离散输入",
    0x03: "读保持寄存器",
    0x04: "读输入寄存器",
    0x05: "写单线圈",
    0x06: "写单寄存器",
    0x0F: "写多个线圈",
    0x10: "写多个寄存器",
}


EXCEPTION_CODES = {
    0x01: "非法功能码",
    0x02: "非法数据地址",
    0x03: "非法数据值",
    0x04: "从站设备故障",
    0x05: "确认",
    0x06: "从站设备忙",
}


REGISTER_TABLE = [
    RegisterInfo(0, 2, "孔位编码", "R", "bitmap"),
    RegisterInfo(3, 3, "清洗针头", "B"),
    RegisterInfo(4, 4, "取液/注液/清洗动作", "B"),
    RegisterInfo(5, 6, "柱塞泵/泵液量", "B", "x1000"),
    RegisterInfo(7, 7, "蠕动泵转速", "B"),
    RegisterInfo(8, 8, "蠕动泵圈数", "B"),
    RegisterInfo(9, 9, "操作位号", "B", "bitmap"),
    RegisterInfo(10, 10, "转盘功能使能", "B", "bit"),
    RegisterInfo(11, 13, "转盘指定位号/电机控制", "B", "bitmap"),
    RegisterInfo(14, 14, "酶孔位制冷", "B"),
    RegisterInfo(15, 16, "光强读数/光源控制", "R/B", "x1000"),
    RegisterInfo(17, 18, "仪器状态", "R", "bitmap"),
    RegisterInfo(19, 32, "异常码", "R"),
    RegisterInfo(99, 99, "转盘加热温度", "B", "x100"),
    RegisterInfo(100, 100, "酶试剂制冷温度", "B", "x100"),
    RegisterInfo(101, 103, "称重反馈", "R"),
    RegisterInfo(119, 119, "参数控制字", "B"),
    RegisterInfo(120, 120, "称重控制", "B"),
    RegisterInfo(129, 131, "机械自测参数", "B"),
    RegisterInfo(149, 162, "速度参数", "B"),
    RegisterInfo(163, 170, "升降臂深度参数", "B", "0.1mm"),
    RegisterInfo(171, 178, "加样臂角度参数", "B"),
    RegisterInfo(179, 182, "转盘角度参数", "B"),
    RegisterInfo(183, 190, "清洗/搅拌参数", "B"),
    RegisterInfo(191, 191, "加热目标温度", "B", "x100"),
    RegisterInfo(192, 192, "制冷目标温度", "B", "x100"),
    RegisterInfo(193, 195, "震荡参数", "B"),
    RegisterInfo(196, 197, "柱塞泵取液/吐液量", "B", "uL"),
]


VALUE_HINTS = {
    3: {
        0: "停止",
        1: "洗内壁",
        2: "洗外壁",
        3: "洗内外",
        4: "排液",
        5: "保养",
    },
    4: {
        0: "停止",
        1: "取液",
        2: "加液",
        3: "混合",
        4: "洗针",
        5: "吸取",
        6: "吸加",
    },
    5: {
        0x0000: "泵停止",
        0x2000: "泵启动",
    },
    11: {
        0x8000: "转盘停止",
        0x9000: "转盘启动/到指定孔位",
        0xA000: "转盘保持",
        0xC000: "转盘回零",
    },
    14: {
        0: "制冷关闭",
        1: "制冷开启",
    },
    15: {
        0: "光源关闭",
        0x0018: "光源/读光触发",
    },
}


def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def append_crc(data: bytes) -> bytes:
    crc = crc16_modbus(data)
    return data + bytes((crc & 0xFF, (crc >> 8) & 0xFF))


def format_hex(data: Iterable[int]) -> str:
    return " ".join(f"{byte:02X}" for byte in data)


def parse_hex_string(text: str) -> bytes:
    cleaned = text.strip().replace("0x", "").replace("0X", "")
    if not cleaned:
        return b""

    if re.search(r"[\s,;:_-]", cleaned):
        tokens = [token for token in re.split(r"[\s,;:_-]+", cleaned) if token]
        if any(len(token) > 2 for token in tokens):
            raise ValueError("每个十六进制字节最多 2 位")
        return bytes(int(token, 16) for token in tokens)

    if len(cleaned) % 2:
        raise ValueError("连续十六进制字符串长度必须是偶数")
    return bytes.fromhex(cleaned)


def find_register(addr: int) -> RegisterInfo | None:
    for item in REGISTER_TABLE:
        if item.start <= addr <= item.end:
            return item
    return None


def register_label(addr: int) -> str:
    info = find_register(addr)
    if info is None:
        return f"{addr} / 0x{addr:04X}"
    suffix = f" {info.unit}" if info.unit else ""
    return f"{addr} / 0x{addr:04X} {info.name}{suffix}"


def value_hint(addr: int, value: int) -> str:
    hints = VALUE_HINTS.get(addr)
    if not hints:
        return ""
    return hints.get(value, "")


def _u16_be(frame: bytes, offset: int) -> int:
    return (frame[offset] << 8) | frame[offset + 1]


def _register_values(data: bytes) -> list[int]:
    return [_u16_be(data, i) for i in range(0, len(data) - 1, 2)]


def _crc_status(frame: bytes) -> tuple[bool, str]:
    if len(frame) < 4:
        return False, "长度不足"
    wire = frame[-2] | (frame[-1] << 8)
    calc = crc16_modbus(frame[:-2])
    if wire == calc:
        return True, "正确"
    return False, f"错误 收到:{wire:04X} 计算:{calc:04X}"


def decode_frame(frame: bytes) -> DecodedFrame:
    raw_hex = format_hex(frame)
    if len(frame) < 4:
        return DecodedFrame(raw_hex, "-", "-", "-", "无效帧", "-", "-", "长度不足", "至少需要 地址+功能码+CRC")

    crc_ok, crc_text = _crc_status(frame)
    slave = frame[0]
    function = frame[1]
    base_function = function & 0x7F
    function_name = FUNCTION_NAMES.get(base_function, "未知功能码")
    crc_display = crc_text if crc_ok else crc_text

    if function & 0x80:
        code = frame[2] if len(frame) >= 5 else None
        text = EXCEPTION_CODES.get(code, "未知异常") if code is not None else "缺少异常码"
        return DecodedFrame(
            raw_hex,
            f"{slave}",
            f"0x{function:02X}",
            f"{function_name}异常",
            "异常响应",
            "-",
            f"异常码 0x{code:02X}" if code is not None else "-",
            crc_display,
            text,
        )

    if function in (0x03, 0x04) and len(frame) == 8:
        start = _u16_be(frame, 2)
        qty = _u16_be(frame, 4)
        return DecodedFrame(
            raw_hex,
            f"{slave}",
            f"0x{function:02X}",
            function_name,
            "读请求",
            register_label(start),
            f"数量 {qty}",
            crc_display,
            "请求主控返回连续寄存器",
        )

    if function in (0x03, 0x04) and len(frame) >= 5:
        byte_count = frame[2]
        data = frame[3:-2]
        values = _register_values(data)
        note = "响应寄存器数据"
        if byte_count != len(data):
            note = f"字节数不匹配 标称:{byte_count} 实际:{len(data)}"
        return DecodedFrame(
            raw_hex,
            f"{slave}",
            f"0x{function:02X}",
            function_name,
            "读响应",
            "-",
            " ".join(f"0x{value:04X}({value})" for value in values),
            crc_display,
            note,
        )

    if function == 0x06 and len(frame) == 8:
        addr = _u16_be(frame, 2)
        value = _u16_be(frame, 4)
        hint = value_hint(addr, value)
        note = hint if hint else "写单寄存器"
        return DecodedFrame(
            raw_hex,
            f"{slave}",
            "0x06",
            function_name,
            "写单寄存器",
            register_label(addr),
            f"0x{value:04X} ({value})",
            crc_display,
            note,
        )

    if function == 0x10 and len(frame) == 8:
        start = _u16_be(frame, 2)
        qty = _u16_be(frame, 4)
        return DecodedFrame(
            raw_hex,
            f"{slave}",
            "0x10",
            function_name,
            "写多寄存器响应",
            register_label(start),
            f"数量 {qty}",
            crc_display,
            "从站确认写入范围",
        )

    if function == 0x10 and len(frame) >= 9:
        start = _u16_be(frame, 2)
        qty = _u16_be(frame, 4)
        byte_count = frame[6]
        data = frame[7:-2]
        if byte_count == len(data):
            values = _register_values(data)
            parts = []
            for index, value in enumerate(values):
                addr = start + index
                hint = value_hint(addr, value)
                if hint:
                    parts.append(f"{addr}:0x{value:04X}({hint})")
                else:
                    parts.append(f"{addr}:0x{value:04X}")
            note = "写多个寄存器请求"
            value_text = " ".join(parts)
            operation = "写多寄存器请求"
        else:
            note = f"字节数不匹配 标称:{byte_count} 实际:{len(data)}"
            value_text = format_hex(data)
            operation = "写多寄存器"
        return DecodedFrame(
            raw_hex,
            f"{slave}",
            "0x10",
            function_name,
            operation,
            register_label(start),
            value_text,
            crc_display,
            note,
        )

    if function in (0x05, 0x0F):
        return DecodedFrame(
            raw_hex,
            f"{slave}",
            f"0x{function:02X}",
            function_name,
            "线圈操作",
            "-",
            format_hex(frame[2:-2]),
            crc_display,
            "当前主要分析保持寄存器，线圈帧仅显示原始数据",
        )

    return DecodedFrame(
        raw_hex,
        f"{slave}",
        f"0x{function:02X}",
        function_name,
        "未细分",
        "-",
        format_hex(frame[2:-2]),
        crc_display,
        "未知或暂未实现的 Modbus 帧格式",
    )
