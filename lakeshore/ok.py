#!/usr/bin/env python3
import serial
import time

PORT = '/dev/ttyUSB0'
BAUDRATES = [9600, 19200, 38400, 57600, 115200, 4800, 2400]
TERMINATORS = [b'\r\n', b'\n', b'\r']

def test_baud_and_terminator(port, baudrate, term, timeout=1):
    try:
        ser = serial.Serial(
            port=port,
            baudrate=baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=timeout,
            write_timeout=timeout
        )
        time.sleep(0.2)  # esperar estabilización
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        
        cmd = b'*IDN?' + term
        print(f"     Enviando: {cmd}")
        ser.write(cmd)
        
        raw = ser.read(64)  # leer hasta 64 bytes
        ser.close()
        return raw
    except Exception as e:
        return f"ERROR: {e}"

def main():
    print(f"Probando en puerto {PORT}\n")
    for baud in BAUDRATES:
        print(f"\n📡 Velocidad: {baud} baudios")
        for term in TERMINATORS:
            print(f"   Terminador: {repr(term)}")
            respuesta = test_baud_and_terminator(PORT, baud, term)
            if isinstance(respuesta, bytes):
                if respuesta:
                    print(f"      → Recibidos {len(respuesta)} bytes: {respuesta[:50]}")
                    try:
                        dec = respuesta.decode('ascii')
                        print(f"      → ASCII: {repr(dec)}")
                    except:
                        print(f"      → Hex: {respuesta.hex()}")
                else:
                    print("      → Sin datos recibidos")
            else:
                print(f"      → Error: {respuesta}")

if __name__ == "__main__":
    main()
