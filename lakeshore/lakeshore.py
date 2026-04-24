#!/usr/bin/env python3
"""
(C) 2025 - Adaptado para Lake Shore Model 335 via USB (serial)

Driver para comunicación con Lake Shore Model 335 Temperature Controller
a través de USB (puerto serie). Los comandos son los mismos que para el Model 350,
pero la capa de transporte es serial.

- Transport: Serial (USB)
- Baudrate: 57600 (por defecto, verificar en el instrumento)
- Parity: None
- Stop bits: 1
- Terminator: \\n

Referencia: Manual Lake Shore Model 335.
"""

import serial
import serial.tools.list_ports
import time
import logging

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

class LakeShore335Error(Exception):
    pass

class LakeShore335:
    """
    Clase para controlar un Lake Shore Model 335 a través de USB (serie).
    """

    SENSOR_TYPE_MAP = {
        0: "Disabled",
        1: "Diode",
        2: "Thermocouple",
        3: "RTD",
        4: "Thermistor",
        5: "Capacitance"
    }

    UNIT_MAP = {
        0: "K",
        1: "C",
        2: "F"
    }

    def __init__(self, port=None, baudrate=57600, timeout=2):
        """
        Inicializa la conexión serie con el Lake Shore 335.

        Args:
            port: Puerto serie (ej. '/dev/ttyUSB0'). Si es None, busca automáticamente
                  un dispositivo con 'Model_335' en la descripción.
            baudrate: Velocidad de comunicación (por defecto 57600).
            timeout: Timeout de lectura/escritura en segundos.
        """
        if port is None:
            port = self._auto_detect_port()
            if port is None:
                raise LakeShore335Error("No se encontró un Lake Shore 335 conectado por USB.")

        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None
        self.connect()

    @staticmethod
    def _auto_detect_port():
        """Busca automáticamente el puerto serie correspondiente a un Lake Shore 335."""
        ports = serial.tools.list_ports.comports()
        for p in ports:
            # Buscar por descripción que contenga "Model_335" o "Lake Shore"
            if "Model_335" in p.description or "Lake Shore" in p.description:
                logging.info(f"Dispositivo encontrado: {p.device} - {p.description}")
                return p.device
            # También puede aparecer como "Silicon Labs" genérico, entonces verificamos por ID
            if p.vid == 0x10C4 and p.pid == 0xEA60:  # Silicon Labs CP210x típico
                logging.info(f"Posible Lake Shore 335 en {p.device} (por VID/PID genérico)")
                return p.device
        logging.warning("No se detectó ningún Lake Shore 335. Asegúrese de que esté conectado y encendido.")
        return None

    def connect(self):
        """Abre la conexión serie y verifica la comunicación."""
        try:
            logging.info(f"Conectando a Lake Shore 335 en {self.port} @ {self.baudrate} baudios")
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=self.timeout,
                write_timeout=self.timeout
            )
            # Limpiar buffers
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
        except Exception as e:
            raise LakeShore335Error(f"Error abriendo puerto {self.port}: {e}")

        # Verificar identificación
        idn = self.is_connected()
        logging.info(f"Conexión establecida: {idn}")

    def disconnect(self):
        """Cierra la conexión serie."""
        try:
            if self.ser and self.ser.is_open:
                logging.info("Cerrando conexión con Lake Shore 335")
                self.ser.close()
        except Exception as e:
            raise LakeShore335Error(f"Error al desconectar: {e}")

    def _write(self, command):
        """Envía un comando al instrumento (añade terminador \\n)."""
        try:
            cmd = command + "\n"
            self.ser.write(cmd.encode())
            logging.debug(f"Write: {repr(cmd)}")
        except Exception as e:
            logging.error(f"Write failed: {command} -> {e}")
            raise LakeShore335Error(f"Error escribiendo: {e}")

    def _read(self):
        """Lee una línea de respuesta (hasta \\n)."""
        try:
            line = self.ser.readline().decode().strip()
            logging.debug(f"Read: {line}")
            return line
        except Exception as e:
            logging.error(f"Read failed: {e}")
            raise LakeShore335Error(f"Error leyendo: {e}")

    def _query(self, command):
        """Envía un comando y retorna la respuesta."""
        with self.ser.lock:  # Aunque no es multihilo, por si acaso
            self._write(command)
            return self._read()

    def is_connected(self):
        """Verifica la comunicación mediante *IDN?."""
        try:
            respuesta = self._query("*IDN?")
            if not respuesta:
                raise LakeShore335Error("Respuesta vacía")
            return respuesta
        except Exception as e:
            raise LakeShore335Error(f"Instrumento no responde: {e}")

    def reset(self):
        """Resetea el instrumento (IEEE-488.2 *RST)."""
        self._write("*RST")
        time.sleep(0.2)

    def read_temperature_K(self, channel):
        """
        Lee temperatura en Kelvin de un canal (A, B, C o D).
        Nota: El Model 335 tiene hasta 4 canales de entrada (A, B, C, D).
        """
        self._validate_channel(channel)
        value = self._query(f"KRDG? {channel}")
        return float(value)

    def read_temperature_C(self, channel):
        """Lee temperatura en Celsius."""
        self._validate_channel(channel)
        value = self._query(f"CRDG? {channel}")
        return float(value)

    def read_all_channels(self, unit="K"):
        """Retorna diccionario con lecturas de todos los canales (A-D)."""
        cmd = "KRDG?" if unit == "K" else "CRDG?"
        data = {}
        for ch in ["A", "B", "C", "D"]:
            try:
                data[ch] = float(self._query(f"{cmd} {ch}"))
            except Exception as e:
                logging.warning(f"No se pudo leer canal {ch}: {e}")
                data[ch] = None
        return data

    def get_input_config(self, channel):
        """
        Obtiene la configuración completa de un canal de entrada.

        Retorna un diccionario con:
        - sensor_type (str)
        - range (int)
        - curve (int)
        - filter (dict)
        - input_name (str)
        - temperature_limit (float)
        - preferred_unit (str)
        """
        self._validate_channel(channel)

        # INTYPE? <channel> devuelve: <type>,<units>,<range>,<resistance range?>
        # Para Model 335, el formato puede variar ligeramente, pero asumimos el mismo.
        intype_raw = self._query(f"INTYPE? {channel}")
        # Ejemplo: "1,0,2,0" -> sensor_type=1, units=0, range=2, ...
        parts = intype_raw.split(',')
        if len(parts) >= 4:
            sensor_type_code = int(parts[0])
            unit_code = int(parts[1])
            range_code = int(parts[2])
        else:
            sensor_type_code = 0
            unit_code = 0
            range_code = 0

        sensor_type = self.SENSOR_TYPE_MAP.get(sensor_type_code, "Unknown")
        preferred_unit = self.UNIT_MAP.get(unit_code, "Unknown")

        # Curva asociada (INCRV?)
        curve = int(self._query(f"INCRV? {channel}"))

        # Filtro (FILTER?)
        filter_raw = self._query(f"FILTER? {channel}")
        # Formato: <state>,<points>,<window>
        filter_parts = filter_raw.split(',')
        filter_cfg = {
            "enabled": bool(int(filter_parts[0])) if len(filter_parts) > 0 else False,
            "points": int(filter_parts[1]) if len(filter_parts) > 1 else 0,
            "window": float(filter_parts[2]) if len(filter_parts) > 2 else 0.0
        }

        # Nombre del canal (INNAME?)
        input_name = self._query(f"INNAME? {channel}")

        # Límite de temperatura (TLIMIT?)
        temp_limit = float(self._query(f"TLIMIT? {channel}"))

        return {
            "channel": channel,
            "sensor_type": sensor_type,
            "range": range_code,
            "curve": curve,
            "filter": filter_cfg,
            "input_name": input_name,
            "temperature_limit": temp_limit,
            "preferred_unit": preferred_unit
        }

    def sensor_status(self, channel):
        """
        Retorna estado del sensor:
        0 = OK
        1 = Open
        2 = Short
        """
        self._validate_channel(channel)
        return int(self._query(f"RDGST? {channel}"))

    @staticmethod
    def _validate_channel(channel):
        if channel not in ["A", "B", "C", "D"]:
            raise ValueError("Canal inválido. Use A, B, C o D.")

# Ejemplo de uso
if __name__ == "__main__":
    # Intentar conexión automática
    try:
        ls = LakeShore335()  # Busca automáticamente el puerto
        # Si quieres especificar el puerto manualmente: LakeShore335(port='/dev/ttyUSB0')

        print("Configuración del canal A:")
        cfg_a = ls.get_input_config("A")
        for k, v in cfg_a.items():
            print(f"  {k}: {v}")

        print("\nConfiguración del canal B:")
        cfg_b = ls.get_input_config("B")
        for k, v in cfg_b.items():
            print(f"  {k}: {v}")

        print("\nTemperaturas en Kelvin:")
        temps = ls.read_all_channels(unit="K")
        for ch, t in temps.items():
            if t is not None:
                print(f"  Canal {ch}: {t:.3f} K")
            else:
                print(f"  Canal {ch}: sin lectura")

        # Lectura individual
        temp_a = ls.read_temperature_K("A")
        print(f"\nTemperatura canal A: {temp_a:.3f} K")

    except LakeShore335Error as e:
        print(f"Error: {e}")
    finally:
        if 'ls' in locals():
            ls.disconnect()
