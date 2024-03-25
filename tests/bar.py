import serial
import time

RS485 = serial.Serial(port='/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0', baudrate=9600)

STx = (35).to_bytes(1, 'big')
_0 = (48).to_bytes(1, 'big')
_1 = (49).to_bytes(1, 'big')
_2 = (50).to_bytes(1, 'big')
_3 = (51).to_bytes(1, 'big')
_4 = (52).to_bytes(1, 'big')
_5 = (53).to_bytes(1, 'big')
_6 = (54).to_bytes(1, 'big')
_7 = (55).to_bytes(1, 'big')
_8 = (56).to_bytes(1, 'big')
_9 = (57).to_bytes(1, 'big')
_A = (65).to_bytes(1, 'big')
_U = (85).to_bytes(1, 'big')
_X = (88).to_bytes(1, 'big')
_I = (73).to_bytes(1, 'big')
_F = (70).to_bytes(1, 'big')
_T = (84).to_bytes(1, 'big')
_E = (69).to_bytes(1, 'big')
_CR = (13).to_bytes(1, 'big')

#65 85 88 49
while(True):
    RS485.write(STx)
    RS485.write(_0)
    RS485.write(_0)
    #RS485.write(_0)
    #RS485.write(_F)    
    #RS485.write(_5)
    #RS485.write(_E)
    #RS485.write(_1)
    #RS485.write(_1)
    
    #RS485.write(_5)
    #RS485.write(_F)
    #RS485.write(_1)
    
    RS485.write(_0)
    RS485.write(_F)
    
    #RS485.write(_A)
    #RS485.write(_1)        
    #RS485.write(_A)
    #RS485.write(_U)
    #RS485.write(_X)
    #RS485.write(_2)
    
    RS485.write(_CR)

    print("XGS600 response: ", end='\n') 

    response = b''
    while True:
        ANS = RS485.read()
        if ANS == b'\r':
            break
        response += ANS

    response_str = response.decode('utf-8')

    start_index = response_str.find('>')
    comma_index = response_str.find(',')

    AUX1 = response_str[start_index + 1 : comma_index]
    AUX2 = response_str[comma_index + 1 :]

    print("AUX1:", AUX1)
    print("AUX2:", AUX2)
    time.sleep(1)
    print()
