import serial

RS232 = serial.Serial(port='/dev/serial/by-id/usb-1a86_USB2.0-Ser_-if00-port0', baudrate=9600)

def calculate_crc(data):
    CRC = 0xFFFF
    for byte in data:
        CRC ^= byte
        for _ in range(8):
            carry = CRC & 1
            CRC >>= 1
            if carry:
                CRC ^= 0xA001
    return CRC.to_bytes(2, 'big')

def read_response():
    A = RS232.read()
    B = RS232.read()
    address = int.from_bytes(A, 'big')
    function_code = int.from_bytes(B, 'big')
    C = int.from_bytes(RS232.read(), 'big')
    data = RS232.read(C)
    CRC_received = RS232.read(2)
    return address, function_code, C, data

strt_mem = int(input("First word address: "))  

chr3 = (strt_mem >> 8) & 0xFF
chr4 = strt_mem & 0xFF

chr0 = 0
chr1 = 1 # address
chr2 = 3 # read

win_amount = int(input("Amount of windows to read: ")) 

chr5 = (win_amount >> 8) & 0xFF
chr6 = win_amount & 0xFF

data = [chr1, chr2, chr3, chr4, chr5, chr6]
crc1, crc2 = calculate_crc(data)

Chr0 = chr0.to_bytes(3, 'big')
Chr1 = chr1.to_bytes(1, 'big')
Chr2 = chr2.to_bytes(1, 'big')
Chr3 = chr3.to_bytes(1, 'big')
Chr4 = chr4.to_bytes(1, 'big')
Chr5 = chr5.to_bytes(1, 'big')
Chr6 = chr6.to_bytes(1, 'big')
CRC1 = crc1.to_bytes(1, 'big')
CRC2 = crc2.to_bytes(1, 'big')

print(Chr3, Chr4, Chr5, Chr6) 

RS232.write(Chr1)
RS232.write(Chr2)
RS232.write(Chr3)
RS232.write(Chr4)
RS232.write(Chr5)
RS232.write(Chr6)
RS232.write(CRC2)
RS232.write(CRC1)

response = read_response()

if response:
    address, function_code, C, data = response
    print(f"Address: {address}")
    print(f"Function Code: {function_code}")
    print(f"Data Length: {C}")
    print(f"Data: {data}")
else:
    print("Error al leer la respuesta.")

RS232.close()

