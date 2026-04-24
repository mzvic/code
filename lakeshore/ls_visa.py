#!/usr/bin/env python3
import serial
import logging
import time

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

class LakeShore335Error(Exception):
    pass

class LakeShore335:
    SENSOR_TYPE_MAP = {
        0: "Disabled",
        1: "Diode",
        2: "Thermocouple",
        3: "RTD",
        4: "Thermistor",
        5: "Capacitance"
    }
    UNIT_MAP = {0: "K", 1: "C", 2: "F"}

    def __init__(self, port='/dev/ttyUSB0', baudrate=57600, timeout=2):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial = None
        self.connect()

    def connect(self):
        try:
            self.serial = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.SEVENBITS,      # 7 bits
                parity=serial.PARITY_ODD,       # Paridad impar
                stopbits=serial.STOPBITS_ONE,   # 1 bit de stop
                timeout=self.timeout,
                write_timeout=self.timeout
            )
            time.sleep(0.2)
            self.serial.reset_input_buffer()
            self.serial.reset_output_buffer()
            idn = self._query("*IDN?")
            logging.info(f"Conectado a {idn}")
        except Exception as e:
            raise LakeShore335Error(f"Conexión fallida: {e}")

    def disconnect(self):
        if self.serial and self.serial.is_open:
            self.serial.close()
            logging.info("Conexión cerrada")

    def _write(self, cmd):
        # Asegurar terminador LF (puede ser también CR+LF, probar ambos)
        if not cmd.endswith('\n'):
            cmd = cmd.rstrip() + '\n'
        self.serial.write(cmd.encode())

    def _read(self):
        raw = self.serial.readline()
        if raw:
            # Decodificar como ASCII (ignorar errores)
            return raw.decode('ascii', errors='ignore').strip()
        return ""

    def _query(self, cmd):
        self._write(cmd)
        return self._read()

    def is_connected(self):
        return bool(self._query("*IDN?"))

    def reset(self):
        self._write("*RST")
        time.sleep(0.2)

    def read_temperature_K(self, channel):
        self._validate_channel(channel)
        return float(self._query(f"KRDG? {channel}"))

    def read_temperature_C(self, channel):
        self._validate_channel(channel)
        return float(self._query(f"CRDG? {channel}"))

    def read_all_channels(self, unit="K"):
        cmd = "KRDG?" if unit == "K" else "CRDG?"
        return {ch: float(self._query(f"{cmd} {ch}")) for ch in ["A", "B", "C", "D"]}

    def get_input_config(self, channel):
        self._validate_channel(channel)
        intype_raw = self._query(f"INTYPE? {channel}")
        parts = [int(x) for x in intype_raw.split(',')]
        sensor_type = self.SENSOR_TYPE_MAP.get(parts[0], "Unknown")
        range_code = parts[2]
        unit_code = parts[3]
        curve = int(self._query(f"INCRV? {channel}"))
        filter_raw = self._query(f"FILTER? {channel}")
        fp = filter_raw.split(',')
        filter_cfg = {
            "enabled": bool(int(fp[0])),
            "points": int(fp[1]),
            "window": float(fp[2])
        }
        input_name = self._query(f"INNAME? {channel}")
        temp_limit = float(self._query(f"TLIMIT? {channel}"))
        return {
            "channel": channel,
            "sensor_type": sensor_type,
            "range": range_code,
            "curve": curve,
            "filter": filter_cfg,
            "input_name": input_name,
            "temperature_limit": temp_limit,
            "preferred_unit": self.UNIT_MAP.get(unit_code, "Unknown")
        }

    def sensor_status(self, channel):
        self._validate_channel(channel)
        return int(self._query(f"RDGST? {channel}"))

    @staticmethod
    def _validate_channel(channel):
        if channel not in ["A", "B", "C", "D"]:
            raise ValueError("Canal inválido. Use A, B, C o D.")

if __name__ == "__main__":
    ls = LakeShore335(port='/dev/ttyUSB0')
    try:
        print("Configuración canal A:")
        cfg = ls.get_input_config("A")
        for k, v in cfg.items():
            print(f"  {k}: {v}")
        print("\nTemperaturas:")
        temps = ls.read_all_channels(unit="K")
        for ch, t in temps.items():
            print(f"  Canal {ch}: {t:.3f} K")
    finally:
        ls.disconnect()
