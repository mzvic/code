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
import psutil
import struct
#python -m grpc_tools.protoc -I /home/code/Development1/core_ba/ --python_out=. --grpc_python_out=. /home/code/Development1/core_ba/core.proto

# Pressure needed on FRG-702 to enable electron gun
reqPress4ElectronGun = 0.00001

# Battery percentage with which the electron gun and pumps automatically turn off
ups_critical_value = 15

# List to store counts from APD
counts = []

# Lists to store FFT data from APD
freq = []
magn = []

# Check if there is another instance
gui_instance = 0

# List to store monitoring data from TwistTorr
twistorr_subscribing_values = []
prevac_subscribing_values = []
rigol_publishing_values = []
laser_publishing_values = []

rigol_pub_size = 5
laser_pub_size = 2  

#Setting vacuum equipment to serial instead of remote controller
os.system('python /home/code/Development/305_008.py')

os.system('python /home/code/Development/74_008.py')

# Custom Axis class to display timestamps as dates
class DateAxis(pg.AxisItem):
    def tickStrings(self, values, scale, spacing):
        return [datetime.fromtimestamp(float(value)).strftime('%H:%M:%S.%f')[:-3] for value in values]

# Custom Axis class to display custom image values
class CustomImageAxis(pg.AxisItem):
    def tickStrings(self, values, scale, spacing):
        return [f"{value/10:.1f}" for value in values]

class UpdateGraph1Thread(QThread):
    bundle = None
    update_signal = pyqtSignal()
    # Thread run method for updating graph 1 data
    def run(self):
        # Check if the thread has been requested to stop
        if self.isInterruptionRequested():
            return

        # Create a bundle and set up a gRPC channel and stub
        bundle = core.Bundle()
        channel = grpc.insecure_channel('localhost:50051')
        stub = core_grpc.BrokerStub(channel)

        # Create a request with the specific data type of interest
        request = core.Interests()
        request.types.append(core.DATA_APD_CVT)

        # Subscribe to the data stream using the stub
        response_stream = stub.Subscribe(request)

        # Check again if the thread has been requested to stop
        if self.isInterruptionRequested():
            return

        # Create a thread for receiving bundles and start it
        thread = threading.Thread(target=self.receive_bundles, args=(response_stream,))
        thread.start()

        # Wait for the receiving thread to finish
        thread.join()

    # Method to receive and process bundles in a separate thread
    def receive_bundles(self, response_stream):
        # Check if the thread has been requested to stop
        if self.isInterruptionRequested():
            return

        # Loop through the response stream to process bundles
        for bundle in response_stream:
            # Check if the thread has been requested to stop
            if self.isInterruptionRequested():
                break
            

            # Check if the 'counts' list is empty
            if not counts:
                # Copy the bundle value to the 'counts' list
                counts[:] = bundle.value
            else:
                # Compare values in the bundle with values in 'counts'
                if (
                    bundle.value[1] != counts[1]
                    and bundle.value[3] != counts[3]
                    and bundle.value[5] != counts[5]
                    and bundle.value[7] != counts[7]
                    and bundle.value[9] != counts[9]
                ):
                    # Update 'counts' with the new bundle value
                    counts[:] = bundle.value


                    # Emit a signal to indicate an update in graph 1
                    self.update_signal.emit()


# Definition of a custom thread class for updating plot 1 data
class UpdatePlot1Thread(QThread):
    # Signal to emit updated plot data
    plot_signal = pyqtSignal(list)
    
    # Constructor for the thread class
    def __init__(self):
        super(UpdatePlot1Thread, self).__init__()
        self.data_queue = queue.Queue()  # Initialize a queue for data
    
    # Run method for the thread
    def run(self):
        while True:
            # Get data from the queue (blocking operation)
            [self.times1, self.data1] = self.data_queue.get()
            
            # Emit a signal with the updated data for plot 1
            self.plot_signal.emit([self.times1, self.data1])
            
            # Empty the queue by consuming all remaining items
            while not self.data_queue.empty():
                self.data_queue.get()
            
            # Pause the thread for a short time
            time.sleep(0.1)
          
# Definition of a custom thread class for updating graph 2 data
class UpdateGraph2Thread(QThread):
    bundle = None
    update_signal = pyqtSignal()
    
    # Run method for the thread
    def run(self):
        bundle = core.Bundle()  # Create an empty Bundle object
        channel = grpc.insecure_channel('localhost:50051')  # Create an insecure channel
        stub = core_grpc.BrokerStub(channel)  # Create a stub for the Broker service
        request = core.Interests()  # Create a request object
        request.types.append(core.DATA_FFT_PARTIAL)  # Add DATA_FFT_PARTIAL type to the request
        response_stream = stub.Subscribe(request)  # Subscribe to the response stream
        thread = threading.Thread(target=self.receive_bundles, args=(response_stream,))
        thread.start()  # Start the thread
        thread.join()  # Wait for the thread to finish
        
    # Method to receive bundles from the response stream
    def receive_bundles(self, response_stream):
        for bundle in response_stream:
            fft = []
            fft[:] = bundle.value  # Copy the bundle value to the fft list
            half_length = len(fft) // 2  # Calculate half length of the fft data
            freq[:] = fft[:half_length]  # Copy first half of fft data to freq list
            magn[:] = fft[half_length:]  # Copy second half of fft data to magn list
            self.update_signal.emit()  # Emit a signal to indicate updated data

            

# Definition of a custom thread class for updating TwisTorr monitoring data

class TwisTorrSubscribe2Broker(QThread):
    bundle = None
    update_signal = pyqtSignal()
    
    # Run method for the thread
    def run(self):
        bundle = core.Bundle()  # Create an empty Bundle object
        channel = grpc.insecure_channel('localhost:50051')  # Create an insecure channel
        stub = core_grpc.BrokerStub(channel)  # Create a stub for the Broker service
        request = core.Interests()  # Create a request object
        request.types.append(core.DATA_TT_MON)  # Add DATA_TT_MON type to the request
        response_stream = stub.Subscribe(request)  # Subscribe to the response stream
        thread = threading.Thread(target=self.receive_bundles, args=(response_stream,))
        thread.start()  # Start the thread
        thread.join()  # Wait for the thread to finish
        
    # Method to receive bundles from the response stream
    def receive_bundles(self, response_stream):
        for bundle in response_stream:
            if len(bundle.value) > 0:  # Check if the bundle value is not empty
                global twistorr_subscribing_values  # Use the global variable for monitoring data
                twistorr_subscribing_values = []
                twistorr_subscribing_values[:] = bundle.value  # Copy the bundle value to the twistorr_subscribing_values list
                self.update_signal.emit()  # Emit a signal to indicate updated data

# Definition of a custom thread class for updating Prevac monitoring data
class PrevacSubscribe2Broker(QThread):
    bundle = None
    update_signal = pyqtSignal()
    
    # Run method for the thread
    def run(self):
        bundle = core.Bundle()  # Create an empty Bundle object
        channel = grpc.insecure_channel('localhost:50051')  # Create an insecure channel
        stub = core_grpc.BrokerStub(channel)  # Create a stub for the Broker service
        request = core.Interests()  # Create a request object
        request.types.append(core.DATA_EG_MON)  # Add DATA_EG_MON type to the request
        response_stream = stub.Subscribe(request)  # Subscribe to the response stream
        thread = threading.Thread(target=self.receive_bundles, args=(response_stream,))
        thread.start()  # Start the thread
        thread.join()  # Wait for the thread to finish
        
    # Method to receive bundles from the response stream
    def receive_bundles(self, response_stream):
        for bundle in response_stream:
            if len(bundle.value) > 0:  # Check if the bundle value is not empty
                global prevac_subscribing_values  # Use the global variable for monitoring data
                prevac_subscribing_values = []
                prevac_subscribing_values[:] = bundle.value  # Copy the bundle value to the prevac_subscribing_values list
                self.update_signal.emit()  # Emit a signal to indicate updated data

class RigolPublish2Broker(QThread):
    send_signal = pyqtSignal()
    
    def run(self):
        global rigol_publishing_values
        global g_global
        while(True):
            rigol_values = rigol_publishing_values
            time.sleep(0.1)
            if g_global is not None and g_global == 1:
                if rigol_values is not None and any(value != 0 for value in rigol_values):
                    channel = grpc.insecure_channel('localhost:50051')
                    stub = core_grpc.BrokerStub(channel)
                    stub.Publish(self.generate_bundles())
                    channel.close()
                    time.sleep(4.9) # Revisar si es necesario que esto vaya en otro hilo  
        
    def generate_bundles(self):
        global rigol_publishing_values
        rigol_values = rigol_publishing_values
        bundle = core.Bundle()
        bundle.timestamp.GetCurrentTime()
        bundle.type = core.DATA_RIGOL_MON
        self.send_signal.emit()
        if rigol_values is not None and len(rigol_values) == rigol_pub_size:
            bundle.value.extend(rigol_values)
            #print("Publishing values: ", rigol_values)    
        #print("Bundle: \n", bundle, "\n\n\n\n\n")
        yield bundle


class LaserPublish2Broker(QThread):
    send_signal = pyqtSignal()
    
    def run(self):
        global laser_publishing_values
        global g2_global
        while(True):
            laser_values = laser_publishing_values
            time.sleep(0.1)
            if g2_global is not None and g2_global == 1:
                if laser_values is not None and any(value != 0 for value in laser_values):
                    channel = grpc.insecure_channel('localhost:50051')
                    stub = core_grpc.BrokerStub(channel)
                    stub.Publish(self.generate_bundles())
                    channel.close()
                    time.sleep(4.9) # Revisar si es necesario que esto vaya en otro hilo 
        
    def generate_bundles(self):
        global laser_publishing_values
        laser_values = laser_publishing_values
        bundle = core.Bundle()
        bundle.timestamp.GetCurrentTime()
        bundle.type = core.DATA_LASER_MON
        self.send_signal.emit()
        if laser_values is not None and len(laser_values) == laser_pub_size:
            bundle.value.extend(laser_values)
            #print("Publishing values: ", laser_values)
        #print("Bundle: \n", bundle, "\n\n\n\n\n")
        yield bundle

# Rigol initial settings
try:
    os.system('usbreset 1ab1:0515')
    rm1 = pyvisa.ResourceManager('@py')
    scope_usb = rm1.open_resource('USB0::6833::1301::MS5A242205632::0::INSTR', timeout = 5000)
    scope_usb.write(":RUN")
    scope_usb.write(":TIM:MODE MAIN")
    scope_usb.write(":CHAN1:DISP 1")
    scope_usb.write(":CHAN2:DISP 1")
    scope_usb.write(":CHAN1:PROB 1")
    scope_usb.write(":CHAN2:PROB 1")
    scope_usb.write(":TRIG:COUP AC")
    scope_usb.write(":LAN:AUT 0")
    scope_usb.write(":LAN:MAN 1")
    scope_usb.write(":LAN:DHCP 1")
    scope_usb.write(":LAN:SMAS 225.225.225.0")
    scope_usb.write(":LAN:GAT 152.74.216.1")
    scope_usb.write(":LAN:IPAD 152.74.216.91")
    scope_usb.write(":LAN:DNS 152.74.16.14")
    #scope_usb.write(":LAN:APPL")
    rigol_ip = scope_usb.query(':LAN:VISA?').strip()
    print(rigol_ip)
    scope_usb.close()
except ValueError as ve:
    print("Rigol MSO5074:", ve)
    rigol_ip = "no"
    scope_usb = None
#rigol_ip = "no"

# Definition of a custom thread class for updating Rigol data        
class RigolDataThread(QThread):
    _Rigol_instance = None
    rigol_data_updated = pyqtSignal(np.ndarray, np.ndarray, float, float, float, str, float, float, str, float, int, int, float)
    

    def __init__(self):
        super().__init__()
        RigolDataThread._Rigol_instance = self
        self.rigol_prev_stat = 0
        self.rigol_status = 2
        self.rigol_chn_n = None
        self.rigol_auto = None
        self.rigol_voltage_set = None
        self.rigol_voltage_value = 0
        self.rigol_frequency_set = None
        self.rigol_frequency_value = 0
        self.rigol_function_set = None
        self.rigol_function_value = 0
        self.rigol_tscale_set = None
        self.rigol_tscale_value = 0.01
        self.rigol_attenuation_set = None
        self.rigol_attenuation_value = 1
        self.rigol_voltage_offset_set = None
        self.rigol_voltage_offset_value = 0
        self.rigol_coupling_set = None
        self.rigol_coupling_value = 0 
        self.rigol_vscale_set = None
        self.rigol_vscale_value = 0 
        self.rigol_g1_set = None
        self.rigol_g1_value = 0 
        self.rigol_g2_set = None
        self.rigol_g2_value = 0  
        self.rigol_laser_voltage_set = None
        self.rigol_laser_voltage_value = 0              
    
    @staticmethod
    def get_instance():
        return RigolDataThread._Rigol_instance

    def set_rigol_status(self, status):
        self.rigol_status = status

    def set_rigol_channel(self, chn_n):
        self.rigol_chn_n = chn_n
        
    def set_rigol_auto(self, auto):
        self.rigol_auto = auto    

    def set_rigol_voltage(self, voltage_set, voltage_value):
        self.rigol_voltage_set = voltage_set  
        self.rigol_voltage_value = voltage_value   

    def set_rigol_frequency(self, frequency_set, frequency_value):
        self.rigol_frequency_set = frequency_set
        self.rigol_frequency_value = frequency_value        

    def set_rigol_function(self, function_set, function_value):
        self.rigol_function_set = function_set
        self.rigol_function_value = function_value  

    def set_rigol_tscale(self, tscale_set, tscale_value):
        self.rigol_tscale_set = tscale_set   
        self.rigol_tscale_value = tscale_value

    def set_rigol_voltage_offset(self, voltage_offset_set, voltage_offset_value):
        self.rigol_voltage_offset_set = voltage_offset_set   
        self.rigol_voltage_offset_value = voltage_offset_value

    def set_rigol_attenuation(self, attenuation_set, attenuation_value):
        self.rigol_attenuation_set = attenuation_set   
        self.rigol_attenuation_value = attenuation_value

    def set_rigol_coupling(self, coupling_set, coupling_value):
        self.rigol_coupling_set = coupling_set   
        self.rigol_coupling_value = coupling_value
        
    def set_rigol_vscale(self, vscale_set, vscale_value):
        self.rigol_vscale_set = vscale_set   
        self.rigol_vscale_value = vscale_value

    def set_rigol_g1(self, g1_set, g1_value):
        self.rigol_g1_set = g1_set   
        self.rigol_g1_value = g1_value       

    def set_rigol_g2(self, g2_set, g2_value):
        self.rigol_g2_set = g2_set   
        self.rigol_g2_value = g2_value

    def set_rigol_laser_voltage(self, laser_voltage_set, laser_voltage_value):
        self.rigol_laser_voltage_set = laser_voltage_set   
        self.rigol_laser_voltage_value = laser_voltage_value          
        
    def run(self):
        rigol_loop_thread = threading.Thread(target=self.rigol_loop)#, args=(,))
        rigol_loop_thread.start()  # Start the thread
        rigol_loop_thread.join()  # Wait for the thread to finish        

    def rigol_loop(self):        
        while True:
            if self.rigol_chn_n is not None:
                status = self.rigol_status
                channel = self.rigol_chn_n
                auto = self.rigol_auto
                voltage = self.rigol_voltage_set
                frequency = self.rigol_frequency_set
                function = self.rigol_function_set
                tscale = self.rigol_tscale_set
                voltage_offset = self.rigol_voltage_offset_set
                attenuation = self.rigol_attenuation_set
                coupling = self.rigol_coupling_set
                vscale = self.rigol_vscale_set
                g1 = self.rigol_g1_set
                g2 = self.rigol_g2_set
                laser_voltage = self.rigol_laser_voltage_set
                voltage_V = self.rigol_voltage_value
                frequency_V = self.rigol_frequency_value
                function_V = self.rigol_function_value
                tscale_V = self.rigol_tscale_value 
                voltage_offset_V = self.rigol_voltage_offset_value
                attenuation_V = self.rigol_attenuation_value
                coupling_V = self.rigol_coupling_value
                vscale_V = self.rigol_vscale_value
                g1_V = self.rigol_g1_value
                g2_V = self.rigol_g2_value
                laser_voltage_V = self.rigol_laser_voltage_value              
                rigol_x_data, rigol_y_data, rigol_attenuation, rigol_voltscale, rigol_voltoffset, rigol_coupling, rigol_voltage, rigol_frequency, rigol_function, rigol_g1, rigol_g2, g2, rigol_laser_voltage = self.get_rigol_data(status, channel, auto, voltage, frequency, function, tscale, voltage_offset, attenuation, coupling, vscale, g1, g2, laser_voltage, voltage_V, frequency_V, function_V, tscale_V, voltage_offset_V, attenuation_V, coupling_V, vscale_V, g1_V, g2_V, laser_voltage_V)
                #print(rigol_x_data, rigol_y_data, rigol_attenuation, rigol_voltscale, rigol_voltoffset, rigol_coupling, rigol_voltage, rigol_frequency, rigol_function, rigol_g1, rigol_g2, g2, rigol_laser_voltage)
                self.rigol_data_updated.emit(rigol_x_data, rigol_y_data, rigol_attenuation, rigol_voltscale, rigol_voltoffset, rigol_coupling, rigol_voltage, rigol_frequency, rigol_function, rigol_g1, rigol_g2, g2_V, rigol_laser_voltage)
            time.sleep(0.1)
     # Get traces from rigol
    
    def get_rigol_data(self, status, channel, auto, voltage, frequency, function, tscale, voltage_offset, attenuation, coupling, vscale, g1, g2, laser_voltage, voltage_V, frequency_V, function_V, tscale_V, voltage_offset_V, attenuation_V, coupling_V, vscale_V, g1_V, g2_V, laser_voltage_V):
        if rigol_ip == "no":
            mainWindow.showWarningSignal.emit("Rigol MSO5074 not found. Please check the USB and network connections, restart the GUI and try again.")
        try:    
            if (status == 1 or g2_V == 1):
                rm = pyvisa.ResourceManager('@py')
                #print(rigol_ip,datetime.now())
                scope = rm.open_resource(rigol_ip, timeout=15000)
                #rigol_g2 = g2_V  
                rigol_g2 = float(scope.query(":OUTP2?"))                     
                rigol_timescale = float(scope.query(":TIM:SCAL?"))
                rigol_timeoffset = float(scope.query(":TIM:OFFS?"))
                rigol_coupling = str(scope.query(":TRIG:COUP?"))
                rigol_voltage = float(scope.query(":VOLT?"))
                rigol_frequency = float(scope.query(":FREQ?"))
                rigol_function = str(scope.query(":FUNC?"))
                rigol_voltscale = float(scope.query(f":CHAN{channel}:SCAL?"))
                rigol_voltoffset = float(scope.query(f":SOUR{channel}:VOLT:OFFS?"))
                rigol_attenuation = float(scope.query(f":CHAN{channel}:PROB?")) 
                rigol_instance = RigolDataThread.get_instance() 
                
                if rigol_instance:

                    if rigol_g2 == 1:
                        rigol_laser_voltage = float(scope.query(":SOUR2:VOLT:OFFS?"))
                        if g2_V == 0:
                            scope.write(":OUTP2 0")
                            rigol_instance.set_rigol_g2(0, 0)
                        else:
                            rigol_instance.set_rigol_g2(0, 1)
                    else:
                        rigol_laser_voltage = 0 
                        if g2_V == 1:
                            scope.write(":OUTP2 1")  
                            rigol_instance.set_rigol_g2(0, 1)
                        else:
                            rigol_instance.set_rigol_g2(0, 0) 

                    if (tscale == 1):
                        if (0.0000002 <= tscale_V <= 0.1):
                            scope.write(f":TIM:MAIN:SCAL {tscale_V}") 
                            rigol_instance.set_rigol_tscale(0, tscale_V) 
                            time.sleep(0.5)
                            if (rigol_timescale < tscale_V):
                                time.sleep(0.6)
                                mainWindow.showWarningSignal.emit("Requesting data, please press 'OK' when the graph updates")
                        else:
                            rigol_instance.set_rigol_tscale(0, 0.001) 
                            mainWindow.showWarningSignal.emit("The 'Time scale' range is between 200 ns and 0.5 s")
                    if (voltage == 1):
                        if (-5 <= voltage_V <= 5):
                            scope.write(f":VOLT {voltage_V}") 
                            rigol_instance.set_rigol_voltage(0, voltage_V) 
                        else:
                            rigol_instance.set_rigol_voltage(0, 0) 
                            mainWindow.showWarningSignal.emit("The 'Voltage' range is between -5 and 5 V")
                    elif (frequency == 1):
                        scope.write(f":FREQ {frequency_V}")
                        rigol_instance.set_rigol_frequency(0, frequency_V)  
                    elif (function == 1):
                        scope.write(f":FUNC {function_V}")
                        rigol_instance.set_rigol_function(0, function_V)  
                    elif (voltage_offset == 1):
                        #scope.write(":OFFS 0")
                        scope.write(f":SOUR1:VOLT:OFFS {voltage_offset_V}")
                        #scope.write(":CHAN1:OFFS 0")
                        #scope.write(f":OFFS {voltage_offset_V}")                     
                        rigol_instance.set_rigol_voltage_offset(0, voltage_offset_V)   
                    elif (attenuation == 1):
                        scope.write(f":CHAN{channel}:PROB {attenuation_V}")
                        rigol_instance.set_rigol_attenuation(0, attenuation_V)  
                    elif (coupling == 1):
                        scope.write(f":TRIG:COUP {coupling_V}")
                        rigol_instance.set_rigol_coupling(0, coupling_V) 
                    elif (vscale == 1):
                        scope.write(f":CHAN{channel}:SCAL {vscale_V}")  
                        rigol_instance.set_rigol_vscale(0, vscale_V) 
                    elif (g1 == 1):
                        if g1_V == 1:
                            scope.write(f":OUTP1 {g1_V}") 
                            rigol_instance.set_rigol_g1(0, 1)
                        else:
                            scope.write(f":OUTP1 {g1_V}") 
                            rigol_instance.set_rigol_g1(0, 0)                       
                    elif (laser_voltage == 1):
                        if 0 <= laser_voltage_V <= 5:
                            scope.write(":SOUR2:FUNC DC") 
                            scope.write(":SOUR2:VOLT 0")
                            scope.write(f":SOUR2:VOLT:OFFS {laser_voltage_V}") 
                            rigol_instance.set_rigol_laser_voltage(0, laser_voltage_V)    
                        else:
                            rigol_instance.set_rigol_laser_voltage(0, 0)
                            mainWindow.showWarningSignal.emit("'Laser voltage' range is between 0 and 5 V")
                    elif (auto == 1):
                        scope.write(":AUT")  
                        time.sleep(0.2)
                        scope.write(f":CHAN{channel}:SCAL 1")
                        time.sleep(0.2)
                        scope.write(f":CHAN{channel}:PROB 1")
                        time.sleep(0.2)
                        rigol_instance.set_rigol_auto(0)
                    
                rigol_g1 = float(scope.query(":OUTP1?"))
                scope.write(f":CHAN{channel}:OFFS 0;:WAV:SOUR CHAN{channel};:WAV:MODE NORM;:WAV:FORM BYTE;:WAV:POIN 1000") 
                rigol_rawdata = scope.query_binary_values(":WAV:DATA? CHAN{channel}", datatype='B', container=np.array)
                rigol_data_0 = rigol_rawdata * -1 + 255
                rigol_data = (-((rigol_data_0 - 127 - rigol_voltoffset / rigol_voltscale) / 25 * rigol_voltscale)*1.05)*rigol_voltscale/rigol_attenuation
                rigol_x_values = np.arange(0, len(rigol_data)) / 100 * rigol_timescale
                return rigol_x_values, rigol_data/rigol_voltscale, rigol_attenuation, rigol_voltscale, rigol_voltoffset, rigol_coupling, rigol_voltage, rigol_frequency, rigol_function, rigol_g1, rigol_g2, g2_V, rigol_laser_voltage
            else:
                return np.array([]), np.array([]), 0, 0, 0, "none", 0, 0, "none", 0, 0, 0, 0
        except Exception as e:
             print("Error:", str(e))
             return np.array([]), np.array([]), 0, 0, 0, "none", 0, 0, "none", 0, 0, 0, 0
        finally:
            if scope is not None:
                scope.close()            
            if (status == 1 or g2_V == 1):
                scope.close()
            elif (status == 2 and g2_V == 0):
                if self.rigol_prev_stat != status:
                    os.system('usbreset 1ab1:0515')
                    self.rigol_prev_stat = status
                time.sleep(0.01)
                return np.array([]), np.array([]), 0, 0, 0, "none", 0, 0, "none", 0, 0, 0, 0
            else:
                time.sleep(0.01)
                return np.array([]), np.array([]), 0, 0, 0, "none", 0, 0, "none", 0, 0, 0, 0

    
