import sys
import time
import socket
from PyQt5.QtWidgets import QApplication, QMainWindow, QPushButton, QVBoxLayout, QWidget, QTextEdit, QHBoxLayout, QLabel, QGridLayout, QLineEdit,QFormLayout
from PyQt5.QtCore import QThread, pyqtSignal, QDateTime, Qt
import pyqtgraph as pg
import numpy as np
import subprocess
from PyQt5 import QtGui, QtCore
from PyQt5.QtWidgets import QTabWidget, QVBoxLayout, QWidget
from PyQt5.QtWidgets import QCheckBox
from PyQt5 import QtGui, QtCore, QtWidgets
from serial.tools import list_ports
from PyQt5.QtWidgets import QLineEdit, QLabel, QComboBox

class DateAxis(pg.AxisItem):
    def tickStrings(self, values, scale, spacing):
        return [QDateTime.fromSecsSinceEpoch(int(value)).toString('hh:mm:ss') for value in values]

class SocketThread(QThread):
    signal = pyqtSignal('PyQt_PyObject')

    def __init__(self, port):
        QThread.__init__(self)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind(('localhost', port))
        self.sock.listen(1)
        self.running = True

    def run(self):
        conn, addr = self.sock.accept()
        with conn:
            print('Connected by', addr)
            while self.running:
                data = conn.recv(262144)
                if not data:
                    break
                self.signal.emit(data.decode())

    def stop(self):
        self.running = False
        #self.sock.shutdown(socket.SHUT_RDWR)
        self.sock.close()

