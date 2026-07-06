import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from modbus_codec import append_crc, crc16_modbus, decode_frame, parse_hex_string


class ModbusCodecTest(unittest.TestCase):
    def test_crc16_known_read_frame(self):
        data = bytes.fromhex("01 03 00 00 00 02")
        self.assertEqual(crc16_modbus(data), 0x0BC4)
        self.assertEqual(append_crc(data).hex(" ").upper(), "01 03 00 00 00 02 C4 0B")

    def test_parse_hex_string_accepts_compact_and_spaced(self):
        self.assertEqual(parse_hex_string("010300000002"), bytes.fromhex("01 03 00 00 00 02"))
        self.assertEqual(parse_hex_string("01 03 00 00 00 02"), bytes.fromhex("01 03 00 00 00 02"))

    def test_decode_write_single_register(self):
        frame = append_crc(bytes.fromhex("01 06 00 0B 90 00"))
        decoded = decode_frame(frame)
        self.assertEqual(decoded.slave, "1")
        self.assertEqual(decoded.function, "0x06")
        self.assertEqual(decoded.operation, "写单寄存器")
        self.assertIn("转盘", decoded.register)
        self.assertIn("0x9000", decoded.values)
        self.assertEqual(decoded.crc, "正确")
        self.assertIn("启动", decoded.note)

    def test_decode_write_multiple_registers(self):
        frame = append_crc(bytes.fromhex("01 10 00 0C 00 03 06 80 00 00 00 00 00"))
        decoded = decode_frame(frame)
        self.assertEqual(decoded.operation, "写多寄存器请求")
        self.assertIn("12:0x8000", decoded.values)
        self.assertEqual(decoded.crc, "正确")

    def test_decode_write_multiple_registers_response(self):
        frame = append_crc(bytes.fromhex("01 10 00 0C 00 03"))
        decoded = decode_frame(frame)
        self.assertEqual(decoded.operation, "写多寄存器响应")
        self.assertIn("数量 3", decoded.values)
        self.assertEqual(decoded.crc, "正确")


if __name__ == "__main__":
    unittest.main()