class MainWindow(QMainWindow):
    showWarningSignal = pyqtSignal(str)
    def __init__(self):
        super(MainWindow, self).__init__()

        self.showWarningSignal.connect(self.show_warning_message)

        #path = os.getcwd()
        path = '/home/code/Development'
        
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
            path + '/core_ba/bin/recorder',
            path + '/core_ba/bin/TwisTorrSS1_code_sw',
            path + '/core_ba/bin/TwisTorrSS2_code_sw',
            path + '/core_ba/bin/PrevacMonitor_code_sw',
            path + '/core_ba/bin/PrevacSetter_code_sw'
        ]

        # Title of the window
        self.setWindowTitle("CoDE Control Software")
        self.closing = 0

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
#        self.tab6 = QWidget()
        self.tab7 = QWidget()

        # Add lateral view to the window for monitoring data
        self.dock = QtWidgets.QDockWidget(self)
        self.addDockWidget(QtCore.Qt.LeftDockWidgetArea, self.dock)
        self.dock.setFeatures(QtWidgets.QDockWidget.DockWidgetMovable | QtWidgets.QDockWidget.DockWidgetFloatable)
        self.dock.setFixedWidth(250)

        # Create grid layout for the dock widget
        self.dock_grid = QtWidgets.QGridLayout()

        # ---- UPS ----
        self.ups_frame = QtWidgets.QFrame()
        self.ups_frame.setFrameShape(QtWidgets.QFrame.Box)

        self.dock_grid.addWidget(self.ups_frame, 0, 0, 1, 2)

        ups_layout = QtWidgets.QVBoxLayout(self.ups_frame)
        self.ups_frame.setLayout(ups_layout)

        self.dock_grid.addWidget(QLabel("       UPS battery:"), 0, 0)
        self.ups_monitor = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.ups_monitor, 0, 1)

        # ---- Datalogger ----
        self.logging_frame = QtWidgets.QFrame()
        self.logging_frame.setFrameShape(QtWidgets.QFrame.Box)

        self.dock_grid.addWidget(self.logging_frame, 1, 0, 1, 2)

        logging_layout = QtWidgets.QVBoxLayout(self.logging_frame)
        self.logging_frame.setLayout(logging_layout)

        self.logging_button = QtWidgets.QPushButton()
        #self.logging_button.setCheckable(True)
        self.logging_button.setStyleSheet("background-color: 53, 53, 53;")
        self.logging_button.setText("Data logging")
        self.logging_button.setFixedWidth(180)
        #self.logging_button.clicked.connect(self.logging_connect)
        logging_layout.addWidget(self.logging_button, alignment=Qt.AlignTop | Qt.AlignHCenter)

        # ---- TRAP SYSTEM ----
        self.trap_frame = QtWidgets.QFrame()
        self.trap_frame.setFrameShape(QtWidgets.QFrame.Box)

        self.dock_grid.addWidget(self.trap_frame, 2, 0, 4, 2)

        trap_layout = QtWidgets.QVBoxLayout(self.trap_frame)
        self.trap_frame.setLayout(trap_layout)

        self.trap_button = QtWidgets.QPushButton()
        self.trap_button.setCheckable(True)
        self.trap_button.setStyleSheet("background-color: 53, 53, 53;")
        self.trap_button.setText("Trap")
        self.trap_button.setFixedWidth(180)
        self.trap_button.clicked.connect(self.toggle_trap_connect)
        self.dock_grid.addWidget(QLabel("       Voltage:"), 3, 0)
        self.voltage_monitor = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.voltage_monitor, 3, 1)
        self.dock_grid.addWidget(QLabel("       Offset voltage:"), 4, 0)
        self.offset_monitor = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.offset_monitor, 4, 1)
        self.dock_grid.addWidget(QLabel("       Frequency:"), 5, 0)
        self.frequency_monitor = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.frequency_monitor, 5, 1)
        trap_layout.addWidget(self.trap_button, alignment=Qt.AlignTop | Qt.AlignHCenter)
        # ---------------------------------------------

        # ---- SYSTEM PRESSURE ----
        self.pressure_frame = QtWidgets.QFrame()
        self.pressure_frame.setFrameShape(QtWidgets.QFrame.Box)

        self.dock_grid.addWidget(self.pressure_frame, 6, 0, 4, 2)

        pressure_layout = QtWidgets.QVBoxLayout(self.pressure_frame)
        self.pressure_frame.setLayout(pressure_layout)

        self.pressure_button = QtWidgets.QPushButton()
        self.pressure_button.setCheckable(True)
        self.pressure_button.setStyleSheet("background-color: 53, 53, 53;")
        self.pressure_button.setText("Vacuum pump")
        self.pressure_button.setFixedWidth(180)
        self.pressure_button.clicked.connect(self.execute_twistorr_bar_btn)
        pressure_layout.addWidget(self.pressure_button, alignment=Qt.AlignTop | Qt.AlignHCenter)

        self.dock_grid.addWidget(QLabel("       305FS frequency:"), 7, 0)
        self.vacuum_frequency = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.vacuum_frequency, 7, 1)

        self.dock_grid.addWidget(QLabel("       74FS frequency:"), 8, 0)
        self.vacuum_frequency2 = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.vacuum_frequency2, 8, 1)        

        self.dock_grid.addWidget(QLabel("       FRG-702 Pressure:"), 9, 0)
        self.FR_pressure = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.FR_pressure, 9, 1)        

        # ---------------------------------------------
        # ---- ELECTRON GUN ----
        self.prevac_frame = QtWidgets.QFrame()
        self.prevac_frame.setFrameShape(QtWidgets.QFrame.Box)

        self.dock_grid.addWidget(self.prevac_frame, 10, 0, 5, 2)

        prevac_layout = QtWidgets.QVBoxLayout(self.prevac_frame)
        self.pressure_frame.setLayout(prevac_layout)

        self.prevac_button = QtWidgets.QPushButton()
        self.prevac_button.setCheckable(True)
        self.prevac_button.setStyleSheet("background-color: 53, 53, 53;")
        self.prevac_button.setText("Electron gun")
        self.prevac_button.setFixedWidth(180)
        self.prevac_button.clicked.connect(self.execute_prevac_bar_btn)
        prevac_layout.addWidget(self.prevac_button, alignment=Qt.AlignTop | Qt.AlignHCenter)

        self.dock_grid.addWidget(QLabel("       ES40 status:"), 11, 0)
        self.EG_status = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.EG_status, 11, 1) 

        self.cathode_failure = QtWidgets.QLabel("              N/C")
        self.dock_grid.addWidget(self.cathode_failure, 12, 0)

        self.cathode_current_limit = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.cathode_current_limit, 12, 1)        

        self.ES_short_circuit = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.ES_short_circuit, 13, 1)  

        self.ES_failure = QtWidgets.QLabel("              N/C")
        self.dock_grid.addWidget(self.ES_failure, 13, 0)

        self.FS_short_circuit = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.FS_short_circuit, 14, 1)        

        self.FS_failure = QtWidgets.QLabel("              N/C")
        self.dock_grid.addWidget(self.FS_failure, 14, 0)                

        # ---------------------------------------------


        # ---- LASER ----
        self.laser_frame = QtWidgets.QFrame()
        self.laser_frame.setFrameShape(QtWidgets.QFrame.Box)
        self.laser_frame.setGeometry(0, 0, 250, 130)  # Establecer posición y tamaño del frame

        self.dock_grid.addWidget(self.laser_frame, 15, 0, 3, 2)

        self.laser_button = QtWidgets.QPushButton(self.laser_frame)
        self.laser_button.setCheckable(True)
        self.laser_button.setStyleSheet("background-color: 53, 53, 53; text-align: center;")
        self.laser_button.setText("Laser")
        self.laser_button.setFixedWidth(180)
        self.laser_button.move(25, 10)  
        self.laser_button.clicked.connect(self.toggle_laser_connect)
        self.rigol_laser_voltage = QLineEdit(self.laser_frame)
        self.rigol_laser_voltage.setText("1")
        self.rigol_laser_voltage.setFixedWidth(70)
        self.rigol_laser_voltage.move(25, 50)  

        self.laser_set_button = QtWidgets.QPushButton(self.laser_frame)
        self.laser_set_button.setStyleSheet("background-color: 53, 53, 53; text-align: center;")
        self.laser_set_button.setText("Volt. set")
        self.laser_set_button.clicked.connect(self.toggle_laser_voltage)
        self.laser_set_button.setFixedWidth(70)
        self.laser_set_button.move(130, 50)  

        self.dock_grid.addWidget(QLabel("       Laser voltage:"), 17, 0)
        self.laser_voltage_monitor = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.laser_voltage_monitor, 17, 1)

        # ---------------------------------------------

        # ---- APD SYSTEM ----
        self.apd_frame = QtWidgets.QFrame()
        self.apd_frame.setFrameShape(QtWidgets.QFrame.Box)

        self.dock_grid.addWidget(self.apd_frame, 20, 0, 1, 2)

        apd_layout = QtWidgets.QVBoxLayout(self.apd_frame)
        self.apd_frame.setLayout(apd_layout)

        self.apd_button = QtWidgets.QPushButton()
        self.apd_button.setCheckable(True)
        self.apd_button.setStyleSheet("background-color: 53, 53, 53;")
        self.apd_button.setText("APD")
        self.apd_button.clicked.connect(self.toggle_apd_connect)
        self.apd_button.setFixedWidth(180)

        apd_layout.addWidget(self.apd_button, alignment=Qt.AlignVCenter | Qt.AlignHCenter)
        # ---------------------------------------------

        # add to the dock widget
        self.dock_widget = QtWidgets.QWidget()
        self.dock_widget.setLayout(self.dock_grid)
        self.dock.setWidget(self.dock_widget)


        # Set names to tabs    
        self.tab_widget.addTab(self.tab1, "APD")
        self.tab_widget.addTab(self.tab2, "Vacuum")
        self.tab_widget.addTab(self.tab3, "Electron gun")
        self.tab_widget.addTab(self.tab4, "Particle trap")
