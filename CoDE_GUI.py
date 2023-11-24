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


# List to store counts from APD
counts = []

# Lists to store FFT data from APD
freq = []
magn = []

# List to store monitoring data from TwistTorporsi

monitoring_TT = []

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
    update_signal1 = pyqtSignal()
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
                    self.update_signal1.emit()


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
    bundle2 = None
    update_signal2 = pyqtSignal()
    
    # Run method for the thread
    def run(self):
        bundle2 = core.Bundle()  # Create an empty Bundle object
        channel2 = grpc.insecure_channel('localhost:50051')  # Create an insecure channel
        stub2 = core_grpc.BrokerStub(channel2)  # Create a stub for the Broker service
        request2 = core.Interests()  # Create a request object
        request2.types.append(core.DATA_FFT_PARTIAL)  # Add DATA_FFT_PARTIAL type to the request
        response_stream2 = stub2.Subscribe(request2)  # Subscribe to the response stream
        thread2 = threading.Thread(target=self.receive_bundles2, args=(response_stream2,))
        thread2.start()  # Start the thread
        thread2.join()  # Wait for the thread to finish
        
    # Method to receive bundles from the response stream
    def receive_bundles2(self, response_stream):
        for bundle2 in response_stream:
            fft = []
            fft[:] = bundle2.value  # Copy the bundle value to the fft list
            half_length = len(fft) // 2  # Calculate half length of the fft data
            freq[:] = fft[:half_length]  # Copy first half of fft data to freq list
            magn[:] = fft[half_length:]  # Copy second half of fft data to magn list
            self.update_signal2.emit()  # Emit a signal to indicate updated data

            

# Definition of a custom thread class for updating TwistTorr monitoring data

class UpdateTTThread(QThread):
    bundle3 = None
    update_signal3 = pyqtSignal()
    
    # Run method for the thread
    def run(self):
        bundle3 = core.Bundle()  # Create an empty Bundle object
        channel3 = grpc.insecure_channel('localhost:50051')  # Create an insecure channel
        stub3 = core_grpc.BrokerStub(channel3)  # Create a stub for the Broker service
        request3 = core.Interests()  # Create a request object
        request3.types.append(core.DATA_TT_MON)  # Add DATA_TT_MON type to the request
        response_stream3 = stub3.Subscribe(request3)  # Subscribe to the response stream
        thread3 = threading.Thread(target=self.receive_bundles3, args=(response_stream3,))
        thread3.start()  # Start the thread
        thread3.join()  # Wait for the thread to finish
        
    # Method to receive bundles from the response stream
    def receive_bundles3(self, response_stream):
        for bundle3 in response_stream:
            if len(bundle3.value) > 0:  # Check if the bundle value is not empty
                global monitoring_TT  # Use the global variable for monitoring data
                monitoring_TT = []
                monitoring_TT[:] = bundle3.value  # Copy the bundle value to the monitoring_TT list
                self.update_signal3.emit()  # Emit a signal to indicate updated data

# Definition of a custom thread class for updating Rigol data        
class RigolDataThread(QThread):
    _Rigol_instance = None
    rigol_data_updated = pyqtSignal(np.ndarray, np.ndarray, float, float, float, str, float, float, str, float, int, int, float)
    
    def __init__(self):
        super().__init__()
        RigolDataThread._Rigol_instance = self
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
                
                rigol_x_data, rigol_y_data, rigol_attenuation, rigol_voltscale, rigol_voltoffset, rigol_coupling, rigol_voltage, rigol_frequency, rigol_function, rigol_g1, rigol_g2, g2, rigol_laser_voltage = get_rigol_data(status, channel, auto, voltage, frequency, function, tscale, voltage_offset, attenuation, coupling, vscale, g1, g2, laser_voltage, voltage_V, frequency_V, function_V, tscale_V, voltage_offset_V, attenuation_V, coupling_V, vscale_V, g1_V, g2_V, laser_voltage_V)
                #print(rigol_x_data, rigol_y_data, rigol_attenuation, rigol_voltscale, rigol_voltoffset, rigol_coupling, rigol_voltage, rigol_frequency, rigol_function, rigol_g1, rigol_g2, g2, rigol_laser_voltage)
                self.rigol_data_updated.emit(rigol_x_data, rigol_y_data, rigol_attenuation, rigol_voltscale, rigol_voltoffset, rigol_coupling, rigol_voltage, rigol_frequency, rigol_function, rigol_g1, rigol_g2, g2_V, rigol_laser_voltage)
            time.sleep(0.1)
    
