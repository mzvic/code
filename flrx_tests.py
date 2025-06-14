from PyQt5 import QtGui, QtCore, QtWidgets
from PyQt5.QtCore import QThread, pyqtSignal, QDateTime, Qt, QTimer
from PyQt5.QtGui import QFont, QImage, QPixmap
from PyQt5.QtWidgets import QTabWidget, QVBoxLayout, QWidget, QCheckBox, QLineEdit, QLabel, QComboBox, QApplication, QMainWindow, QPushButton, QTextEdit, QHBoxLayout, QGridLayout, QLineEdit, QFormLayout, QMessageBox, QButtonGroup
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
import core.core_pb2 as core
import core.core_pb2_grpc as core_grpc
import queue
import gc
import os
import psutil
import struct
#python -m grpc_tools.protoc -I /home/code/Development1/core/ --python_out=. --grpc_python_out=. /home/code/Development1/core/core.proto

# Check if there is another instance
gui_instance = 0

# List to store monitoring data from TwistTorr
twistorr_subscribing_values = []

#Setting vacuum equipment to serial instead of remote controller
os.system('python /home/code/Development/305_008.py')

# Custom Axis class to display timestamps as dates
class DateAxis(pg.AxisItem):
    def tickStrings(self, values, scale, spacing):
        return [datetime.fromtimestamp(float(value)).strftime('%H:%M:%S.%f')[:-3] for value in values]


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



class MainWindow(QMainWindow):
    showWarningSignal = pyqtSignal(str)
    def __init__(self):
        super(MainWindow, self).__init__()

        self.showWarningSignal.connect(self.show_warning_message)

        #path = os.getcwd()
        path = '/home/code/Development'
        
        # Paths to c++ processes
        self.binary_paths = [
            path + '/core/bin/APD_broker2_code_sw',
            path + '/core/bin/APD_plot_cvt_code_sw',
            path + '/core/bin/APD_publisher_code_sw',
            path + '/core/bin/APD_fft_partial_code_sw',
            path + '/core/bin/APD_reg_zero_code_sw', 
            path + '/core/bin/APD_reg_proc_code_sw', 
            path + '/core/bin/APD_reg_fft_1_code_sw',
            path + '/core/bin/APD_reg_fft_01_code_sw',
            path + '/core/bin/APD_fft_full_code_sw',
            path + '/core/bin/TwisTorrIO_code_sw',
            path + '/core/bin/TwisTorrSetter_code_sw',
            path + '/core/bin/TwisTorrMonitor_code_sw',            
            path + '/core/bin/storage',
            path + '/core/bin/recorder',
            path + '/core/bin/TwisTorrSS1_code_sw',
            path + '/core/bin/TwisTorrSS2_code_sw',
            path + '/core/bin/PrevacMonitor_code_sw',
            path + '/core/bin/PrevacSetter_code_sw'
        ]

        # Title of the window
        self.setWindowTitle("CePIA Control Software")
        self.closing = 0

        # Icon for the window
        icon = QtGui.QIcon(path + "/LCT.png")    
        self.setWindowIcon(icon)        
        self.tab_widget = QTabWidget()
        self.setCentralWidget(self.tab_widget)

        # Creates different tabs
        #self.tab1 = QWidget()
        self.tab2 = QWidget()
        #self.tab3 = QWidget()
        #self.tab4 = QWidget()