#        self.tab_widget.addTab(self.tab5, "Temperature")
        #self.tab_widget.addTab(self.tab6, "Data processing")
        self.tab_widget.addTab(self.tab7, "Data logging")                

        # Amont of processes depends on amount of c++ processes indicated earlier
        self.processes = [None] * 18
        self.threads = []
        #self.processes[9] = subprocess.Popen([self.binary_paths[9]])

        # ------------------------------------------------------------------------------------------- #
        # APD tab
        # ------------------------------------------------------------------------------------------- #
        self.layout1 = QVBoxLayout(self.tab1) 
 
        # Plot 1 object 
        self.graph1 = pg.PlotWidget(axisItems={'bottom': DateAxis(orientation='bottom')})
        
        # Plot 1 height
        self.graph1.setMinimumHeight(180)
        self.graph1.setMaximumHeight(300)

        # Plot 1 displayed in the APD tab
        self.layout1.addWidget(self.graph1) 
        plotItem1 = self.graph1.getPlotItem()
        plotItem1.showGrid(True, True, 0.7)
        
        # Initial data for plot 1
        self.plot1 = self.graph1.plot([0,1,2,3], [0,0,0,0], pen=pg.mkPen(color=(255, 0, 0)))
        
        # Axis labels
        self.graph1.setLabel('left', 'Counts')
        self.graph1.setLabel('bottom', 'Time', units='hh:mm:ss.µµµµµµ')
        
        # List to be displayed in plot 1
        self.data1 = []
        self.times1 = []      

        # Plot 2 object 
        self.graph2 = pg.PlotWidget()
        
        # Plot 2 height
        self.graph2.setMinimumHeight(180)
        self.graph2.setMaximumHeight(300)
        
        # Plot 2 displayed in the APD tab
        self.layout1.addWidget(self.graph2)
        plotItem2 = self.graph2.getPlotItem()
        plotItem2.showGrid(True, True, 0.7)
        
        # Axis labels
        self.graph2.setLabel("left", "|Power|")
        self.graph2.setLabel("bottom", "Frequency", "Hz")
        self.graph2.plotItem.setLogMode(x=True)
        self.h_line = pg.InfiniteLine(pos=0, angle=0, pen=pg.mkPen(color=(0, 255, 0), width=1))
        self.graph2.addItem(self.h_line) 
        
        # Boolean for enable cursor ID (yellow bar) and spectrometer
        self.cursor_position = None
        self.y_bar = False
        self.spec = False

        # List to be displayed in plot 2
        self.data2 = []
        self.freq2 = []

        # Plot 3 object 
        self.color_map = pg.ImageView(view=pg.PlotItem(axisItems={'bottom': CustomImageAxis(orientation='bottom')}))
        
        # Plot 3 displayed in the APD tab
        self.layout1.addWidget(self.color_map)
        plot_item = self.color_map.getView()

        # Plot 3 height
        self.color_map.setMinimumHeight(180)
        self.color_map.setMaximumHeight(400)

        # Some parameters and settings for plot 3
        self.color_map.setColorMap(pg.colormap.get('plasma')) # Histogram color setting
        self.fft_magnitudes = 500000
        self.color_map.getView().autoRange() 
        self.avg_count = 0
        plot_item.showGrid(x=True, y=True)
        self.t_fft = int(time.time())     

        # Connect UpdateGraph1Thread() to a thread (receives data for counts vs. time [APD])
        self.update_graph1_thread = UpdateGraph1Thread()
        self.update_graph1_thread.update_signal.connect(self.update_graph1)

        # Connect UpdatePlot1Thread() to a thread (display the counts vs. time in plot 1 [APD])
        self.update_plot1_thread = UpdatePlot1Thread()
        self.update_plot1_thread.plot_signal.connect(self.update_plot1)
        self.update_plot1_thread.start()

        # Connect UpdateGraph2Thread() to a thread (receives and display fft data in plot 2 [APD])
        self.update_graph2_thread = UpdateGraph2Thread()
        self.update_graph2_thread.update_signal.connect(self.update_graph2)
         
        self.rigolpublish2broker = RigolPublish2Broker()
        self.rigolpublish2broker.send_signal.connect(self.send_rigol_publishing_values)
        self.rigol_values = []

        self.laserpublish2broker = LaserPublish2Broker()
        self.laserpublish2broker.send_signal.connect(self.send_laser_publishing_values)
        self.laser_values = []

        # Buttons in APD tab
        button_names_1a = ["Server", "Counts plot"] 
        button_names_1b = ["Plot counts", "Plot FFT", "Export counts data [100kHz]", "Export counts data [1kHz]", "Export FFT data [0.1Hz resolution]", "Export FFT data [0.01Hz resolution]"]
        
        # Input parameters in APD tab: Hidden, first, second and third row
        hidden_layout_1 = QHBoxLayout() 
        first_layout_1 = QHBoxLayout()    
        second_layout_1 = QHBoxLayout()
        third_layout_1 = QHBoxLayout() 

        # FPGA serial port select (first_layout_1)
        serialPortsLabel = QLabel("FPGA serial port:")
        serialPortsLabel.setFixedWidth(150)
        self.serialPortsCombobox = QComboBox(self)
        self.update_serial_ports()
        index_p = self.serialPortsCombobox.findText('usb-Digilent_Digilent_USB_Device_210319B58455-if01-port0', QtCore.Qt.MatchFixedString) # Default value
        if index_p >= 0:
             self.serialPortsCombobox.setCurrentIndex(index_p)        
        self.serialPortsCombobox.setMaximumWidth(150)
        first_layout_1.addWidget(serialPortsLabel)
        first_layout_1.addWidget(self.serialPortsCombobox)


                
        # FFT window type select (second_layout_1)
        windowTypeLabel = QLabel("FFT Window Type:")
        windowTypeLabel.setFixedWidth(150)
        self.windowTypeCombobox = QComboBox(self)
        self.windowTypeCombobox.addItems(['Hamming', 'Hann', 'Blackman-Harris 4', 'Blackman-Harris 7', 'No window'])
        index_w = self.windowTypeCombobox.findText('No window', QtCore.Qt.MatchFixedString) # Default value
        if index_w >= 0:
             self.windowTypeCombobox.setCurrentIndex(index_w)
        self.windowTypeCombobox.setMaximumWidth(150) 
        second_layout_1.addWidget(windowTypeLabel)
        second_layout_1.addWidget(self.windowTypeCombobox)
        
        self.window_type_values = {
            0: 1,  # 'Hamming'
            1: 2,  # 'Hann'
            2: 3,  # 'Blackmann-Harris 4'
            3: 4,  # 'Blackmann-Harris 7'
            4: 5,  # 'No window'
        } 

        # Time axis lenght for counts vs. time plot in 'first_layout_1' (we only update the width of this field because of the buttons/inputs distribution of the firts row)  
        self.resizeEvent = self.update_input_width  
        self.apd_counts_secs_label = QLabel("T-axis length in counts:")
        #self.apd_counts_secs_label.setFixedWidth(195)
        self.apd_counts_secs_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.apd_counts_secs_input = QLineEdit(self)
        self.apd_counts_secs_input.setFixedWidth(150)
        self.apd_counts_secs_input.setText("10") # Default value      
        second_layout_1.addWidget(self.apd_counts_secs_label)
        second_layout_1.addWidget(self.apd_counts_secs_input)

        # Buttons creation (button_names_1a/b)     
        self.buttons = []

        # Hidden row
        for i in range(2): 
            start_stop_button = QPushButton(button_names_1a[i])
            start_stop_button.setCheckable(True)
            start_stop_button.toggled.connect(lambda checked, i=i: self.toggle_process(i, checked))            
            hidden_layout_1.addWidget(start_stop_button)
            self.buttons.append(start_stop_button)      
        
        # First, seconds and third row
        for i in range(2, 8):
            start_stop_button = QPushButton(button_names_1b[i - 2])
            start_stop_button.setCheckable(True)
            start_stop_button.toggled.connect(lambda checked, i=i: self.toggle_process(i, checked))
            if i==2 or i==3: #or i==4 or i ==5 :
                first_layout_1.addWidget(start_stop_button)
            else:
                if i==4 or i==5 or i==6 or i==7:
                    pass
                else:
                    second_layout_1.addWidget(start_stop_button)

            self.buttons.append(start_stop_button) 
        
        # Shows a yellow bar to see the magnitude of the frequency below the mouse cursor (in 'second_layout_1')
        self.toggle_button = QPushButton("Peak ID with cursor")
        self.toggle_button.setCheckable(True) 
        self.toggle_button.clicked.connect(self.toggle_cursor)
        first_layout_1.addWidget(self.toggle_button)                                       

        # Averaging period for the FFTs in seconds (in 'third_layout_1')
        avg_time_label = QLabel("Averaging period in spectrometer (s):")
        avg_time_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.avg_time_input = QLineEdit(self)
        self.avg_time_input.setFixedWidth(90) 
        self.avg_time_input.setText("1") # Default value       
        third_layout_1.addWidget(avg_time_label)
        third_layout_1.addWidget(self.avg_time_input)     

        # Amount of averaging periods to show in spectrometer (in 'third_layout_1')
        spectrum_amount_label = QLabel("Periods to show in spectrometer:")
        spectrum_amount_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.spectrum_amount_input = QLineEdit(self)
        self.spectrum_amount_input.setFixedWidth(90) 
        self.spectrum_amount_input.setText("120") # Default value      
        third_layout_1.addWidget(spectrum_amount_label)
        third_layout_1.addWidget(self.spectrum_amount_input)   
        
        # Button to start/stop showing spectrometer (in 'third_layout_1')
        self.toggle_button_spec = QPushButton("Show spectrum averages")
        self.toggle_button_spec.setCheckable(True)  # Enables alternancy
        self.toggle_button_spec.clicked.connect(self.toggle_spec)
        second_layout_1.addWidget(self.toggle_button_spec)
        
        # Button to clean spectrometer (in 'third_layout_1')
        self.toggle_button_clean_spec = QPushButton("Clean spectrometer")
        self.toggle_button_clean_spec.clicked.connect(self.toggle_clean_spec)
        second_layout_1.addWidget(self.toggle_button_clean_spec)
        
        # Start of the frequency range of interest (in 'third_layout_1')
        f_i_label = QLabel("FFT initial frequency:")
        f_i_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.f_i_input = QLineEdit(self)
        self.f_i_input.setFixedWidth(90) 
        self.f_i_input.setText("10") # Default value    
        third_layout_1.addWidget(f_i_label)
        third_layout_1.addWidget(self.f_i_input) 

        # End of the frequency range of interest (in 'third_layout_1')
        f_f_label = QLabel("FFT final frequency:")
        f_f_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.f_f_input = QLineEdit(self)
        self.f_f_input.setFixedWidth(90) 
        self.f_f_input.setText("150") # Default value     
        third_layout_1.addWidget(f_f_label)
        third_layout_1.addWidget(self.f_f_input)  
        
        # Converts the frequency range to 'int' values
        self.f_i = int(self.f_i_input.text()) 
        self.f_f = int(self.f_f_input.text())                
            
        # Just a note
        self.note = QtWidgets.QLabel("Important: To be able to graph the FFT, the 'Plot counts' button must be enabled. Also, if the FFT settings are modified, 'Plot FFT' must be disabled and then enabled for the changes to take effect.")
        self.layout1.addWidget(self.note)

        # Enable hidden buttons
        self.buttons[0].setChecked(True)  # 'Server'
        self.buttons[1].setChecked(True)  # 'Counts plot'
        selected_port = self.serialPortsCombobox.currentText()

        #################################################################################################################################################################### APD--> revisar
        #self.processes[2] = subprocess.Popen([self.binary_paths[2], str(selected_port)])

        # Hidden row
        self.buttons[0].hide()  
        self.buttons[1].hide()  

        # Displays the buttons and text inputs in the APD tab
        self.layout1.addLayout(hidden_layout_1)
        self.layout1.addLayout(first_layout_1) 
        self.layout1.addLayout(second_layout_1)
        self.layout1.addLayout(third_layout_1)

        # ------------------------------------------------------------------------------------------- #
        # Vacuum TAB
        # ------------------------------------------------------------------------------------------- #
        self.layout2 = QGridLayout(self.tab2)

        # Connect TwisTorrSubscribe2Broker() to a thread (receives TwisTorr data [Vacuum])
        self.twistorrSubs2Broker = TwisTorrSubscribe2Broker()
        self.twistorrSubs2Broker.update_signal.connect(self.update_vacuum_values)
        self.twistorrSubs2Broker.start()     

        # Labels for the column titles
        label_parameter = QLabel("Parameter")
        label_monitor = QLabel("Monitor")
        label_parameter.setStyleSheet("font-weight: bold;")
        label_monitor.setStyleSheet("font-weight: bold;")
        label_parameter2 = QLabel("Parameter")
        label_monitor2 = QLabel("Monitor")
        label_parameter2.setStyleSheet("font-weight: bold;")
        label_monitor2.setStyleSheet("font-weight: bold;")


        # Labels for the column titles
        label_305fs = QLabel("TwisTorr 305FS")
        label_305fs.setStyleSheet("text-decoration: underline; font-weight: bold;")

        # Set row index order for the titles
        self.layout2.addWidget(label_305fs, 0, 0, 1, 2)
        self.btn_tt_startstop1 = QPushButton("Start/Stop")
        self.btn_tt_startstop1.setCheckable(True)  
        self.btn_tt_startstop1.setStyleSheet("background-color: 53, 53, 53;")  
        self.btn_tt_startstop1.clicked.connect(self.tt_startstop1_button)
        self.btn_tt_startstop1.setFixedWidth(300) 
        self.layout2.addWidget(self.btn_tt_startstop1, 1, 0, 1, 2) 

        # Set row index order for the titles
        self.layout2.addWidget(label_parameter, 2, 0)
        self.layout2.addWidget(label_monitor, 2, 1)
        #self.layout2.addWidget(label_setpoint, 0, 2)   
        
        # Setpoint field for vacuum pressure
        self.monitor_vacuum_current = QLabel("N/C")
        #self.set_vacuum_pressure = QLineEdit()
        #self.set_vacuum_pressure.setText("1002") # Default value
        #self.set_vacuum_pressure.setFixedWidth(100)
        #btn_vacuum_pressure = QPushButton("Set")
        self.layout2.addWidget(QLabel("Pump current:"), 3, 0)
        self.layout2.addWidget(self.monitor_vacuum_current, 3, 1)
        #self.layout2.addWidget(self.set_vacuum_pressure, 1, 2)
        #self.layout2.addWidget(btn_vacuum_pressure, 1, 3)

        # Setpoint field for motor speed
        self.monitor_vacuum_voltage = QLabel("N/C")
        #self.set_speed_motor = QLineEdit()
        #self.set_speed_motor.setText("5000") # Default value
        #self.set_speed_motor.setFixedWidth(100)
        #btn_speed_motor = QPushButton("Set")
        self.layout2.addWidget(QLabel("Pump voltage:"), 4, 0)
        self.layout2.addWidget(self.monitor_vacuum_voltage, 4, 1)
        #self.layout2.addWidget(self.set_speed_motor, 2, 2)
        #self.layout2.addWidget(btn_speed_motor, 2, 3)

        # Setpoint field for valve state
        self.monitor_vacuum_power = QLabel("N/C")
        #self.set_valve_state = QLineEdit()
        #self.set_valve_state.setText("1") # Default value
        #self.set_valve_state.setFixedWidth(100)
        #btn_valve_state = QPushButton("Set")
        self.layout2.addWidget(QLabel("Pump power:"), 5, 0)
        self.layout2.addWidget(self.monitor_vacuum_power, 5, 1)
        #self.layout2.addWidget(self.set_valve_state, 3, 2)
        #self.layout2.addWidget(btn_valve_state, 3, 3)

        # For monitoring bomb power variable
        self.monitor_vacuum_frequency = QLabel("N/C")
        self.layout2.addWidget(QLabel("Pump frequency:"), 6, 0)
        self.layout2.addWidget(self.monitor_vacuum_frequency, 6, 1)

        # For monitoring temperature variable
        self.monitor_vacuum_temperature = QLabel("N/C")
        self.layout2.addWidget(QLabel("Pump temperature:"), 7, 0)
        self.layout2.addWidget(self.monitor_vacuum_temperature, 7, 1)


        # Labels for the column titles
        label_74fs = QLabel("TwisTorr 74FS")
        label_74fs.setStyleSheet("text-decoration: underline; font-weight: bold;")

        # Set row index order for the titles
        self.layout2.addWidget(label_74fs, 0, 2, 1, 2)

        self.btn_tt_startstop2 = QPushButton("Start/Stop")
        self.btn_tt_startstop2.setCheckable(True)  
        self.btn_tt_startstop2.setStyleSheet("background-color: 53, 53, 53;")  
        self.btn_tt_startstop2.clicked.connect(self.tt_startstop2_button) 
        self.btn_tt_startstop2.setFixedWidth(300) 
        self.layout2.addWidget(self.btn_tt_startstop2, 1, 2, 1, 2) 
        
        # Set row index order for the titles
        self.layout2.addWidget(label_parameter2, 2, 2)
        self.layout2.addWidget(label_monitor2, 2, 3)
        #self.layout2.addWidget(label_setpoint, 0, 2)

        # Setpoint field for vacuum pressure
        self.monitor_vacuum_current2 = QLabel("N/C")
        #self.set_vacuum_pressure = QLineEdit()
        #self.set_vacuum_pressure.setText("1002") # Default value
        #self.set_vacuum_pressure.setFixedWidth(100)
        #btn_vacuum_pressure = QPushButton("Set")
        self.layout2.addWidget(QLabel("Pump current:"), 3, 2)
        self.layout2.addWidget(self.monitor_vacuum_current2, 3, 3)
        #self.layout2.addWidget(self.set_vacuum_pressure, 1, 2)
        #self.layout2.addWidget(btn_vacuum_pressure, 1, 3)

        # Setpoint field for motor speed
        self.monitor_vacuum_voltage2 = QLabel("N/C")
        #self.set_speed_motor = QLineEdit()
        #self.set_speed_motor.setText("5000") # Default value
        #self.set_speed_motor.setFixedWidth(100)
        #btn_speed_motor = QPushButton("Set")
        self.layout2.addWidget(QLabel("Pump voltage:"), 4, 2)
        self.layout2.addWidget(self.monitor_vacuum_voltage2, 4, 3)
        #self.layout2.addWidget(self.set_speed_motor, 2, 2)
        #self.layout2.addWidget(btn_speed_motor, 2, 3)

        # Setpoint field for valve state
        self.monitor_vacuum_power2 = QLabel("N/C")
        #self.set_valve_state = QLineEdit()
        #self.set_valve_state.setText("1") # Default value
        #self.set_valve_state.setFixedWidth(100)
        #btn_valve_state = QPushButton("Set")
        self.layout2.addWidget(QLabel("Pump power:"), 5, 2)
        self.layout2.addWidget(self.monitor_vacuum_power2, 5, 3)
        #self.layout2.addWidget(self.set_valve_state, 3, 2)
        #self.layout2.addWidget(btn_valve_state, 3, 3)

        # For monitoring bomb power variable
        self.monitor_vacuum_frequency2 = QLabel("N/C")
        self.layout2.addWidget(QLabel("Pump frequency:"), 6, 2)
        self.layout2.addWidget(self.monitor_vacuum_frequency2, 6, 3)

        # For monitoring temperature variable
        self.monitor_vacuum_temperature2 = QLabel("N/C")
        self.layout2.addWidget(QLabel("Pump temperature:"), 7, 2)
        self.layout2.addWidget(self.monitor_vacuum_temperature2, 7, 3)        
    
        # Labels for the column titles
        label_pressure = QLabel("XGS-600")
        label_pressure.setStyleSheet("text-decoration: underline; font-weight: bold;")

        # Set row index order for the titles
        self.layout2.addWidget(label_pressure, 0, 4, 1, 2)

        self.pressure_plotting_state = 0

        self.btn_pressure = QPushButton("Show pressure data")
        self.btn_pressure.setCheckable(True)  
        self.btn_pressure.setStyleSheet("background-color: 53, 53, 53;")  
        self.btn_pressure.clicked.connect(self.pressure_plot_button) 
        self.btn_pressure.setFixedWidth(300) 
        self.layout2.addWidget(self.btn_pressure, 1, 4, 1, 2) 

        self.pressure1_checkbox = QCheckBox("FRG-702 (Red)")
        self.pressure1_checkbox.setChecked(False)
        self.layout2.addWidget(self.pressure1_checkbox, 2, 4)

        self.pressure2_checkbox = QCheckBox("CDG-500 (Green)")
        self.pressure2_checkbox.setChecked(False)
        self.layout2.addWidget(self.pressure2_checkbox, 2, 5)

        self.pressure_secs_label = QLabel("T-axis length (in seconds):")
        #self.apd_counts_secs_label.setFixedWidth(195)
        #self.pressure_secs_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.pressure_secs_input = QLineEdit(self)
        self.pressure_secs_input.setFixedWidth(150)
        self.pressure_secs_input.setText("86400") # Default value      
        self.layout2.addWidget(self.pressure_secs_label, 3, 4)
        self.layout2.addWidget(self.pressure_secs_input, 3, 5)

        self.graph_pressure_vacuum = pg.PlotWidget(axisItems={'bottom': DateAxis(orientation='bottom')})

        #self.graph_pressure_vacuum.invertY()
        
        self.graph_pressure_vacuum.showGrid(x=True, y=True, alpha=1)  
        #self.graph_pressure_vacuum.plotItem.setLogMode(x=True)         
        self.graph_pressure_vacuum.setLabel('bottom', 'Time', units='hh:mm:ss.µµµµµµ')
        self.graph_pressure_vacuum.setLabel('left', 'Pressure [Torr]')
        self.pressure_data1 = []
        self.pressure_data2 = []
        self.pressure_time = []          
        self.steady_state_1 = True
        self.transient_threshold1 = 0.2
        self.transient_value_1 = 12 
        self.steady_state_2 = True 
        self.transient_threshold2 = 0.2
        self.transient_value_2 = 12
        self.previous_power1 = None 
        self.previous_power2 = None 
        self.max_freq_1 = 1010
        self.max_freq_2 = 1167 
        self.vacuum_power1_history = [] 
        self.vacuum_power2_history = []                   
        self.vacuum_plot_time = 0 
        self.setting_twistorr = 0

        self.layout2.addWidget(self.graph_pressure_vacuum, 8, 0, 1, 6) 
        
        ## Initial data for plot 1
        self.pressure_plot1 = self.graph_pressure_vacuum.plot([time.time()-3,time.time()-2,time.time()-1,time.time()], [0,0,0,0], pen=pg.mkPen(color=(255, 0, 0), width=2))
        self.pressure_plot2 = self.graph_pressure_vacuum.plot([time.time()-3,time.time()-2,time.time()-1,time.time()], [0,0,0,0], pen=pg.mkPen(color=(0, 255, 0), width=2))
        self.btn_vacuum_monitor = QPushButton("Connect to vacuum equipment")
        self.btn_vacuum_monitor.setCheckable(True)  
        self.btn_vacuum_monitor.setStyleSheet("background-color: 53, 53, 53;")  
        self.btn_vacuum_monitor.clicked.connect(self.execute_twistorr_btn) 
        self.layout2.addWidget(self.btn_vacuum_monitor, 11, 0, 1, 6)

        
        #self.layout2.setRowStretch(8, 1)


        # ------------------------------------------------------------------------------------------- #
        # Electron gun TAB
        # ------------------------------------------------------------------------------------------- #
        
        self.layout3 = QGridLayout(self.tab3)

        # Connect PrevacSubscribe2Broker() to a thread (receives prevac data [electron gun])
        self.prevacSubs2Broker = PrevacSubscribe2Broker()
        self.prevacSubs2Broker.update_signal.connect(self.update_electrongun_values)
        self.prevacSubs2Broker.start()      

        self.vacuum_pressure1 = 0 
        self.vacuum_pressure2 = 0

        # Labels for the column titles
        eg_label1 = QLabel("Parameter")
        eg_label2 = QLabel("ES40 reading")
        eg_label3 = QLabel("Set value")
        eg_label4 = QLabel("Set button")
        eg_label1.setStyleSheet("font-weight: bold;")
        eg_label2.setStyleSheet("font-weight: bold;")
        eg_label3.setStyleSheet("font-weight: bold;")
        eg_label4.setStyleSheet("font-weight: bold;")

        self.eg_connection_btn = QPushButton("Connect to ES40")
        self.eg_connection_btn.setCheckable(True)  
        self.eg_connection_btn.setStyleSheet("background-color: 53, 53, 53;")  
        self.eg_connection_btn.clicked.connect(self.execute_electrongun_btn)
        #self.eg_connection_btn.setFixedWidth(300) 
        self.layout3.addWidget(self.eg_connection_btn, 0, 0, 1, 5) 

        self.secure_eg_btn = QPushButton("Enable operate")
        self.secure_eg_btn.setCheckable(True)
        self.secure_eg_btn.clicked.connect(self.enb_eg_onoff)#FUNCION

        self.operate_eg_btn = QPushButton("Operate")
        self.operate_eg_btn.setCheckable(True)
        self.operate_eg_btn.clicked.connect(self.enb_eg_operate)#FUNCION

        self.standby_eg_btn = QPushButton("Stand by")
        self.standby_eg_btn.setCheckable(True)
        self.standby_eg_btn.clicked.connect(self.enb_eg_standby)#FUNCION                

        self.layout3.addWidget(self.secure_eg_btn, 1, 0)
        self.layout3.addWidget(self.operate_eg_btn, 1, 1, 1, 2)
        self.layout3.addWidget(self.standby_eg_btn, 1, 3, 1, 2)

        # Set row index order for the titles
        self.layout3.addWidget(eg_label1, 2, 0)
        self.layout3.addWidget(eg_label2, 2, 1)
        self.layout3.addWidget(eg_label3, 2, 2)#, 1, 2)
        self.layout3.addWidget(eg_label4, 2, 4)
        
        # Temporales
        test_value = str(int("0"))
        #self.eg_read_energy_voltage.setText(test_value+" [V]")
        #self.eg_read_focus_voltage.setText(test_value+" []")
        #self.eg_read_wehnelt_volatge.setText(test_value+" [V]")
        #self.eg_read_emission_current.setText(test_value+" [µA]")
        #self.eg_read_tpd.setText(test_value+" [µs]")
        #self.eg_read_position_x.setText(test_value+" [mm]")
        #self.eg_read_position_y.setText(test_value+" [mm]")
        #self.eg_read_area_x.setText(test_value+" [mm]")
        #self.eg_read_area_y.setText(test_value+" [mm]")
        #self.eg_read_grid_x.setText(test_value+" [mm]")
        #self.eg_read_grid_y.setText(test_value+" [mm]")

        self.eg_read_energy_voltage = QLabel("N/C")
        self.eg_read_focus_voltage = QLabel("N/C")
        self.eg_read_wehnelt_voltage = QLabel("N/C")
        self.eg_read_emission_current = QLabel("N/C")
        self.eg_read_tpd = QLabel("N/C")
        self.eg_read_position_x = QLabel("N/C")
        self.eg_read_position_y = QLabel("N/C")
        self.eg_read_area_x = QLabel("N/C")
        self.eg_read_area_y = QLabel("N/C")
        self.eg_read_grid_x = QLabel("N/C")
        self.eg_read_grid_y = QLabel("N/C")
        self.eg_read_grid_y.setFixedWidth(350)

        # Energy voltage
        self.eg_energy_voltage_setval = QLineEdit()
        self.eg_energy_voltage_setval.setPlaceholderText("0.0 ... 5000.0") 
        #self.eg_energy_voltage_setval.setText("0") #self.eg_energy_voltage_setval.setFixedWidth(220) 
        self.eg_energy_voltage_setbtn = QPushButton("Set") #self.eg_energy_voltage_setbtn.clicked.connect(func_eg_energy_voltage_setval)
        self.eg_energy_voltage_setbtn.clicked.connect(self.prevac_set_ev)
        self.eg_energy_voltage_setval.setFixedWidth(200)
        self.eg_energy_voltage_setbtn.setFixedWidth(150) 

        self.layout3.addWidget(QLabel("Energy voltage:"), 3, 0) 
        self.layout3.addWidget(self.eg_read_energy_voltage, 3, 1)
        self.layout3.addWidget(self.eg_energy_voltage_setval, 3, 2)
        self.layout3.addWidget(QLabel("[V]"), 3, 3)
        self.layout3.addWidget(self.eg_energy_voltage_setbtn, 3, 4) 

        # Focus voltage
        self.eg_focus_voltage_setval = QLineEdit()
        self.eg_focus_voltage_setval.setPlaceholderText("0 ... Uenergy") 
        #self.eg_focus_voltage_setval.setText("0") #self.eg_focus_voltage_setval.setFixedWidth(220)
        self.eg_focus_voltage_setbtn = QPushButton("Set") 
        self.eg_focus_voltage_setbtn.clicked.connect(self.prevac_set_fv)
        self.eg_focus_voltage_setval.setFixedWidth(200)
        self.eg_focus_voltage_setbtn.setFixedWidth(150) 

        self.layout3.addWidget(QLabel("Focus voltage:"), 4, 0) 
        self.layout3.addWidget(self.eg_read_focus_voltage, 4, 1)
        self.layout3.addWidget(self.eg_focus_voltage_setval, 4, 2)
        self.layout3.addWidget(QLabel("[V]"), 4, 3)
        self.layout3.addWidget(self.eg_focus_voltage_setbtn, 4, 4) 

        # Wehnelt voltage
        self.eg_wehnelt_voltage_setval = QLineEdit()
        self.eg_wehnelt_voltage_setval.setPlaceholderText("0.0 ... 300.0") 
        #self.eg_wehnelt_voltage_setval.setText("0") #self.eg_wehnelt_voltage_setval.setFixedWidth(220)
        self.eg_wehnelt_voltage_setbtn = QPushButton("Set") 
        self.eg_wehnelt_voltage_setbtn.clicked.connect(self.prevac_set_wv)
        self.eg_wehnelt_voltage_setval.setFixedWidth(200)
        self.eg_wehnelt_voltage_setbtn.setFixedWidth(150) 

        self.layout3.addWidget(QLabel("Wehnelt voltage:"), 5, 0) 
        self.layout3.addWidget(self.eg_read_wehnelt_voltage, 5, 1)
        self.layout3.addWidget(self.eg_wehnelt_voltage_setval, 5, 2)
        self.layout3.addWidget(QLabel("[V]"), 5, 3)
        self.layout3.addWidget(self.eg_wehnelt_voltage_setbtn, 5, 4)  

        # Emission current
        self.eg_emission_current_setval = QLineEdit()
        self.eg_emission_current_setval.setPlaceholderText("0.10 ... 300.00") 
        #self.eg_emission_current_setval.setText("0") #self.eg_emission_current_setval.setFixedWidth(220)
        self.eg_emission_current_setbtn = QPushButton("Set") 
        self.eg_emission_current_setbtn.clicked.connect(self.prevac_set_ec)
        self.eg_emission_current_setval.setFixedWidth(200)
        self.eg_emission_current_setbtn.setFixedWidth(150) 

        self.layout3.addWidget(QLabel("Emission current:"), 6, 0) 
        self.layout3.addWidget(self.eg_read_emission_current, 6, 1)
        self.layout3.addWidget(self.eg_emission_current_setval, 6, 2)
        self.layout3.addWidget(QLabel("[µA]"), 6, 3)
        self.layout3.addWidget(self.eg_emission_current_setbtn, 6, 4)  

        # Time per dot
        self.eg_tpd_setval = QLineEdit()
        self.eg_tpd_setval.setPlaceholderText("20 ... 30000") 
        #self.eg_tpd_setval.setText("0") #self.eg_tpd_setval.setFixedWidth(220)
        self.eg_tpd_setbtn = QPushButton("Set")
        self.eg_tpd_setbtn.clicked.connect(self.prevac_set_tpd)
        self.eg_tpd_setval.setFixedWidth(200)
        self.eg_tpd_setbtn.setFixedWidth(150) 

        self.layout3.addWidget(QLabel("Time per dot:"), 7, 0) 
        self.layout3.addWidget(self.eg_read_tpd, 7, 1)
        self.layout3.addWidget(self.eg_tpd_setval, 7, 2)
        self.layout3.addWidget(QLabel("[µs]"), 7, 3)
        self.layout3.addWidget(self.eg_tpd_setbtn, 7, 4)    

        # Scan position X
        self.eg_position_x_setval = QLineEdit()
        self.eg_position_x_setval.setPlaceholderText("-5.00 ... 5.00")
        #self.eg_position_x_setval.setText("0") #self.eg_position_x_setval.setFixedWidth(220)
        self.eg_position_x_setbtn = QPushButton("Set") 
        self.eg_position_x_setbtn.clicked.connect(self.prevac_set_px)
        self.eg_position_x_setval.setFixedWidth(200)
        self.eg_position_x_setbtn.setFixedWidth(150) 

        self.layout3.addWidget(QLabel("Scan position X:"), 8, 0) 
        self.layout3.addWidget(self.eg_read_position_x, 8, 1)
        self.layout3.addWidget(self.eg_position_x_setval, 8, 2)
        self.layout3.addWidget(QLabel("[mm]"), 8, 3)
        self.layout3.addWidget(self.eg_position_x_setbtn, 8, 4) 

        # Scan position Y
        self.eg_position_y_setval = QLineEdit()
        self.eg_position_y_setval.setPlaceholderText("-5.00 ... 5.00")
        #self.eg_position_y_setval.setText("0") #self.eg_position_y_setval.setFixedWidth(220)
        self.eg_position_y_setbtn = QPushButton("Set") 
        self.eg_position_y_setbtn.clicked.connect(self.prevac_set_py)
        self.eg_position_y_setval.setFixedWidth(200)
        self.eg_position_y_setbtn.setFixedWidth(150) 

        self.layout3.addWidget(QLabel("Scan position Y:"), 9, 0) 
        self.layout3.addWidget(self.eg_read_position_y, 9, 1)
        self.layout3.addWidget(self.eg_position_y_setval, 9, 2)
        self.layout3.addWidget(QLabel("[mm]"), 9, 3)
        self.layout3.addWidget(self.eg_position_y_setbtn, 9, 4)                                         

        # Scan area X
        self.eg_area_x_setval = QLineEdit()
        self.eg_area_x_setval.setPlaceholderText("0.00 ... 10.00")
        #self.eg_area_x_setval.setText("0") #self.eg_area_x_setval.setFixedWidth(220)
        self.eg_area_x_setbtn = QPushButton("Set") 
        self.eg_area_x_setbtn.clicked.connect(self.prevac_set_ax)
        self.eg_area_x_setval.setFixedWidth(200)
        self.eg_area_x_setbtn.setFixedWidth(150) 

        self.layout3.addWidget(QLabel("Scan area X:"), 10, 0) 
        self.layout3.addWidget(self.eg_read_area_x, 10, 1)
        self.layout3.addWidget(self.eg_area_x_setval, 10, 2)
        self.layout3.addWidget(QLabel("[mm]"), 10, 3)
        self.layout3.addWidget(self.eg_area_x_setbtn, 10, 4) 

        # Scan area Y
        self.eg_area_y_setval = QLineEdit()
        self.eg_area_y_setval.setPlaceholderText("0.00 ... 10.00")
        #self.eg_area_y_setval.setText("0") #self.eg_area_y_setval.setFixedWidth(220)
        self.eg_area_y_setbtn = QPushButton("Set") 
        self.eg_area_y_setbtn.clicked.connect(self.prevac_set_ay)
        self.eg_area_y_setval.setFixedWidth(200)
        self.eg_area_y_setbtn.setFixedWidth(150) 

        self.layout3.addWidget(QLabel("Scan area Y:"), 11, 0) 
        self.layout3.addWidget(self.eg_read_area_y, 11, 1)
        self.layout3.addWidget(self.eg_area_y_setval, 11, 2)
        self.layout3.addWidget(QLabel("[mm]"), 11, 3)
        self.layout3.addWidget(self.eg_area_y_setbtn, 11, 4)  

        # Scan grid X
        self.eg_grid_x_setval = QLineEdit()
        self.eg_grid_x_setval.setPlaceholderText("0.01 ... 2.00")
        #self.eg_grid_x_setval.setText("0") #self.eg_grid_x_setval.setFixedWidth(220)
        self.eg_grid_x_setbtn = QPushButton("Set") 
        self.eg_grid_x_setbtn.clicked.connect(self.prevac_set_gx)
        self.eg_grid_x_setval.setFixedWidth(200)
        self.eg_grid_x_setbtn.setFixedWidth(150) 

        self.layout3.addWidget(QLabel("Scan grid X:"), 12, 0) 
        self.layout3.addWidget(self.eg_read_grid_x, 12, 1)
        self.layout3.addWidget(self.eg_grid_x_setval, 12, 2)
        self.layout3.addWidget(QLabel("[mm]"), 12, 3)
        self.layout3.addWidget(self.eg_grid_x_setbtn, 12, 4) 

        # Scan grid Y
        self.eg_grid_y_setval = QLineEdit()
        self.eg_grid_y_setval.setPlaceholderText("0.01 ... 2.00")
        #self.eg_grid_y_setval.setText("0") #self.eg_grid_y_setval.setFixedWidth(220)
        self.eg_grid_y_setbtn = QPushButton("Set")
        self.eg_grid_y_setbtn.clicked.connect(self.prevac_set_gy)
        self.eg_grid_y_setval.setFixedWidth(200)
        self.eg_grid_y_setbtn.setFixedWidth(150) 

        self.layout3.addWidget(QLabel("Scan grid Y:"), 13, 0) 
        self.layout3.addWidget(self.eg_read_grid_y, 13, 1)
        self.layout3.addWidget(self.eg_grid_y_setval, 13, 2)
        self.layout3.addWidget(QLabel("[mm]"), 13, 3)
        self.layout3.addWidget(self.eg_grid_y_setbtn, 13, 4) 

        # Plot object 
        self.graph_eg = pg.PlotWidget(axisItems={'bottom': DateAxis(orientation='bottom')})
        
        # Plot height
        self.graph_eg.setMinimumHeight(180)
        self.graph_eg.setMaximumHeight(10000)

        # Plot displayed in the electron gun tab
        self.graph_eg.showGrid(x=True, y=True, alpha=1)           
        #self.graph_eg.setLogMode(y=True)
        
        # Initial data for plot
        self.plot_eg = self.graph_eg.plot([0,1,2,3], [0,0,0,0], pen=pg.mkPen(color=(255, 0, 0)), width=2)
        
        # Axis labels
        self.graph_eg.setLabel('left', 'Frequency [Hz]')
        self.graph_eg.setLabel('bottom', 'Time', units='hh:mm:ss.µµµµµµ')
        
        # Labels for the column titles
        eg_plot_label = QLabel("Dominant frequency vs. time (configure to obtain FFT in APD tab first)")
        eg_plot_label.setStyleSheet("font-weight: bold;")
        self.layout3.addWidget(eg_plot_label, 14 ,0, 1, 2)

        self.eg_plot_state = 0
        self.secure_eg = 0
        self.prevac_flags = [0] * 12
        
        self.times_eg = []
        self.data_eg = []
        
        self.eg_secs_label = QLabel("T-axis length in seconds:")
        self.eg_secs_label.setFixedWidth(195)
        self.eg_secs_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.eg_secs_input = QLineEdit(self)
        self.eg_secs_input.setFixedWidth(200)
        self.eg_secs_input.setText("86400") # Default value      

        self.enable_eg_plot_btn = QPushButton("Enable plot")
        self.enable_eg_plot_btn.setCheckable(True)
        self.enable_eg_plot_btn.clicked.connect(self.enb_eg_plot)#FUNCION
        self.layout3.addWidget(self.enable_eg_plot_btn, 15, 0)
        self.layout3.addWidget(self.eg_secs_label, 15, 1)
        self.layout3.addWidget(self.eg_secs_input, 15, 2)

        # Plot eg displayed in the APD tab
        self.layout3.addWidget(self.graph_eg, 16, 0, 2, 5)
        self.layout3.setRowStretch(17, 5)

        # ------------------------------------------------------------------------------------------- #
        # ESI TAB
        # ------------------------------------------------------------------------------------------- #

        # ------------------------------------------------------------------------------------------- #
        # Particle Trap TAB
        # ------------------------------------------------------------------------------------------- #
        self.layout4 = QGridLayout(self.tab4)

        self.layout4.addWidget(QLabel("<b>q Calculator:</b>"), 1, 4)

        input_mass = QLineEdit()
        input_mass.setFixedWidth(320)
        self.layout4.addWidget(QLabel("Mass:"), 2, 4)
        self.layout4.addWidget(input_mass, 2, 5)
        self.layout4.addWidget(QLabel("[kg]"), 2, 6)

        input_charge = QLineEdit()
        input_charge.setFixedWidth(320)
        self.layout4.addWidget(QLabel("Charge:"), 3, 4)
        self.layout4.addWidget(input_charge, 3, 5)
        self.layout4.addWidget(QLabel("[C]"), 3, 6)

        input_geometrical = QLineEdit()
        input_geometrical.setFixedWidth(320)
        self.layout4.addWidget(QLabel("Geometrical Parameter:"), 4, 4)
        self.layout4.addWidget(input_geometrical, 4, 5)
        #self.layout4.addWidget(QLabel("[unit]"), 3, 2)

        input_voltage = QLineEdit()
        input_voltage.setFixedWidth(320)
        self.layout4.addWidget(QLabel("Voltage:"), 5, 4)
        self.layout4.addWidget(input_voltage, 5, 5)
        self.layout4.addWidget(QLabel("[V]"), 5, 6)

        input_frequency = QLineEdit()
        input_frequency.setFixedWidth(320)
        self.layout4.addWidget(QLabel("Frequency:"), 6, 4)
        self.layout4.addWidget(input_frequency, 6, 5)
        self.layout4.addWidget(QLabel("[Hz]"), 6, 6)

        btn_calculate = QPushButton("Calculate")
        self.layout4.addWidget(btn_calculate, 8, 4, 1, 2)

        q_calculated = QLabel("N/C")
        self.layout4.addWidget(QLabel("q:"), 7, 4)
        self.layout4.addWidget(q_calculated, 7, 5)

        # Connection status
        self.rigol = False
        self.rigol_connect = QPushButton("Connect to Rigol MSO5074")
        self.rigol_connect.setCheckable(True)  
        self.rigol_connect.setStyleSheet("background-color: 53, 53, 53;")  
        self.rigol_connect.clicked.connect(self.toggle_rigol_connect) 
        self.layout4.addWidget(self.rigol_connect, 1, 0, 1, 3)
        
        # Osciloscope title
        self.layout4.addWidget(QLabel("<b>Osciloscope settings (CH 1-2):</b>"), 2, 0)
        
        # Channel
        self.rigol_chn = True
        self.rigol_chn_n = 1
        self.rigol_channel = QPushButton("Set oscilloscope channel")
        self.rigol_channel.setCheckable(True)  
        self.rigol_channel.clicked.connect(self.toggle_rigol_channel) 
        self.layout4.addWidget(self.rigol_channel, 2, 1)  

        # Auto
        self.rigol_auto_btn = QPushButton("Auto [Oscilloscope]")
        self.rigol_auto_btn.clicked.connect(self.btn_rigol_auto) 
        self.layout4.addWidget(self.rigol_auto_btn, 2, 2) 
  
        # Voltage display range
        self.layout4.addWidget(QLabel("Voltage display range (min, max)[V]:"), 3, 0)
        self.rigol_voltage_min = QLineEdit()
        self.rigol_voltage_min.setText("-3") 
        self.rigol_voltage_min.setFixedWidth(220)
        self.rigol_voltage_max = QLineEdit()
        self.rigol_voltage_max.setText("3")  
        self.rigol_voltage_max.setFixedWidth(220)           
        self.layout4.addWidget(self.rigol_voltage_min, 3, 1)
        self.layout4.addWidget(self.rigol_voltage_max, 3, 2)  

        # Attenuation: :CHANnel<n>:PROBe <atten>
        self.rigol_attenuation_btn = QPushButton("Set")
        self.layout4.addWidget(QLabel("Attenuation:"), 4, 0)
        self.attenuationCombobox = QComboBox(self)
        self.attenuationCombobox.addItems(['1', '0.0001', '0.0002', '0.0005', '0.001', '0.002', '0.005', '0.01', '0.02', '0.05', '0.1', '0.2', '0.5', '2', '5', '10', '20', '50', '100', '200', '500', '1000', '2000', '5000', '10000', '20000', '50000'])
        self.rigol_attenuation_btn.clicked.connect(self.toggle_rigol_attenuation) 
        self.layout4.addWidget(self.attenuationCombobox,4, 1)
        self.layout4.addWidget(self.rigol_attenuation_btn,4, 2)   
        
        # Coupling: :CHANnel<n>:COUPling <coupling>
        self.rigol_coupling_btn = QPushButton("Set")
        self.layout4.addWidget(QLabel("Coupling:"), 5, 0)
        self.couplingCombobox = QComboBox(self)
        self.couplingCombobox.addItems(['AC', 'DC', 'GND'])
        self.rigol_coupling_btn.clicked.connect(self.toggle_rigol_coupling) 
        self.layout4.addWidget(self.couplingCombobox,5, 1)
        self.layout4.addWidget(self.rigol_coupling_btn,5, 2)                     
        
        # Time scale
        self.rigol_time_scale = QLineEdit()
        self.rigol_time_scale.setText("0.001") 
        self.rigol_time_scale.setFixedWidth(220)
        self.rigol_time_scale_btn = QPushButton("Set")
        self.layout4.addWidget(QLabel("Time scale [s]:"), 6, 0) 
        self.rigol_time_scale_btn.clicked.connect(self.toggle_rigol_tscale)
        self.layout4.addWidget(self.rigol_time_scale, 6, 1)
        self.layout4.addWidget(self.rigol_time_scale_btn, 6, 2) 

        # Voltage scale
        self.rigol_vscale = QLineEdit()
        self.rigol_vscale.setText("1") 
        self.rigol_vscale.setFixedWidth(220)
        self.rigol_vscale_btn = QPushButton("Set")
        self.layout4.addWidget(QLabel("Voltage scale [V]:"), 7, 0) 
        self.rigol_vscale_btn.clicked.connect(self.toggle_rigol_vscale)
        self.layout4.addWidget(self.rigol_vscale, 7, 1)
        self.layout4.addWidget(self.rigol_vscale_btn, 7, 2) 
                
        # Particle trap
        self.layout4.addWidget(QLabel("<b>Particle trap control (function generator, channel 1):</b>"), 8, 0, 1, 2)

        # Enable trap particle
        self.rigol_particle_trap_enable_btn = QPushButton("Enable particle trap")
        self.rigol_particle_trap_enable_btn.setCheckable(True) 
        self.rigol_particle_trap_enable_btn.clicked.connect(self.toggle_rigol_particle_trap_enable)
        self.layout4.addWidget(self.rigol_particle_trap_enable_btn, 8, 2)

        # Voltage
        self.rigol_voltage = QLineEdit()
        self.rigol_voltage.setText("2") 
        self.rigol_voltage.setFixedWidth(220)
        self.rigol_voltage_btn = QPushButton("Set")
        self.layout4.addWidget(QLabel("Voltage [Vpp]:"), 9, 0)
        self.rigol_voltage_btn.clicked.connect(self.toggle_rigol_voltage)
        self.layout4.addWidget(self.rigol_voltage, 9, 1)
        self.layout4.addWidget(self.rigol_voltage_btn, 9, 2)

        # Voltage offset: :CHANnel<n>:OFFSet <offset>
        self.rigol_voltage_offset = QLineEdit()
        self.rigol_voltage_offset.setText("0") 
        self.rigol_voltage_offset.setFixedWidth(220)
        self.rigol_voltage_offset_btn = QPushButton("Set")
        self.layout4.addWidget(QLabel("Voltage offset [V]:"), 10, 0)
        self.rigol_voltage_offset_btn.clicked.connect(self.toggle_rigol_voltage_offset)
        self.layout4.addWidget(self.rigol_voltage_offset, 10, 1)
        self.layout4.addWidget(self.rigol_voltage_offset_btn, 10, 2)        

        # Frequency
        self.rigol_frequency = QLineEdit()
        self.rigol_frequency.setText("10000") 
        self.rigol_frequency.setFixedWidth(220)
        self.rigol_frequency_btn = QPushButton("Set")
        self.layout4.addWidget(QLabel("Frequency [Hz]:"), 11, 0)
        self.rigol_frequency_btn.clicked.connect(self.toggle_rigol_frequency)
        self.layout4.addWidget(self.rigol_frequency, 11, 1)
        self.layout4.addWidget(self.rigol_frequency_btn, 11, 2)                                 

        # Function
        self.rigol_function_btn = QPushButton("Set")
        self.layout4.addWidget(QLabel("Function:"), 12, 0)
        self.functionTypeCombobox = QComboBox(self)
        self.functionTypeCombobox.addItems(['Sinusoid', 'Square', 'Ramp', 'Pulse', 'DC'])
        self.rigol_function_btn.clicked.connect(self.toggle_rigol_function) 
        self.layout4.addWidget(self.functionTypeCombobox,12, 1)
        self.layout4.addWidget(self.rigol_function_btn,12, 2)

        self.graph_voltage_trap = pg.PlotWidget()
        self.layout4.addWidget(self.graph_voltage_trap, 13, 0, 1, 7)
        self.rigol_data_x = []
        self.rigol_data_y = []     

        self.graph_voltage_trap.showGrid(x=True, y=True, alpha=1)           
        self.graph_voltage_trap.setLabel('left', 'Voltage [V]')
        self.graph_voltage_trap.setLabel('bottom', 'Time [seconds]')
 
        self.rigol_thread = RigolDataThread()
        self.rigol_thread.rigol_data_updated.connect(self.update_rigol)
        self.rigol_thread.start()
        
        def calculate_q():
            # Get input values from the text fields
            geometrical = input_geometrical.text()
            frequency = input_frequency.text()
            mass = input_mass.text()
            charge = input_charge.text()
            voltage = input_voltage.text()

            # Check for missing or zero values
            if geometrical == "" or frequency == "" or mass == "" or charge == "" or voltage == "":
                q = "Missing data"
            elif float(geometrical) == 0 or float(frequency) == 0 or float(mass) == 0:
                q = "Division by zero"
            else:
                # Convert input values to floating-point numbers
                mass = float(mass)
                charge = float(charge)
                geometrical = float(geometrical)
                voltage = float(voltage)
                frequency = float(frequency)

                # Calculate q using the given formula
                q = (4 * charge * voltage) / (geometrical**2 * frequency * mass)

            # Display the calculated q value on the UI
            q_calculated.setText(str(q))

        # Connect the "Calculate" button click event to the calculate_q function
        btn_calculate.clicked.connect(calculate_q)

        # ------------------------------------------------------------------------------------------- #
        # Temperature TAB
        # ------------------------------------------------------------------------------------------- #

        # ------------------------------------------------------------------------------------------- #
        # Data Processing TAB
        # ------------------------------------------------------------------------------------------- #

        # ------------------------------------------------------------------------------------------- #
        # Registers TAB
        # ------------------------------------------------------------------------------------------- #
        self.layout7 = QGridLayout(self.tab7)


        # -------------------------------------------------------------------- #
        # Rigol #
        rigol_data_group = QtWidgets.QGroupBox("Rigol MSO 5074 data:")
        rigol_data_grid_layout = QGridLayout()           

        self.rigol_1_checkbox = QCheckBox("Particle trap voltage")
        self.rigol_1_checkbox.setChecked(False)
        rigol_data_grid_layout.addWidget(self.rigol_1_checkbox, 1, 0)

        self.rigol_2_checkbox = QCheckBox("Particle trap offset voltage")
        self.rigol_2_checkbox.setChecked(False)
        rigol_data_grid_layout.addWidget(self.rigol_2_checkbox, 2, 0)

        self.rigol_3_checkbox = QCheckBox("Particle trap frequency")
        self.rigol_3_checkbox.setChecked(False)
        rigol_data_grid_layout.addWidget(self.rigol_3_checkbox, 3, 0)

        self.rigol_4_checkbox = QCheckBox("Particle trap function")
        self.rigol_4_checkbox.setChecked(False)
        rigol_data_grid_layout.addWidget(self.rigol_4_checkbox, 4, 0)

        self.rigol_5_checkbox = QCheckBox("Particle trap function generator status")
        self.rigol_5_checkbox.setChecked(False)
        rigol_data_grid_layout.addWidget(self.rigol_5_checkbox, 5, 0)  

        mark_all_rigol_button = QPushButton("Check all")
        mark_all_rigol_button.clicked.connect(self.mark_all_rigol_checkboxes)

        unmark_all_rigol_button = QPushButton("Uncheck all")
        unmark_all_rigol_button.clicked.connect(self.unmark_all_rigol_checkboxes)
  
        rigol_data_grid_layout.addWidget(mark_all_rigol_button, 6, 0)
        rigol_data_grid_layout.addWidget(unmark_all_rigol_button, 6, 1)        
        # -------------------------------------------------------------------- #
        # Laser #        
        laser_data_group = QtWidgets.QGroupBox("Laser data:")
        laser_data_grid_layout = QGridLayout()

        self.laser_1_checkbox = QCheckBox("Laser voltage")
        self.laser_1_checkbox.setChecked(False)
        laser_data_grid_layout.addWidget(self.laser_1_checkbox, 1, 0)

        self.laser_2_checkbox = QCheckBox("Laser state")
        self.laser_2_checkbox.setChecked(False)
        laser_data_grid_layout.addWidget(self.laser_2_checkbox, 2, 0)

        mark_all_laser_button = QPushButton("Check all")
        mark_all_laser_button.clicked.connect(self.mark_all_laser_checkboxes)

        unmark_all_laser_button = QPushButton("Uncheck all")
        unmark_all_laser_button.clicked.connect(self.unmark_all_laser_checkboxes)
  
        laser_data_grid_layout.addWidget(mark_all_laser_button, 3, 0)
        laser_data_grid_layout.addWidget(unmark_all_laser_button, 3, 1)   

        # -------------------------------------------------------------------- #
        
        # APD #
        apd_data_group = QtWidgets.QGroupBox("APD data:")
        apd_data_grid_layout = QGridLayout()

        #self.apd_1_checkbox = QCheckBox("APD Counts @1kHz")
        #self.apd_1_checkbox.setChecked(False)
        #apd_data_grid_layout.addWidget(self.apd_1_checkbox, 1, 0)

        self.apd_2_checkbox = QCheckBox("APD Counts @100kHz")
        self.apd_2_checkbox.setChecked(False)
        apd_data_grid_layout.addWidget(self.apd_2_checkbox, 2, 0)

        #self.apd_3_checkbox = QCheckBox("APD FFT @0.1Hz resolution")
        #self.apd_3_checkbox.setChecked(False)
        #apd_data_grid_layout.addWidget(self.apd_3_checkbox, 3, 0)

        self.apd_4_checkbox = QCheckBox("APD FFT @0.01Hz resolution")
        self.apd_4_checkbox.setChecked(False)
        apd_data_grid_layout.addWidget(self.apd_4_checkbox, 4, 0)

        mark_all_apd_button = QPushButton("Check all")
        mark_all_apd_button.clicked.connect(self.mark_all_apd_checkboxes)

        unmark_all_apd_button = QPushButton("Uncheck all")
        unmark_all_apd_button.clicked.connect(self.unmark_all_apd_checkboxes)
  
        apd_data_grid_layout.addWidget(mark_all_apd_button, 8, 0)
        apd_data_grid_layout.addWidget(unmark_all_apd_button, 8, 1)

        # -------------------------------------------------------------------- #
        # Twistorr #
        twistorr_data_group = QtWidgets.QGroupBox("Vacuum data:")
        twistorr_data_grid_layout = QGridLayout()   

        self.twistorr_1_checkbox = QCheckBox("Pump current (305FS)")
        self.twistorr_1_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_1_checkbox, 1, 0)

        self.twistorr_2_checkbox = QCheckBox("Pump voltage (305FS)")
        self.twistorr_2_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_2_checkbox, 2, 0)

        self.twistorr_3_checkbox = QCheckBox("Pump power (305FS)")
        self.twistorr_3_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_3_checkbox, 3, 0)

        self.twistorr_4_checkbox = QCheckBox("Pump frequency (305FS)")
        self.twistorr_4_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_4_checkbox, 4, 0)

        self.twistorr_5_checkbox = QCheckBox("Pump temperature (305FS)")
        self.twistorr_5_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_5_checkbox, 5, 0) 

        self.pressure_1_checkbox = QCheckBox("Pressure (FRG-702)")
        self.pressure_1_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.pressure_1_checkbox, 6, 0)  

        self.ups_checkbox = QCheckBox("UPS batteries (%)")
        self.ups_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.ups_checkbox, 7, 0)                 

        self.twistorr_6_checkbox = QCheckBox("Pump current (74FS)")
        self.twistorr_6_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_6_checkbox, 1, 1)

        self.twistorr_7_checkbox = QCheckBox("Pump voltage (74FS)")
        self.twistorr_7_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_7_checkbox, 2, 1)

        self.twistorr_8_checkbox = QCheckBox("Pump power (74FS)")
        self.twistorr_8_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_8_checkbox, 3, 1)

        self.twistorr_9_checkbox = QCheckBox("Pump frequency (74FS)")
        self.twistorr_9_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_9_checkbox, 4, 1)

        self.twistorr_10_checkbox = QCheckBox("Pump temperature (74FS)")
        self.twistorr_10_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_10_checkbox, 5, 1)             

        self.pressure_2_checkbox = QCheckBox("Pressure (CDG-500)")
        self.pressure_2_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.pressure_2_checkbox, 6, 1)  


        #self.twistorr_6_checkbox = QCheckBox("TwisTorr 305 FS error code") #error_comentado
        #self.twistorr_6_checkbox.setChecked(False)
        #twistorr_data_grid_layout.addWidget(self.twistorr_6_checkbox, 6, 0)                

        mark_all_twistorr_button = QPushButton("Check all")
        mark_all_twistorr_button.clicked.connect(self.mark_all_twistorr_checkboxes)

        unmark_all_twistorr_button = QPushButton("Uncheck all")
        unmark_all_twistorr_button.clicked.connect(self.unmark_all_twistorr_checkboxes)
  
        twistorr_data_grid_layout.addWidget(mark_all_twistorr_button, 8, 0)
        twistorr_data_grid_layout.addWidget(unmark_all_twistorr_button, 8, 1)



        # -------------------------------------------------------------------- #
        # Electron gun #
        eg_data_group = QtWidgets.QGroupBox("Electron gun data:")
        eg_data_grid_layout = QGridLayout()      

        self.eg_12_checkbox = QCheckBox("Operate status")
        self.eg_12_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_12_checkbox, 1, 0)

        self.eg_13_checkbox = QCheckBox("Status flags")
        self.eg_13_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_13_checkbox, 2, 0)

        self.eg_1_checkbox = QCheckBox("Energy voltage")
        self.eg_1_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_1_checkbox, 3, 0)

        self.eg_2_checkbox = QCheckBox("Focus voltage")
        self.eg_2_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_2_checkbox, 4, 0)

        self.eg_3_checkbox = QCheckBox("Wehnelt voltage")
        self.eg_3_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_3_checkbox, 5, 0)

        self.eg_4_checkbox = QCheckBox("Emission current")
        self.eg_4_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_4_checkbox, 6, 0)

        self.eg_5_checkbox = QCheckBox("Time per dot")
        self.eg_5_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_5_checkbox, 7, 0)

        self.eg_6_checkbox = QCheckBox("Scan position X")
        self.eg_6_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_6_checkbox, 1, 1)

        self.eg_7_checkbox = QCheckBox("Scan position Y")
        self.eg_7_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_7_checkbox, 2, 1)

        self.eg_8_checkbox = QCheckBox("Scan area X")
        self.eg_8_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_8_checkbox, 3, 1)

        self.eg_9_checkbox = QCheckBox("Scan area Y")
        self.eg_9_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_9_checkbox, 4, 1)

        self.eg_10_checkbox = QCheckBox("Scan grid X")
        self.eg_10_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_10_checkbox, 5, 1)

        self.eg_11_checkbox = QCheckBox("Scan grid Y")
        self.eg_11_checkbox.setChecked(False)
        eg_data_grid_layout.addWidget(self.eg_11_checkbox, 6, 1)               

        mark_all_eg_button = QPushButton("Check all")
        mark_all_eg_button.clicked.connect(self.mark_all_eg_checkboxes)

        unmark_all_eg_button = QPushButton("Uncheck all")
        unmark_all_eg_button.clicked.connect(self.unmark_all_eg_checkboxes)
  
        eg_data_grid_layout.addWidget(mark_all_eg_button, 8, 0)
        eg_data_grid_layout.addWidget(unmark_all_eg_button, 8, 1)

        # -------------------------------------------------------------------- #
        # FFT AVG #
        fft_data_group = QtWidgets.QGroupBox()
        fft_data_grid_layout = QGridLayout()      

        avg_fft_label = QLabel("Number of FFTs to average:")
        avg_fft_label.setFixedWidth(650) 
        #avg_fft_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.avg_fft_input = QLineEdit(self)
        self.avg_fft_input.setFixedWidth(60) 
        self.avg_fft_input.setText("5") # Default value  
        #self.avg_fft_input.setPlaceholderText("Number of FFTs to average")     
        fft_data_grid_layout.addWidget(avg_fft_label, 1, 0)
        fft_data_grid_layout.addWidget(self.avg_fft_input, 1, 1)   

        self.fft_note = QtWidgets.QLabel("Important: Consider that each FFT is obtained with a window of 100 seconds, i.e., if the field contains a '5', a record of the average of the FFTs obtained in a window of 500 seconds will be generated.")
        self.fft_note.setWordWrap(True) 
        self.fft_note.setStyleSheet("font-weight: bold;")
        #self.fft_note.setFixedWidth(300) 
        fft_data_grid_layout.addWidget(self.fft_note, 2, 0, 1, 2)

        twistorr_data_group.setLayout(twistorr_data_grid_layout)
        rigol_data_group.setLayout(rigol_data_grid_layout)    
        laser_data_group.setLayout(laser_data_grid_layout)
        apd_data_group.setLayout(apd_data_grid_layout)
        eg_data_group.setLayout(eg_data_grid_layout)
        fft_data_group.setFixedWidth(750) 
        fft_data_group.setLayout(fft_data_grid_layout)

        self.layout7.addWidget(laser_data_group, 1, 0)
        self.layout7.addWidget(apd_data_group, 2, 0)
        self.layout7.addWidget(twistorr_data_group, 0, 0)
        self.layout7.addWidget(rigol_data_group, 1, 1)
        self.layout7.addWidget(eg_data_group, 0, 1)
        self.layout7.addWidget(fft_data_group, 2, 1)


        self.storage_filename_group = QtWidgets.QGroupBox("Storage filename:")
        self.storage_filename_grid_layout = QGridLayout()                 

        self.storage_line_edit = QLineEdit()
        self.storage_line_edit.setFixedWidth(1150)
        self.storage_line_edit.setPlaceholderText("Enter a file name or press the 'Set datetime as filename' button...")
        self.storage_line_edit.setAlignment(QtCore.Qt.AlignRight)
        self.storage_filename_grid_layout.addWidget(self.storage_line_edit, 9, 0)
        
        h5_label = QLabel(".h5")
        h5_label.setFixedWidth(200)
        h5_label.setAlignment(QtCore.Qt.AlignLeft)
        self.storage_filename_grid_layout.addWidget(h5_label, 9, 1)

        self.auto_store_name_button = QPushButton("Set datetime as filename")
        self.auto_store_name_button.clicked.connect(self.set_datetime_as_filename)
        self.storage_filename_grid_layout.addWidget(self.auto_store_name_button, 10, 0, 1, 2)  

        self.storage_filename_group.setLayout(self.storage_filename_grid_layout)
        self.layout7.addWidget(self.storage_filename_group, 9, 0, 1, 2)

        storage_button_group = QtWidgets.QGroupBox("Data logging (files stored in /home/code folder):")
        storage_button_grid_layout = QGridLayout()      

        self.begin_logging_button = QPushButton("Begin data logging")
        self.begin_logging_button.setCheckable(True) 
        self.begin_logging_button.clicked.connect(self.logging_connect)
        storage_button_grid_layout.addWidget(self.begin_logging_button, 10, 0)  

        self.stop_logging_button = QPushButton("Stop data logging") 
        self.stop_logging_button.setCheckable(True) 
        self.stop_logging_button.clicked.connect(self.logging_disconnect)
        storage_button_grid_layout.addWidget(self.stop_logging_button, 10, 1)     

        storage_button_group.setLayout(storage_button_grid_layout)
        self.layout7.addWidget(storage_button_group, 10, 0, 1, 2)        
        
        #image_label = QLabel()
        #pixmap = QPixmap("hdf.png") 
        #image_label.setPixmap(pixmap)
        #image_label.setAlignment(Qt.AlignCenter)  
        #self.layout7.addWidget(image_label, 4, 8, 1, 3)        
        



        # ------------------------------------------------------------------------------------------------------------------ #
        #----------------------- FUNCTIONS --------------------------------------------------------------------------------- #
        # ------------------------------------------------------------------------------------------------------------------ #
    def logging_connect(self):
        # --------------------------#
        # Filename and checkbox verification #
        if not (self.laser_1_checkbox.isChecked() or self.laser_2_checkbox.isChecked() or
                self.apd_2_checkbox.isChecked() or self.apd_4_checkbox.isChecked() or
                self.twistorr_1_checkbox.isChecked() or self.twistorr_2_checkbox.isChecked() or
                self.twistorr_3_checkbox.isChecked() or self.twistorr_4_checkbox.isChecked() or
                self.twistorr_5_checkbox.isChecked() or self.twistorr_6_checkbox.isChecked() or 
                self.twistorr_7_checkbox.isChecked() or self.twistorr_8_checkbox.isChecked() or
                self.twistorr_9_checkbox.isChecked() or self.twistorr_10_checkbox.isChecked() or
                self.pressure_1_checkbox.isChecked() or self.pressure_2_checkbox.isChecked() or  
                self.rigol_1_checkbox.isChecked() or self.rigol_2_checkbox.isChecked() or
                self.rigol_3_checkbox.isChecked() or self.rigol_4_checkbox.isChecked() or
                self.rigol_5_checkbox.isChecked() or self.eg_1_checkbox.isChecked() or
                self.eg_2_checkbox.isChecked() or self.eg_3_checkbox.isChecked() or
                self.eg_4_checkbox.isChecked() or self.eg_5_checkbox.isChecked() or
                self.eg_6_checkbox.isChecked() or self.eg_7_checkbox.isChecked() or
                self.eg_8_checkbox.isChecked() or self.eg_9_checkbox.isChecked() or 
                self.eg_10_checkbox.isChecked() or self.eg_11_checkbox.isChecked() or
                self.ups_checkbox.isChecked()):
            self.showWarningSignal.emit("Select the variables to record before begin data logging...")
            self.begin_logging_button.setChecked(False)
        if not self.storage_line_edit.text():
            self.showWarningSignal.emit("Give the file a name or press the 'Set datetime as filename' button before starting data logging....")
            self.begin_logging_button.setChecked(False)

        if (self.laser_1_checkbox.isChecked() or self.laser_2_checkbox.isChecked() or
                self.apd_2_checkbox.isChecked() or self.apd_4_checkbox.isChecked() or
                self.twistorr_1_checkbox.isChecked() or self.twistorr_2_checkbox.isChecked() or
                self.twistorr_3_checkbox.isChecked() or self.twistorr_4_checkbox.isChecked() or
                self.twistorr_5_checkbox.isChecked() or self.twistorr_6_checkbox.isChecked() or 
                self.twistorr_7_checkbox.isChecked() or self.twistorr_8_checkbox.isChecked() or
                self.twistorr_9_checkbox.isChecked() or self.twistorr_10_checkbox.isChecked() or 
                self.pressure_1_checkbox.isChecked() or self.pressure_2_checkbox.isChecked() or
                self.rigol_1_checkbox.isChecked() or self.rigol_2_checkbox.isChecked() or
                self.rigol_3_checkbox.isChecked() or self.rigol_4_checkbox.isChecked() or
                self.rigol_5_checkbox.isChecked() or self.eg_1_checkbox.isChecked() or
                self.eg_2_checkbox.isChecked() or self.eg_3_checkbox.isChecked() or
                self.eg_4_checkbox.isChecked() or self.eg_5_checkbox.isChecked() or
                self.eg_6_checkbox.isChecked() or self.eg_7_checkbox.isChecked() or
                self.eg_8_checkbox.isChecked() or self.eg_9_checkbox.isChecked() or 
                self.eg_10_checkbox.isChecked() or self.eg_11_checkbox.isChecked() or
                self.ups_checkbox.isChecked()) and (self.storage_line_edit.text()):   

            if self.begin_logging_button.isChecked():
                self.begin_logging_button.setStyleSheet("background-color: darkblue;")
                self.begin_logging_button.setText("Begin data logging")
                self.logging_button.setStyleSheet("background-color: darkblue;")# color: black;")
                self.logging_button.setText("Data logging")
                self.begin_logging_button.setChecked(True)

                storage_list = []
                # Laser #
                storage_list.append("_-_"+self.storage_line_edit.text()+".h5")
                if self.laser_1_checkbox.isChecked():
                    storage_list.append("laser_voltage") 
                    print("Ejecutando recorder 'Laser voltage'")
                if self.laser_2_checkbox.isChecked():
                    storage_list.append("laser_state") 
                    print("Ejecutando recorder 'Laser state'")
                # --------------------------#
                # APD #
                #if self.apd_1_checkbox.isChecked():
                #    storage_list.append("apd_counts_partial") 
                #    print("Ejecutando recorder 'APD Counts @1kHz'")
                if self.apd_2_checkbox.isChecked():
                    storage_list.append("apd_counts_full") 
                    print("Ejecutando recorder 'APD Counts @100kHz'")
                #if self.apd_3_checkbox.isChecked():
                #    storage_list.append("apd_fft_partial") 
                #    print("Ejecutando recorder 'APD FFT @0.1Hz resolution'")
                if self.apd_4_checkbox.isChecked():
                    storage_list.append("apd_fft_full") 
                    print("Ejecutando recorder 'APD FFT @0.01Hz resolution'")       
                # --------------------------#
                # Twistor #
                if self.twistorr_1_checkbox.isChecked():
                    storage_list.append("pump1_current") 
                    print("Ejecutando recorder '305FS current'")
                if self.twistorr_2_checkbox.isChecked():
                    storage_list.append("pump1_voltage") 
                    print("Ejecutando recorder '305FS voltage'")
                if self.twistorr_3_checkbox.isChecked():
                    storage_list.append("pump1_power") 
                    print("Ejecutando recorder '305FS power'")
                if self.twistorr_4_checkbox.isChecked():
                    storage_list.append("pump1_frequency") 
                    print("Ejecutando recorder '305FS frequency'") 
                if self.twistorr_5_checkbox.isChecked():
                    storage_list.append("pump1_temperature") 
                    print("Ejecutando recorder '305FS temperature'")   
                #if self.twistorr_6_checkbox.isChecked(): error_comentado
                #    storage_list.append("pump1_error") 
                #    print("Ejecutando recorder 'TwisTorr 305 FS error code'")                                                
                if self.twistorr_6_checkbox.isChecked():
                    storage_list.append("pump2_current") 
                    print("Ejecutando recorder '74FS current'")
                if self.twistorr_7_checkbox.isChecked():
                    storage_list.append("pump2_voltage") 
                    print("Ejecutando recorder '74FS voltage'")
                if self.twistorr_8_checkbox.isChecked():
                    storage_list.append("pump2_power") 
                    print("Ejecutando recorder '74FS power'")
                if self.twistorr_9_checkbox.isChecked():
                    storage_list.append("pump2_frequency") 
                    print("Ejecutando recorder '74FS frequency'") 
                if self.twistorr_10_checkbox.isChecked():
                    storage_list.append("pump2_temperature") 
                    print("Ejecutando recorder '74FS temperature'")
                if self.pressure_1_checkbox.isChecked():
                    storage_list.append("pressure_1") 
                    print("Ejecutando recorder 'FRG-702'")
                if self.pressure_2_checkbox.isChecked():
                    storage_list.append("pressure_2") 
                    print("Ejecutando recorder 'CDG-500'")   
                if self.ups_checkbox.isChecked():
                    storage_list.append("ups_status") 
                    print("Ejecutando recorder 'UPS Batteries'")                                                            
                # --------------------------#
                # Rigol #
                if self.rigol_1_checkbox.isChecked():
                    storage_list.append("rigol_voltage") 
                    print("Ejecutando recorder 'Particle trap voltage'")
                if self.rigol_2_checkbox.isChecked():
                    storage_list.append("rigol_voltage_offset") 
                    print("Ejecutando recorder 'Particle trap offset voltage'")
                if self.rigol_3_checkbox.isChecked():
                    storage_list.append("rigol_frequency") 
                    print("Ejecutando recorder 'Particle trap frequency'")
                if self.rigol_4_checkbox.isChecked():
                    storage_list.append("rigol_function") 
                    print("Ejecutando recorder 'Particle trap function'") 
                if self.rigol_5_checkbox.isChecked():
                    storage_list.append("rigol_status") 
                    print("Ejecutando recorder 'Particle trap function generator status'")                     
                # --------------------------#
                # Electron gun #
                if self.eg_1_checkbox.isChecked():
                    storage_list.append("energy_voltage") 
                    print("Ejecutando recorder 'Energy voltage'")
                if self.eg_2_checkbox.isChecked():
                    storage_list.append("focus_voltage") 
                    print("Ejecutando recorder 'Focus voltage'")
                if self.eg_3_checkbox.isChecked():
                    storage_list.append("wehnelt_voltage") 
                    print("Ejecutando recorder 'Wehnelt voltage'")
                if self.eg_4_checkbox.isChecked():
                    storage_list.append("emission_current") 
                    print("Ejecutando recorder 'Emission current'") 
                if self.eg_5_checkbox.isChecked():
                    storage_list.append("time_per_dot") 
                    print("Ejecutando recorder 'Time per dot'")
                if self.eg_6_checkbox.isChecked():
                    storage_list.append("pos_x") 
                    print("Ejecutando recorder 'Scan position X'")
                if self.eg_7_checkbox.isChecked():
                    storage_list.append("pos_y") 
                    print("Ejecutando recorder 'Scan position Y'") 
                if self.eg_8_checkbox.isChecked():
                    storage_list.append("area_x") 
                    print("Ejecutando recorder 'Scan area X'") 
                if self.eg_9_checkbox.isChecked():
                    storage_list.append("area_y") 
                    print("Ejecutando recorder 'Scan area Y'")
                if self.eg_10_checkbox.isChecked():
                    storage_list.append("grid_x") 
                    print("Ejecutando recorder 'Scan grid X'")
                if self.eg_11_checkbox.isChecked():
                    storage_list.append("grid_y") 
                    print("Ejecutando recorder 'Scan grid Y'")  
                if self.eg_12_checkbox.isChecked():
                    storage_list.append("status") 
                    print("Ejecutando recorder 'Operate status'")                                                             
                if self.eg_13_checkbox.isChecked():
                    storage_list.append("status_flags") 
                    print("Ejecutando recorder 'Status flags'")                                                                 

                if storage_list:
                    storage_command = [self.binary_paths[12]] + [str(stg_arg) for stg_arg in storage_list]
                    print("Executing command:", " ".join(storage_command))
                    
                    # Execute storage with all elements of storage_list as arguments
                    self.processes[12] = subprocess.Popen(storage_command)
                    if self.apd_4_checkbox.isChecked():
                        if int(self.avg_fft_input.text()) >= 1:
                            print(int(self.avg_fft_input.text()))                           
                            command_fft_avg = [self.binary_paths[8], str(int(self.avg_fft_input.text()))]
                            self.processes[8] = subprocess.Popen(command_fft_avg)
                        else:
                            self.showWarningSignal.emit("Error: 'Number of FFTs to average' must be an integer greater than or equal to 1. Please stop data logging, set a valid number and try again.")

                record_list = []
                if (self.laser_1_checkbox.isChecked() or self.laser_2_checkbox.isChecked()):
                    record_list.append("DATA_LASER_MON")  
                if (self.apd_2_checkbox.isChecked()):   
                    record_list.append("DATA_APD_FULL")
                #if (self.apd_3_checkbox.isChecked() or self.apd_4_checkbox.isChecked()):     
                if (self.apd_4_checkbox.isChecked()):     
                    record_list.append("DATA_FFT_FULL")   
                if (self.twistorr_1_checkbox.isChecked() or self.twistorr_2_checkbox.isChecked() or
                        self.twistorr_3_checkbox.isChecked() or self.twistorr_4_checkbox.isChecked() or
                        self.twistorr_5_checkbox.isChecked() or self.twistorr_6_checkbox.isChecked() or
                        self.twistorr_7_checkbox.isChecked() or self.twistorr_8_checkbox.isChecked() or
                        self.twistorr_9_checkbox.isChecked() or self.twistorr_10_checkbox.isChecked() or
                        self.pressure_1_checkbox.isChecked() or self.pressure_2_checkbox.isChecked()): 
                    record_list.append("DATA_TT_MON")    
                if (self.rigol_1_checkbox.isChecked() or self.rigol_2_checkbox.isChecked() or
                        self.rigol_3_checkbox.isChecked() or self.rigol_4_checkbox.isChecked()):   
                    record_list.append("DATA_RIGOL_MON")
                if (self.eg_1_checkbox.isChecked() or self.eg_2_checkbox.isChecked() or
                        self.eg_3_checkbox.isChecked() or self.eg_4_checkbox.isChecked() or
                        self.eg_5_checkbox.isChecked() or self.eg_6_checkbox.isChecked() or
                        self.eg_7_checkbox.isChecked() or self.eg_8_checkbox.isChecked() or
                        self.eg_9_checkbox.isChecked() or self.eg_10_checkbox.isChecked() or
                        self.eg_11_checkbox.isChecked() or self.eg_12_checkbox.isChecked() or
                        self.eg_13_checkbox.isChecked()):
                    record_list.append("DATA_EG_MON")    
                print(record_list)
                if record_list:
                    record_command = [self.binary_paths[13]] + [str(rec_arg) for rec_arg in record_list]
                    print("Executing command:", " ".join(record_command))
                    # Execute record with all elements of record_list as arguments
                    self.processes[13] = subprocess.Popen(record_command)
            else:
                self.begin_logging_button.setChecked(True)

    def mark_all_laser_checkboxes(self):
        self.laser_1_checkbox.setChecked(True)
        self.laser_2_checkbox.setChecked(True)

    def unmark_all_laser_checkboxes(self):
        self.laser_1_checkbox.setChecked(False)
        self.laser_2_checkbox.setChecked(False)

    def mark_all_apd_checkboxes(self):
        #self.apd_1_checkbox.setChecked(True)
        self.apd_2_checkbox.setChecked(True)
        #self.apd_3_checkbox.setChecked(True)
        self.apd_4_checkbox.setChecked(True)

    def unmark_all_apd_checkboxes(self):
        #self.apd_1_checkbox.setChecked(False)
        self.apd_2_checkbox.setChecked(False)
        #self.apd_3_checkbox.setChecked(False)
        self.apd_4_checkbox.setChecked(False)    

    def mark_all_twistorr_checkboxes(self):
        self.twistorr_1_checkbox.setChecked(True)
        self.twistorr_2_checkbox.setChecked(True)
        self.twistorr_3_checkbox.setChecked(True)
        self.twistorr_4_checkbox.setChecked(True)
        self.twistorr_5_checkbox.setChecked(True)
        self.twistorr_6_checkbox.setChecked(True)
        self.twistorr_7_checkbox.setChecked(True)
        self.twistorr_8_checkbox.setChecked(True)
        self.twistorr_9_checkbox.setChecked(True)
        self.twistorr_10_checkbox.setChecked(True)
        self.pressure_1_checkbox.setChecked(True)
        self.pressure_2_checkbox.setChecked(True)
        self.ups_checkbox.setChecked(True)
        #self.twistorr_6_checkbox.setChecked(True)  #error_comentado      

    def unmark_all_twistorr_checkboxes(self):
        self.twistorr_1_checkbox.setChecked(False)
        self.twistorr_2_checkbox.setChecked(False)
        self.twistorr_3_checkbox.setChecked(False)
        self.twistorr_4_checkbox.setChecked(False)
        self.twistorr_5_checkbox.setChecked(False)
        self.twistorr_6_checkbox.setChecked(False)
        self.twistorr_7_checkbox.setChecked(False)
        self.twistorr_8_checkbox.setChecked(False)
        self.twistorr_9_checkbox.setChecked(False)
        self.twistorr_10_checkbox.setChecked(False)
        self.pressure_1_checkbox.setChecked(False)
        self.pressure_2_checkbox.setChecked(False)  
        self.ups_checkbox.setChecked(False)      
        #self.twistorr_6_checkbox.setChecked(False)  #error_comentado   


    def mark_all_eg_checkboxes(self):
        self.eg_1_checkbox.setChecked(True)
        self.eg_2_checkbox.setChecked(True)
        self.eg_3_checkbox.setChecked(True)
        self.eg_4_checkbox.setChecked(True)
        self.eg_5_checkbox.setChecked(True)
        self.eg_6_checkbox.setChecked(True)
        self.eg_7_checkbox.setChecked(True)
        self.eg_8_checkbox.setChecked(True)
        self.eg_9_checkbox.setChecked(True)
        self.eg_10_checkbox.setChecked(True)
        self.eg_11_checkbox.setChecked(True) 
        self.eg_12_checkbox.setChecked(True)
        self.eg_13_checkbox.setChecked(True) 

    def unmark_all_eg_checkboxes(self):
        self.eg_1_checkbox.setChecked(False)
        self.eg_2_checkbox.setChecked(False)
        self.eg_3_checkbox.setChecked(False)
        self.eg_4_checkbox.setChecked(False)
        self.eg_5_checkbox.setChecked(False)
        self.eg_6_checkbox.setChecked(False)
        self.eg_7_checkbox.setChecked(False)
        self.eg_8_checkbox.setChecked(False)
        self.eg_9_checkbox.setChecked(False)
        self.eg_10_checkbox.setChecked(False)
        self.eg_11_checkbox.setChecked(False)     
        self.eg_12_checkbox.setChecked(False)
        self.eg_13_checkbox.setChecked(False) 

    def mark_all_rigol_checkboxes(self):
        self.rigol_1_checkbox.setChecked(True)
        self.rigol_2_checkbox.setChecked(True)
        self.rigol_3_checkbox.setChecked(True)
        self.rigol_4_checkbox.setChecked(True)
        self.rigol_5_checkbox.setChecked(True)

    def unmark_all_rigol_checkboxes(self):
        self.rigol_1_checkbox.setChecked(False)
        self.rigol_2_checkbox.setChecked(False)
        self.rigol_3_checkbox.setChecked(False)
        self.rigol_4_checkbox.setChecked(False)
        self.rigol_5_checkbox.setChecked(False) 

    def logging_disconnect(self):
        if self.stop_logging_button.isChecked():
            self.begin_logging_button.setChecked(False)
            self.begin_logging_button.setStyleSheet("background-color: 53, 53, 53;")
            self.begin_logging_button.setText("Begin data logging")
            self.logging_button.setStyleSheet("background-color: 53, 53, 53;")# color: black;")
            self.logging_button.setText("Data logging")
            self.stop_logging_button.setChecked(False)
            ### KIL STORAGE
            if self.apd_4_checkbox.isChecked():
                subprocess.run(['pkill', '-f', self.processes[8].args[0]], check=True)
            subprocess.run(['pkill', '-f', self.processes[12].args[0]], check=True)
            subprocess.run(['pkill', '-f', self.processes[13].args[0]], check=True)

    def set_datetime_as_filename(self):
        date_time = datetime.now()
        formatted_datetime = date_time.strftime("CoDE_dataset_%Y%m%d_%H:%M:%S")
        self.storage_line_edit.setText(formatted_datetime)

    def toggle_apd_connect(self):    
        if self.apd_button.isChecked():
            self.apd_button.setStyleSheet("background-color: darkblue;")
            self.toggle_process(2, True)
            self.buttons[2].setChecked(True)
            self.buttons[2].setStyleSheet("background-color: darkblue;")
            self.apd_button.setText("APD")
        else:
            self.apd_button.setStyleSheet("background-color: 53, 53, 53;")
            self.toggle_process(2, False)
            self.buttons[2].setChecked(False)
            self.buttons[2].setStyleSheet("background-color: 53, 53, 53;")
            self.apd_button.setText("APD")

    # Start asking for data to rigol
    def toggle_rigol_connect(self):
        if self.rigol_connect.isChecked():
            self.rigol_connect.setStyleSheet("background-color: darkblue;")
            self.rigol = True
            self.trap_button.setChecked(True)
            self.toggle_trap_connect()
            self.rigol_channel.setChecked(True)
            self.toggle_rigol_channel()
            self.rigol_thread.set_rigol_status(1)
        else:
            self.rigol_connect.setStyleSheet("background-color: 53, 53, 53;")
            self.rigol = False 
            self.rigol_chn = False
            self.rigol_thread.set_rigol_status(2)
            self.rigol_channel.setStyleSheet("background-color: 53, 53, 53;")
            self.trap_button.setChecked(False)
            self.toggle_trap_connect()
            self.rigol_channel.setChecked(False)
            self.rigol_channel.setText("Set oscilloscope channel")
            self.toggle_rigol_particle_trap_enable()

    # Set input channel 1/2
    def toggle_rigol_channel(self):
        if self.rigol_connect.isChecked():
            self.rigol_chn = True
            if self.rigol_channel.isChecked():
                self.rigol_channel.setStyleSheet("background-color: yellow; color: black;")
                self.rigol_thread.set_rigol_channel(1)
                self.rigol_channel.setText("Channel 1 [Oscilloscope]")
                self.color = 'y'
                self.rigol_chn_n = 1
            else:
                self.rigol_channel.setStyleSheet("background-color: cyan; color: black;")
                self.rigol_thread.set_rigol_channel(2)
                self.rigol_channel.setText("Channel 2 [Oscilloscope]")  
                self.color = 'c'
                self.rigol_chn_n = 2
        else:
            self.rigol_channel.setChecked(False)  
            self.rigol_chn = False    

    def btn_rigol_auto(self):
        self.rigol_thread.set_rigol_auto(1)

    def toggle_rigol_tscale(self):
        tscale = float(self.rigol_time_scale.text())/10
        self.rigol_thread.set_rigol_tscale(1, tscale)

    def toggle_rigol_voltage(self):
        volt = float(self.rigol_voltage.text())
        self.rigol_thread.set_rigol_voltage(1, volt)
        
    def toggle_rigol_voltage_offset(self):
        volt_offset = float(self.rigol_voltage_offset.text())
        self.rigol_thread.set_rigol_voltage_offset(1, volt_offset)

    def toggle_rigol_frequency(self):
        freq = float(self.rigol_frequency.text())
        self.rigol_thread.set_rigol_frequency(1, freq)

    def toggle_rigol_function(self):
        function = self.functionTypeCombobox.currentText()
        self.rigol_thread.set_rigol_function(1, function)
        
    def toggle_rigol_coupling(self):
        coupling = self.couplingCombobox.currentText()
        self.rigol_thread.set_rigol_coupling(1, coupling)
        
    def toggle_rigol_attenuation(self): 
        attenuation = float(self.attenuationCombobox.currentText())
        if int(attenuation) >= 1:
            attenuation = int(self.attenuationCombobox.currentText())
        self.rigol_thread.set_rigol_attenuation(1, attenuation)

    def toggle_rigol_vscale(self):
        vscale = float(self.rigol_vscale.text())
        self.rigol_thread.set_rigol_vscale(1, vscale)        

    def toggle_trap_connect(self):
        global g_global
        if self.trap_button.isChecked():
            if self.rigol_connect.isChecked():
                pass
            else:
                self.rigol_connect.setChecked(True)
                self.rigol_channel.setChecked(True)
                self.toggle_rigol_connect()
            self.trap_button.setStyleSheet("background-color: darkblue;")# color: black;")
            self.trap_button.setText("Trap")
            g_global = 1
        else:
            if self.rigol_connect.isChecked():
                self.rigol_connect.setChecked(False)
                self.rigol_channel.setChecked(False)
                self.toggle_rigol_connect()
            else:
                pass
            self.trap_button.setStyleSheet("background-color: 53, 53, 53;")
            self.trap_button.setText("Trap")
            self.voltage_monitor.setText("N/C")
            self.offset_monitor.setText("N/C")
            self.frequency_monitor.setText("N/C")
            g_global = 0

    def toggle_laser_connect(self):
        global g2_global
        if self.trap_button.isChecked():
            pass
        else:
            self.rigol_thread.set_rigol_channel(3)
            self.rigol_chn_n = 3

        if self.laser_button.isChecked():
            self.laser_button.setStyleSheet("background-color: darkblue;")# color: black;")
            self.rigol_thread.set_rigol_g2(1, 1)
            g2_global = 1
        else:
            self.laser_button.setStyleSheet("background-color: 53, 53, 53;")
            self.rigol_thread.set_rigol_g2(1, 0)
            g2_global = 0

    def toggle_rigol_particle_trap_enable(self):
        if self.rigol_connect.isChecked():
            if self.rigol_particle_trap_enable_btn.isChecked():
                self.rigol_particle_trap_enable_btn.setStyleSheet("background-color: darkblue;")
                self.rigol_thread.set_rigol_g1(1, 1)
            else:
                self.rigol_particle_trap_enable_btn.setStyleSheet("background-color: 53, 53, 53;")
                self.rigol_thread.set_rigol_g1(1, 0)
        else:
            self.rigol_particle_trap_enable_btn.setChecked(False)
            self.rigol_particle_trap_enable_btn.setStyleSheet("background-color: 53, 53, 53;")

    def toggle_laser_voltage(self):
        laser_voltage = float(self.rigol_laser_voltage.text())
        self.rigol_thread.set_rigol_laser_voltage(1, laser_voltage)

    def execute_twistorr_bar_btn(self):
        if self.pressure_button.isChecked():
            self.pressure_button.setStyleSheet("background-color: darkblue;")
            self.btn_vacuum_monitor.setChecked(True)
            self.btn_vacuum_monitor.setStyleSheet("background-color: darkblue;")
            self.execute_twistorr_monitor()
        else:
            if ((self.secure_eg == 1) and (self.eg_connection_btn.isChecked())):
                self.btn_vacuum_monitor.setChecked(True)
                self.showWarningSignal.emit("Some security methods that depend on vacuum data were implemented to protect the electron gun, you cannot disconnect from the vacuum equipment if you are connected to the electron gun...")
            else:
                self.pressure_button.setStyleSheet("background-color: 53, 53, 53;")
                self.btn_vacuum_monitor.setChecked(False)
                self.btn_vacuum_monitor.setStyleSheet("background-color: 53, 53, 53;")
                self.kill_twistorr_monitor()

    def execute_prevac_bar_btn(self):
        if self.prevac_button.isChecked():
            self.prevac_button.setStyleSheet("background-color: darkblue;")
            self.eg_connection_btn.setChecked(True)
            self.eg_connection_btn.setStyleSheet("background-color: darkblue;")
            self.execute_electrongun_monitor()
        else:
            self.prevac_button.setStyleSheet("background-color: 53, 53, 53;")
            self.eg_connection_btn.setChecked(False)
            self.eg_connection_btn.setStyleSheet("background-color: 53, 53, 53;")
            self.kill_electrongun_monitor()

    def execute_twistorr_btn(self):
        if self.btn_vacuum_monitor.isChecked():
            self.btn_vacuum_monitor.setStyleSheet("background-color: darkblue;")
            self.pressure_button.setChecked(True)
            self.pressure_button.setStyleSheet("background-color: darkblue;")
            self.execute_twistorr_monitor()
        else:
            if ((self.secure_eg == 1) and (self.eg_connection_btn.isChecked())):
                self.pressure_button.setChecked(True)
                self.showWarningSignal.emit("Some security methods that depend on vacuum data were implemented to protect the electron gun, you cannot disconnect from the vacuum equipment if you are connected to the electron gun...")
            else:
                self.btn_vacuum_monitor.setStyleSheet("background-color: 53, 53, 53;")
                self.pressure_button.setChecked(False)
                self.pressure_button.setStyleSheet("background-color: 53, 53, 53;")
                self.kill_twistorr_monitor()

    def tt_startstop1_button(self):
        if self.btn_vacuum_monitor.isChecked():
            if self.btn_tt_startstop1.isChecked():
                if (float(self.vacuum_pressure1)>0.5):
                    self.btn_tt_startstop1.setChecked(False) 
                    self.showWarningSignal.emit("Pressure in FRG-702 is very high, make sure the robust pumps are running...")                
                else:
                    self.btn_tt_startstop1.setStyleSheet("background-color: darkblue;")
                    #os.system('echo code | sudo -S systemctl stop twistorrmonitor.service') 
                    self.execute_twistorr_ss1("1")  
                    #os.system('echo code | sudo -S systemctl start twistorrmonitor.service') 
                    time.sleep(0.1) 
            else:
                self.btn_tt_startstop1.setStyleSheet("background-color: 53, 53, 53;")
                #os.system('echo code | sudo -S systemctl stop twistorrmonitor.service') 
                self.execute_twistorr_ss1("0")
                #os.system('echo code | sudo -S systemctl start twistorrmonitor.service') 
                time.sleep(0.1)

    def execute_twistorr_ss1(self, start_stop1):
        self.setting_twistorr = 1
        subprocess.run(['pkill', '-f', self.processes[11].args[0]], check=True)
        time.sleep(0.1)
        command_ss1 = [self.binary_paths[14], str(start_stop1)]
        self.processes[14] = subprocess.Popen(command_ss1)
        time.sleep(0.1)
        subprocess.run(['pkill', '-f', self.processes[14].args[0]], check=True)  
        self.processes[11] = subprocess.Popen([self.binary_paths[11]])
        time.sleep(0.1)
        self.setting_twistorr = 0

    def tt_startstop2_button(self):
        if self.btn_vacuum_monitor.isChecked():
            if self.btn_tt_startstop2.isChecked():
                if (float(self.vacuum_pressure1)>0.5):
                    self.btn_tt_startstop2.setChecked(False) 
                    self.showWarningSignal.emit("Pressure in FRG-702 is very high, make sure the robust pumps are running...")                
                else:
                    self.btn_tt_startstop2.setStyleSheet("background-color: darkblue;")
                    #os.system('echo code | sudo -S systemctl stop twistorrmonitor.service') 
                    self.execute_twistorr_ss2("1")  
                    #os.system('echo code | sudo -S systemctl start twistorrmonitor.service') 
                    time.sleep(0.1) 
            else:
                self.btn_tt_startstop2.setStyleSheet("background-color: 53, 53, 53;")
                #os.system('echo code | sudo -S systemctl stop twistorrmonitor.service') 
                self.execute_twistorr_ss2("0")
                #os.system('echo code | sudo -S systemctl start twistorrmonitor.service') 
                time.sleep(0.1)

    def execute_twistorr_ss2(self, start_stop2):
        self.setting_twistorr = 1
        subprocess.run(['pkill', '-f', self.processes[11].args[0]], check=True)
        time.sleep(0.1)
        command_ss2 = [self.binary_paths[15], str(start_stop2)]
        self.processes[15] = subprocess.Popen(command_ss2)
        time.sleep(0.1)
        subprocess.run(['pkill', '-f', self.processes[15].args[0]], check=True)   
        self.processes[11] = subprocess.Popen([self.binary_paths[11]])   
        time.sleep(0.1)  
        self.setting_twistorr = 0  

    def pressure_plot_button(self):
        if self.btn_pressure.isChecked():
            self.btn_pressure.setStyleSheet("background-color: darkblue;")
            self.pressure_plotting_state = 1 
            time.sleep(0.1) 
        else:
            self.btn_pressure.setStyleSheet("background-color: 53, 53, 53;")
            self.pressure_plotting_state = 0 
            time.sleep(0.1)    

    def update_vacuum_values(self):
        # Update the prevac-related values from twistorr_subscribing_values
        current_time = int(time.time())
        current_time_float = float(time.time())
        last_digit = current_time % 10
        datetime_obj = datetime.fromtimestamp(current_time)
        update_transient_status = 0

        process_name = self.binary_paths[11]
        if (self.setting_twistorr == 0 and self.closing == 0):
            try:
                result = subprocess.run(["pgrep", "-f", process_name], capture_output=True, text=True)
                if result.returncode == 0:
                    pass
                else:
                    print("TwisTorr monitor not running, starting again...")
                    self.processes[11] = subprocess.Popen([self.binary_paths[11]])
            except Exception as e:
                print("Error checking TwisTorr monitor:", str(e))
        
        if current_time_float - self.vacuum_plot_time >= 0.5:
            if self.btn_vacuum_monitor.isChecked():
                self.update_graph_pressure()
            self.vacuum_plot_time = current_time_float  
            update_transient_status = 1
            if last_digit == 0:
                self.kill_twistorr_monitor()
                #print("Restarting TwisTorr monitor... Time: ", datetime_obj)
                #os.system('echo code | sudo -S systemctl restart twistorrmonitor.service') 
        
        if self.btn_vacuum_monitor.isChecked():
            if len(twistorr_subscribing_values) >= 14:
                vacuum_status1 = int(twistorr_subscribing_values[0])
                vacuum_current1 = str(int(twistorr_subscribing_values[1]))
                vacuum_voltage1 = str(int(twistorr_subscribing_values[2]))
                vacuum_power1 = str(int(twistorr_subscribing_values[3]))
                vacuum_frequency1 = str(int(twistorr_subscribing_values[4]))
                vacuum_temperature1 = str(int(twistorr_subscribing_values[5]))
                vacuum_status2 = int(twistorr_subscribing_values[6])
                vacuum_current2 = str(int(twistorr_subscribing_values[7]))
                vacuum_voltage2 = str(int(twistorr_subscribing_values[8]))
                vacuum_power2 = str(int(twistorr_subscribing_values[9]))
                vacuum_frequency2 = str(int(twistorr_subscribing_values[10]))
                vacuum_temperature2 = str(int(twistorr_subscribing_values[11]))  
                UPS_Battery = str(int(twistorr_subscribing_values[14])) 
                UPS_Battery_int = int(twistorr_subscribing_values[14])

                if UPS_Battery_int < ups_critical_value:
                    self.showWarningSignal.emit("UPS batteries are below {}%, turning off electron gun and vacuum pumps (305FS and 74FS)...".format(ups_critical_value))
                    ## Turning off electron gun
                    if self.eg_connection_btn.isChecked():
                        self.secure_eg = 0
                        arg_standby = "0 14 0 1"
                        self.execute_prevac_setter(arg_standby)                                     
                        self.secure_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                        self.secure_eg_btn.setChecked(False)  
                        self.standby_eg_btn.setStyleSheet("background-color: darkblue; color: white;")
                        self.standby_eg_btn.setChecked(True) 
                        self.operate_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                        self.operate_eg_btn.setChecked(False)   
                    # Turning off pumps
                    self.execute_twistorr_ss1("0")
                    self.execute_twistorr_ss2("0")

                if update_transient_status == 1:
                    if self.previous_power1 is not None:
                        if len(self.vacuum_power1_history) == 10:
                            self.vacuum_power1_history.pop(0)
                        self.vacuum_power1_history.append(float(vacuum_power1))                     
                        x_305 = np.arange(len(self.vacuum_power1_history))
                        slope_305, _ = np.polyfit(x_305, self.vacuum_power1_history, 1)
                        print("Pendiente 305: ", slope_305)
                        print(self.vacuum_power1_history)
                        
                        if ((self.vacuum_power1_history.count(0) >= 2) and (slope_305 > 0.01)):
                            self.steady_state_1 = False
                        elif ((slope_305 < 0.001) and (float(vacuum_power1) < self.transient_value_1)):
                            self.steady_state_1 = True
                        #else:
                        #    self.steady_state_1 = True
                    self.previous_power1 = float(vacuum_power1)

                    if self.previous_power2 is not None:
                        if len(self.vacuum_power2_history) == 10:
                            self.vacuum_power2_history.pop(0)
                        self.vacuum_power2_history.append(float(vacuum_power2))
                        x_74 = np.arange(len(self.vacuum_power2_history))
                        slope_74, _ = np.polyfit(x_74, self.vacuum_power2_history, 1)                                
                        print("Pendiente 74: ", slope_74) 
                        print(self.vacuum_power2_history)

                        if ((self.vacuum_power2_history.count(0) >= 2) and (slope_74 > 0.01)):
                            self.steady_state_2 = False
                        elif ((slope_74 < 0.001) and (float(vacuum_power2) < self.transient_value_2)):
                            self.steady_state_2 = True
                        #else:
                        #    self.steady_state_2 = True                
                    self.previous_power2 = float(vacuum_power2)

                    print("Steady state 305FS: ", self.steady_state_1, "... Zeros: ", self.vacuum_power1_history.count(0))
                    print("Steady state _74FS: ", self.steady_state_2, "... Zeros: ", self.vacuum_power2_history.count(0))
                    if self.steady_state_1 or self.steady_state_2:                   
                        if ((float(vacuum_power1) > self.transient_value_1) and (vacuum_frequency1 == self.max_freq_1)):
                            #print("¡Alerta! Aumento en la potencia de 305FS.")
                            self.showWarningSignal.emit("TwisTorr 305FS pump increased its power to {}W, a command has been sent to turn it off...".format(float(vacuum_power1)))
                            self.execute_twistorr_ss1("0") 
                            #time.sleep(0.2) 
                        if ((float(vacuum_power2) > self.transient_value_2) and (vacuum_frequency2 == self.max_freq_2)):
                            #print("¡Alerta! Aumento en la potencia de 74FS.")   
                            self.showWarningSignal.emit("TwisTorr 74FS pump increased its power to {}W, a command has been sent to turn it off...".format(float(vacuum_power2)))
                            self.execute_twistorr_ss2("0")
                            #time.sleep(0.2)  
                    update_transient_status = 0

                pre_vacuum_pressure1 = float(twistorr_subscribing_values[12])
                pre_vacuum_pressure2 = float(twistorr_subscribing_values[13])   

                self.vacuum_pressure1 = str("{:.2E}".format(pre_vacuum_pressure1))
                self.vacuum_pressure2 = str("{:.2E}".format(pre_vacuum_pressure2))

                # Update the labels with the vacuum-related values
                self.ups_monitor.setText(UPS_Battery+"%")
                self.monitor_vacuum_current.setText(vacuum_current1+" [mA]")
                self.monitor_vacuum_voltage.setText(vacuum_voltage1+" [Vdc]")
                self.monitor_vacuum_power.setText(vacuum_power1+" [W]")
                self.monitor_vacuum_frequency.setText(vacuum_frequency1+" [Hz]")
                self.monitor_vacuum_temperature.setText(vacuum_temperature1+" [°C]")
                self.vacuum_frequency.setText(vacuum_frequency1+" [Hz]")
                self.monitor_vacuum_current2.setText(vacuum_current2+" [mA]")
                self.monitor_vacuum_voltage2.setText(vacuum_voltage2+" [Vdc]")
                self.monitor_vacuum_power2.setText(vacuum_power2+" [W]")
                self.monitor_vacuum_frequency2.setText(vacuum_frequency2+" [Hz]")
                self.monitor_vacuum_temperature2.setText(vacuum_temperature2+" [°C]")
                self.vacuum_frequency2.setText(vacuum_frequency2+" [Hz]")
                self.FR_pressure.setText(self.vacuum_pressure1+" [Torr]")
                
                if (vacuum_status1 == 1):
                    self.btn_tt_startstop1.setChecked(True)
                    self.btn_tt_startstop1.setStyleSheet("background-color: darkblue;")
                else:
                    self.btn_tt_startstop1.setChecked(False)
                    self.btn_tt_startstop1.setStyleSheet("background-color: 53, 53, 53;") 

                if (vacuum_status2 == 1):
                    self.btn_tt_startstop2.setChecked(True)
                    self.btn_tt_startstop2.setStyleSheet("background-color: darkblue;")
                else:
                    self.btn_tt_startstop2.setChecked(False)
                    self.btn_tt_startstop2.setStyleSheet("background-color: 53, 53, 53;")                                        
        else:        
            self.monitor_vacuum_current.setText("N/C")
            self.monitor_vacuum_voltage.setText("N/C")
            self.monitor_vacuum_power.setText("N/C")
            self.monitor_vacuum_frequency.setText("N/C")
            self.monitor_vacuum_temperature.setText("N/C")
            self.vacuum_frequency.setText("N/C")      
            self.monitor_vacuum_current2.setText("N/C")
            self.monitor_vacuum_voltage2.setText("N/C")
            self.monitor_vacuum_power2.setText("N/C")
            self.monitor_vacuum_frequency2.setText("N/C")
            self.monitor_vacuum_temperature2.setText("N/C")
            self.vacuum_frequency2.setText("N/C")  
            self.FR_pressure.setText("N/C")
            self.ups_monitor.setText("N/C")
            self.btn_tt_startstop1.setChecked(False)
            self.btn_tt_startstop1.setStyleSheet("background-color: 53, 53, 53;") 
            self.btn_tt_startstop2.setChecked(False)
            self.btn_tt_startstop2.setStyleSheet("background-color: 53, 53, 53;")                        
        # Sleep briefly to avoid excessive updates
        #time.sleep(0.01)

    def start_update_tt_timer(self):
        # Start a QTimer to periodically update vacuum-related values
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_vacuum_values)
        self.timer.start(10)  # Update interval for Twistorr monitoring

    def stop_update_tt_timer(self):
        # Stop the QTimer used for updating vacuum-related values
        if hasattr(self, 'timer'):
            self.timer.stop()

    def execute_twistorr_monitor(self):
        self.processes[11] = subprocess.Popen([self.binary_paths[11]])
        #os.system('echo code | sudo -S systemctl start twistorrmonitor.service')  
 
    def kill_twistorr_monitor(self):
        #os.system('echo code | sudo -S systemctl stop twistorrmonitor.service')
        subprocess.run(['pkill', '-f', self.processes[11].args[0]], check=True)

    def execute_electrongun_monitor(self):
        self.processes[16] = subprocess.Popen([self.binary_paths[16]])
        #os.system('echo code | sudo -S systemctl start prevacmonitor.service')

    def kill_electrongun_monitor(self):
        #os.system('echo code | sudo -S systemctl stop prevacmonitor.service')
        subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True)

    def execute_electrongun_btn(self):
        if self.eg_connection_btn.isChecked():
            self.eg_connection_btn.setStyleSheet("background-color: darkblue;")
            self.prevac_button.setStyleSheet("background-color: darkblue;")
            self.eg_connection_btn.setChecked(True)
            self.execute_electrongun_monitor()
        else:
            self.eg_connection_btn.setStyleSheet("background-color: 53, 53, 53;")
            self.prevac_button.setStyleSheet("background-color: 53, 53, 53;")
            self.eg_connection_btn.setChecked(False)
            self.kill_electrongun_monitor()

    def execute_prevac_setter(self, prevac_arg):
        prevac_command = [self.binary_paths[17], *prevac_arg.split()]
        self.processes[17] = subprocess.Popen(prevac_command)
        print(prevac_command)
        #subprocess.run(['pkill', '-f', self.processes[17].args[0]], check=True)  
        self.processes[16] = subprocess.Popen([self.binary_paths[16]])

    def enb_eg_onoff(self):
        # Toggle the spec mode and update toggle button style
        if self.eg_connection_btn.isChecked():
            if self.btn_vacuum_monitor.isChecked():
                if (0 < float(self.vacuum_pressure1) < reqPress4ElectronGun):
                    if self.secure_eg_btn.isChecked():
                        self.secure_eg_btn.setStyleSheet("background-color: red; color: white;")
                        self.secure_eg = 1  
                    else:
                        self.secure_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                        self.secure_eg = 0  
                        self.secure_eg_btn.setChecked(False)   
                else:
                    self.secure_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                    self.secure_eg = 0  
                    self.secure_eg_btn.setChecked(False) 
                    self.showWarningSignal.emit("To enable the electron gun, the pressure measured by the FRG-702 sensor must be less than {} [Torr], please wait until it reaches a pressure lower than that indicated...".format(reqPress4ElectronGun))  
            else:
                self.secure_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                self.secure_eg = 0  
                self.secure_eg_btn.setChecked(False) 
                self.showWarningSignal.emit("To enable the electron gun you must first be connected to the vacuum equipment...")                
        else:
            self.secure_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
            self.secure_eg = 0  
            self.secure_eg_btn.setChecked(False)    

    def enb_eg_operate(self):
        if self.eg_connection_btn.isChecked():
            if (self.secure_eg == 1):
                float_v = 0
                if self.operate_eg_btn.isChecked():
                    float_v = 1
                    self.operate_eg_btn.setStyleSheet("background-color: green; color: white;")
                else:
                    float_v = 0
                    self.operate_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True)
                decimal_part1 = (float_v >> 8) & 0xFF
                decimal_part2 = float_v & 0xFF
                arg = "0 13 " + str(decimal_part1) + " " + str(decimal_part2)
                print("AQUI SE ENVIARÍA EL COMANDO " + arg + " (OPERATE)")
                self.execute_prevac_setter(arg)  
                time.sleep(0.1) 
            else:
                self.operate_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                self.operate_eg_btn.setChecked(False) 
                self.showWarningSignal.emit("First you must press the 'Enable power on/off' button...")                     

    def enb_eg_standby(self):        
        if self.eg_connection_btn.isChecked():
            if (self.secure_eg == 1):
                float_v = 0
                if self.standby_eg_btn.isChecked():
                    float_v = 1
                    self.standby_eg_btn.setStyleSheet("background-color: darkblue; color: white;")
                    self.secure_eg = 0
                    self.secure_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                    self.secure_eg_btn.setChecked(False) 
                else:
                    float_v = 0
                    self.standby_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True)
                decimal_part1 = (float_v >> 8) & 0xFF
                decimal_part2 = float_v & 0xFF
                arg = "0 14 " + str(decimal_part1) + " " + str(decimal_part2)
                print("AQUI SE ENVIARÍA EL COMANDO " + arg + " (STAND BY)")
                self.execute_prevac_setter(arg)  
                time.sleep(0.1) 
            else:
                self.standby_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                self.standby_eg_btn.setChecked(False) 
                self.showWarningSignal.emit("First you must press the 'Enable power on/off' button...") 

    def prevac_set_ev(self):
        if self.eg_connection_btn.isChecked():
            subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True) 
            float_text = self.eg_energy_voltage_setval.text()
            float_v = float(float_text)
            hex_str = hex(struct.unpack('<I', struct.pack('<f', float_v))[0])[2:]
            hex_1 = hex_str[:4] 
            hex_2 = hex_str[4:8] 
            if hex_1 != '':
                decimal_part1 = int(hex_1, 16)
            else:
                decimal_part1 = 0
            if hex_2 != '':
                decimal_part2 = int(hex_2, 16)
            else:
                decimal_part2 = 0 
            arg = "0 15 " + str(decimal_part1) + " " + str(decimal_part2) 
            self.execute_prevac_setter(arg)  
            time.sleep(0.1) 

    def prevac_set_fv(self):
        if self.eg_connection_btn.isChecked():
            subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True) 
            float_text = self.eg_focus_voltage_setval.text()
            float_v = float(float_text)
            hex_str = hex(struct.unpack('<I', struct.pack('<f', float_v))[0])[2:]
            hex_1 = hex_str[:4] 
            hex_2 = hex_str[4:8] 
            if hex_1 != '':
                decimal_part1 = int(hex_1, 16)
            else:
                decimal_part1 = 0
            if hex_2 != '':
                decimal_part2 = int(hex_2, 16)
            else:
                decimal_part2 = 0 
            arg = "0 17 " + str(decimal_part1) + " " + str(decimal_part2)
            self.execute_prevac_setter(arg)  
            time.sleep(0.1) 

    def prevac_set_wv(self):
        if self.eg_connection_btn.isChecked():
            subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True) 
            float_text = self.eg_wehnelt_voltage_setval.text()
            float_v = float(float_text)
            hex_str = hex(struct.unpack('<I', struct.pack('<f', float_v))[0])[2:]
            hex_1 = hex_str[:4] 
            hex_2 = hex_str[4:8] 
            if hex_1 != '':
                decimal_part1 = int(hex_1, 16)
            else:
                decimal_part1 = 0
            if hex_2 != '':
                decimal_part2 = int(hex_2, 16)
            else:
                decimal_part2 = 0 
            arg = "0 19 " + str(decimal_part1) + " " + str(decimal_part2)
            self.execute_prevac_setter(arg)  
            time.sleep(0.1) 

    def prevac_set_ec(self):
        if self.eg_connection_btn.isChecked():
            subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True) 
            float_text = self.eg_emission_current_setval.text()
            float_v = float(float_text)
            hex_str = hex(struct.unpack('<I', struct.pack('<f', float_v))[0])[2:]
            hex_1 = hex_str[:4] 
            hex_2 = hex_str[4:8] 
            if hex_1 != '':
                decimal_part1 = int(hex_1, 16)
            else:
                decimal_part1 = 0
            if hex_2 != '':
                decimal_part2 = int(hex_2, 16)
            else:
                decimal_part2 = 0 
            arg = "0 21 " + str(decimal_part1) + " " + str(decimal_part2)
            self.execute_prevac_setter(arg)  
            time.sleep(0.1) 

    def prevac_set_tpd(self):
        if self.eg_connection_btn.isChecked():
            subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True) 
            float_text = self.eg_tpd_setval.text()
            float_v = int(float_text)
            decimal_part1 = (float_v >> 8) & 0xFF
            decimal_part2 = float_v & 0xFF
            arg = "0 37 " + str(decimal_part1) + " " + str(decimal_part2)
            self.execute_prevac_setter(arg)  
            time.sleep(0.1) 

    def prevac_set_px(self):
        if self.eg_connection_btn.isChecked():
            subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True) 
            float_text = self.eg_position_x_setval.text()
            float_v = float(float_text)
            hex_str = hex(struct.unpack('<I', struct.pack('<f', float_v))[0])[2:]
            hex_1 = hex_str[:4] 
            hex_2 = hex_str[4:8] 
            if hex_1 != '':
                decimal_part1 = int(hex_1, 16)
            else:
                decimal_part1 = 0
            if hex_2 != '':
                decimal_part2 = int(hex_2, 16)
            else:
                decimal_part2 = 0 
            arg = "0 23 " + str(decimal_part1) + " " + str(decimal_part2)
            self.execute_prevac_setter(arg)  
            time.sleep(0.1) 

    def prevac_set_py(self):
        if self.eg_connection_btn.isChecked():
            subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True) 
            float_text = self.eg_position_y_setval.text()
            float_v = float(float_text)
            hex_str = hex(struct.unpack('<I', struct.pack('<f', float_v))[0])[2:]
            hex_1 = hex_str[:4] 
            hex_2 = hex_str[4:8] 
            if hex_1 != '':
                decimal_part1 = int(hex_1, 16)
            else:
                decimal_part1 = 0
            if hex_2 != '':
                decimal_part2 = int(hex_2, 16)
            else:
                decimal_part2 = 0 
            arg = "0 25 " + str(decimal_part1) + " " + str(decimal_part2)
            self.execute_prevac_setter(arg)  
            time.sleep(0.1) 

    def prevac_set_ax(self):
        if self.eg_connection_btn.isChecked():
            subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True) 
            float_text = self.eg_area_x_setval.text()
            float_v = float(float_text)
            hex_str = hex(struct.unpack('<I', struct.pack('<f', float_v))[0])[2:]
            hex_1 = hex_str[:4] 
            hex_2 = hex_str[4:8] 
            if hex_1 != '':
                decimal_part1 = int(hex_1, 16)
            else:
                decimal_part1 = 0
            if hex_2 != '':
                decimal_part2 = int(hex_2, 16)
            else:
                decimal_part2 = 0 
            arg = "0 27 " + str(decimal_part1) + " " + str(decimal_part2)
            self.execute_prevac_setter(arg)  
            time.sleep(0.1) 

    def prevac_set_ay(self):
        if self.eg_connection_btn.isChecked():
            subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True) 
            float_text = self.eg_area_y_setval.text()
            float_v = float(float_text)
            hex_str = hex(struct.unpack('<I', struct.pack('<f', float_v))[0])[2:]
            hex_1 = hex_str[:4] 
            hex_2 = hex_str[4:8] 
            if hex_1 != '':
                decimal_part1 = int(hex_1, 16)
            else:
                decimal_part1 = 0
            if hex_2 != '':
                decimal_part2 = int(hex_2, 16)
            else:
                decimal_part2 = 0 
            arg = "0 29 " + str(decimal_part1) + " " + str(decimal_part2)
            self.execute_prevac_setter(arg)  
            time.sleep(0.1) 

    def prevac_set_gx(self):
        if self.eg_connection_btn.isChecked():
            subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True) 
            float_text = self.eg_grid_x_setval.text()
            float_v = float(float_text)
            hex_str = hex(struct.unpack('<I', struct.pack('<f', float_v))[0])[2:]
            hex_1 = hex_str[:4] 
            hex_2 = hex_str[4:8] 
            if hex_1 != '':
                decimal_part1 = int(hex_1, 16)
            else:
                decimal_part1 = 0
            if hex_2 != '':
                decimal_part2 = int(hex_2, 16)
            else:
                decimal_part2 = 0 
            arg = "0 31 " + str(decimal_part1) + " " + str(decimal_part2)
            self.execute_prevac_setter(arg)  
            time.sleep(0.1) 

    def prevac_set_gy(self):
        if self.eg_connection_btn.isChecked():
            subprocess.run(['pkill', '-f', self.processes[16].args[0]], check=True) 
            float_text = self.eg_grid_y_setval.text()
            float_v = float(float_text)
            hex_str = hex(struct.unpack('<I', struct.pack('<f', float_v))[0])[2:]
            hex_1 = hex_str[:4] 
            hex_2 = hex_str[4:8] 
            if hex_1 != '':
                decimal_part1 = int(hex_1, 16)
            else:
                decimal_part1 = 0
            if hex_2 != '':
                decimal_part2 = int(hex_2, 16)
            else:
                decimal_part2 = 0 
            arg = "0 33 " + str(decimal_part1) + " " + str(decimal_part2)
            self.execute_prevac_setter(arg)  
            time.sleep(0.1) 

    def update_electrongun_values(self):
        if self.eg_connection_btn.isChecked():
            if len(prevac_subscribing_values) >= 13:
                prevac_operate = int(prevac_subscribing_values[0])
                flags = bin(int(prevac_subscribing_values[1]))[2:]
                prevac_energy_voltage = str(round(float(prevac_subscribing_values[2]),3))
                prevac_focus_voltage = str(round(float(prevac_subscribing_values[3]),3))
                prevac_wehnelt_voltage = str(round(float(prevac_subscribing_values[4]),3))
                prevac_emission_current = str(round(float(prevac_subscribing_values[5]),3))
                prevac_scan_positionX = str(round(float(prevac_subscribing_values[6]),3))
                prevac_scan_positionY = str(round(float(prevac_subscribing_values[7]),3))
                prevac_scan_areaX = str(round(float(prevac_subscribing_values[8]),3))
                prevac_scan_areaY = str(round(float(prevac_subscribing_values[9]),3))
                prevac_scan_gridX = str(round(float(prevac_subscribing_values[10]),3))
                prevac_scan_gridY = str(round(float(prevac_subscribing_values[11]),3)) 
                prevac_time_per_dot = str(round(float(prevac_subscribing_values[12]),3))                

                flags = flags.zfill(12)

                self.prevac_flags = [int(bit) for bit in flags]

                #print("Prevac operate state: " + str(prevac_operate) + " --- Prevac status flags: " + str(self.prevac_flags))

                if (self.prevac_flags[9] == 1):
                    self.cathode_failure.setText("       Cathode FLR")
                    self.cathode_failure.setStyleSheet("color: red; font-weight: bold")
                else:
                    self.cathode_failure.setText("       Cathode FLR")
                    self.cathode_failure.setStyleSheet("color: white; font-weight: normal")                    

                if (self.prevac_flags[8] == 1):
                    self.cathode_current_limit.setText("C. current lim.")
                    self.cathode_current_limit.setStyleSheet("color: red; font-weight: bold")
                else:
                    self.cathode_current_limit.setText("C. current lim.")
                    self.cathode_current_limit.setStyleSheet("color: white; font-weight: normal")                    

                if (self.prevac_flags[3] == 1):
                    self.ES_short_circuit.setText("ES short circ.")
                    self.ES_short_circuit.setStyleSheet("color: red; font-weight: bold")
                else:
                    self.ES_short_circuit.setText("ES short circ.")
                    self.ES_short_circuit.setStyleSheet("color: white; font-weight: normal")                    

                if (self.prevac_flags[2] == 1):
                    self.ES_failure.setText("       ES FLR")
                    self.ES_failure.setStyleSheet("color: red; font-weight: bold")
                else:
                    self.ES_failure.setText("       ES FLR")
                    self.ES_failure.setStyleSheet("color: white; font-weight: normal")                    

                if (self.prevac_flags[1] == 1):
                    self.FS_short_circuit.setText("FS short circ.")
                    self.FS_short_circuit.setStyleSheet("color: red; font-weight: bold")
                else:
                    self.FS_short_circuit.setText("FS short circ.")
                    self.FS_short_circuit.setStyleSheet("color: white; font-weight: normal")                    

                if (self.prevac_flags[0] == 1):
                    self.FS_failure.setText("       FS FLR")
                    self.FS_failure.setStyleSheet("color: red; font-weight: bold")
                else:
                    self.FS_failure.setText("       FS FLR")
                    self.FS_failure.setStyleSheet("color: white; font-weight: normal")                    

                if (prevac_operate == 1):
                    self.EG_status.setText(" Operate")
                    self.EG_status.setStyleSheet("color: white; font-weight: bold")
                else:
                    self.EG_status.setText(" Stand by")
                    self.EG_status.setStyleSheet("color: white; font-weight: bold")                    

                if (prevac_operate == 1) and (self.secure_eg == 0):
                    self.secure_eg = 1
                    self.secure_eg_btn.setStyleSheet("background-color: red; color: white;")
                    self.secure_eg_btn.setChecked(True)  
                    if prevac_operate == 1:
                        self.operate_eg_btn.setStyleSheet("background-color: green; color: white;")
                        self.operate_eg_btn.setChecked(True) 
                
                if prevac_operate == 1:
                    self.operate_eg_btn.setStyleSheet("background-color: green; color: white;")
                    self.operate_eg_btn.setChecked(True) 
                    self.standby_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                    self.standby_eg_btn.setChecked(False) 
                
                if prevac_operate == 0:
                    self.standby_eg_btn.setStyleSheet("background-color: darkblue; color: white;")
                    self.standby_eg_btn.setChecked(True) 
                    self.operate_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                    self.operate_eg_btn.setChecked(False)  

                if ((self.secure_eg == 1) and (float(self.vacuum_pressure1) > reqPress4ElectronGun)):
                    self.secure_eg = 0
                    arg_standby = "0 14 0 1"
                    self.execute_prevac_setter(arg_standby)                                     
                    self.secure_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                    self.secure_eg_btn.setChecked(False)  
                    self.standby_eg_btn.setStyleSheet("background-color: darkblue; color: white;")
                    self.standby_eg_btn.setChecked(True) 
                    self.operate_eg_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                    self.operate_eg_btn.setChecked(False)                     
                    self.showWarningSignal.emit("The pressure in the FRG-702 sensor is greater than {} [Torr], a command has been sent to turn off the electron gun...".format(reqPress4ElectronGun))

                # Update the labels with the prevac-related values
                self.eg_read_energy_voltage.setText(prevac_energy_voltage+" [V]")
                self.eg_read_focus_voltage.setText(prevac_focus_voltage+" [V]")
                self.eg_read_wehnelt_voltage.setText(prevac_wehnelt_voltage+" [V]")
                self.eg_read_emission_current.setText(prevac_emission_current+" [µA]")
                self.eg_read_tpd.setText(prevac_time_per_dot+" [µs]")
                self.eg_read_position_x.setText(prevac_scan_positionX+" [mm]")
                self.eg_read_position_y.setText(prevac_scan_positionY+" [mm]")
                self.eg_read_area_x.setText(prevac_scan_areaX+" [mm]")
                self.eg_read_area_y.setText(prevac_scan_areaY+" [mm]")
                self.eg_read_grid_x.setText(prevac_scan_gridX+" [mm]")
                self.eg_read_grid_y.setText(prevac_scan_gridY+" [mm]")

                #print(self.eg_read_energy_voltage, self.eg_read_focus_voltage, self.eg_read_wehnelt_voltage, self.eg_read_emission_current, self.eg_read_tpd, self.eg_read_position_x, self.eg_read_position_y, self.eg_read_area_x, self.eg_read_area_y, self.eg_read_grid_x, self.eg_read_grid_y)

                                     
        else:        
            self.eg_read_energy_voltage.setText("N/C")
            self.eg_read_focus_voltage.setText("N/C")
            self.eg_read_wehnelt_voltage.setText("N/C")
            self.eg_read_emission_current.setText("N/C")
            self.eg_read_tpd.setText("N/C")
            self.eg_read_position_x.setText("N/C")
            self.eg_read_position_y.setText("N/C")
            self.eg_read_area_x.setText("N/C")
            self.eg_read_area_y.setText("N/C")
            self.eg_read_grid_x.setText("N/C")
            self.eg_read_grid_y.setText("N/C")     
            
            self.EG_status.setText("N/C")
            self.cathode_failure.setText("              N/C")  
            self.cathode_current_limit.setText("N/C") 
            self.ES_short_circuit.setText("N/C")   
            self.ES_failure.setText("              N/C")    
            self.FS_short_circuit.setText("N/C")    
            self.FS_failure.setText("              N/C") 
            
            self.EG_status.setStyleSheet("color: white; font-weight: normal") 
            self.cathode_failure.setStyleSheet("color: white; font-weight: normal")   
            self.cathode_current_limit.setStyleSheet("color: white; font-weight: normal")  
            self.ES_short_circuit.setStyleSheet("color: white; font-weight: normal")   
            self.ES_failure.setStyleSheet("color: white; font-weight: normal")     
            self.FS_short_circuit.setStyleSheet("color: white; font-weight: normal")    
            self.FS_failure.setStyleSheet("color: white; font-weight: normal")              

        # Sleep briefly to avoid excessive updates
        #time.sleep(0.01)

    def start_update_eg_timer(self):
        # Start a QTimer to periodically update prevac-related values
        self.timer_prevac = QTimer()
        self.timer_prevac.timeout.connect(self.update_electrongun_values)
        self.timer_prevac.start(1000)  # Update interval for Prevac monitoring

    def stop_update_eg_timer(self):
        # Stop the QTimer used for updating prevac-related values
        if hasattr(self, 'timer_prevac'):
            self.timer_prevac.stop()

    def update_input_width(self, event=None):
        # Update the width of input fields based on the window size
        x = self.width()  
        y = -680.107 + 0.693 * x - 0.0001 * x ** 2
        y_rounded = round(y) 
        self.apd_counts_secs_label.setFixedWidth(y_rounded)
        self.update()

    def update_serial_ports(self):
        # Update the available serial ports in the ComboBox
        self.serialPortsCombobox.clear()

        # Use 'ls /dev/serial/by-id/' to get the ports by ID
        try:
            result = subprocess.run(['ls', '/dev/serial/by-id/'], capture_output=True, text=True)
            if result.returncode == 0:
                ports_by_id = result.stdout.split('\n')
                for port_by_id in ports_by_id:
                    self.serialPortsCombobox.addItem(port_by_id)
            else:
                print(f"Error: {result.stderr}")
        except Exception as e:
            print(f"Error: {e}")

    def toggle_process(self, i, checked):
        # Get the sender of the signal and the corresponding button names
        sender = self.sender()
        button_names = ["Server", "Counts plot", "Plot counts", "Plot FFT", "Export counts data [100kHz]",
                        "Export counts data [1kHz]", "Export FFT data [0.1Hz resolution]",
                        "Export FFT data [0.01Hz resolution]", "Show spectrum averages"]
        
        if checked:
            if i == 2: 
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: darkblue; color: white;")
                self.apd_button.setChecked(True)
                self.apd_button.setText("APD")
                self.apd_button.setStyleSheet("background-color: darkblue;")
                selected_port = self.serialPortsCombobox.currentText()
                self.processes[i] = subprocess.Popen([self.binary_paths[i], str(selected_port)])

                self.update_graph1_thread.start()
            
            # Handle different cases based on the value of 'i'
            if i == 3:  
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: darkblue; color: white;")
                fft_window_type = self.windowTypeCombobox.currentIndex()
                fft_window_value = self.window_type_values[fft_window_type]   
                self.f_i = int(self.f_i_input.text()) 
                self.f_f = int(self.f_f_input.text())              
                self.processes[i] = subprocess.Popen([self.binary_paths[i], str(self.f_i), str(self.f_f), str(fft_window_value)])
                self.update_graph2_thread.start() 
                
            if i == 6:  
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: darkblue; color: white;")
                avg_period = self.avg_time_input.text()              
                self.processes[i] = subprocess.Popen([self.binary_paths[i], avg_period])
                self.update_graph2_thread.start()              
                
            if i == 7:  
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: darkblue; color: white;")
                avg_period = self.avg_time_input.text()  
                fft_window_type = self.windowTypeCombobox.currentIndex()
                fft_window_value = self.window_type_values[fft_window_type]                               
                self.processes[i+1] = subprocess.Popen([self.binary_paths[i+1], str(self.f_i), str(self.f_f), str(fft_window_value)])
                self.processes[i] = subprocess.Popen([self.binary_paths[i], avg_period])
                self.update_graph2_thread.start()                       
            
            else:
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: darkblue; color: white;")
                self.processes[i] = subprocess.Popen([self.binary_paths[i]])
            print(f"Process {i + 1} started.")
        else:
            if self.processes[i]:
                if i == 2:
                    self.apd_button.setChecked(False)
                    self.apd_button.setText("APD")
                    self.apd_button.setStyleSheet("background-color: 53, 53, 53;")
                    self.update_graph1_thread.requestInterruption()
                    # self.update_graph1_thread.wait()
                if i == 3:
                    self.update_graph2_thread.requestInterruption()
                    # self.update_graph2_thread.wait()                    
                
                # Handle different cases based on the value of 'i'

                if i == 7:
                    sender.setText(button_names[i+1])
                    sender.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                    subprocess.run(['pkill', '-f', self.processes[i+1].args[0]], check=True)
                    self.processes[i+1] = None
                    print(f"Process {i + 2} stopped.")
                    
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                subprocess.run(['pkill', '-f', self.processes[i].args[0]], check=True)
                self.processes[i] = None
                print(f"Process {i + 1} stopped.")


    def update_graph1(self):
        len_cvt = 10
        for i in range(0, len_cvt, 2):

            timestamp = float(counts[i + 1])  # Extract timestamp from data
            value = float(counts[i])  # Extract value from data

            self.times1.append(timestamp)  # Add timestamp to times1 list
            self.data1.append(value)  # Add value to data1 list

        # Calculate the cut-off time based on the input value
        current_time = time.time()
        cut_off_time = current_time - int(self.apd_counts_secs_input.text())

        # Keep only the data within the specified time range
        self.times1 = [t for t in self.times1 if t >= cut_off_time]
        self.data1 = self.data1[-len(self.times1):]

        # Put the updated data into the data_queue for plotting
        self.update_plot1_thread.data_queue.put([self.times1, self.data1])

    def update_plot1(self, data):
        if (self.closing == 0):
            process_name = self.binary_paths[1]
            try:
                result = subprocess.run(["pgrep", "-f", process_name], capture_output=True, text=True)
                if result.returncode == 0:
                    pass
                else:
                    print("APD CVT not running, starting again...")
                    self.processes[1] = subprocess.Popen([self.binary_paths[1]])
            except Exception as e:
                print("Error checking APD CVT :", str(e))
        # Update the plot with the new data
        self.plot1.setData(self.times1, self.data1)
    

    def update_graph2(self):
        # Get the FFT frequency range from the input fields
        self.f_i = int(self.f_i_input.text())
        self.f_f = int(self.f_f_input.text())

        # Clear existing data and set up the graph
        self.data2.clear()
        self.freq2.clear()
        self.graph2.clear()
        self.graph2.plotItem.setYRange(-0.1, 1.1)
        self.graph2.plotItem.setXRange(np.log10(self.f_i), np.log10(self.f_f))
        self.graph2.addItem(self.h_line)
        self.freq2.extend(freq)
        self.data2.extend(magn)

        # Calculate the fundamental frequency
        fundamental_freq = self.calculate_fundamental_frequency(self.freq2, self.data2)

        # Create a bold font for the text item
        font = QFont()
        font.setBold(True)

        # Show the fundamental frequency text item
        if self.y_bar:
            y_bar_freq, y_bar_magn = self.print_nearest_frequency()
            v_line_y = pg.InfiniteLine(pos=np.log10(y_bar_freq), angle=90, pen=pg.mkPen(color=(255, 255, 0), width=2))
            self.graph2.addItem(v_line_y)
            text_item2 = pg.TextItem(text=f"Freq: {y_bar_freq} Hz, Magnitude: {y_bar_magn}", color=(255, 255, 0))
            text_item2.setFont(font)
            text_item2.setPos(np.log10(self.f_i), 0)
            self.graph2.addItem(text_item2)
        else:
            self.cursor_position = None

        # Create an infinite horizontal line for the cursor
        self.h_line = pg.InfiniteLine(pos=0, angle=0, pen=pg.mkPen(color=(0, 255, 0), width=1))

        # Show the bar graph if a valid fundamental frequency is calculated
        if fundamental_freq > 0:
            bar_graph = pg.BarGraphItem(x=np.log10(freq), height=magn, width=(np.log10(self.f_f) - np.log10(self.f_i)) * 0.002, brush='g')
            self.graph2.addItem(bar_graph)

        # Update the graph
        self.graph2.update()

        # Show the color map if enabled
        if self.spec:
            self.color_map.getView().setLabel('bottom', f"Frequency (Hz).\nEach row represents the average of the last {int(self.avg_time_input.text())} seconds.")
            self.color_map.getView().setLabel('left', f"Last {int(self.avg_time_input.text()) * int(self.spectrum_amount_input.text())} seconds.")
            graph3_thread = threading.Thread(target=self.update_graph3)
            graph3_thread.start()
    

    def update_graph3(self):
        # Set the color map levels
        self.color_map.setLevels(0, 1)
        
        # Get the spectrum amount and average time from input fields
        self.spectrum_amount = int(self.spectrum_amount_input.text())
        self.avg_time = int(self.avg_time_input.text())

        # Initialize data matrix for averaging
        if not hasattr(self, 'data_matrix_avg'):
            self.data_matrix_avg = np.zeros(self.fft_magnitudes)

        # Accumulate data for averaging
        for i in range(len(self.data2)):
            freq_value = int(self.freq2[i] * 10)
            magn_value = float(self.data2[i])
            self.data_matrix_avg[freq_value] += magn_value

        # Increment the average count and time
        self.avg_count = self.avg_count + 1
        time_i = int(time.time())

        # Perform averaging and update spectrum matrix
        if (time_i - self.t_fft >= self.avg_time):
            self.data_matrix_avg = self.data_matrix_avg / self.avg_count
            self.avg_count = 0

            if not hasattr(self, 'spectrum_matrix'):
                self.spectrum_matrix = np.zeros((1, self.fft_magnitudes))

            self.spectrum_matrix = np.vstack((self.data_matrix_avg, self.spectrum_matrix))
            
            if (self.eg_plot_state == 1):
                max_index = np.argmax(self.data_matrix_avg[(self.f_i * 10):(self.f_f * 10)])
                max_count = np.count_nonzero(self.data_matrix_avg[(self.f_i * 10):(self.f_f * 10)] == self.data_matrix_avg[(self.f_i * 10):(self.f_f * 10)][max_index])
                if max_count == 1:
                    max_value_index = max_index + (self.f_i * 10)
                    current_time = time.time()
                    self.update_graph_eg(current_time, max_value_index/10)
                else:
                    max_value_index = None

            self.data_matrix_avg = np.zeros(self.fft_magnitudes)

            # Trim the spectrum matrix to the specified spectrum amount
            while self.spectrum_matrix.shape[0] > self.spectrum_amount:
                self.spectrum_matrix = self.spectrum_matrix[:self.spectrum_amount, :]

            # Transpose the spectrum matrix for plotting
            self.plot_matrix = np.transpose(self.spectrum_matrix)
            self.pm = self.plot_matrix[:self.f_f * 10, :]

            # Update the color map with the new data
            self.color_map.setImage(self.pm)
            self.color_map.getView().setRange(xRange=(self.f_i * 10, self.f_f * 10))
            self.t_fft = int(time.time())


    def enb_eg_plot(self):
        # Toggle the spec mode and update toggle button style
        if self.toggle_button_spec.isChecked():
            if self.enable_eg_plot_btn.isChecked():
                self.enable_eg_plot_btn.setStyleSheet("background-color: darkblue; color: white;")
                self.eg_plot_state = 1  
            else:
                self.enable_eg_plot_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
                self.eg_plot_state = 0   
        else:
            self.enable_eg_plot_btn.setChecked(False)   
            self.enable_eg_plot_btn.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
            self.eg_plot_state = 0     
            self.showWarningSignal.emit("Please make sure that in the APD tab, the 'Plot counts', 'Plot FFT' and 'Show spectrum averages' buttons are enabled and try again...")         

    def update_graph_eg(self, timestamp, value):
        self.times_eg.append(timestamp)  # Add timestamp 
        self.data_eg.append(value)  # Add value

        # Calculate the cut-off time based on the input value
        current_time = time.time()
        cut_off_time = current_time - int(self.eg_secs_input.text())

        # Keep only the data within the specified time range
        self.times_eg = [t for t in self.times_eg if t >= cut_off_time]
        self.data_eg = self.data_eg[-len(self.times_eg):]
        self.plot_eg.setData(self.times_eg, self.data_eg)

    def update_graph_pressure(self):
        if (self.pressure_plotting_state == 1):
            timestamp = float(time.time())  # Extract timestamp from data
            value1 = float(twistorr_subscribing_values[12])  # Extract value from data
            value2 = float(twistorr_subscribing_values[13])  # Extract value from data
            self.pressure_time.append(timestamp)  # Add timestamp to times1 list
            self.pressure_data1.append(value1)  # Add value to data1 list
            self.pressure_data2.append(value2)  # Add value to data2 list

            # Calculate the cut-off time based on the input value
            current_time = time.time()
            cut_off_time = current_time - int(self.pressure_secs_input.text())

            # Keep only the data within the specified time range
            self.pressure_time = [t for t in self.pressure_time if t >= cut_off_time]
            self.pressure_data1 = self.pressure_data1[-len(self.pressure_time):]
            self.pressure_data2 = self.pressure_data2[-len(self.pressure_time):]
            if (self.pressure1_checkbox.isChecked() and self.pressure2_checkbox.isChecked()):
                pass
            elif (self.pressure1_checkbox.isChecked()):
                self.pressure_data2 = [np.nan] * len(self.pressure_time)
            elif (self.pressure2_checkbox.isChecked()):
                self.pressure_data1 = [np.nan] * len(self.pressure_time)
            else:
                self.pressure_data1 = [np.nan] * len(self.pressure_time)
                self.pressure_data2 = [np.nan] * len(self.pressure_time)  

            if (self.pressure1_checkbox.isChecked() and self.pressure2_checkbox.isChecked()):
                self.pressure_plot1.setData(self.pressure_time, self.pressure_data1)
                self.pressure_plot2.setData(self.pressure_time, self.pressure_data2)
            elif (self.pressure1_checkbox.isChecked()):
                self.pressure_plot1.setData(self.pressure_time, self.pressure_data1)
                self.pressure_plot2.setData(self.pressure_time, self.pressure_data2)
            elif (self.pressure2_checkbox.isChecked()):
                self.pressure_plot1.setData(self.pressure_time, self.pressure_data1)
                self.pressure_plot2.setData(self.pressure_time, self.pressure_data2)

    def send_rigol_publishing_values(self):#, values):
        global rigol_publishing_values
        self.rigolpublish2broker.start()  
        rigol_publishing_values = self.rigol_values
        #print(rigol_publishing_values)
        
    def send_laser_publishing_values(self):#, values):
        global laser_publishing_values
        self.laserpublish2broker.start()         
        laser_publishing_values = self.laser_values
        #print(laser_publishing_values)

     # Updating data grom rigol
    def update_rigol(self, rigol_x_data, rigol_y_data, rigol_attenuation, rigol_voltscale, rigol_voltoffset, rigol_coupling, rigol_voltage, rigol_frequency, rigol_function, rigol_g1, rigol_g2, g2_V, rigol_laser_voltage):
        global g1_global
        if self.rigol_particle_trap_enable_btn.isChecked():
            g1_global = 1
        else:
            g1_global = 0

        if g2_V:
            self.laser_voltage_monitor.setText(f"{rigol_laser_voltage} [V]")

        if self.rigol and self.rigol_chn:
            self.graph_voltage_trap.clear()

            rigol_max = float(self.rigol_voltage_max.text())
            rigol_min = float(self.rigol_voltage_min.text())
            rigol_x_last = rigol_x_data[-1] if len(rigol_x_data) > 0 else 0

            # Add TextItems individually
            text_items = [
                pg.TextItem("Oscilloscope settings:", anchor=(0, 0)),
                pg.TextItem(f"Attenuation: x{rigol_attenuation}", anchor=(0, 0)),
                pg.TextItem(f"Voltage scale: {rigol_voltscale} V", anchor=(0, 0)),
                pg.TextItem(f"Coupling: {rigol_coupling}", anchor=(0, 0)),
                pg.TextItem(""),
                pg.TextItem("Particle trap settings:", anchor=(0, 0)),
                pg.TextItem(f"Voltage: {rigol_voltage} Vpp", anchor=(0, 0)),
                pg.TextItem(f"Voltage offset: {rigol_voltoffset} V", anchor=(0, 0)),
                pg.TextItem(f"Frequency: {rigol_frequency} Hz", anchor=(0, 0)),
                pg.TextItem(f"Function: {rigol_function}", anchor=(0, 0)),
            ]

            for i, item in enumerate(text_items):
                item.setPos(rigol_x_last * 0.88, rigol_max - (rigol_max - rigol_min) / 18 * i)
                self.graph_voltage_trap.addItem(item)

            if rigol_g1 == 1 and not self.rigol_particle_trap_enable_btn.isChecked():
                self.rigol_particle_trap_enable_btn.setStyleSheet("background-color: darkblue;")
                self.rigol_particle_trap_enable_btn.setChecked(True)
            elif rigol_g1 == 0 and self.rigol_particle_trap_enable_btn.isChecked():
                self.rigol_particle_trap_enable_btn.setStyleSheet("background-color: 53, 53, 53;")
                self.rigol_particle_trap_enable_btn.setChecked(False)

            if self.rigol_chn_n < 3 and len(rigol_x_data) > 0:
                self.graph_voltage_trap.setXRange(0, rigol_x_last)
                self.graph_voltage_trap.setYRange(rigol_min, rigol_max)

                # Batch Plot Data Update
                self.rigol_plot = self.graph_voltage_trap.plot(pen=self.color)
                self.rigol_plot.setData(x=rigol_x_data, y=rigol_y_data)

                self.voltage_monitor.setText(f"{rigol_voltage} [Vpp]")
                self.offset_monitor.setText(f"{rigol_voltoffset} [V]")
                self.frequency_monitor.setText(f"{rigol_frequency} [Hz]")
        else:
            self.graph_voltage_trap.clear()


        if rigol_function == "SIN\n":
            function_number = 1
        elif rigol_function == "SQU\n":
            function_number = 2
        elif rigol_function == "RAMP\n":
            function_number = 3
        elif rigol_function == "PULS\n":
            function_number = 4
        elif rigol_function == "DC\n":
            function_number = 5
        else:
            function_number = 0
            
        self.rigol_values = [rigol_voltage, rigol_voltoffset, rigol_frequency, function_number, g1_global]
        self.laser_values = [rigol_laser_voltage, g2_V]
        
        if any(self.rigol_values):
            self.send_rigol_publishing_values()#self.rigol_values)
            self.rigol_values = []
        if any(self.laser_values):    
            self.send_laser_publishing_values()#self.laser_values)
            self.laser_values = []               
        
    def calculate_fundamental_frequency(self, freq, magn):
        # Find valid indices with frequency > 1.1 and magnitude > 0.5
        valid_indices = [i for i in range(len(magn)) if freq[i] > 1.1 and magn[i] > 0.5]
        
        # Get the 100 largest magnitude indices
        max_magn_indices = heapq.nlargest(100, valid_indices, key=lambda i: magn[i])
        max_freqs = [freq[i] for i in max_magn_indices]
        
        if max_freqs:
            # Return the smallest frequency among the max_freqs
            fundamental_freq = min(max_freqs)
            return fundamental_freq
        else:
            return 0 

    def print_nearest_frequency(self):
        # Get the cursor position in the plot's view coordinates
        cursor_pos = self.graph2.plotItem.vb.mapSceneToView(self.graph2.mapFromGlobal(QtGui.QCursor.pos()))
        x_pos = cursor_pos.x()

        # Get the x-axis range and view rectangle
        x_range, _ = self.graph2.plotItem.viewRange()
        view_rect = self.graph2.plotItem.viewRect()

        # Calculate relative x-position in the view rectangle
        relative_x = (x_pos - view_rect.left()) / view_rect.width()
        cursor_graph2 = 10 ** (x_range[0] + relative_x * (x_range[1] - x_range[0]))

        x_data = np.array(self.freq2)

        # Find the index of the closest frequency to the cursor position
        closest_index = np.argmin(np.abs(x_data - cursor_graph2))
        closest_frequency = x_data[closest_index]
        closest_magnitude = self.data2[closest_index]
        return closest_frequency, closest_magnitude

    def clear_nearest_frequency(self):
        self.cursor_position = None

    def closeEvent(self, event):
        # Terminate running processes and stop timers before closing
        os.remove("lockfile.lock") 
        self.closing = 1
        for process in self.processes:
            if process is not None:
                subprocess.run(['pkill', '-f', process.args[0]], check=True)

        #os.system('echo code | sudo -S systemctl stop twistorrmonitor.service')        
        #os.system('echo code | sudo -S systemctl stop prevacmonitor.service')
        self.stop_update_tt_timer()
        self.stop_update_eg_timer()
        event.accept()

    def toggle_cursor(self):
        # Toggle the y_bar cursor mode and update toggle button style
        self.y_bar = not self.y_bar
        if self.toggle_button.isChecked():
            self.toggle_button.setStyleSheet("background-color: yellow; color: black;")
        else:
            self.toggle_button.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")

    def toggle_spec(self):
        # Toggle the spec mode and update toggle button style
        if self.toggle_button_spec.isChecked():
            self.toggle_button_spec.setStyleSheet("background-color: darkblue; color: white;")
        else:
            self.toggle_button_spec.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
        self.spec = not self.spec

    def toggle_save_spec(self):
        # Toggle the spec mode and update toggle button style
        if self.toggle_button_save_spec.isChecked():
            self.toggle_button_save_spec.setStyleSheet("background-color: darkblue; color: white;")
        else:
            self.toggle_button_save_spec.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
        self.spec = not self.spec

    def toggle_clean_spec(self):
        # Toggle the spec mode, clear stored data, and update toggle button style
        if self.toggle_button_clean_spec.isChecked():
            self.toggle_button_clean_spec.setStyleSheet("background-color: darkblue; color: white;")
        else:
            self.toggle_button_clean_spec.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")
        # Clear stored data and free memory
        del self.data_matrix_avg
        del self.spectrum_matrix
        del self.plot_matrix
        del self.pm
        self.pm = np.zeros((1, self.fft_magnitudes))
        self.color_map.setImage(self.pm)

    def show_warning_message(self, message):
        msg = QMessageBox()
        msg.setWindowTitle("CoDE Warning")
        msg.setText(message)
        msg.setIcon(QMessageBox.Critical)
        msg.exec_()

  
