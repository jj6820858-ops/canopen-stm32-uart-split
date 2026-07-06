from __future__ import annotations

import csv
from datetime import datetime
import queue
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

try:
    import serial
    from serial.tools import list_ports
except Exception:  # pragma: no cover - GUI shows the install hint.
    serial = None
    list_ports = None

from modbus_codec import append_crc, decode_frame, parse_hex_string


DEFAULT_BAUDRATE = 115200
DEFAULT_FRAME_GAP_MS = 20


class SerialReader(threading.Thread):
    def __init__(self, port: str, baudrate: int, frame_gap_ms: int, out_queue: queue.Queue):
        super().__init__(daemon=True)
        self.port = port
        self.baudrate = baudrate
        self.frame_gap_ms = frame_gap_ms
        self.out_queue = out_queue
        self._stop_event = threading.Event()
        self._write_lock = threading.Lock()
        self._serial = None

    def run(self) -> None:
        if serial is None:
            self.out_queue.put(("status", "未安装 pyserial，无法打开串口"))
            return

        try:
            self._serial = serial.Serial(
                self.port,
                self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.02,
                write_timeout=1.0,
            )
        except Exception as exc:
            self.out_queue.put(("status", f"打开串口失败: {exc}"))
            return

        self.out_queue.put(("status", f"已连接 {self.port} @ {self.baudrate}"))
        buffer = bytearray()
        last_rx_time = 0.0

        while not self._stop_event.is_set():
            try:
                waiting = self._serial.in_waiting
                data = self._serial.read(waiting or 1)
            except Exception as exc:
                self.out_queue.put(("status", f"读取串口失败: {exc}"))
                break

            now = time.monotonic()
            if data:
                buffer.extend(data)
                last_rx_time = now
                continue

            if buffer and (now - last_rx_time) * 1000 >= self.frame_gap_ms:
                self.out_queue.put(("frame", "RX", bytes(buffer)))
                buffer.clear()

        if buffer:
            self.out_queue.put(("frame", "RX", bytes(buffer)))
        self._close()
        self.out_queue.put(("status", "串口已断开"))

    def write(self, data: bytes) -> None:
        if self._serial is None or not self._serial.is_open:
            raise RuntimeError("串口未连接")
        with self._write_lock:
            self._serial.write(data)
            self._serial.flush()

    def stop(self) -> None:
        self._stop_event.set()

    def _close(self) -> None:
        if self._serial is None:
            return
        try:
            self._serial.close()
        except Exception:
            pass


