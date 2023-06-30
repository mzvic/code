from PyQt5 import QtGui, QtCore, QtWidgets
from PyQt5.QtCore import QThread, pyqtSignal, QDateTime, Qt
from PyQt5.QtGui import QFont, QImage, QPixmap
from PyQt5.QtWidgets import QTabWidget, QVBoxLayout, QWidget, QCheckBox, QLineEdit, QLabel, QComboBox, QApplication, QMainWindow, QPushButton, QTextEdit, QHBoxLayout, QGridLayout, QLineEdit, QFormLayout
import pyqtgraph as pg
from pyqtgraph import QtGui, ImageItem
from serial.tools import list_ports
import datetime
from datetime import datetime
import time
import threading
import numpy as np
import subprocess
import sys
import heapq
import broker

class DateAxis(pg.AxisItem):
    def tickStrings(self, values, scale, spacing):
        return [datetime.fromtimestamp(float(value)).strftime('%H:%M:%S.%f')[:-3] for value in values]

class UpdateGraph1Thread(QThread):
    update_signal1 = pyqtSignal()
    def run(self):
        while True:
            if self.isInterruptionRequested():
                break
            self.update_signal1.emit()
            time.sleep(0.01)
                       
class UpdateGraph2Thread(QThread):
    update_signal2 = pyqtSignal()
    def run(self):
        while True:
            if self.isInterruptionRequested():
                break
            self.update_signal2.emit()
            time.sleep(0.02)    

