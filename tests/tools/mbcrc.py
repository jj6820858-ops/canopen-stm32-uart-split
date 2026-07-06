"""
Modbus CRC16 计算工具
用法:
  python tests/tools/mbcrc.py 01 06 00 0A 00 01             -> 输出完整带 CRC 的帧
  python tests/tools/mbcrc.py --check 01 06 00 0A 00 01 68 08 -> 验证 CRC 是否正确
"""

import sys

def modbus_crc(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc

def format_frame(data: bytes, crc: int) -> str:
    """按空格分隔的十六进制格式输出，并追加 CRC。"""
    crc_bytes = crc.to_bytes(2, 'little')
    hex_str = ' '.join(f'{b:02X}' for b in data)
    return f'{hex_str} {crc_bytes[0]:02X} {crc_bytes[1]:02X}'

def parse_hex(args):
    """把命令行十六进制参数解析为字节。"""
    hex_str = ''.join(args).replace(' ', '').replace('-', '')
    return bytes.fromhex(hex_str)

if __name__ == '__main__':
    args = sys.argv[1:]

    if not args:
        print('用法:')
        print('  python tools/mbcrc.py <hex数据>    计算CRC并输出完整帧')
        print('  python tools/mbcrc.py --check <帧>  验证CRC')
        print()
        print('示例:')
        print('  python tools/mbcrc.py 01 06 00 0A 00 01')
        print('  python tools/mbcrc.py 01 03 00 63 00 01')
        sys.exit(0)

    if args[0] == '--check':
        # 验证模式: 最后2字节是CRC
        frame = parse_hex(args[1:])
        if len(frame) < 3:
            print('帧太短')
            sys.exit(1)
        data = frame[:-2]
        wire_crc = frame[-2] | (frame[-1] << 8)
        calc_crc = modbus_crc(data)
        ok = wire_crc == calc_crc
        print(f'数据: {" ".join(f"{b:02X}" for b in data)}')
        print(f'线缆CRC: {frame[-2]:02X} {frame[-1]:02X}  (0x{wire_crc:04X})')
        print(f'计算CRC: {calc_crc.to_bytes(2,"little")[0]:02X} {calc_crc.to_bytes(2,"little")[1]:02X}  (0x{calc_crc:04X})')
        print(f'结果: {"✅ 正确" if ok else "❌ 错误"}')
        sys.exit(0 if ok else 1)

    # 计算模式
    data = parse_hex(args)
    crc = modbus_crc(data)
    full = format_frame(data, crc)

    print(f'数据: {" ".join(f"{b:02X}" for b in data)}')
    print(f'CRC:  {crc.to_bytes(2,"little")[0]:02X} {crc.to_bytes(2,"little")[1]:02X}')
    print(f'完整帧: {full}')
    print(f'长度: {len(data) + 2} 字节')
