from PyQt5 import QtGui, QtCore, QtWidgets
from PyQt5.QtCore import QThread, pyqtSignal, QDateTime, Qt, QTimer
from PyQt5.QtGui import QFont, QImage, QPixmap
from PyQt5.QtWidgets import QTabWidget, QVBoxLayout, QWidget, QCheckBox, QLineEdit, QLabel, QComboBox, QApplication, QMainWindow, QPushButton, QTextEdit, QHBoxLayout, QGridLayout, QLineEdit, QFormLayout, QMessageBox
import pyqtgraph as pg
from pyqtgraph import QtGui, ImageItem
from serial.tools import list_ports #pyserial
import datetime
from datetime import datetime
import time
import threading
import numpy as np
import subprocess
import sys
import heapq
import time
import threading
import grpc #grpcio, grpcio-tools
from google.protobuf.timestamp_pb2 import Timestamp
import core_ba.core_pb2 as core
import core_ba.core_pb2_grpc as core_grpc
import queue
import gc
import os
import pyvisa
import random

# List to store monitoring data from TwistTor
subscribing_values = []
publishing_values = []


# Definition of a custom thread class for updating TwisTorr monitoring data
class Subscribe2Broker(QThread):
    bundle = None
    update_signal = pyqtSignal()
    
    # Run method for the thread
    def run(self):
        bundle = core.Bundle()  # Create an empty Bundle object
        channel = grpc.insecure_channel('localhost:50051')  # Create an insecure channel
        stub = core_grpc.BrokerStub(channel)  # Create a stub for the Broker service
        request = core.Interests()  # Create a request object
        request.types.append(core.DATA_TT_MON)#(core.DATA_TT_MON)  # Add DATA_TT_MON type to the request
        response_stream = stub.Subscribe(request)  # Subscribe to the response stream
        thread = threading.Thread(target=self.receive_bundles, args=(response_stream,))
        thread.start()  # Start the thread
        thread.join()  # Wait for the thread to finish
        
    # Method to receive bundles from the response stream
    def receive_bundles(self, response_stream):
        for bundle in response_stream:
            if len(bundle.value) > 0:  # Check if the bundle value is not empty
                global subscribing_values  # Use the global variable for monitoring data
                subscribing_values = []
                subscribing_values[:] = bundle.value  # Copy the bundle value to the subscribing_values list
                #print("subscribing_values: ",subscribing_values)
                self.update_signal.emit()  # Emit a signal to indicate updated data


class Publish2Broker(QThread):
    send_signal = pyqtSignal()
    
    def run(self):
        while(True):
            channel = grpc.insecure_channel('localhost:50051')
            stub = core_grpc.BrokerStub(channel)
            stub.Publish(self.generate_bundles())
            channel.close()
            time.sleep(0.5) 
        
    def generate_bundles(self):
        global publishing_values

        bundle = core.Bundle()
        bundle.timestamp.GetCurrentTime()
        bundle.type = core.DATA_TT_MON
        self.send_signal.emit()
        if publishing_values is not None and len(publishing_values) == 5:
            bundle.value.extend(publishing_values)
        else:
            bundle.value.extend([0, 0, 0, 0, 0])
        print("Publishing values: ", publishing_values)
        print("Bundle: \n", bundle, "\n\n\n\n\n")
        yield bundle




