class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.setWindowTitle("CoDE Control Software")
        icon = QtGui.QIcon("/home/code/Development/CoDE.png")  
        self.setWindowIcon(icon)        
        self.tab_widget = QTabWidget()
        self.setCentralWidget(self.tab_widget)

        self.tab1 = QWidget()
        self.tab2 = QWidget()
        self.tab3 = QWidget()
        self.tab4 = QWidget()
        self.tab5 = QWidget()
        self.tab6 = QWidget()
        self.tab7 = QWidget()                

        self.tab_widget.addTab(self.tab1, "APD")
        self.tab_widget.addTab(self.tab2, "Vacuum")
        self.tab_widget.addTab(self.tab3, "ESI")
        self.tab_widget.addTab(self.tab4, "Particle trap")
        self.tab_widget.addTab(self.tab5, "Temperature")
        self.tab_widget.addTab(self.tab6, "Data processing")
        self.tab_widget.addTab(self.tab7, "Registers")                

        self.layout = QVBoxLayout(self.tab1)
        self.threads = []
        self.processes = [None] * 6

        self.graph1 = pg.PlotWidget(axisItems={'bottom': DateAxis(orientation='bottom')})
        self.layout.addWidget(self.graph1)
        plotItem1 = self.graph1.getPlotItem()
        plotItem1.showGrid(True, True, 0.7)

        self.data1 = []
        self.times1 = []

        self.graph2 = pg.PlotWidget()
        self.layout.addWidget(self.graph2)
        plotItem2 = self.graph2.getPlotItem()
        plotItem2.showGrid(True, True, 0.7)

        self.data2 = []
        self.freq2 = []

        self.binary_paths = [
            '/home/code/Development/coress/bin/APD_broker',
            '/home/code/Development/coress/bin/APD_plot_cvt',
            '/home/code/Development/coress/bin/APD_publisher',
            '/home/code/Development/coress/bin/fft',
            '/home/code/Development/coress/bin/APD_reg_zero', # 'APD_reg' for RAW data with timestamp (TS) from the t0, 'APD_reg_zero' for RAW data with TS from zero...
            '/home/code/Development/coress/bin/APD_reg_proc' # 'APD_reg_proc' for data @ 100Hz with TS from zero...
        ]

        button_layout_1 = QHBoxLayout() 
        button_names_1 = ["Server", "Counts plot"] 

        button_layout_2 = QHBoxLayout() 
        button_names_2 = ["Plot counts", "Plot FFT", "Export counts data [100kHz]", "Export counts data [1kHz]"]

        arg_input_layout = QHBoxLayout() # Parámetros de entrada
        serialPortsLabel = QLabel("                    FPGA serial port:")
        self.serialPortsCombobox = QComboBox(self)
        self.update_serial_ports()
        index_p = self.serialPortsCombobox.findText('/dev/ttyUSB1', QtCore.Qt.MatchFixedString)
        if index_p >= 0:
             self.serialPortsCombobox.setCurrentIndex(index_p)        
        arg_input_layout.addWidget(serialPortsLabel)
        arg_input_layout.addWidget(self.serialPortsCombobox)

        samplingFreqLabel = QLabel("                    Sampling frequency:")
        self.samplingFreqCombobox = QComboBox(self)
        self.samplingFreqCombobox.addItems(['100Hz', '500Hz', '1kHz', '2kHz', '4kHz', '10kHz', '50kHz', '100kHz'])
        index_f = self.samplingFreqCombobox.findText('4kHz', QtCore.Qt.MatchFixedString)
        if index_f >= 0:
             self.samplingFreqCombobox.setCurrentIndex(index_f)         
        arg_input_layout.addWidget(samplingFreqLabel)
        arg_input_layout.addWidget(self.samplingFreqCombobox)
        self.sampling_freq_values = {
            0: 100, # '100Hz' 
            1: 500, # '500Hz' 
            2: 1000, # '1kHz'          
            3: 2000, # '2kHz'          
            4: 4000, # '4kHz'
            5: 10000, # '10kHz' 
            6: 50000, # '50kHz'                                              
            7: 100000, # '100kHz'                                                          
        }

        #apd_fft_arg_label = QLabel("                    FFT samples:")
        #self.apd_fft_arg_input = QLineEdit(self)
        #self.apd_fft_arg_input.setFixedWidth(100) 
        #self.apd_fft_arg_input.setText("2048")       
        #arg_input_layout.addWidget(apd_fft_arg_label)
        #arg_input_layout.addWidget(self.apd_fft_arg_input)
        
        windowTypeLabel = QLabel("                    FFT Window Type:")
        self.windowTypeCombobox = QComboBox(self)
        self.windowTypeCombobox.addItems(['Hamming', 'Hann', 'Blackman-Harris 4', 'Blackman-Harris 7', 'No window'])
        index_w = self.windowTypeCombobox.findText('Blackman-Harris 7', QtCore.Qt.MatchFixedString)
        if index_w >= 0:
             self.windowTypeCombobox.setCurrentIndex(index_w)
        arg_input_layout.addWidget(windowTypeLabel)
        arg_input_layout.addWidget(self.windowTypeCombobox)
        self.layout.addLayout(arg_input_layout) 
        self.window_type_values = {
            0: 1,  # 'Hamming'
            1: 2,  # 'Hann'
            2: 3,  # 'Blackmann-Harris 4'
            3: 4,  # 'Blackmann-Harris 7'
            4: 5,  # 'No window'
        }   
        
        self.buttons = [] # Creación de botones para ejecutar procesos
        
        for i in range(2):
            self.threads.append(SocketThread(12345 + i))
            if (i==4):
                self.threads[-1].signal.connect(self.update_graph2)
            else:
                self.threads[-1].signal.connect(self.update_graph1)
            self.threads[-1].start()
            start_stop_button = QPushButton(button_names_1[i])
            start_stop_button.setCheckable(True)
            start_stop_button.toggled.connect(lambda checked, i=i: self.toggle_process(i, checked))            
            button_layout_1.addWidget(start_stop_button)
            self.buttons.append(start_stop_button)
            

        for i in range(2, 6):
            self.threads.append(SocketThread(12345 + i))
            if (i==3):
                self.threads[-1].signal.connect(self.update_graph2)
            else:
                self.threads[-1].signal.connect(self.update_graph1)
            self.threads[-1].start()
            start_stop_button = QPushButton(button_names_2[i - 2])
            
            
            start_stop_button = QPushButton(button_names_2[i - 2])
            start_stop_button.setCheckable(True)
            start_stop_button.toggled.connect(lambda checked, i=i: self.toggle_process(i, checked))
            button_layout_2.addWidget(start_stop_button)
            self.buttons.append(start_stop_button)  
            
        self.buttons[0].setChecked(True)  # 'Server'
        self.buttons[1].setChecked(True)  # 'Counts plot'
        self.threads.append(SocketThread(12352))  
        self.layout.addLayout(button_layout_1)
        self.layout.addLayout(button_layout_2)
        self.note = QtWidgets.QLabel("Important: To be able to graph the FFT, the 'Plot counts' button must be enabled. Also, if the FFT settings are modified, 'Plot FFT' must be disabled and then enabled for the changes to take effect.")
        self.layout.addWidget(self.note)
        self.buttons[0].hide()  ###### Se esconden los primeros 2 botones...
        self.buttons[1].hide()

        # -----------------------------------------
        # ----------------- Tab 2 -----------------
        self.layout2 = QGridLayout(self.tab2)

        label_instrument = QLabel("Instrument")
        label_monitor = QLabel("Monitor")
        label_setpoint = QLabel("Setpoint")
        label_instrument.setStyleSheet("text-decoration: underline; font-weight: bold;")
        label_monitor.setStyleSheet("text-decoration: underline; font-weight: bold;")
        label_setpoint.setStyleSheet("text-decoration: underline; font-weight: bold;")

        self.layout2.addWidget(label_instrument, 0, 0)
        self.layout2.addWidget(label_monitor, 0, 1)
        self.layout2.addWidget(label_setpoint, 0, 2)

        monitor_vacuum_pressure = QLabel("N/A")
        set_vacuum_pressure = QLineEdit()
        set_vacuum_pressure.setPlaceholderText("Set pressure")
        set_vacuum_pressure.setFixedWidth(100)
        btn_vacuum_pressure = QPushButton("Set")
        self.layout2.addWidget(QLabel("Vacuum Presure:"), 1, 0)
        self.layout2.addWidget(monitor_vacuum_pressure, 1, 1)
        self.layout2.addWidget(set_vacuum_pressure, 1, 2)
        self.layout2.addWidget(btn_vacuum_pressure, 1, 3)

        monitor_speed_motor = QLabel("N/A")
        set_speed_motor = QLineEdit()
        set_speed_motor.setPlaceholderText("Set speed")
        set_speed_motor.setFixedWidth(100)
        btn_speed_motor = QPushButton("Set")
        self.layout2.addWidget(QLabel("Speed Motor:"), 2, 0)
        self.layout2.addWidget(monitor_speed_motor, 2, 1)
        self.layout2.addWidget(set_speed_motor, 2, 2)
        self.layout2.addWidget(btn_speed_motor, 2, 3)

        monitor_valve_state = QLabel("N/A")
        set_valve_state = QLineEdit()
        set_valve_state.setPlaceholderText("Set state")
        set_valve_state.setFixedWidth(100)
        btn_valve_state = QPushButton("Set")
        self.layout2.addWidget(QLabel("Valve State:"), 3, 0)
        self.layout2.addWidget(monitor_valve_state, 3, 1)
        self.layout2.addWidget(set_valve_state, 3, 2)
        self.layout2.addWidget(btn_valve_state, 3, 3)

        monitor_bomb_power = QLabel("N/A")
        # btn_boost = QPushButton("Set")
        self.layout2.addWidget(QLabel("Bomb Power:"), 4, 0)
        self.layout2.addWidget(monitor_bomb_power, 4, 1)
        # self.layout2.addWidget(btn_boost, 4, 3)

        monitor_temperature = QLabel("N/A")
        self.layout2.addWidget(QLabel("Temperature:"), 5, 0)
        self.layout2.addWidget(monitor_temperature, 5, 1)

        self.layout2.setRowStretch(6, 1)

        # ---------------------------------------- 
        # ----------------- Tab 3 -----------------
        # ----------------------------------------  
        # ----------------- Tab 4 -----------------
        self.layout4 = QGridLayout(self.tab4)

        input_mass = QLineEdit()
        self.layout4.addWidget(QLabel("Mass:"), 1, 0)
        self.layout4.addWidget(input_mass, 1, 1)

        input_charge = QLineEdit()
        self.layout4.addWidget(QLabel("Charge:"), 2, 0)
        self.layout4.addWidget(input_charge, 2, 1)

        input_geometrical = QLineEdit()
        self.layout4.addWidget(QLabel("Geometrical Parameter:"), 3, 0)
        self.layout4.addWidget(input_geometrical, 3, 1)

        input_voltage = QLineEdit()
        self.layout4.addWidget(QLabel("Voltage:"), 4, 0)
        self.layout4.addWidget(input_voltage, 4, 1)

        input_frequency = QLineEdit()
        self.layout4.addWidget(QLabel("Frequency:"), 5, 0)
        self.layout4.addWidget(input_frequency, 5, 1)

        btn_trap = QPushButton("On/Off")
        btn_trap.setCheckable(True)
        btn_trap.setFixedHeight(200)
        btn_trap.setFixedWidth(200)
        self.layout4.addWidget(btn_trap, 1, 3, 6, 1)

        btn_calculate = QPushButton("Calculate")
        self.layout4.addWidget(btn_calculate, 7, 1)

        q_calculated = QLabel("N/A")
        self.layout4.addWidget(QLabel("q:"), 6, 0)
        self.layout4.addWidget(q_calculated, 6, 1)


        self.graph_voltage_trap = pg.PlotWidget(axisItems={'bottom': DateAxis(orientation='bottom')})
        self.layout4.addWidget(self.graph_voltage_trap, 8, 0, 1, 4)
        self.graph_voltage_trap.setLabel('left', 'Voltage (V)')
        self.graph_voltage_trap.setLabel('bottom', 'Time (s)')
        self.graph_voltage_trap.showGrid(x=True, y=True)
        self.graph_voltage_trap.setYRange(-0.1, 0.1)
        self.graph_voltage_trap.setXRange(0, 10)

        def calculate_q():
            geometrical = input_geometrical.text()
            frequency = input_frequency.text()
            mass = input_mass.text()
            charge = input_charge.text()
            voltage = input_voltage.text()
            if geometrical == "" or frequency == "" or mass == "" or charge == "" or voltage == "":
                q = "Missing data"
            elif float(geometrical) == 0 or float(frequency) == 0 or float(mass) == 0:
                q = "Division by zero"
            else:
                mass = float(mass)
                charge = float(charge)
                geometrical = float(geometrical)
                voltage = float(voltage)
                frequency = float(frequency)
                q = (4 * charge * voltage) / (geometrical**2 * frequency * mass)
            q_calculated.setText(str(q))
            
        btn_calculate.clicked.connect(calculate_q)


        # ----------------------------------------
        # ----------------- Tab 5 -----------------
        # ----------------------------------------
        # ----------------- Tab 6 -----------------
        # ----------------------------------------
        # ----------------- Tab 7 -----------------
        # ----------------------------------------   


    # ------------- Functions ----------------
    def update_serial_ports(self):    
        self.serialPortsCombobox.clear()
        ports = list_ports.comports()
        for port in ports:
            self.serialPortsCombobox.addItem(port.device) 

    def toggle_process(self, i, checked):
        sender = self.sender()
        button_names = ["Server", "Counts plot", "Plot counts", "Plot FFT", "Export counts data [100kHz]", "Export counts data [1kHz]"] 
        if checked:
            if i == 2: 
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: darkblue; color: white;") 
                selected_port = self.serialPortsCombobox.currentText()
                self.processes[i] = subprocess.Popen([self.binary_paths[i], str(selected_port)]) 
                self.threads[i].start()       
            if i == 3:  
                self.threads[-1].signal.connect(self.update_graph2)
                self.threads[-1].start()            
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: darkblue; color: white;") 
                #apd_fft_arg = self.apd_fft_arg_input.text()
                fft_window_type = self.windowTypeCombobox.currentIndex()
                fft_window_value = self.window_type_values[fft_window_type]
                sampling_freq = self.samplingFreqCombobox.currentIndex()
                sampling_freq_value = self.sampling_freq_values[sampling_freq]
                #self.processes[i] = subprocess.Popen([self.binary_paths[i], apd_fft_arg, str(fft_window_value), str(sampling_freq_value)])
                self.processes[i] = subprocess.Popen([self.binary_paths[i], str(fft_window_value), str(sampling_freq_value)])
                self.threads[i].start()
            else:
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: darkblue; color: white;")             	
                self.processes[i] = subprocess.Popen([self.binary_paths[i]])
                self.threads[i].start()
            print(f"Process {i + 1} started.")
        else:
            if self.processes[i]:
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")            
                self.threads[i].stop()
                subprocess.run(['pkill', '-f', self.processes[i].args[0]], check=True)
                self.processes[i] = None
                print(f"Process {i + 1} stopped.")




    def update_graph1(self, text):
        parts_counts = text.split()
        counts_data_len = len(parts_counts) 
 
        for i in range(0, counts_data_len, 2):
            timestamp = float(parts_counts[i])
            value = float(parts_counts[i + 1])
            self.times1.append(timestamp) 
            self.data1.append(value)
                      
        current_time = time.time()
        cut_off_time = current_time - 15
        self.times1 = [t for t in self.times1 if t >= cut_off_time]
        self.data1 = self.data1[-len(self.times1):]

        self.graph1.clear()
        self.graph1.plot(self.times1, self.data1, pen=pg.mkPen(color=(255, 0, 0)))
        self.graph1.setLabel('left', 'Counts')
        self.graph1.setLabel('bottom', 'Time', units='hh:mm:ss')

    def update_graph2(self, text):
        self.data2.clear()
        self.freq2.clear()
        self.graph2.clear()
        parts_fft = text.split()
        fft_data_len = len(parts_fft)        
        if fft_data_len % 2 != 0:
            self.data2.clear()
            self.freq2.clear()
            self.graph2.clear()
            return

        freq_vals = []
        magn_vals = []

        for i in range(0, fft_data_len, 2):
            freq = float(parts_fft[i])
            magn = float(parts_fft[i + 1])
            freq_vals.append(freq)
            magn_vals.append(magn)
            
        #if freq_vals[0] != 0.001:
        #    self.data2.clear()
        #    self.freq2.clear()
        #    self.graph2.clear()
        #    return            

        sorted_data = sorted(zip(freq_vals, magn_vals))
        freq_vals, magn_vals = map(list, zip(*sorted_data))

        self.freq2.extend(freq_vals)
        self.data2.extend(magn_vals)
        
        self.graph2.plot(self.freq2, self.data2, pen=pg.mkPen(color=(0, 255, 0)))
        self.graph2.setLabel('left', '|Power|')
        self.graph2.setLabel('bottom', 'Frequency', 'Hz')
        self.graph2.plotItem.setLogMode(x=True)
        sampling_freq_index = self.samplingFreqCombobox.currentIndex()
        if sampling_freq_index == 0:  # 100Hz
            self.graph2.plotItem.setXRange(np.log10(1), np.log10(55))
        elif sampling_freq_index == 1:  # 500Hz
            self.graph2.plotItem.setXRange(np.log10(1), np.log10(275))
        elif sampling_freq_index == 2:  # 1kHz
            self.graph2.plotItem.setXRange(np.log10(1), np.log10(550))
        elif sampling_freq_index == 3:  # 2kHz
            self.graph2.plotItem.setXRange(np.log10(10), np.log10(1100))                        
        elif sampling_freq_index == 4:  # 4kHz
            self.graph2.plotItem.setXRange(np.log10(10), np.log10(2200))            
        elif sampling_freq_index == 5:  # 10kHz
            self.graph2.plotItem.setXRange(np.log10(10), np.log10(5500))            
        elif sampling_freq_index == 6:  # 50kHz
            self.graph2.plotItem.setXRange(np.log10(100), np.log10(27500))                        
        else:  # 100kHz
            self.graph2.plotItem.setXRange(np.log10(100), np.log10(55000))
        self.graph2.plotItem.setYRange(-0.1, 1.1)
        self.graph2.update()  

    def closeEvent(self, event):
        for process in self.processes:
            if process is not None:
                subprocess.run(['pkill', '-f', process.args[0]], check=True)
        for thread in self.threads:
            if thread is not None:
                thread.stop()
        for i in range(5):
            self.threads[i].stop()
        event.accept()


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
        mainWindow = MainWindow()
        mainWindow.show()
        sys.exit(app.exec_())
    except Exception as e:
        error_message = "An unexpected error has occurred: {}".format(str(e))
        QtWidgets.QMessageBox.critical(None, "Error", error_message)
        with open("error.log", "a") as log_file:
            log_file.write(error_message)