class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()

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
            path + '/core_ba/bin/TwisTorrSetter_code_sw'        
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
        self.tab5 = QWidget()
        self.tab6 = QWidget()
        self.tab7 = QWidget()

        # Add lateral view to the window for monitoring data
        self.dock = QtWidgets.QDockWidget(self)
        self.addDockWidget(QtCore.Qt.LeftDockWidgetArea, self.dock)
        self.dock.setFeatures(QtWidgets.QDockWidget.DockWidgetMovable | QtWidgets.QDockWidget.DockWidgetFloatable)
        self.dock.setFixedWidth(250)

        # Create grid layout for the dock widget
        self.dock_grid = QtWidgets.QGridLayout()

        # ---- TRAP SYSTEM ----
        self.trap_frame = QtWidgets.QFrame()
        self.trap_frame.setFrameShape(QtWidgets.QFrame.Box)

        self.dock_grid.addWidget(self.trap_frame, 1, 0, 4, 2)

        trap_layout = QtWidgets.QVBoxLayout(self.trap_frame)
        self.trap_frame.setLayout(trap_layout)

        self.trap_button = QtWidgets.QPushButton()
        self.trap_button.setCheckable(True)
        self.trap_button.setStyleSheet("background-color: 53, 53, 53;")
        self.trap_button.setText("Trap")
        self.trap_button.setFixedWidth(180)
        self.trap_button.clicked.connect(self.toggle_trap_connect)
        self.dock_grid.addWidget(QLabel("       Voltage:"), 2, 0)
        self.voltage_monitor = QtWidgets.QLabel("N/A")
        self.dock_grid.addWidget(self.voltage_monitor, 2, 1)
        self.dock_grid.addWidget(QLabel("       Offset voltage:"), 3, 0)
        self.offset_monitor = QtWidgets.QLabel("N/A")
        self.dock_grid.addWidget(self.offset_monitor, 3, 1)
        self.dock_grid.addWidget(QLabel("       Frequency:"), 4, 0)
        self.frequency_monitor = QtWidgets.QLabel("N/A")
        self.dock_grid.addWidget(self.frequency_monitor, 4, 1)
        trap_layout.addWidget(self.trap_button, alignment=Qt.AlignTop | Qt.AlignHCenter)
        # ---------------------------------------------

        # ---- SYSTEM PRESSURE ----
        self.pressure_frame = QtWidgets.QFrame()
        self.pressure_frame.setFrameShape(QtWidgets.QFrame.Box)

        self.dock_grid.addWidget(self.pressure_frame, 5, 0, 2, 2)

        pressure_layout = QtWidgets.QVBoxLayout(self.pressure_frame)
        self.pressure_frame.setLayout(pressure_layout)

        self.pressure_button = QtWidgets.QPushButton()
        self.pressure_button.setCheckable(True)
        self.pressure_button.setStyleSheet("background-color: 53, 53, 53;")
        self.pressure_button.setText("Pressure system")
        self.pressure_button.setFixedWidth(180)

        pressure_layout.addWidget(self.pressure_button, alignment=Qt.AlignTop | Qt.AlignHCenter)

        self.dock_grid.addWidget(QLabel("       Pressure:"), 6, 0)
        self.pressure_monitor = QtWidgets.QLabel("N/A")
        self.dock_grid.addWidget(self.pressure_monitor, 6, 1)
        # ---------------------------------------------

        self.laser_frame = QtWidgets.QFrame()
        self.laser_frame.setFrameShape(QtWidgets.QFrame.Box)
        self.laser_frame.setGeometry(0, 0, 250, 100)  # Establecer posición y tamaño del frame

        self.dock_grid.addWidget(self.laser_frame, 8, 0, 3, 2)

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
        self.rigol_laser_voltage.move(25, 85)  

        self.laser_set_button = QtWidgets.QPushButton(self.laser_frame)
        self.laser_set_button.setStyleSheet("background-color: 53, 53, 53; text-align: center;")
        self.laser_set_button.setText("Volt. set")
        self.laser_set_button.clicked.connect(self.toggle_laser_voltage)
        self.laser_set_button.setFixedWidth(70)
        self.laser_set_button.move(130, 85)  

        self.dock_grid.addWidget(QLabel("       Laser voltage:"), 10, 0)
        self.laser_voltage_monitor = QtWidgets.QLabel("N/A")
        self.dock_grid.addWidget(self.laser_voltage_monitor, 10, 1)

        # ---------------------------------------------

        # ---- APD SYSTEM ----
        self.apd_frame = QtWidgets.QFrame()
        self.apd_frame.setFrameShape(QtWidgets.QFrame.Box)

        self.dock_grid.addWidget(self.apd_frame, 14, 0, 1, 2)

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
        self.tab_widget.addTab(self.tab3, "ESI")
        self.tab_widget.addTab(self.tab4, "Particle trap")
        self.tab_widget.addTab(self.tab5, "Temperature")
        self.tab_widget.addTab(self.tab6, "Data processing")
        self.tab_widget.addTab(self.tab7, "Registers")                

        # Amont of processes depends on amount of c++ processes indicated earlier
        self.processes = [None] * 11
        self.threads = []
        self.processes[9] = subprocess.Popen([self.binary_paths[9]])

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
        self.color_map.setMaximumHeight(300)

        # Some parameters and settings for plot 3
        self.color_map.setColorMap(pg.colormap.get('plasma')) # Histogram color setting
        self.fft_magnitudes = 500000
        self.color_map.getView().autoRange() 
        self.avg_count = 0
        plot_item.showGrid(x=True, y=True)
        self.t_fft = int(time.time())     

        # Connect UpdateGraph1Thread() to a thread (receives data for counts vs. time [APD])
        self.update_graph1_thread = UpdateGraph1Thread()
        self.update_graph1_thread.update_signal1.connect(self.update_graph1)

        # Connect UpdatePlot1Thread() to a thread (display the counts vs. data in plot 1 [APD])
        self.update_plot1_thread = UpdatePlot1Thread()
        self.update_plot1_thread.plot_signal.connect(self.update_plot1)
        self.update_plot1_thread.start()
        
        # Connect UpdateGraph2Thread() to a thread (receives and display fft data in plot 2 [APD])
        self.update_graph2_thread = UpdateGraph2Thread()
        self.update_graph2_thread.update_signal2.connect(self.update_graph2)

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
        serialPortsLabel.setFixedWidth(100)
        self.serialPortsCombobox = QComboBox(self)
        self.update_serial_ports()
        index_p = self.serialPortsCombobox.findText('/dev/ttyUSB1', QtCore.Qt.MatchFixedString) # Default value
        if index_p >= 0:
             self.serialPortsCombobox.setCurrentIndex(index_p)        
        self.serialPortsCombobox.setMaximumWidth(120)
        first_layout_1.addWidget(serialPortsLabel)
        first_layout_1.addWidget(self.serialPortsCombobox)
        
        # FFT window type select (second_layout_1)
        windowTypeLabel = QLabel("FFT Window Type:")
        windowTypeLabel.setFixedWidth(100)
        self.windowTypeCombobox = QComboBox(self)
        self.windowTypeCombobox.addItems(['Hamming', 'Hann', 'Blackman-Harris 4', 'Blackman-Harris 7', 'No window'])
        index_w = self.windowTypeCombobox.findText('No window', QtCore.Qt.MatchFixedString) # Default value
        if index_w >= 0:
             self.windowTypeCombobox.setCurrentIndex(index_w)
        self.windowTypeCombobox.setMaximumWidth(120) 
        second_layout_1.addWidget(windowTypeLabel)
        second_layout_1.addWidget(self.windowTypeCombobox)
        
        self.window_type_values = {
            0: 1,  # 'Hamming'
            1: 2,  # 'Hann'
            2: 3,  # 'Blackmann-Harris 4'
            3: 4,  # 'Blackmann-Harris 7'
            4: 5,  # 'No window'
        } 

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
            if i==2 or i==4 or i ==5 :
                first_layout_1.addWidget(start_stop_button)
            else:
                second_layout_1.addWidget(start_stop_button)
            self.buttons.append(start_stop_button) 
        
        # Time axis lenght for counts vs. time plot in 'first_layout_1' (we only update the width of this field because of the buttons/inputs distribution of the firts row)  
        self.resizeEvent = self.update_input_width  
        self.apd_counts_secs_label = QLabel("T-axis length in counts:")
        self.apd_counts_secs_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.apd_counts_secs_input = QLineEdit(self)
        self.apd_counts_secs_input.setText("10") # Default value      
        first_layout_1.addWidget(self.apd_counts_secs_label)
        first_layout_1.addWidget(self.apd_counts_secs_input)

        # Shows a yellow bar to see the magnitude of the frequency below the mouse cursor (in 'second_layout_1')
        self.toggle_button = QPushButton("Peak ID with cursor")
        self.toggle_button.setCheckable(True) 
        self.toggle_button.clicked.connect(self.toggle_cursor)
        second_layout_1.addWidget(self.toggle_button)                                       

        # Averaging period for the FFTs in seconds (in 'third_layout_1')
        avg_time_label = QLabel("Averaging period in spectrometer (s):")
        avg_time_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.avg_time_input = QLineEdit(self)
        self.avg_time_input.setFixedWidth(30) 
        self.avg_time_input.setText("120") # Default value       
        third_layout_1.addWidget(avg_time_label)
        third_layout_1.addWidget(self.avg_time_input)     

        # Amount of averaging periods to show in spectrometer (in 'third_layout_1')
        spectrum_amount_label = QLabel("Periods to show in spectrometer:")
        spectrum_amount_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.spectrum_amount_input = QLineEdit(self)
        self.spectrum_amount_input.setFixedWidth(40) 
        self.spectrum_amount_input.setText("120") # Default value      
        third_layout_1.addWidget(spectrum_amount_label)
        third_layout_1.addWidget(self.spectrum_amount_input)   
        
        # Button to start/stop showing spectrometer (in 'third_layout_1')
        self.toggle_button_spec = QPushButton("Show spectrum averages")
        self.toggle_button_spec.setCheckable(True)  # Enables alternancy
        self.toggle_button_spec.clicked.connect(self.toggle_spec)
        third_layout_1.addWidget(self.toggle_button_spec)
        
        # Button to clean spectrometer (in 'third_layout_1')
        self.toggle_button_clean_spec = QPushButton("Clean spectrometer")
        self.toggle_button_clean_spec.clicked.connect(self.toggle_clean_spec)
        third_layout_1.addWidget(self.toggle_button_clean_spec)
        
        # Start of the frequency range of interest (in 'third_layout_1')
        f_i_label = QLabel("FFT initial freq:")
        f_i_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.f_i_input = QLineEdit(self)
        self.f_i_input.setFixedWidth(60) 
        self.f_i_input.setText("10") # Default value    
        third_layout_1.addWidget(f_i_label)
        third_layout_1.addWidget(self.f_i_input) 

        # End of the frequency range of interest (in 'third_layout_1')
        f_f_label = QLabel("FFT final freq:")
        f_f_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.f_f_input = QLineEdit(self)
        self.f_f_input.setFixedWidth(60) 
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

        # Connect UpdateTTThread() to a thread (receives TwistTorr data [Vacuum])
        self.update_TT_thread = UpdateTTThread()
        self.update_TT_thread.update_signal3.connect(self.update_vacuum_values)
        self.update_TT_thread.start()        

        # Labels for the column titles
        label_parameter = QLabel("Parameter")
        label_monitor = QLabel("Monitor")
        label_setpoint = QLabel("Setpoint")
        label_parameter.setStyleSheet("text-decoration: underline; font-weight: bold;")
        label_monitor.setStyleSheet("text-decoration: underline; font-weight: bold;")
        label_setpoint.setStyleSheet("text-decoration: underline; font-weight: bold;")

        # Set row index order for the titles
        self.layout2.addWidget(label_parameter, 0, 0)
        self.layout2.addWidget(label_monitor, 0, 1)
        self.layout2.addWidget(label_setpoint, 0, 2)
        
        # Setpoint field for vacuum pressure
        self.monitor_vacuum_pressure = QLabel("N/A")
        self.set_vacuum_pressure = QLineEdit()
        self.set_vacuum_pressure.setText("1002") # Default value
        self.set_vacuum_pressure.setFixedWidth(100)
        btn_vacuum_pressure = QPushButton("Set")
        self.layout2.addWidget(QLabel("Vacuum Presure:"), 1, 0)
        self.layout2.addWidget(self.monitor_vacuum_pressure, 1, 1)
        self.layout2.addWidget(self.set_vacuum_pressure, 1, 2)
        self.layout2.addWidget(btn_vacuum_pressure, 1, 3)

        # Setpoint field for motor speed
        self.monitor_speed_motor = QLabel("N/A")
        self.set_speed_motor = QLineEdit()
        self.set_speed_motor.setText("5000") # Default value
        self.set_speed_motor.setFixedWidth(100)
        btn_speed_motor = QPushButton("Set")
        self.layout2.addWidget(QLabel("Speed Motor:"), 2, 0)
        self.layout2.addWidget(self.monitor_speed_motor, 2, 1)
        self.layout2.addWidget(self.set_speed_motor, 2, 2)
        self.layout2.addWidget(btn_speed_motor, 2, 3)

        # Setpoint field for valve state
        self.monitor_valve_state = QLabel("N/A")
        self.set_valve_state = QLineEdit()
        self.set_valve_state.setText("1") # Default value
        self.set_valve_state.setFixedWidth(100)
        btn_valve_state = QPushButton("Set")
        self.layout2.addWidget(QLabel("Valve State:"), 3, 0)
        self.layout2.addWidget(self.monitor_valve_state, 3, 1)
        self.layout2.addWidget(self.set_valve_state, 3, 2)
        self.layout2.addWidget(btn_valve_state, 3, 3)

        # For monitoring bomb power variable
        self.monitor_bomb_power = QLabel("N/A")
        self.layout2.addWidget(QLabel("Bomb Power:"), 4, 0)
        self.layout2.addWidget(self.monitor_bomb_power, 4, 1)

        # For monitoring temperature variable
        self.monitor_temperature = QLabel("N/A")
        self.layout2.addWidget(QLabel("Temperature:"), 5, 0)
        self.layout2.addWidget(self.monitor_temperature, 5, 1)

        self.graph_pressure_vacuum = pg.PlotWidget(axisItems={'left': DateAxis(orientation='left')})
        self.layout2.addWidget(self.graph_pressure_vacuum, 6, 0, 1, 4)
        self.graph_pressure_vacuum.invertY()
        
        self.graph_pressure_vacuum.showGrid(x=True, y=True, alpha=1)           
        self.graph_pressure_vacuum.setLabel('left', 'Time', units='hh:mm:ss.µµµµµµ')
        self.graph_pressure_vacuum.setLabel('bottom', 'Pressure [Torr]')
        
        # Connect any set button with execute_twistorr_set() function
        btn_vacuum_pressure.clicked.connect(lambda: self.execute_twistorr_set_pressure())
        btn_speed_motor.clicked.connect(lambda: self.execute_twistorr_set_motor_speed())
        btn_valve_state.clicked.connect(lambda: self.execute_twistorr_set_valve_state())
        
        self.layout2.setRowStretch(6, 1)


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

        q_calculated = QLabel("N/A")
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

        # Time scale
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
 



        # ------------------------------------------------------------------------------------------------------------------ #
        #----------------------- FUNCTIONS --------------------------------------------------------------------------------- #
        # ------------------------------------------------------------------------------------------------------------------ #
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
        if self.trap_button.isChecked():
            if self.rigol_connect.isChecked():
                pass
            else:
                self.rigol_connect.setChecked(True)
                self.rigol_channel.setChecked(True)
                self.toggle_rigol_connect()
            self.trap_button.setStyleSheet("background-color: darkblue;")# color: black;")
            self.trap_button.setText("Trap")
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

    def toggle_laser_connect(self):
        if self.trap_button.isChecked():
            pass
        else:
            self.rigol_thread.set_rigol_channel(3)
            self.rigol_chn_n = 3

        if self.laser_button.isChecked():
            self.laser_button.setStyleSheet("background-color: darkblue;")# color: black;")
            self.rigol_thread.set_rigol_g2(1, 1)
        else:
            self.laser_button.setStyleSheet("background-color: 53, 53, 53;")
            self.rigol_thread.set_rigol_g2(1, 0)

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

    def execute_twistorr_set_pressure(self):
        # Execute the TwisTorr Setter binary with pressure, motor, and valve parameters
        self.processes[10] = subprocess.Popen([self.binary_paths[10], str(self.pressure), str("0")])
        # Uncomment the following line if you want to stop the process after a short delay
        # subprocess.run(['pkill', '-f', self.processes[10].args[0]], check=True)

    def execute_twistorr_set_motor_speed(self):
        # Execute the TwisTorr Setter binary with pressure, motor, and valve parameters
        self.processes[10] = subprocess.Popen([self.binary_paths[10], str(self.motor), str("1")])
        # Uncomment the following line if you want to stop the process after a short delay
        # subprocess.run(['pkill', '-f', self.processes[10].args[0]], check=True)   

    def execute_twistorr_set_valve_state(self):
        # Execute the TwisTorr Setter binary with pressure, motor, and valve parameters
        self.processes[10] = subprocess.Popen([self.binary_paths[10], str(self.valve), str("2")])
        # Uncomment the following line if you want to stop the process after a short delay
        # subprocess.run(['pkill', '-f', self.processes[10].args[0]], check=True)                

    def update_vacuum_values(self):
        # Update the vacuum-related values from monitoring_TT
        self.pressure = self.set_vacuum_pressure.text()
        self.motor = self.set_speed_motor.text()
        self.valve = self.set_valve_state.text()

        if len(monitoring_TT) >= 5:
            vacuum_pressure = str(round(monitoring_TT[0], 2))
            speed_motor = str(round(monitoring_TT[1], 2))
            valve_state = "Open" if monitoring_TT[2] >= 1 else "Closed"
            bomb_power = str(round(monitoring_TT[3], 2))
            temperature = str(round(monitoring_TT[4], 2))

            # Update the labels with the vacuum-related values
            self.monitor_vacuum_pressure.setText(vacuum_pressure)
            self.monitor_speed_motor.setText(speed_motor)
            self.monitor_valve_state.setText(valve_state)
            self.monitor_bomb_power.setText(bomb_power)
            self.monitor_temperature.setText(temperature)

        # Sleep briefly to avoid excessive updates
        time.sleep(0.001)

    def start_update_tt_timer(self):
        # Start a QTimer to periodically update vacuum-related values
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_vacuum_values)
        self.timer.start(10)  # Update interval for Twistorr monitoring

    def stop_update_tt_timer(self):
        # Stop the QTimer used for updating vacuum-related values
        if hasattr(self, 'timer'):
            self.timer.stop()

    def update_input_width(self, event=None):
        # Update the width of input fields based on the window size
        window_width = self.width() - 260  # Adjust for layout
        input_width = window_width // 24
        self.apd_counts_secs_label.setFixedWidth(input_width * 4)
        self.apd_counts_secs_input.setFixedWidth(input_width * 2)
        self.update()

    def update_serial_ports(self):
        # Update the available serial ports in the ComboBox
        self.serialPortsCombobox.clear()
        ports = list_ports.comports()
        for port in ports:
            self.serialPortsCombobox.addItem(port.device)


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
     
    # Updating data grom rigol
    def update_rigol(self, rigol_x_data, rigol_y_data, rigol_attenuation, rigol_voltscale, rigol_voltoffset, rigol_coupling, rigol_voltage, rigol_frequency, rigol_function, rigol_g1, rigol_g2, g2_V, rigol_laser_voltage):
        #print(g2_V,rigol_laser_voltage)
        if g2_V:
            self.laser_voltage_monitor.setText(str(rigol_laser_voltage)+" [V]")

        if (self.rigol and self.rigol_chn): 
            self.graph_voltage_trap.clear()
            osc_text = pg.TextItem("Oscilloscope settings:", anchor=(0, 0))
            text_att = pg.TextItem(f"Attenuation: x{rigol_attenuation}", anchor=(0, 0))
            text_vscale = pg.TextItem(f"Voltage scale: {rigol_voltscale} V", anchor=(0, 0))
            text_coup = pg.TextItem(f"Coupling: {rigol_coupling}", anchor=(0, 0))
            
            trap_text = pg.TextItem("Particle trap settings:", anchor=(0, 0))
            text_volt = pg.TextItem(f"Voltage: {rigol_voltage} Vpp", anchor=(0, 0))
            text_voffset = pg.TextItem(f"Voltage offset: {rigol_voltoffset} V", anchor=(0, 0))
            text_freq = pg.TextItem(f"Frequency: {rigol_frequency} Hz", anchor=(0, 0))
            text_function = pg.TextItem(f"Function: {rigol_function}", anchor=(0, 0))

            osc_text.setPos(rigol_x_data[-1]*0.85, float(self.rigol_voltage_max.text())-(float(self.rigol_voltage_max.text())-float(self.rigol_voltage_min.text()))/18*-1)  
            text_att.setPos(rigol_x_data[-1]*0.88, float(self.rigol_voltage_max.text())-(float(self.rigol_voltage_max.text())-float(self.rigol_voltage_min.text()))/18*0)  
            text_vscale.setPos(rigol_x_data[-1]*0.88, float(self.rigol_voltage_max.text())-(float(self.rigol_voltage_max.text())-float(self.rigol_voltage_min.text()))/18*1)  
            text_coup.setPos(rigol_x_data[-1]*0.88, float(self.rigol_voltage_max.text())-(float(self.rigol_voltage_max.text())-float(self.rigol_voltage_min.text()))/18*2) 
             
            trap_text.setPos(rigol_x_data[-1]*0.85, float(self.rigol_voltage_max.text())-(float(self.rigol_voltage_max.text())-float(self.rigol_voltage_min.text()))/18*13)  
            text_volt.setPos(rigol_x_data[-1]*0.88, float(self.rigol_voltage_max.text())-(float(self.rigol_voltage_max.text())-float(self.rigol_voltage_min.text()))/18*14) 
            text_voffset.setPos(rigol_x_data[-1]*0.88, float(self.rigol_voltage_max.text())-(float(self.rigol_voltage_max.text())-float(self.rigol_voltage_min.text()))/18*15)  
            text_freq.setPos(rigol_x_data[-1]*0.88, float(self.rigol_voltage_max.text())-(float(self.rigol_voltage_max.text())-float(self.rigol_voltage_min.text()))/18*16)  
            text_function.setPos(rigol_x_data[-1]*0.88, float(self.rigol_voltage_max.text())-(float(self.rigol_voltage_max.text())-float(self.rigol_voltage_min.text()))/18*17) 

            #print(int(rigol_g1),(not self.rigol_connect.isChecked()))
            
            if rigol_g1 == 1:
                if not self.rigol_particle_trap_enable_btn.isChecked():
                    self.rigol_particle_trap_enable_btn.setStyleSheet("background-color: darkblue;")
                    self.rigol_particle_trap_enable_btn.setChecked(True)

            if rigol_g1 == 0:
                if self.rigol_particle_trap_enable_btn.isChecked():
                    self.rigol_particle_trap_enable_btn.setStyleSheet("background-color: 53, 53, 53;")
                    self.rigol_particle_trap_enable_btn.setChecked(False)
                    
            if self.rigol_chn_n < 3: 
                if len(rigol_x_data) > 0:
                    self.graph_voltage_trap.setXRange(0, rigol_x_data[-1])
                    self.graph_voltage_trap.setYRange(float(self.rigol_voltage_min.text()),float(self.rigol_voltage_max.text()))
                    self.graph_voltage_trap.addItem(osc_text)
                    self.graph_voltage_trap.addItem(text_att)
                    self.graph_voltage_trap.addItem(text_vscale)
                    self.graph_voltage_trap.addItem(text_coup)
                    self.graph_voltage_trap.addItem(trap_text)
                    self.graph_voltage_trap.addItem(text_volt)
                    self.graph_voltage_trap.addItem(text_voffset)
                    self.graph_voltage_trap.addItem(text_freq)
                    self.graph_voltage_trap.addItem(text_function)
                    self.rigol_plot = self.graph_voltage_trap.plot(pen=self.color)
                    self.rigol_plot.setData(x=rigol_x_data, y=rigol_y_data) 
                    self.voltage_monitor.setText(str(rigol_voltage)+" [Vpp]")
                    self.offset_monitor.setText(str(rigol_voltoffset)+" [V]")
                    self.frequency_monitor.setText(str(rigol_frequency)+" [Hz]")
        else:
            self.graph_voltage_trap.clear()  

        #if self.laser_button.isChecked():                    
        
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
        for process in self.processes:
            if process is not None:
                subprocess.run(['pkill', '-f', process.args[0]], check=True)

        self.stop_update_tt_timer()

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