class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.setWindowTitle("CoDE Control Software")
        icon = QtGui.QIcon("/home/asus/Documentos/CoDE/Core_SSobarzo/core/CoDE_logo.png")  
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
        self.processes = [None] * 8

        self.graph1 = pg.PlotWidget(axisItems={'bottom': DateAxis(orientation='bottom')})
        self.graph1.setMinimumHeight(180)
        self.graph1.setMaximumHeight(300)
        self.layout.addWidget(self.graph1)
        plotItem1 = self.graph1.getPlotItem()
        plotItem1.showGrid(True, True, 0.7)

        self.data1 = []
        self.times1 = []

        self.graph2 = pg.PlotWidget()
        self.graph2.setMinimumHeight(180)
        self.graph2.setMaximumHeight(300)
        self.layout.addWidget(self.graph2)
        plotItem2 = self.graph2.getPlotItem()
        plotItem2.showGrid(True, True, 0.7)

        self.cursor_position = None
        self.y_bar = False
        self.spec = False

        self.data2 = []
        self.freq2 = []

        #self.color_map = pg.ImageView(view=pg.PlotItem(axisItems={'left': DateAxis(orientation='left')}))
        self.color_map = pg.ImageView(view=pg.PlotItem())
        self.layout.addWidget(self.color_map)
        plot_item = self.color_map.getView()
        self.color_map.setMinimumHeight(180)
        self.color_map.setMaximumHeight(300)
        self.color_map.setColorMap(pg.colormap.get('plasma'))
        #self.color_map.getView().setLogMode(x=True)
        self.fft_magnitudes = 50000
        #self.spectrum_amount = 100
        #self.color_map.getView().setRange(xRange=(0, self.fft_magnitudes))
        #self.color_map.getView().setRange(yRange=(0, self.spectrum_amount))
        self.color_map.getView().autoRange() 
        self.color_map.getView().setLabel('bottom', 'Frequency [Hz]')
        self.color_map.getView().setLabel('left', 'Time past [X * avg. spec. time]')     
        plot_item.showGrid(x=True, y=True)
        self.t_fft = int(time.time())     

        self.binary_paths = [
            '/home/asus/Descargas/cores/bin/APD_broker',
            '/home/asus/Descargas/cores/bin/APD_plot_cvt',
            '/home/asus/Descargas/cores/bin/APD_publisher',
            '/home/asus/Descargas/cores/bin/APD_fft3',
            '/home/asus/Descargas/cores/bin/APD_reg_zero', # 'APD_reg' for RAW data with timestamp (TS) from the t0, 'APD_reg_zero' for RAW data with TS from zero...
            '/home/asus/Descargas/cores/bin/APD_reg_proc', # 'APD_reg_proc' for data @ 100Hz with TS from zero...
            '/home/asus/Descargas/cores/bin/APD_reg_fft',
            '/home/asus/Descargas/cores/bin/APD_reg_fft_avg'
        ]

        hidden_layout = QHBoxLayout() 
        button_names_1 = ["Server", "Counts plot"] 

        second_layout = QHBoxLayout() 
        button_names_2 = ["Plot counts", "Plot FFT", "Export counts data [RAW]", "Export counts data [1kHz]", "Export FFT data [RAW]", "Export FFT data [Avg]", "Show averages"]
        
        first_layout = QHBoxLayout() # Parámetros de entrada
        third_layout = QHBoxLayout() 
        serialPortsLabel = QLabel("FPGA serial port:")
        serialPortsLabel.setFixedWidth(100)
        self.serialPortsCombobox = QComboBox(self)
        self.update_serial_ports()
        index_p = self.serialPortsCombobox.findText('/dev/ttyUSB1', QtCore.Qt.MatchFixedString)
        if index_p >= 0:
             self.serialPortsCombobox.setCurrentIndex(index_p)        
        self.serialPortsCombobox.setMaximumWidth(120)
        first_layout.addWidget(serialPortsLabel)
        first_layout.addWidget(self.serialPortsCombobox)
        
        windowTypeLabel = QLabel("FFT Window Type:")
        windowTypeLabel.setFixedWidth(100)
        self.windowTypeCombobox = QComboBox(self)
        self.windowTypeCombobox.addItems(['Hamming', 'Hann', 'Blackman-Harris 4', 'Blackman-Harris 7', 'No window'])
        index_w = self.windowTypeCombobox.findText('No window', QtCore.Qt.MatchFixedString)
        if index_w >= 0:
             self.windowTypeCombobox.setCurrentIndex(index_w)
        self.windowTypeCombobox.setMaximumWidth(120) 
        second_layout.addWidget(windowTypeLabel)
        second_layout.addWidget(self.windowTypeCombobox)
        
        self.window_type_values = {
            0: 1,  # 'Hamming'
            1: 2,  # 'Hann'
            2: 3,  # 'Blackmann-Harris 4'
            3: 4,  # 'Blackmann-Harris 7'
            4: 5,  # 'No window'
        } 

        self.buttons = [] # Creación de botones para ejecutar procesos
        
        for i in range(2):
            start_stop_button = QPushButton(button_names_1[i])
            start_stop_button.setCheckable(True)
            start_stop_button.toggled.connect(lambda checked, i=i: self.toggle_process(i, checked))            
            hidden_layout.addWidget(start_stop_button)
            self.buttons.append(start_stop_button)      

        for i in range(2, 8):
            start_stop_button = QPushButton(button_names_2[i - 2])
            start_stop_button.setCheckable(True)
            start_stop_button.toggled.connect(lambda checked, i=i: self.toggle_process(i, checked))
            if i==2 or i==4 or i ==5 :
                first_layout.addWidget(start_stop_button)
            else:
                second_layout.addWidget(start_stop_button)
            self.buttons.append(start_stop_button) 
            
        apd_counts_secs_label = QLabel("   Time-axis length in counts:")
        
        self.apd_counts_secs_input = QLineEdit(self)
        self.apd_counts_secs_input.setFixedWidth(30) 
        self.apd_counts_secs_input.setText("5")       
        first_layout.addWidget(apd_counts_secs_label)
        first_layout.addWidget(self.apd_counts_secs_input)              

        # boton para mostrar promedios
        #self.toggle_button_save_spec = QPushButton("Export FFT data [Avg]")
        #self.toggle_button_save_spec.clicked.connect(self.toggle_save_spec)
        #second_layout.addWidget(self.toggle_button_save_spec)

        avg_time_label = QLabel("          Averaging period in spectrometer (s):")
        self.avg_time_input = QLineEdit(self)
        self.avg_time_input.setFixedWidth(20) 
        self.avg_time_input.setText("1")       
        third_layout.addWidget(avg_time_label)
        third_layout.addWidget(self.avg_time_input)     

        spectrum_amount_label = QLabel("          Periods to show in spectrometer:")
        self.spectrum_amount_input = QLineEdit(self)
        self.spectrum_amount_input.setFixedWidth(40) 
        self.spectrum_amount_input.setText("60")       
        third_layout.addWidget(spectrum_amount_label)
        third_layout.addWidget(self.spectrum_amount_input)   
        
        # boton para mostrar promedios
        self.toggle_button_spec = QPushButton("Show spectrum averages")
        self.toggle_button_spec.setCheckable(True)  # Habilitar la opción de alternancia
        self.toggle_button_spec.clicked.connect(self.toggle_spec)
        third_layout.addWidget(self.toggle_button_spec)
        
        # boton para mostrar promedios
        self.toggle_button_clean_spec = QPushButton("Clean spectrometer")
        self.toggle_button_clean_spec.clicked.connect(self.toggle_clean_spec)
        third_layout.addWidget(self.toggle_button_clean_spec)

        self.buttons[0].setChecked(True)  # 'Server'
        self.buttons[1].setChecked(True)  # 'Counts plot'

        broker_thread = threading.Thread(target=self.run_broker)
        broker_thread.daemon = True
        broker_thread.start()        
        
        self.update_graph1_thread = UpdateGraph1Thread()
        self.update_graph1_thread.update_signal1.connect(self.update_graph1)
        self.update_graph2_thread = UpdateGraph2Thread()
        self.update_graph2_thread.update_signal2.connect(self.update_graph2)
 
        
        
        
        f_i_label = QLabel("          FFT initial freq:")
        self.f_i_input = QLineEdit(self)
        self.f_i_input.setFixedWidth(60) 
        self.f_i_input.setText("40")       
        third_layout.addWidget(f_i_label)
        third_layout.addWidget(self.f_i_input) 

        f_f_label = QLabel("          FFT final freq:")
        self.f_f_input = QLineEdit(self)
        self.f_f_input.setFixedWidth(60) 
        self.f_f_input.setText("6000")       
        third_layout.addWidget(f_f_label)
        third_layout.addWidget(self.f_f_input)  
        
                      
        
          
        
        
        
        
        self.layout.addLayout(hidden_layout)
        self.layout.addLayout(first_layout) 
        self.layout.addLayout(second_layout)
        self.layout.addLayout(third_layout)
        
        
        
        self.note = QtWidgets.QLabel("Important: To be able to graph the FFT, the 'Plot counts' button must be enabled. Also, if the FFT settings are modified, 'Plot FFT' must be disabled and then enabled for the changes to take effect.")
        self.layout.addWidget(self.note)
        self.buttons[0].hide()  ###### Se esconden los primeros 2 botones...
        self.buttons[1].hide()

        # boton para mostrar peaks (barra amarilla)
        self.toggle_button = QPushButton("Peak ID with cursor")
        self.toggle_button.setCheckable(True)  # Habilitar la opción de alternancia
        self.toggle_button.clicked.connect(self.toggle_cursor)
        second_layout.addWidget(self.toggle_button)                 

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
        button_names = ["Server", "Counts plot", "Plot counts", "Plot FFT", "Export counts data [RAW]", "Export counts data [1kHz]", "Export FFT data [RAW]", "Export FFT data [Avg]", "Show averages"]
        if checked:
            if i == 2: 
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: darkblue; color: white;") 
                selected_port = self.serialPortsCombobox.currentText()
                self.processes[i] = subprocess.Popen([self.binary_paths[i], str(selected_port)])
                self.update_graph1_thread.start()       
            if i == 3:  
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: darkblue; color: white;") 
                fft_window_type = self.windowTypeCombobox.currentIndex()
                fft_window_value = self.window_type_values[fft_window_type]                
                self.processes[i] = subprocess.Popen([self.binary_paths[i], str(fft_window_value)])
                self.update_graph2_thread.start() 
            if i == 7:  
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: darkblue; color: white;") 
                avg_period = self.avg_time_input.text()              
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
                    self.update_graph1_thread.requestInterruption()
                    self.update_graph1_thread.wait()
                if i == 3:
                    self.update_graph2_thread.requestInterruption()
                    self.update_graph2_thread.wait()    
                sender.setText(button_names[i])
                sender.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")            
                subprocess.run(['pkill', '-f', self.processes[i].args[0]], check=True)
                self.processes[i] = None
                print(f"Process {i + 1} stopped.")

    def update_graph1(self):
        len_apd = len(broker.bundle.pa)
        len_times = len(self.times1)
        if((len_times == 0) and (len_apd > 0)):
            for i in range(len_apd):
                timestamp = float(broker.bundle.apdtime[i])
                value = float(broker.bundle.pa[i])
                self.times1.append(timestamp) 
                self.data1.append(value)  
        else:                
            if ((len_apd > 0) and (broker.bundle.apdtime[len_apd-1] != self.times1[-1])): 
                for i in range(len_apd):
                    timestamp = float(broker.bundle.apdtime[i])
                    value = float(broker.bundle.pa[i])
                    
                    self.times1.append(timestamp) 
                    self.data1.append(value)  
             
            current_time = time.time()
            cut_off_time = current_time - float(self.apd_counts_secs_input.text())
            self.times1 = [t for t in self.times1 if t >= cut_off_time]
            self.data1 = self.data1[-len(self.times1):]

            self.graph1.clear()
            self.graph1.plot(self.times1, self.data1, pen=pg.mkPen(color=(255, 0, 0)))
            self.graph1.setLabel('left', 'Counts')
            self.graph1.setLabel('bottom', 'Time', units='hh:mm:ss.uuuuuu')

    def update_graph2(self):
        self.f_i = int(self.f_i_input.text()) 
        self.f_f = int(self.f_f_input.text()) 
        freq_vals = []
        magn_vals = []
        result = broker.bundle
        #print(result)
        if len(result.freq) > 0:
            self.data2.clear()
            self.freq2.clear()
            self.graph2.clear()
            freq_vals = result.freq
            magn_vals = result.fft

            self.freq2.extend(freq_vals)
            self.data2.extend(magn_vals)
            
            h_line = pg.InfiniteLine(pos=0, angle=0, pen=pg.mkPen(color=(0, 255, 0), width=2))
            self.graph2.addItem(h_line)        
            
            fundamental_freq = self.calculate_fundamental_frequency(freq_vals, magn_vals)
            if (fundamental_freq > 0):
                v_line = pg.InfiniteLine(
                    pos=np.log10(fundamental_freq), angle=90, pen=pg.mkPen(color=(255, 0, 255), width=2)
                )
                self.graph2.addItem(v_line)
            text_item = pg.TextItem(text=f"Fundamental Frequency: {fundamental_freq} Hz", color=(255, 0, 255))
            font = QFont()
            font.setBold(True)
            text_item.setFont(font)
                                    

            text_item.setPos(np.log10(self.f_i), 1.15)

            self.graph2.addItem(text_item)

            if self.y_bar:
                y_bar_freq, y_bar_magn = self.print_nearest_frequency()
                v_line_y = pg.InfiniteLine(pos=np.log10(y_bar_freq), angle=90, pen=pg.mkPen(color=(255, 255, 0), width=2))
                self.graph2.addItem(v_line_y)
                text_item2 = pg.TextItem(text=f"Freq: {y_bar_freq} Hz, Magnitude: {y_bar_magn}", color=(255, 255, 0))
                text_item2.setFont(font)
                text_item2.setPos(np.log10(self.f_i), 0)
                self.graph2.addItem(text_item2)

            if (fundamental_freq > 0):
                bar_graph = pg.BarGraphItem(x=np.log10(freq_vals), height=magn_vals, width=(np.log10(self.f_f)-np.log10(self.f_i))*0.001, brush='g')
                self.graph2.addItem(bar_graph)

            self.graph2.setLabel("left", "|Power|")
            self.graph2.setLabel("bottom", "Frequency", "Hz")
            self.graph2.plotItem.setLogMode(x=True)

            self.graph2.plotItem.setXRange(np.log10(self.f_i), np.log10(self.f_f))
            self.graph2.plotItem.setYRange(-0.1, 1.1)
            self.graph2.update()
            if self.spec:
                self.update_graph3()

    def update_graph3(self):
        self.spectrum_amount = int(self.spectrum_amount_input.text())
        self.avg_time = int(self.avg_time_input.text())
        if not hasattr(self, 'data_matrix_avg'):
            self.data_matrix_avg = np.zeros((1, self.fft_magnitudes))

        new_row = np.zeros((1, self.fft_magnitudes))
        self.data_matrix_avg = np.vstack((new_row, self.data_matrix_avg))

        self.data_matrix_avg[0, np.array(self.freq2, dtype=int) - 1] = self.data2


        time_i = int(time.time())
        if (time_i - self.t_fft >= self.avg_time):
            avg_vector = np.mean(self.data_matrix_avg, axis=0)
            m_shape = self.data_matrix_avg.shape
            del self.data_matrix_avg

            if not hasattr(self, 'spectrum_matrix'):
                self.spectrum_matrix = np.zeros((1, self.fft_magnitudes))

            self.spectrum_matrix = np.vstack((avg_vector, self.spectrum_matrix))

            while self.spectrum_matrix.shape[0] >= self.spectrum_amount:
                self.spectrum_matrix = np.delete(self.spectrum_matrix, -1, axis=0)

            self.plot_matrix = np.transpose(self.spectrum_matrix)
            self.pm = self.plot_matrix[:self.f_f, :]
            self.color_map.setImage(self.pm)
            self.color_map.getView().setRange(xRange=(self.f_i, self.f_f))
            self.t_fft = int(time.time())
        
    def calculate_fundamental_frequency(self, freq_vals, magn_vals):
        valid_indices = [i for i in range(len(magn_vals)) if freq_vals[i] > 6 and magn_vals[i] > 0.9]
        max_magn_indices = heapq.nlargest(100, valid_indices, key=lambda i: magn_vals[i])
        max_freqs = [freq_vals[i] for i in max_magn_indices]
        
        if max_freqs:
            fundamental_freq = min(max_freqs)
            return fundamental_freq
        else:
            return 0 

    def print_nearest_frequency(self):
        cursor_pos = self.graph2.plotItem.vb.mapSceneToView(self.graph2.mapFromGlobal(QtGui.QCursor.pos()))
        x_pos = cursor_pos.x()

        x_range, _ = self.graph2.plotItem.viewRange()
        view_rect = self.graph2.plotItem.viewRect()

        relative_x = (x_pos - view_rect.left()) / view_rect.width()
        cursor_graph2 = 10 ** (x_range[0] + relative_x * (x_range[1] - x_range[0]))

        x_data = np.array(self.freq2)

        closest_index = np.argmin(np.abs(x_data - cursor_graph2))
        closest_frequency = x_data[closest_index]
        closest_magnitude = self.data2[closest_index]
        return closest_frequency, closest_magnitude

    def clear_nearest_frequency(self):
        self.cursor_position = None  

    def run_broker(self):
        while True:
            broker.main()  

    def closeEvent(self, event):
        for process in self.processes:
            if process is not None:
                subprocess.run(['pkill', '-f', process.args[0]], check=True)
        event.accept()

    def toggle_cursor(self):
        self.y_bar = not self.y_bar
        if self.y_bar:
            self.cursor_position = self.graph2.getViewBox().mapFromScene(QtGui.QCursor.pos())
            self.print_nearest_frequency()
        else:
            self.clear_nearest_frequency()
        if self.toggle_button.isChecked():
            self.toggle_button.setStyleSheet("background-color: yellow; color: black;") 
        else:
            self.toggle_button.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")     

    def toggle_spec(self):
        if self.toggle_button_spec.isChecked():
            self.toggle_button_spec.setStyleSheet("background-color: darkblue; color: white;") 
        else:
            self.toggle_button_spec.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")  
               
        self.spec = not self.spec
        
    def toggle_save_spec(self):
        if self.toggle_button_save_spec.isChecked():
            self.toggle_button_save_spec.setStyleSheet("background-color: darkblue; color: white;") 
        else:
            self.toggle_button_save_spec.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")  
        self.spec = not self.spec    
        
    def toggle_clean_spec(self):
        if self.toggle_button_clean_spec.isChecked():
            self.toggle_button_clean_spec.setStyleSheet("background-color: darkblue; color: white;") 
        else:
            self.toggle_button_clean_spec.setStyleSheet("background-color: 53, 53, 53; color: 53, 53, 53;")  
        del self.data_matrix_avg
        del self.spectrum_matrix
        del self.plot_matrix
        del self.pm
        self.pm = np.zeros((1, self.fft_magnitudes))
        self.color_map.setImage(self.pm)   


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
