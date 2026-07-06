import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from modbus_codec import append_crc
from modbus_host import analyze_probe_response, build_read_request, port_display_name, port_sort_key


class ModbusHostPortTest(unittest.TestCase):
    def test_port_display_name_includes_description(self):
        self.assertEqual(
            port_display_name("COM10", "蓝牙链接上的标准串行 (COM10)"),
            "COM10 - 蓝牙链接上的标准串行 (COM10)",
        )

    def test_port_sort_key_sorts_com_numbers_naturally(self):
        ports = ["COM10", "COM9", "COM2"]
        self.assertEqual(sorted(ports, key=port_sort_key), ["COM2", "COM9", "COM10"])

    def test_build_read_request_uses_safe_status_registers(self):
        expected = append_crc(bytes.fromhex("01 03 00 11 00 02"))
        self.assertEqual(build_read_request(1), expected)

    def test_analyze_probe_response_accepts_matching_read_response(self):
        frame = append_crc(bytes.fromhex("01 03 04 00 01 00 02"))
        matched, online, message = analyze_probe_response(frame, 1, 0x03, 2)
        self.assertTrue(matched)
        self.assertTrue(online)
        self.assertIn("在线", message)

    def test_analyze_probe_response_rejects_crc_error(self):
        frame = bytearray(append_crc(bytes.fromhex("01 03 04 00 01 00 02")))
        frame[-1] ^= 0xFF
        matched, online, message = analyze_probe_response(bytes(frame), 1, 0x03, 2)
        self.assertTrue(matched)
        self.assertFalse(online)
        self.assertIn("CRC", message)

    def test_analyze_probe_response_ignores_unrelated_slave(self):
        frame = append_crc(bytes.fromhex("02 03 04 00 01 00 02"))
        matched, online, message = analyze_probe_response(frame, 1, 0x03, 2)
        self.assertFalse(matched)
        self.assertFalse(online)
        self.assertEqual(message, "")


if __name__ == "__main__":
    unittest.main()
