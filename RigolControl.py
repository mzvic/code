
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

# Get traces from rigol
rigol_prev_stat = 0
def get_rigol_data(status, channel, auto, voltage, frequency, function, tscale, voltage_offset, attenuation, coupling, vscale, g1, g2, laser_voltage, voltage_V, frequency_V, function_V, tscale_V, voltage_offset_V, attenuation_V, coupling_V, vscale_V, g1_V, g2_V, laser_voltage_V):
    global rigol_prev_stat
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
                    #scope.write(f":SOUR1:VOLT:OFFS {voltage_offset_V}")
                    scope.write(":CHAN1:OFFS 0")
                    scope.write(f":OFFS {voltage_offset_V}")                     
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

