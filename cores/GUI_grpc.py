import sys
import time
import datetime
import socket
from PyQt5.QtWidgets import QApplication, QMainWindow, QPushButton, QVBoxLayout, QWidget, QTextEdit, QHBoxLayout, QLabel, QGridLayout, QLineEdit,QFormLayout
from PyQt5.QtCore import QThread, pyqtSignal, QDateTime, Qt
import pyqtgraph as pg
from pyqtgraph import QtGui
import numpy as np
import subprocess
import heapq
from PyQt5.QtGui import QFont
from PyQt5 import QtGui, QtCore
from PyQt5.QtWidgets import QTabWidget, QVBoxLayout, QWidget
from PyQt5.QtWidgets import QCheckBox
from PyQt5 import QtGui, QtCore, QtWidgets
from serial.tools import list_ports
from PyQt5.QtWidgets import QLineEdit, QLabel, QComboBox
from PyQt5.QtGui import QImage, QPixmap
from pyqtgraph import ImageItem
import broker
import threading

class DateAxis(pg.AxisItem):
    def tickStrings(self, values, scale, spacing):
        return [QDateTime.fromSecsSinceEpoch(int(value)).toString('hh:mm:ss') for value in values]

class UpdateGraph1Thread(QThread):
    update_signal1 = pyqtSignal()
    def run(self):
        while True:
            if self.isInterruptionRequested():
                break
            self.update_signal1.emit()
            time.sleep(0.005) 
            
class UpdateGraph2Thread(QThread):
    update_signal2 = pyqtSignal()
    def run(self):
        while True:
            if self.isInterruptionRequested():
                break
            self.update_signal2.emit()
            time.sleep(0.01)                         