class AlertWindow(QMainWindow):
    showWarningSignal = pyqtSignal(str)
    def __init__(self):
        super(AlertWindow, self).__init__()

        self.showWarningSignal.connect(self.show_warning_message)
    
    def show_warning_message(self, message):
        msg = QMessageBox()
        msg.setWindowTitle("CoDE Warning")
        msg.setText(message)
        msg.setIcon(QMessageBox.Critical)
        msg.exec_()


# Apply dark theme to the GUI            
def apply_dark_theme(app):
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

def check_lock_file():
    return os.path.exists("lockfile.lock")

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    apply_dark_theme(app)
    app.setApplicationName("CoDE Control Software")
    # Verifies if there is a lock file
    if check_lock_file():
        alertWindow = AlertWindow()
        alertWindow.showWarningSignal.emit("There is currently an instance of 'CoDE Control Software', please close the old one to open a new one, or continue working with the old one...")      
    else:    
        # Creates a lock file
        with open("lockfile.lock", "w") as lock_file:
            lock_file.write("1")

        try:
            # Create the main window and display it
            mainWindow = MainWindow()
            mainWindow.show()
            # Start the update timer for Twistorr and Prevac 
            mainWindow.start_update_tt_timer()
            mainWindow.start_update_eg_timer()
            # Execute the application event loop
            sys.exit(app.exec_())
        except Exception as e:
            # Handle unexpected exceptions by displaying an error message
            error_message = "An unexpected error has occurred: {}".format(str(e))
            #QtWidgets.QMessageBox.critical(None, "Error", error_message)
            # Append the error message to an error log file
            with open("error.log", "a") as log_file:
                log_file.write(error_message)
        finally:
            # Delete the lock file at the end
            os.remove("lockfile.lock")            