class MainWindow(QMainWindow):
    showWarningSignal = pyqtSignal(str)
    def __init__(self):
        super(MainWindow, self).__init__()

        self.showWarningSignal.connect(self.show_warning_message)

        #path = os.getcwd()
        path = '/home/code/Development1'
        
        # Paths to c++ processes
        self.binary_paths = [
            path + '/core_ba/bin/APD_broker2_code_sw',
            path + '/core_ba/bin/APD_plot_cvt_code_sw',
            path + '/core_ba/bin/APD_publisher_code_sw',
            path + '/core_ba/bin/APD_fft_partial_code_sw',
            path + '/core_ba/bin/APD_reg_zero_code_sw', 
            path + '/core_ba/bin/APD_reg_proc_code_sw', 
            path + '/core_ba/bin/APD_reg_fft_1_code_sw',
            path + '/core_ba/bin/APD_reg_fft_01_code_sw',
            path + '/core_ba/bin/APD_fft_full_code_sw',
            path + '/core_ba/bin/TwisTorrIO_code_sw',
            path + '/core_ba/bin/TwisTorrSetter_code_sw',
            path + '/core_ba/bin/TwisTorrMonitor_code_sw',            
            path + '/core_ba/bin/storage',
            path + '/core_ba/bin/recorder'         
        ]

        # Title of the window
        self.setWindowTitle("CoDE Control Software")
        
        # Icon for the window
        icon = QtGui.QIcon(path + "/CoDE.png")    
        self.setWindowIcon(icon)        
        self.tab_widget = QTabWidget()
        self.setCentralWidget(self.tab_widget)

        # Creates different tabs
        self.tab1 = QWidget()
        self.tab2 = QWidget()
        self.tab3 = QWidget()
        self.tab4 = QWidget()
