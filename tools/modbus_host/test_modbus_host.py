import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from modbus_host import port_display_name, port_sort_key


class ModbusHostPortTest(unittest.TestCase):
    def test_port_display_name_includes_description(self):
        self.assertEqual(
            port_display_name("COM10", "蓝牙链接上的标准串行 (COM10)"),
            "COM10 - 蓝牙链接上的标准串行 (COM10)",
        )

    def test_port_sort_key_sorts_com_numbers_naturally(self):
        ports = ["COM10", "COM9", "COM2"]
        self.assertEqual(sorted(ports, key=port_sort_key), ["COM2", "COM9", "COM10"])


if __name__ == "__main__":
    unittest.main()
