import serial

RS232 = serial.Serial(port='/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0', baudrate=9600)

xor_STX = 2
xor_ADDR = 128
xor_WIN1 = 48 
xor_WIN2 = 48
xor_WIN3 = 56
xor_WR = 49
xor_OnOff = 48
xor_ETX = 3

xor = hex(xor_ADDR ^ xor_WIN1 ^ xor_WIN2 ^ xor_WIN3 ^ xor_WR ^ xor_OnOff ^ xor_ETX)

STX = xor_STX.to_bytes(1, 'big')
ADDR = xor_ADDR.to_bytes(1, 'big')
WIN1 = xor_WIN1.to_bytes(1, 'big')
WIN2 = xor_WIN2.to_bytes(1, 'big')
WIN3 = xor_WIN3.to_bytes(1, 'big')
WR = xor_WR.to_bytes(1, 'big')
OnOff = xor_OnOff.to_bytes(1, 'big')
ETX = xor_ETX.to_bytes(1, 'big')

crc_1 = xor[2].upper().encode()
crc_2 = xor[3].upper().encode()

RS232.write(STX)
RS232.write(ADDR)
RS232.write(WIN1)
RS232.write(WIN2)
RS232.write(WIN3)
RS232.write(WR)
RS232.write(OnOff)
RS232.write(ETX)
RS232.write(crc_1)
RS232.write(crc_2)

ETX = False
ETX_count = 0

while True:
    ANS = RS232.read()
    print(ANS, end='')  

    if ETX:
        ETX_count = ETX_count + 1
        if ETX_count >= 2:
            print("...")
            break
    elif ANS == b'\x03':
        ETX = True

