import sys
import serial
from PyQt5.QtWidgets import QApplication, QMessageBox
from PyQt5 import QtGui, QtCore, QtWidgets
from PyQt5.QtGui import QFont, QImage, QPixmap

def pumpNotFound(): # Function to handle if 305 pump is not found
    msg = QMessageBox()
    msg.setWindowTitle("CoDE Warning")
    msg.setText("The TwisTorr 305 FS pump driver has not been found, if you want to use it, then connect it to the RS485 adapter, check that it is powered on and restart the GUI...")
    msg.setIcon(QMessageBox.Critical)
    msg.exec_()
    raise PumpNotFoundException("The TwisTorr 305 FS pump driver has not been found, if you want to use it, then connect it to the RS485 adapter, check that it is powered on and restart the GUI...")

def apply_dark_theme(app): # GUI Setup
    dark_palette = QtGui.QPalette()
    dark_palette.setColor(QtGui.QPalette.Window, QtGui.QColor(53, 53, 53))
    dark_palette.setColor(QtGui.QPalette.WindowText, QtCore.Qt.white)
    dark_palette.setColor(QtGui.QPalette.Base, QtGui.QColor(25, 25, 25))
    dark_palette.setColor(QtGui.QPalette.AlternateBase, QtGui.QColor(53, 53, 53))
    dark_palette.setColor(QtGui.QPalette.ToolTipBase, QtCore.Qt.white)
    dark_palette.setColor(QtGui.QPalette.ToolTipText, QtCore.Qt.white)
    dark_palette.setColor(QtGui.QPalette.Text, QtCore.Qt.white)
    dark_palette.setColor(QtGui.QPalette.Button, QtGui.QColor(53, 53, 53))
    dark_palette.setColor(QtGui.QPalette.ButtonText, QtCore.Qt.white)
    dark_palette.setColor(QtGui.QPalette.BrightText, QtCore.Qt.red)
    dark_palette.setColor(QtGui.QPalette.Link, QtGui.QColor(42, 130, 218))
    dark_palette.setColor(QtGui.QPalette.LinkVisited, QtGui.QColor(42, 130, 218))
    dark_palette.setColor(QtGui.QPalette.Highlight, QtGui.QColor(42, 130, 218))
    dark_palette.setColor(QtGui.QPalette.HighlightedText, QtCore.Qt.black)
    dark_palette.setColor(QtGui.QPalette.Disabled, QtGui.QPalette.Text, QtGui.QColor(127, 127, 127))
    dark_palette.setColor(QtGui.QPalette.Disabled, QtGui.QPalette.ButtonText, QtGui.QColor(127, 127, 127))
    dark_palette.setColor(QtGui.QPalette.Disabled, QtGui.QPalette.HighlightedText, QtGui.QColor(127, 127, 127))
    app.setPalette(dark_palette)
    app.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }")
    
app = QApplication(sys.argv) 
apply_dark_theme(app)

# Open RS485 port of the 305 pump
RS232 = serial.Serial(port='/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0', baudrate=9600)

# Enable Serial Control
xor_STX = 2
xor_ADDR = 128
xor_WIN1 = 48 
xor_WIN2 = 48
xor_WIN3 = 56
xor_WR = 49
xor_OnOff = 48
xor_ETX = 3

# Calculate Checksum XOR
xor = hex(xor_ADDR ^ xor_WIN1 ^ xor_WIN2 ^ xor_WIN3 ^ xor_WR ^ xor_OnOff ^ xor_ETX)

# Convert XOR to bytes
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

# Send Command
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

# End of transmission character
ETX = False
ETX_count = 0

timeout = 2  
RS232.timeout = 1 

while True:
    ANS = RS232.read() # Read RS485 response
    if ANS: # If any response is received
        print(ANS, end='')  # Print response
        if ETX:
            ETX_count = ETX_count + 1
            if ETX_count >= 2:
                print("...")
                break
        elif ANS == b'\x03': # If end of transmission character is received
            ETX = True # Set ETX as True
    else:
        timeout -= 1
        if timeout <= 0:
            pumpNotFound()
            break
