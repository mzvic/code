import serial

RS232 = serial.Serial(port='/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0', baudrate=2000000)

xor_STX =  (2)
xor_ADDR = (129)

xor_WIN = input("Enter WINDOW: ")
xor_WIN1 = ord(xor_WIN[0].encode()) 
xor_WIN2 = ord(xor_WIN[1].encode())
xor_WIN3 = ord(xor_WIN[2].encode())
xor_WR = ord(input("Enter R\W: ").encode())
xor_DATATYPE = input("Enter DATA TYPE (L/N/A): ").upper()
xor_OnOff = ord(input("Enter Off/On: ").encode())

xor_ETX = (3)

xor = hex(xor_ADDR ^ xor_WIN1 ^ xor_WIN2 ^ xor_WIN3 ^ xor_WR ^ xor_OnOff ^ xor_ETX)

STX = xor_STX.to_bytes(1, 'big')
ADDR = xor_ADDR.to_bytes(1, 'big')
WIN1 = xor_WIN1.to_bytes(1, 'big')
WIN2 = xor_WIN2.to_bytes(1, 'big')
WIN3 = xor_WIN3.to_bytes(1, 'big')
WR = xor_WR.to_bytes(1, 'big')

if xor_DATATYPE == "L":
    OnOff = xor_OnOff.to_bytes(1, 'big')
elif xor_DATATYPE == "N":
    OnOff = xor_OnOff.to_bytes(6, 'big')
elif xor_DATATYPE == "A":
    OnOff = xor_OnOff.to_bytes(10, 'big')

ETX = xor_ETX.to_bytes(1, 'big')

print(xor)

crc_1 = xor[2].upper().encode()
crc_2 = xor[3].upper().encode()



print("--------------------------------------------------------------------")
print("| STX | ADDRESS | WINDOW         | W/R  | ON-OFF | ETX | CRC       |")
print("|",hex(xor_STX),"|",hex(xor_ADDR),"   |",hex(xor_WIN1),hex(xor_WIN2),hex(xor_WIN3),"|",hex(xor_WR),"|",hex(xor_OnOff),"  |",hex(xor_ETX),"|",hex(int.from_bytes(crc_1, "big")),hex(int.from_bytes(crc_2, "big")),"|")
print("DataType: ", xor_DATATYPE)
print("--------------------------------------------------------------------")

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

while True:
    ANS = RS232.read()
    print(ANS)