#        self.tab5 = QWidget()
        self.tab6 = QWidget()
        self.tab7 = QWidget()

        # Set names to tabs    
        self.tab_widget.addTab(self.tab2, "Vacuum")

        # Amont of processes depends on amount of c++ processes indicated earlier
        self.processes = [None] * 14
        self.threads = []

        self.processes[0] = subprocess.Popen([self.binary_paths[0]])

        # ------------------------------------------------------------------------------------------- #
        # Vacuum TAB
        # ------------------------------------------------------------------------------------------- #
        self.layout2 = QGridLayout(self.tab2)

        # Connect Subscribe2Broker() to a thread (receives TwisTorr data [Vacuum])
        self.subscribe2broker = Subscribe2Broker()
        self.subscribe2broker.update_signal.connect(self.update_vacuum_values)
        self.subscribe2broker.start()    
        # Connect Publish2Broker() to a thread (Sends TwisTorr data [Vacuum])
        self.publish2broker = Publish2Broker()
        self.publish2broker.send_signal.connect(self.send_vacuum_values)
        self.publish2broker.start()  
        
                      
        label_parameter = QLabel("Parameter")
        label_monitor = QLabel("Monitor")
        label_parameter.setStyleSheet("text-decoration: underline; font-weight: bold;")
        label_monitor.setStyleSheet("text-decoration: underline; font-weight: bold;")
        self.layout2.addWidget(label_parameter, 0, 0)
        self.layout2.addWidget(label_monitor, 0, 1)
        self.monitor_vacuum_current = QLabel("N/C")
        self.layout2.addWidget(QLabel("Pump current:"), 1, 0)
        self.layout2.addWidget(self.monitor_vacuum_current, 1, 1)
        self.monitor_vacuum_voltage = QLabel("N/C")
        self.layout2.addWidget(QLabel("Pump voltage:"), 2, 0)
        self.layout2.addWidget(self.monitor_vacuum_voltage, 2, 1)
        self.monitor_vacuum_power = QLabel("N/C")
        self.layout2.addWidget(QLabel("Pump power:"), 3, 0)
        self.layout2.addWidget(self.monitor_vacuum_power, 3, 1)
        self.monitor_vacuum_frequency = QLabel("N/C")
        self.layout2.addWidget(QLabel("Pump frequency:"), 4, 0)
        self.layout2.addWidget(self.monitor_vacuum_frequency, 4, 1)
        self.monitor_vacuum_temperature = QLabel("N/C")
        self.layout2.addWidget(QLabel("Pump temperature:"), 5, 0)
        self.layout2.addWidget(self.monitor_vacuum_temperature, 5, 1)
        self.btn_vacuum_monitor = QPushButton("Start reading from TwisTorr")
        self.btn_vacuum_monitor.setCheckable(True)  
        self.btn_vacuum_monitor.setStyleSheet("background-color: 53, 53, 53;")  
        self.btn_vacuum_monitor.clicked.connect(self.execute_twistorr_btn) 
        self.layout2.addWidget(self.btn_vacuum_monitor, 0, 2)
        self.layout2.setRowStretch(6, 1)

    def execute_twistorr_btn(self):
        if self.btn_vacuum_monitor.isChecked():
            self.execute_twistorr_monitor()
        else:
            self.kill_twistorr_monitor()

    def execute_twistorr_monitor(self):
        self.processes[11] = subprocess.Popen([self.binary_paths[11]])
 
    def kill_twistorr_monitor(self):
        subprocess.run(['pkill', '-f', self.processes[11].args[0]], check=True)

    def send_vacuum_values(self):
        global publishing_values
        publishing_values = [random.randint(0, 100) for _ in range(5)]
        #print(publishing_values)

    def update_vacuum_values(self):
        if self.btn_vacuum_monitor.isChecked():
            if len(subscribing_values) >= 5:
                vacuum_current = str(int(subscribing_values[0]))
                vacuum_voltage = str(int(subscribing_values[1]))
                vacuum_power = str(int(subscribing_values[2]))
                vacuum_frequency = str(int(subscribing_values[3]))
                vacuum_temperature = str(int(subscribing_values[4]))

                # Update the labels with the vacuum-related values
                self.monitor_vacuum_current.setText(vacuum_current+" [mA]")
                self.monitor_vacuum_voltage.setText(vacuum_voltage+" [Vdc]")
                self.monitor_vacuum_power.setText(vacuum_power+" [W]")
                self.monitor_vacuum_frequency.setText(vacuum_frequency+" [Hz]")
                self.monitor_vacuum_temperature.setText(vacuum_temperature+" [°C]")

        else:        
            self.monitor_vacuum_current.setText("N/C")
            self.monitor_vacuum_voltage.setText("N/C")
            self.monitor_vacuum_power.setText("N/C")
            self.monitor_vacuum_frequency.setText("N/C")
            self.monitor_vacuum_temperature.setText("N/C")
   
        # Sleep briefly to avoid excessive updates
        time.sleep(0.001)

    def start_update_tt_timer(self):
        # Start a QTimer to periodically update vacuum-related values
        self.timer_s = QTimer()
        self.timer_s.timeout.connect(self.update_vacuum_values)
        self.timer_s.start(100)  # Update interval for Twistorr monitoring

    def stop_update_tt_timer(self):
        # Stop the QTimer used for updating vacuum-related values
        if hasattr(self, 'timer_s'):
            self.timer_s.stop()

    def closeEvent(self, event):
        # Terminate running processes and stop timers before closing
        for process in self.processes:
            if process is not None:
                subprocess.run(['pkill', '-f', process.args[0]], check=True)
        self.stop_update_tt_timer()
        event.accept()

    def show_warning_message(self, message):
        msg = QMessageBox()
        msg.setWindowTitle("CoDE Warning")
        msg.setText(message)
        msg.setIcon(QMessageBox.Critical)
        msg.exec_()

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    app.setApplicationName("CoDE Control Software")
    try:
        # Create the main window and display it
        mainWindow = MainWindow()
        mainWindow.show()
        # Start the update timer for Twistorr monitoring
        mainWindow.start_update_tt_timer()
        # Execute the application event loop
        sys.exit(app.exec_())
    except Exception as e:
        # Handle unexpected exceptions by displaying an error message
        error_message = "An unexpected error has occurred: {}".format(str(e))
        #QtWidgets.QMessageBox.critical(None, "Error", error_message)
        # Append the error message to an error log file
        with open("error.log", "a") as log_file:
            log_file.write(error_message)