# Reset Rigol USB
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
scope_usb.write(":LAN:APPL")
rigol_ip = scope_usb.query(':LAN:VISA?').strip()
scope_usb.close()

def show_warning_message(message):
    try:
        msg = QMessageBox()
        msg.setWindowTitle("CoDE Warning")
        msg.setText(message)
        msg.setIcon(QMessageBox.Critical)
        msg.exec_()
    except:
        pass

# Get traces from rigol
rigol_prev_stat = 0
def get_rigol_data(status, channel, auto, voltage, frequency, function, tscale, voltage_offset, attenuation, coupling, vscale, g1, g2, laser_voltage, voltage_V, frequency_V, function_V, tscale_V, voltage_offset_V, attenuation_V, coupling_V, vscale_V, g1_V, g2_V, laser_voltage_V):
    global rigol_prev_stat
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
            rigol_voltoffset = float(scope.query(f":CHAN{channel}:OFFS?"))
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
                            threading.Thread(target=show_warning_message, args=("Requesting data, please press 'OK' when the graph updates",)).start()
                    else:
                        rigol_instance.set_rigol_tscale(0, 0.001) 
                        threading.Thread(target=show_warning_message, args=("The 'Time scale' range is between 200 ns and 0.5 s",)).start()
                if (voltage == 1):
                    if (-5 <= voltage_V <= 5):
                        scope.write(f":VOLT {voltage_V}") 
                        rigol_instance.set_rigol_voltage(0, voltage_V) 
                    else:
                        rigol_instance.set_rigol_voltage(0, 0) 
                        threading.Thread(target=show_warning_message, args=("The 'Voltage' range is between -5 and 5 V",)).start()
                elif (frequency == 1):
                    scope.write(f":FREQ {frequency_V}")
                    rigol_instance.set_rigol_frequency(0, frequency_V)  
                elif (function == 1):
                    scope.write(f":FUNC {function_V}")
                    rigol_instance.set_rigol_function(0, function_V)  
                elif (voltage_offset == 1):
                    scope.write(":OFFS 0")
                    scope.write(f":SOUR1:VOLT:OFFS {voltage_offset_V}")
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
                        threading.Thread(target=show_warning_message, args=("'Laser voltage' range is between 0 and 5 V",)).start()
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
        if (status == 1 or g2_V == 1):
            scope.close()
        elif (status == 2 and g2_V == 0):
            if rigol_prev_stat != status:
                os.system('usbreset 1ab1:0515')
                rigol_prev_stat = status
            time.sleep(0.01)
            return np.array([]), np.array([]), 0, 0, 0, "none", 0, 0, "none", 0, 0, 0, 0
        else:
            time.sleep(0.01)
            return np.array([]), np.array([]), 0, 0, 0, "none", 0, 0, "none", 0, 0, 0, 0


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

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    apply_dark_theme(app)
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
        QtWidgets.QMessageBox.critical(None, "Error", error_message)
        # Append the error message to an error log file
        with open("error.log", "a") as log_file:
            log_file.write(error_message)