#        self.tab5 = QWidget()
#        self.tab6 = QWidget()
        self.tab7 = QWidget()

        # Add lateral view to the window for monitoring data
        self.dock = QtWidgets.QDockWidget(self)
        self.addDockWidget(QtCore.Qt.TopDockWidgetArea, self.dock)
        self.dock.setFeatures(QtWidgets.QDockWidget.DockWidgetMovable | QtWidgets.QDockWidget.DockWidgetFloatable)
        #self.dock.setFixedWidth(250)

        # Create grid layout for the dock widget
        self.dock_grid = QtWidgets.QGridLayout()
        
        # ---- Datalogger ----
        self.logging_frame = QtWidgets.QFrame()
        self.logging_frame.setFrameShape(QtWidgets.QFrame.Box)

        self.dock_grid.addWidget(self.logging_frame, 0, 5)

        logging_layout = QtWidgets.QVBoxLayout(self.logging_frame)
        self.logging_frame.setLayout(logging_layout)

        self.logging_button = QtWidgets.QPushButton()
        #self.logging_button.setCheckable(True)
        self.logging_button.setStyleSheet("background-color: 53, 53, 53;")
        self.logging_button.setText("Data logging")
        #self.logging_button.setFixedWidth(120)
        #self.logging_button.clicked.connect(self.logging_connect)
        logging_layout.addWidget(self.logging_button, alignment=Qt.AlignTop | Qt.AlignHCenter)
        
        # ---------------------------------------------
        # ---- SYSTEM PRESSURE ----
        self.pressure_frame = QtWidgets.QFrame()
        self.pressure_frame.setFrameShape(QtWidgets.QFrame.Box)

        self.dock_grid.addWidget(self.pressure_frame, 0, 0, 1, 5)

        pressure_layout = QtWidgets.QVBoxLayout(self.pressure_frame)
        self.pressure_frame.setLayout(pressure_layout)

        self.pressure_button = QtWidgets.QPushButton()
        self.pressure_button.setCheckable(True)
        self.pressure_button.setStyleSheet("background-color: 53, 53, 53;")
        self.pressure_button.setText("Vacuum pump")
        #self.pressure_button.setFixedWidth(120)
        self.pressure_button.clicked.connect(self.execute_twistorr_bar_btn)
        pressure_layout.addWidget(self.pressure_button, alignment=Qt.AlignTop | Qt.AlignLeft)

        self.dock_grid.addWidget(QLabel("       84FS frequency:"), 0, 1)
        self.vacuum_frequency = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.vacuum_frequency, 0, 2)  

        self.dock_grid.addWidget(QLabel("       FRG-700 Pressure:"), 0, 3)
        self.FR_pressure = QtWidgets.QLabel("N/C")
        self.dock_grid.addWidget(self.FR_pressure, 0, 4)        
        
        # ---------------------------------------------

        # add to the dock widget
        self.dock_widget = QtWidgets.QWidget()
        self.dock_widget.setLayout(self.dock_grid)
        self.dock.setWidget(self.dock_widget)

        # Set names to tabs    

        self.tab_widget.addTab(self.tab2, "Vacuum")
        self.tab_widget.addTab(self.tab7, "Data logging")                

        # Amont of processes depends on amount of c++ processes indicated earlier
        self.processes = [None] * 18
        self.threads = []
        #self.processes[9] = subprocess.Popen([self.binary_paths[9]])

        self.processes[0] = subprocess.Popen([self.binary_paths[0]])

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
  
        # Labels for the column titles
        label_305fs = QLabel("TwisTorr 84FS")
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
        #label_pressure = QLabel("XGS-600")
        #label_pressure.setStyleSheet("text-decoration: underline; font-weight: bold;")

        # Set row index order for the titles
        #self.layout2.addWidget(label_pressure, 0, 2, 1, 2)

        self.pressure_plotting_state = 0

        self.btn_pressure = QPushButton("Show pressure data")
        self.btn_pressure.setCheckable(True)  
        self.btn_pressure.setStyleSheet("background-color: 53, 53, 53;")  
        self.btn_pressure.clicked.connect(self.pressure_plot_button) 
        self.btn_pressure.setFixedWidth(300) 
        self.layout2.addWidget(self.btn_pressure, 1, 2, 1, 2) 

        self.pressure1_checkbox = QCheckBox("FRG-700")
        self.pressure1_checkbox.setChecked(True)
        self.layout2.addWidget(self.pressure1_checkbox, 2, 2)

        self.pressure_secs_label = QLabel("T-axis length (in seconds):")
        #self.apd_counts_secs_label.setFixedWidth(195)
        #self.pressure_secs_label.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.pressure_secs_input = QLineEdit(self)
        self.pressure_secs_input.setFixedWidth(150)
        self.pressure_secs_input.setText("86400") # Default value      
        self.layout2.addWidget(self.pressure_secs_label, 3, 2)
        self.layout2.addWidget(self.pressure_secs_input, 3, 3)

        self.graph_pressure_vacuum = pg.PlotWidget(axisItems={'bottom': DateAxis(orientation='bottom')})

        #self.graph_pressure_vacuum.invertY()
        
        self.graph_pressure_vacuum.showGrid(x=True, y=True, alpha=1)  
        #self.graph_pressure_vacuum.plotItem.setLogMode(x=True)         
        self.graph_pressure_vacuum.setLabel('bottom', 'Time', units='hh:mm:ss.µµµµµµ')
        self.graph_pressure_vacuum.setLabel('left', 'Pressure [Torr]')
        self.pressure_data1 = []
        self.pressure_time = []          
        self.vacuum_plot_time = 0 
        self.setting_twistorr = 0

        self.layout2.addWidget(self.graph_pressure_vacuum, 8, 0, 1, 4) 
        
        ## Initial data for plot 1
        self.pressure_plot1 = self.graph_pressure_vacuum.plot([time.time()-3,time.time()-2,time.time()-1,time.time()], [0,0,0,0], pen=pg.mkPen(color=(255, 0, 0), width=2))
        self.btn_vacuum_monitor = QPushButton("Connect to vacuum equipment")
        self.btn_vacuum_monitor.setCheckable(True)  
        self.btn_vacuum_monitor.setStyleSheet("background-color: 53, 53, 53;")  
        self.btn_vacuum_monitor.clicked.connect(self.execute_twistorr_btn) 
        self.layout2.addWidget(self.btn_vacuum_monitor, 9, 0, 1, 4)

        
        #self.layout2.setRowStretch(8, 1)


        # ------------------------------------------------------------------------------------------- #
        # Registers TAB
        # ------------------------------------------------------------------------------------------- #
        self.layout7 = QGridLayout(self.tab7)

        # -------------------------------------------------------------------- #
        # Twistorr #
        twistorr_data_group = QtWidgets.QGroupBox("Vacuum data:")
        twistorr_data_grid_layout = QGridLayout()   

        self.twistorr_1_checkbox = QCheckBox("Pump current (84FS)")
        self.twistorr_1_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_1_checkbox, 1, 0)

        self.twistorr_2_checkbox = QCheckBox("Pump voltage (84FS)")
        self.twistorr_2_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_2_checkbox, 2, 0)

        self.twistorr_3_checkbox = QCheckBox("Pump power (84FS)")
        self.twistorr_3_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_3_checkbox, 3, 0)

        self.twistorr_4_checkbox = QCheckBox("Pump frequency (84FS)")
        self.twistorr_4_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_4_checkbox, 1, 1)

        self.twistorr_5_checkbox = QCheckBox("Pump temperature (84FS)")
        self.twistorr_5_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.twistorr_5_checkbox, 2, 1) 

        self.pressure_1_checkbox = QCheckBox("Pressure (FRG-700)")
        self.pressure_1_checkbox.setChecked(False)
        twistorr_data_grid_layout.addWidget(self.pressure_1_checkbox, 3, 1)  

        mark_all_twistorr_button = QPushButton("Check all")
        mark_all_twistorr_button.clicked.connect(self.mark_all_twistorr_checkboxes)

        unmark_all_twistorr_button = QPushButton("Uncheck all")
        unmark_all_twistorr_button.clicked.connect(self.unmark_all_twistorr_checkboxes)
  
        twistorr_data_grid_layout.addWidget(mark_all_twistorr_button, 4, 0)
        twistorr_data_grid_layout.addWidget(unmark_all_twistorr_button, 4, 1)

        twistorr_data_group.setLayout(twistorr_data_grid_layout)

        self.layout7.addWidget(twistorr_data_group, 0, 0, 1, 2)

        #----------------------- Folder name and log filename settings --------------------------------------------------------------------------------- #
        self.storage_filename_group = QtWidgets.QGroupBox("Data logging folder name and log filename:")
        self.storage_filename_grid_layout = QGridLayout()                 

        self.folder_line_edit = QLineEdit()
        self.folder_line_edit.setFixedWidth(700)
        self.folder_line_edit.setPlaceholderText("Enter a dataset folder name, otherwise files will be stored in '/home/user/CePIADataLogging'")
        self.folder_line_edit.setAlignment(QtCore.Qt.AlignRight)
        self.storage_filename_grid_layout.addWidget(self.folder_line_edit, 10, 0)

        self.storage_line_edit = QLineEdit()
        self.storage_line_edit.setFixedWidth(700)
        self.storage_line_edit.setPlaceholderText("Enter a file name or press the 'Set datetime as filename' button...")
        self.storage_line_edit.setAlignment(QtCore.Qt.AlignRight)
        self.storage_filename_grid_layout.addWidget(self.storage_line_edit, 11, 0)
        
        h5_label = QLabel(".h5")
        h5_label.setFixedWidth(200)
        h5_label.setAlignment(QtCore.Qt.AlignLeft)
        self.storage_filename_grid_layout.addWidget(h5_label, 11, 1)

        self.auto_store_name_button = QPushButton("Set datetime as filename")
        self.auto_store_name_button.clicked.connect(self.set_datetime_as_filename)
        self.storage_filename_grid_layout.addWidget(self.auto_store_name_button, 12, 0, 1, 2)  

        self.storage_filename_group.setLayout(self.storage_filename_grid_layout)
        self.layout7.addWidget(self.storage_filename_group, 11, 0, 1, 2)

        #----------------------- Logging period settings --------------------------------------------------------------------------------- #

        self.logging_duration_button_group = QtWidgets.QGroupBox("Data logging period (when 'Not defined' is checked, data logging stops when the user presses the 'Stop data logging' button):")
        self.logging_duration_button_grid_layout = QGridLayout()      

        self.logging_period_notdefined_checkbox = QCheckBox("Not defined")
        self.logging_period_notdefined_checkbox.setChecked(True)
        self.logging_period_notdefined_checkbox.setFixedWidth(110) 
        self.logging_duration_button_grid_layout.addWidget(self.logging_period_notdefined_checkbox, 1, 0)

        self.logging_period_defined_checkbox = QCheckBox("User defined")
        self.logging_period_defined_checkbox.setChecked(False)
        self.logging_period_defined_checkbox.setFixedWidth(110)
        self.logging_duration_button_grid_layout.addWidget(self.logging_period_defined_checkbox, 1, 1)

        logging_minutes_label = QLabel("Data logging period in minutes: ")
        logging_minutes_label.setFixedWidth(250) 
        self.logging_duration_button_grid_layout.addWidget(logging_minutes_label, 1, 2)        

        self.logging_minutes_input = QLineEdit(self)
        self.logging_minutes_input.setFixedWidth(80) 
        #self.logging_minutes_input.setText("10") # Default value  
        self.logging_minutes_input.setPlaceholderText("e.g. '180'")     
        self.logging_duration_button_grid_layout.addWidget(self.logging_minutes_input, 1, 3) 

        # Crear un QButtonGroup y agregar los checkboxes
        self.period_button_group = QButtonGroup()
        self.period_button_group.addButton(self.logging_period_notdefined_checkbox)
        self.period_button_group.addButton(self.logging_period_defined_checkbox)

        # Establecer el modo de exclusividad
        self.period_button_group.setExclusive(True)

        self.logging_time_stop =  9999999999

        self.logging_duration_button_group.setLayout(self.logging_duration_button_grid_layout)
        self.logging_duration_button_group.setLayout(self.logging_duration_button_grid_layout)
        self.layout7.addWidget(self.logging_duration_button_group, 12, 0, 1, 2)       


        #----------------------- Minutes per file settings --------------------------------------------------------------------------------- #

        self.minutes_per_file_button_group = QtWidgets.QGroupBox("Data logging file size defined by period (when 'Defined' is checked, different files are generated every X minutes, otherwise only one file is generated):")
        self.minutes_per_file_button_grid_layout = QGridLayout()      

        self.minutes_per_file_notdefined_checkbox = QCheckBox("Not defined")
        self.minutes_per_file_notdefined_checkbox.setChecked(True)
        self.minutes_per_file_notdefined_checkbox.setFixedWidth(110) 
        self.minutes_per_file_button_grid_layout.addWidget(self.minutes_per_file_notdefined_checkbox, 1, 0)

        self.minutes_per_file_defined_checkbox = QCheckBox("User defined")
        self.minutes_per_file_defined_checkbox.setChecked(False)
        self.minutes_per_file_defined_checkbox.setFixedWidth(110)
        self.minutes_per_file_button_grid_layout.addWidget(self.minutes_per_file_defined_checkbox, 1, 1)

        minutes_per_file_label = QLabel("Max period per file in minutes: ")
        minutes_per_file_label.setFixedWidth(250) 
        self.minutes_per_file_button_grid_layout.addWidget(minutes_per_file_label, 1, 2)        

        self.minutes_per_file_input = QLineEdit(self)
        self.minutes_per_file_input.setFixedWidth(80) 
        self.minutes_per_file_input.setPlaceholderText("e.g. '10'")     
        self.minutes_per_file_button_grid_layout.addWidget(self.minutes_per_file_input, 1, 3) 

        # Crear un QButtonGroup y agregar los checkboxes
        self.min_per_file_button_group = QButtonGroup()
        self.min_per_file_button_group.addButton(self.minutes_per_file_notdefined_checkbox)
        self.min_per_file_button_group.addButton(self.minutes_per_file_defined_checkbox)

        # Establecer el modo de exclusividad
        self.min_per_file_button_group.setExclusive(True)

        self.minutes_per_file_argument = 1215752191      

        self.minutes_per_file_button_group.setLayout(self.minutes_per_file_button_grid_layout)
        self.minutes_per_file_button_group.setLayout(self.minutes_per_file_button_grid_layout)
        self.layout7.addWidget(self.minutes_per_file_button_group, 13, 0, 1, 2)  


        #----------------------- Start - stop buttons  --------------------------------------------------------------------------------- #
        storage_button_group = QtWidgets.QGroupBox("Data logging (files stored in /home/user/CePIADataLogging folder):")
        storage_button_grid_layout = QGridLayout()      

        self.begin_logging_button = QPushButton("Begin data logging")
        self.begin_logging_button.setCheckable(True) 
        self.begin_logging_button.clicked.connect(self.logging_connect)
        storage_button_grid_layout.addWidget(self.begin_logging_button, 14, 0)  

        self.stop_logging_button = QPushButton("Stop data logging") 
        self.stop_logging_button.setCheckable(True) 
        self.stop_logging_button.clicked.connect(self.logging_disconnect)
        storage_button_grid_layout.addWidget(self.stop_logging_button, 14, 1)     

        storage_button_group.setLayout(storage_button_grid_layout)
        self.layout7.addWidget(storage_button_group, 14, 0, 1, 2)        
          
        

        # ------------------------------------------------------------------------------------------------------------------ #
        #----------------------- FUNCTIONS --------------------------------------------------------------------------------- #
        # ------------------------------------------------------------------------------------------------------------------ #
    def logging_connect(self):
        # --------------------------#
        # Filename and checkbox verification #
        if not (self.twistorr_1_checkbox.isChecked() or self.twistorr_2_checkbox.isChecked() or
                self.twistorr_3_checkbox.isChecked() or self.twistorr_4_checkbox.isChecked() or
                self.twistorr_5_checkbox.isChecked() or self.pressure_1_checkbox.isChecked()):
            self.showWarningSignal.emit("Select the variables to record before begin data logging...")
            self.begin_logging_button.setChecked(False)
        if not self.storage_line_edit.text():
            self.showWarningSignal.emit("Give the file a name or press the 'Set datetime as filename' button before starting data logging...")
            self.begin_logging_button.setChecked(False)
        
        self.input_period_text = 0

        if self.logging_period_defined_checkbox.isChecked():
            try:
                input_period_text = int(self.logging_minutes_input.text())
            except ValueError:
                self.begin_logging_button.setChecked(False)
                self.input_period_text = -1
                self.showWarningSignal.emit("You have selected the User-defined Data logging period option, please enter a valid value (integer greater than zero)...")
                                
        if self.logging_period_defined_checkbox.isChecked():
            if input_period_text <= 0:       
                self.input_period_text = -1
                self.begin_logging_button.setChecked(False)
                self.showWarningSignal.emit("You have selected the User-defined Data logging period option, please enter a valid value (integer greater than zero)...")   
            else:
                self.logging_time_stop = int(time.time()) + (int(input_period_text)*60)

        if self.minutes_per_file_defined_checkbox.isChecked():
            try:
                input_minutes_per_file_text = int(self.minutes_per_file_input.text())
            except ValueError:
                self.begin_logging_button.setChecked(False)
                self.input_minutes_per_file_text = -1
                self.showWarningSignal.emit("You have selected the User-defined Data logging file size defined by period option, please enter a valid value (integer greater than zero)...")

        if self.minutes_per_file_defined_checkbox.isChecked():
            if input_minutes_per_file_text <= 0:       
                self.input_minutes_per_file_text = -1
                self.begin_logging_button.setChecked(False)
                self.showWarningSignal.emit("You have selected the User-defined Data logging file size defined by period option, please enter a valid value (integer greater than zero)...")
            else:
                self.minutes_per_file_argument = (int(input_minutes_per_file_text)*60)        


        if (self.twistorr_1_checkbox.isChecked() or self.twistorr_2_checkbox.isChecked() or
                self.twistorr_3_checkbox.isChecked() or self.twistorr_4_checkbox.isChecked() or
                self.twistorr_5_checkbox.isChecked() or self.pressure_1_checkbox.isChecked()) and (self.storage_line_edit.text()):   

            if self.begin_logging_button.isChecked():
                self.begin_logging_button.setStyleSheet("background-color: darkblue;")
                self.begin_logging_button.setText("Begin data logging")
                self.logging_button.setStyleSheet("background-color: darkblue;")# color: black;")
                self.logging_button.setText("Data logging")
                self.begin_logging_button.setChecked(True)

                storage_list = []

                if not self.folder_line_edit.text():
                    storage_list.append("/home/code/CepiaDataLogging/"+self.storage_line_edit.text())
                else:
                    storage_list.append("/home/code/CepiaDataLogging/"+self.folder_line_edit.text()+"/"+self.storage_line_edit.text())
                storage_list.append(str(self.minutes_per_file_argument))
    
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
                if self.pressure_1_checkbox.isChecked():
                    storage_list.append("pressure_1") 
                    print("Ejecutando recorder 'FRG-702'")                                                               

                if storage_list:
                    storage_command = [self.binary_paths[12]] + [str(stg_arg) for stg_arg in storage_list]
                    print("Executing command:", " ".join(storage_command))
                    
                    # Execute storage with all elements of storage_list as arguments
                    self.processes[12] = subprocess.Popen(storage_command)
                    
                record_list = []

                if (self.twistorr_1_checkbox.isChecked() or self.twistorr_2_checkbox.isChecked() or
                        self.twistorr_3_checkbox.isChecked() or self.twistorr_4_checkbox.isChecked() or
                        self.twistorr_5_checkbox.isChecked() or self.pressure_1_checkbox.isChecked()): 
                    record_list.append("DATA_TT_MON")    
                  
                print(record_list)
                if record_list:
                    record_command = [self.binary_paths[13]] + [str(rec_arg) for rec_arg in record_list]
                    print("Executing command:", " ".join(record_command))
                    # Execute record with all elements of record_list as arguments
                    self.processes[13] = subprocess.Popen(record_command)
            else:
                if self.input_period_text >= 0:
                    self.begin_logging_button.setChecked(True)

    def mark_all_twistorr_checkboxes(self):
        self.twistorr_1_checkbox.setChecked(True)
        self.twistorr_2_checkbox.setChecked(True)
        self.twistorr_3_checkbox.setChecked(True)
        self.twistorr_4_checkbox.setChecked(True)
        self.twistorr_5_checkbox.setChecked(True)
        self.pressure_1_checkbox.setChecked(True)

    def unmark_all_twistorr_checkboxes(self):
        self.twistorr_1_checkbox.setChecked(False)
        self.twistorr_2_checkbox.setChecked(False)
        self.twistorr_3_checkbox.setChecked(False)
        self.twistorr_4_checkbox.setChecked(False)
        self.twistorr_5_checkbox.setChecked(False)
        self.pressure_1_checkbox.setChecked(False)


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
        formatted_datetime = date_time.strftime("dataset_%Y%m%d_%H:%M:%S")
        self.storage_line_edit.setText(formatted_datetime)

    def execute_twistorr_bar_btn(self):
        if self.pressure_button.isChecked():
            self.pressure_button.setStyleSheet("background-color: darkblue;")
            self.btn_vacuum_monitor.setChecked(True)
            self.btn_vacuum_monitor.setStyleSheet("background-color: darkblue;")
            self.execute_twistorr_monitor()
        else:
            self.pressure_button.setStyleSheet("background-color: 53, 53, 53;")
            self.btn_vacuum_monitor.setChecked(False)
            self.btn_vacuum_monitor.setStyleSheet("background-color: 53, 53, 53;")
            self.kill_twistorr_monitor()

    def execute_twistorr_btn(self):
        if self.btn_vacuum_monitor.isChecked():
            self.btn_vacuum_monitor.setStyleSheet("background-color: darkblue;")
            self.pressure_button.setChecked(True)
            self.pressure_button.setStyleSheet("background-color: darkblue;")
            self.execute_twistorr_monitor()
        else:
            self.btn_vacuum_monitor.setStyleSheet("background-color: 53, 53, 53;")
            self.pressure_button.setChecked(False)
            self.pressure_button.setStyleSheet("background-color: 53, 53, 53;")
            self.kill_twistorr_monitor()

    def tt_startstop1_button(self):
        if self.btn_vacuum_monitor.isChecked():
            if self.btn_tt_startstop1.isChecked():
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

                pre_vacuum_pressure1 = float(twistorr_subscribing_values[12])

                self.vacuum_pressure1 = str("{:.2E}".format(pre_vacuum_pressure1))

                # Update the labels with the vacuum-related values
                self.monitor_vacuum_current.setText(vacuum_current1+" [mA]")
                self.monitor_vacuum_voltage.setText(vacuum_voltage1+" [Vdc]")
                self.monitor_vacuum_power.setText(vacuum_power1+" [W]")
                self.monitor_vacuum_frequency.setText(vacuum_frequency1+" [Hz]")
                self.monitor_vacuum_temperature.setText(vacuum_temperature1+" [°C]")
                self.vacuum_frequency.setText(vacuum_frequency1+" [Hz]")
                self.FR_pressure.setText(self.vacuum_pressure1+" [Torr]")
                
                if (vacuum_status1 == 1):
                    self.btn_tt_startstop1.setChecked(True)
                    self.btn_tt_startstop1.setStyleSheet("background-color: darkblue;")
                else:
                    self.btn_tt_startstop1.setChecked(False)
                    self.btn_tt_startstop1.setStyleSheet("background-color: 53, 53, 53;")                                   
        else:        
            self.monitor_vacuum_current.setText("N/C")
            self.monitor_vacuum_voltage.setText("N/C")
            self.monitor_vacuum_power.setText("N/C")
            self.monitor_vacuum_frequency.setText("N/C")
            self.monitor_vacuum_temperature.setText("N/C")
            self.vacuum_frequency.setText("N/C")      
            self.FR_pressure.setText("N/C")
            self.btn_tt_startstop1.setChecked(False)
            self.btn_tt_startstop1.setStyleSheet("background-color: 53, 53, 53;")                    
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
    
    def update_logging_stop(self):
        #print(int(time.time()) - int(self.logging_time_stop))
        if int(time.time()) > int(self.logging_time_stop):
            self.stop_logging_button.click()

    def start_update_logging_timer(self):
        # Start a QTimer to periodically update logging time stop
        self.timer_logging = QTimer()
        self.timer_logging.timeout.connect(self.update_logging_stop)
        self.timer_logging.start(1000)  # Update interval for logging stop        

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

    def update_graph_pressure(self):
        if (self.pressure_plotting_state == 1):
            timestamp = float(time.time())  # Extract timestamp from data
            value1 = float(twistorr_subscribing_values[12])  # Extract value from data
            self.pressure_time.append(timestamp)  # Add timestamp to times1 list
            self.pressure_data1.append(value1)  # Add value to data1 list

            # Calculate the cut-off time based on the input value
            current_time = time.time()
            cut_off_time = current_time - int(self.pressure_secs_input.text())

            # Keep only the data within the specified time range
            self.pressure_time = [t for t in self.pressure_time if t >= cut_off_time]
            self.pressure_data1 = self.pressure_data1[-len(self.pressure_time):]

            if (self.pressure1_checkbox.isChecked()):
                self.pressure_plot1.setData(self.pressure_time, self.pressure_data1)

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

    def show_warning_message(self, message):
        msg = QMessageBox()
        msg.setWindowTitle("Warning!!!")
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
        msg.setWindowTitle("Warning!!!")
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
    app.setApplicationName("CePIA Control Software")
    # Verifies if there is a lock file
    if check_lock_file():
        alertWindow = AlertWindow()
        alertWindow.showWarningSignal.emit("There is currently an instance of 'CePIA Control Software', please close the old one to open a new one, or continue working with the old one...")      
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
            mainWindow.start_update_logging_timer()
            # Execute the application event loop
            sys.exit(app.exec_())
        except Exception as e:
            # Handle unexpected exceptions by displaying an error message
            error_message = "\n" +"An unexpected error has occurred: {}".format(str(e))
            #QtWidgets.QMessageBox.critical(None, "Error", error_message)
            # Append the error message to an error log file
            with open("error.log", "a") as log_file:
                log_file.write(error_message)
        finally:
            # Delete the lock file at the end
            os.remove("lockfile.lock")            