class ModbusHostApp:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Modbus RTU 实时分析上位机")
        self.root.geometry("1180x720")
        self.root.minsize(960, 560)

        self.events: queue.Queue = queue.Queue()
        self.reader: SerialReader | None = None
        self.rows: list[dict[str, str]] = []

        self.port_var = tk.StringVar()
        self.baud_var = tk.StringVar(value=str(DEFAULT_BAUDRATE))
        self.gap_var = tk.StringVar(value=str(DEFAULT_FRAME_GAP_MS))
        self.append_crc_var = tk.BooleanVar(value=True)
        self.status_var = tk.StringVar(value="未连接")
        self.send_var = tk.StringVar()

        self._build_ui()
        self.refresh_ports()
        self.root.after(50, self._poll_events)
        self.root.protocol("WM_DELETE_WINDOW", self.on_close)

    def _build_ui(self) -> None:
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(1, weight=1)

        toolbar = ttk.Frame(self.root, padding=(10, 8))
        toolbar.grid(row=0, column=0, sticky="ew")
        toolbar.columnconfigure(1, weight=1)

        ttk.Label(toolbar, text="串口").grid(row=0, column=0, sticky="w")
        self.port_combo = ttk.Combobox(toolbar, textvariable=self.port_var, width=18, state="readonly")
        self.port_combo.grid(row=0, column=1, sticky="w", padx=(6, 12))

        ttk.Button(toolbar, text="刷新", command=self.refresh_ports).grid(row=0, column=2, padx=(0, 12))

        ttk.Label(toolbar, text="波特率").grid(row=0, column=3)
        baud_combo = ttk.Combobox(
            toolbar,
            textvariable=self.baud_var,
            width=10,
            values=("9600", "19200", "38400", "57600", "115200", "230400"),
        )
        baud_combo.grid(row=0, column=4, padx=(6, 12))

        ttk.Label(toolbar, text="分帧间隔ms").grid(row=0, column=5)
        ttk.Spinbox(toolbar, from_=2, to=200, textvariable=self.gap_var, width=7).grid(row=0, column=6, padx=(6, 12))

        self.connect_button = ttk.Button(toolbar, text="连接", command=self.toggle_connection)
        self.connect_button.grid(row=0, column=7, padx=(0, 8))
        ttk.Button(toolbar, text="清空", command=self.clear_log).grid(row=0, column=8, padx=(0, 8))
        ttk.Button(toolbar, text="保存CSV", command=self.save_csv).grid(row=0, column=9)

        main = ttk.PanedWindow(self.root, orient=tk.VERTICAL)
        main.grid(row=1, column=0, sticky="nsew", padx=10, pady=(0, 8))

        table_frame = ttk.Frame(main)
        table_frame.rowconfigure(0, weight=1)
        table_frame.columnconfigure(0, weight=1)
        main.add(table_frame, weight=4)

        columns = ("time", "dir", "raw", "slave", "func", "op", "reg", "values", "crc", "note")
        self.tree = ttk.Treeview(table_frame, columns=columns, show="headings", height=16)
        headings = {
            "time": "时间",
            "dir": "方向",
            "raw": "原始HEX",
            "slave": "站号",
            "func": "功能码",
            "op": "操作",
            "reg": "寄存器",
            "values": "值",
            "crc": "CRC",
            "note": "说明",
        }
        widths = {
            "time": 95,
            "dir": 54,
            "raw": 250,
            "slave": 54,
            "func": 70,
            "op": 110,
            "reg": 210,
            "values": 220,
            "crc": 110,
            "note": 220,
        }
        for column in columns:
            self.tree.heading(column, text=headings[column])
            self.tree.column(column, width=widths[column], minwidth=50, stretch=column in {"raw", "reg", "values", "note"})

        y_scroll = ttk.Scrollbar(table_frame, orient=tk.VERTICAL, command=self.tree.yview)
        x_scroll = ttk.Scrollbar(table_frame, orient=tk.HORIZONTAL, command=self.tree.xview)
        self.tree.configure(yscrollcommand=y_scroll.set, xscrollcommand=x_scroll.set)
        self.tree.grid(row=0, column=0, sticky="nsew")
        y_scroll.grid(row=0, column=1, sticky="ns")
        x_scroll.grid(row=1, column=0, sticky="ew")
        self.tree.bind("<<TreeviewSelect>>", self.show_selected_detail)

        detail_frame = ttk.Frame(main, padding=(0, 8, 0, 0))
        detail_frame.rowconfigure(0, weight=1)
        detail_frame.columnconfigure(0, weight=1)
        main.add(detail_frame, weight=1)

        self.detail = tk.Text(detail_frame, height=6, wrap="word", state="disabled")
        self.detail.grid(row=0, column=0, sticky="nsew")
        detail_scroll = ttk.Scrollbar(detail_frame, orient=tk.VERTICAL, command=self.detail.yview)
        self.detail.configure(yscrollcommand=detail_scroll.set)
        detail_scroll.grid(row=0, column=1, sticky="ns")

        send_bar = ttk.Frame(self.root, padding=(10, 0, 10, 8))
        send_bar.grid(row=2, column=0, sticky="ew")
        send_bar.columnconfigure(1, weight=1)
        ttk.Label(send_bar, text="发送HEX").grid(row=0, column=0, sticky="w")
        ttk.Entry(send_bar, textvariable=self.send_var).grid(row=0, column=1, sticky="ew", padx=(6, 8))
        ttk.Checkbutton(send_bar, text="自动追加CRC", variable=self.append_crc_var).grid(row=0, column=2, padx=(0, 8))
        ttk.Button(send_bar, text="发送", command=self.send_hex).grid(row=0, column=3, padx=(0, 12))
        ttk.Label(send_bar, textvariable=self.status_var).grid(row=0, column=4, sticky="e")

    def refresh_ports(self) -> None:
        ports = []
        if list_ports is not None:
            ports = [item.device for item in list_ports.comports()]
        if not ports:
            ports = [f"COM{i}" for i in range(1, 33)]
        self.port_combo["values"] = ports
        if not self.port_var.get() or self.port_var.get() not in ports:
            self.port_var.set(ports[0])

    def toggle_connection(self) -> None:
        if self.reader is not None:
            self.disconnect()
            return
        self.connect()

    def connect(self) -> None:
        if serial is None:
            messagebox.showerror("缺少依赖", "未安装 pyserial，请先运行: python -m pip install pyserial")
            return

        port = self.port_var.get().strip()
        if not port:
            messagebox.showwarning("请选择串口", "先选择一个 COM 口")
            return

        try:
            baudrate = int(self.baud_var.get())
            gap_ms = int(self.gap_var.get())
        except ValueError:
            messagebox.showwarning("参数错误", "波特率和分帧间隔必须是数字")
            return

        self.reader = SerialReader(port, baudrate, gap_ms, self.events)
        self.reader.start()
        self.connect_button.configure(text="断开")
        self.status_var.set("正在连接...")

    def disconnect(self) -> None:
        if self.reader is not None:
            self.reader.stop()
            self.reader = None
        self.connect_button.configure(text="连接")
        self.status_var.set("正在断开...")

    def send_hex(self) -> None:
        if self.reader is None:
            messagebox.showwarning("未连接", "请先连接串口")
            return
        try:
            data = parse_hex_string(self.send_var.get())
            if self.append_crc_var.get():
                data = append_crc(data)
            self.reader.write(data)
        except Exception as exc:
            messagebox.showerror("发送失败", str(exc))
            return
        self._add_frame("TX", data)

    def _poll_events(self) -> None:
        while True:
            try:
                event = self.events.get_nowait()
            except queue.Empty:
                break

            kind = event[0]
            if kind == "frame":
                _, direction, data = event
                self._add_frame(direction, data)
            elif kind == "status":
                self.status_var.set(event[1])
                if event[1].startswith("串口已断开") or event[1].startswith("打开串口失败"):
                    self.reader = None
                    self.connect_button.configure(text="连接")

        self.root.after(50, self._poll_events)

    def _add_frame(self, direction: str, data: bytes) -> None:
        decoded = decode_frame(data)
        row = {
            "time": datetime.now().strftime("%H:%M:%S.%f")[:-3],
            "dir": direction,
            "raw": decoded.raw_hex,
            "slave": decoded.slave,
            "func": f"{decoded.function} {decoded.function_name}",
            "op": decoded.operation,
            "reg": decoded.register,
            "values": decoded.values,
            "crc": decoded.crc,
            "note": decoded.note,
        }
        self.rows.append(row)
        item_id = self.tree.insert("", tk.END, values=tuple(row[key] for key in row))
        self.tree.see(item_id)
        self.tree.selection_set(item_id)

    def show_selected_detail(self, _event=None) -> None:
        selected = self.tree.selection()
        if not selected:
            return
        values = self.tree.item(selected[0], "values")
        labels = ("时间", "方向", "原始HEX", "站号", "功能码", "操作", "寄存器", "值", "CRC", "说明")
        text = "\n".join(f"{label}: {value}" for label, value in zip(labels, values))
        self.detail.configure(state="normal")
        self.detail.delete("1.0", tk.END)
        self.detail.insert(tk.END, text)
        self.detail.configure(state="disabled")

    def clear_log(self) -> None:
        self.rows.clear()
        for item in self.tree.get_children():
            self.tree.delete(item)
        self.detail.configure(state="normal")
        self.detail.delete("1.0", tk.END)
        self.detail.configure(state="disabled")

    def save_csv(self) -> None:
        if not self.rows:
            messagebox.showinfo("没有数据", "当前没有可保存的抓包数据")
            return
        filename = filedialog.asksaveasfilename(
            defaultextension=".csv",
            filetypes=(("CSV 文件", "*.csv"), ("所有文件", "*.*")),
            initialfile=f"modbus_capture_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv",
        )
        if not filename:
            return
        with open(filename, "w", newline="", encoding="utf-8-sig") as file:
            writer = csv.DictWriter(file, fieldnames=list(self.rows[0].keys()))
            writer.writeheader()
            writer.writerows(self.rows)
        self.status_var.set(f"已保存 {filename}")

    def on_close(self) -> None:
        if self.reader is not None:
            self.reader.stop()
        self.root.destroy()


def main() -> None:
    root = tk.Tk()
    try:
        style = ttk.Style()
        if "vista" in style.theme_names():
            style.theme_use("vista")
    except Exception:
        pass
    ModbusHostApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