class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()
        self.setWindowTitle("CoDE Control Software")
        icon = QtGui.QIcon("/home/asus/Documentos/CoDE/Core_SSobarzo/core/CoDE_logo.png")  
        self.setWindowIcon(icon)        
        self.tab_widget = QTabWidget()
        self.setCentralWidget(self.tab_widget)
        self.tab1 = QWidget()
        self.tab_widget.addTab(self.tab1, "APD")
        self.layout = QVBoxLayout(self.tab1)
        self.threads = []
        self.processes = [None] * 6
        self.graph1 = pg.PlotWidget(axisItems={'bottom': DateAxis(orientation='bottom')})
        self.layout.addWidget(self.graph1)
        plotItem1 = self.graph1.getPlotItem()
        plotItem1.showGrid(True, True, 1)
        self.data1 = []
        self.times1 = []
        self.graph2 = pg.PlotWidget()
        self.layout.addWidget(self.graph2)
        plotItem2 = self.graph2.getPlotItem()
        plotItem2.showGrid(True, True, 1)
        self.data2 = []
        self.freq2 = []
        self.color_map = pg.ImageView(view=pg.PlotItem())
        self.layout.addWidget(self.color_map)
        plot_item = self.color_map.getView()
        self.color_map.setMinimumHeight(200)
        self.color_map.setColorMap(pg.colormap.get('viridis'))
        self.fft_magnitudes = 2048
        self.spectrum_amount = 200
        self.color_map.getView().setRange(xRange=(0, self.fft_magnitudes))
        self.color_map.getView().setLabel('bottom', 'Frequency [Hz]')
        self.color_map.getView().setLabel('left', 'Time')     
        plot_item.showGrid(x=True, y=True)
        self.avg_time = int(1)
        self.t_fft = int(time.time())
               
        self.binary_paths = [
            '/home/asus/Descargas/cores/bin/APD_broker',
            '/home/asus/Descargas/cores/bin/APD_plot_cvt',
            '/home/asus/Descargas/cores/bin/APD_publisher',
            '/home/asus/Descargas/cores/bin/APD_fft3',
            '/home/asus/Descargas/cores/bin/APD_reg_proc'
        ]

        button_layout_1 = QHBoxLayout() 
        button_names_1 = ["Server", "Counts plot"] 

        button_layout_2 = QHBoxLayout() 
        button_names_2 = ["Plot counts", "Plot FFT"]

        arg_input_layout = QHBoxLayout() # Parámetros de entrada
        serialPortsLabel = QLabel("                    FPGA serial port:")
        self.serialPortsCombobox = QComboBox(self)
        self.update_serial_ports()
        index_p = self.serialPortsCombobox.findText('/dev/ttyUSB1', QtCore.Qt.MatchFixedString)
        if index_p >= 0:
             self.serialPortsCombobox.setCurrentIndex(index_p)        
        arg_input_layout.addWidget(serialPortsLabel)
        arg_input_layout.addWidget(self.serialPortsCombobox)

        self.buttons = [] # Creación de botones para ejecutar procesos
        
        for i in range(2):
            start_stop_button = QPushButton(button_names_1[i])
            start_stop_button.setCheckable(True)
            start_stop_button.toggled.connect(lambda checked, i=i: self.toggle_process(i, checked))            
            button_layout_1.addWidget(start_stop_button)
            self.buttons.append(start_stop_button) 

        for i in range(2, 4):
            start_stop_button = QPushButton(button_names_2[i - 2])
            start_stop_button.setCheckable(True)
            start_stop_button.toggled.connect(lambda checked, i=i: self.toggle_process(i, checked))
            button_layout_2.addWidget(start_stop_button)
            self.buttons.append(start_stop_button)  
            
        self.buttons[0].setChecked(True)  # 'Server'
        self.buttons[1].setChecked(True)  # 'Counts plot'
        
        broker_thread = threading.Thread(target=self.run_broker)
        broker_thread.daemon = True
        broker_thread.start()        
        self.update_graph1_thread = UpdateGraph1Thread()
        self.update_graph1_thread.update_signal1.connect(self.update_graph1)
        self.update_graph2_thread = UpdateGraph2Thread()
        self.update_graph2_thread.update_signal2.connect(self.update_graph2)
        self.layout.addLayout(button_layout_1)
        self.layout.addLayout(button_layout_2)
        self.buttons[0].hide()  ###### Se esconden los primeros 2 botones...
        self.buttons[1].hide()  
    def update_serial_ports(self):    
        self.serialPortsCombobox.clear()
        ports = list_ports.comports()
        for port in ports:
            self.serialPortsCombobox.addItem(port.device) 

    def toggle_process(self, i, checked):
        sender = self.sender()
        button_names = ["Server", "Counts plot", "Plot counts", "Plot FFT"] 
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
                self.processes[i] = subprocess.Popen([self.binary_paths[i]])
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
        if (len_apd > 0): 
            for i in range(len_apd):
                timestamp = float(broker.bundle.ts[i])
                value = float(broker.bundle.pa[i])
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
        
    def update_graph2(self):
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
        else:
            return
        
        self.freq2.extend(freq_vals)
        self.data2.extend(magn_vals)
        
        h_line = pg.InfiniteLine(pos=0, angle=0, pen=pg.mkPen(color=(0, 255, 0), width=2))
        self.graph2.addItem(h_line)        
        
        bar_graph = pg.BarGraphItem(x=np.log10(freq_vals), height=magn_vals, width=0.005, brush='g')
        self.graph2.addItem(bar_graph)
        self.graph2.setLabel("left", "|Power|")
        self.graph2.setLabel("bottom", "Frequency", "Hz")
        self.graph2.plotItem.setLogMode(x=True)

        self.graph2.plotItem.setXRange(np.log10(self.f_i), np.log10(self.f_f))
        
        self.graph2.plotItem.setYRange(-0.1, 1.1)
        self.graph2.update()
        update_graph3()
        
    def update_graph3(self):      
        if not hasattr(self, 'data_matrix_avg'):
            self.data_matrix_avg = np.zeros((1, self.fft_magnitudes))
        new_row = np.zeros((1, self.fft_magnitudes)) 
        self.data_matrix_avg = np.vstack((new_row, self.data_matrix_avg))   
        self.data_matrix_avg[0, np.array(self.freq2, dtype=int)] = self.data2
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
            plot_matrix = np.transpose(self.spectrum_matrix)           
            self.color_map.setImage(plot_matrix)
            self.t_fft = int(time.time())
        
    def calculate_fundamental_frequency(self, freq_vals, magn_vals):
        valid_indices = [i for i in range(len(magn_vals)) if freq_vals[i] > 6 and magn_vals[i] > 0.9]
        max_magn_indices = heapq.nlargest(100, valid_indices, key=lambda i: magn_vals[i])
        max_freqs = [freq_vals[i] for i in max_magn_indices]
        fundamental_freq = min(max_freqs)
        return fundamental_freq

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_A:
            if not self.y_bar:
                self.y_bar = True
                self.cursor_position = self.graph2.getViewBox().mapFromScene(QtGui.QCursor.pos())
                self.print_nearest_frequency()
            else:
                self.y_bar = False
                self.clear_nearest_frequency()

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

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
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

