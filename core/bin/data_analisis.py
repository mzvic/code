# -*- coding: utf-8 -*-
# """
# Created on Tue Oct 11 12:54:50 2022

# @author: Nicolas
# """
import numpy as np

import matplotlib.pyplot as plt
import math as math
from scipy.fft import fft, fftfreq
from scipy.signal import find_peaks



#Define variables to hold data
 
Time=[]
Power=[]


#Open File to analyze and get data

Data=open('APD_logs_02-05-2023_16:39:34.txt','r')

for line in Data:
    info=line.split(';')
    Time.append(info[0])
    Power.append(info[1])

Data.close() #always remeber to close files when not using



#turn data into float 


count=0

for num in range(0,len(Time)):
    Time[num]=float(Time[num])
    Power[num]=float(Power[num])
    # count=count+1
    # if Power[num]<2.0:
    #     Power[num]=0



#If time is being mishandeled from rigol data use this array as time instead
# Time=2+np.arange(Time[0],Time[np.size(Time)-1],(Time[np.size(Time)-1]-Time[0])/np.size(Time))


#turn data into numpy array
Time=np.array(Time) 
Power=np.array(Power)





#Fft of the sample
Frequencies=fftfreq(len(Time),(Time[1]-Time[0]))
fft_power=fft(Power-np.average(Power))
N=len(fft_power)
fft_power=fft_power/np.linalg.norm(fft_power) #normalized

peaks,_=find_peaks((fft_power),height=0.4,distance=4)

fig, ax=plt.subplots(2)

ax[0].plot(Time,Power,color='blue')
# ax[0].tittle("Power vs Time")
ax[0].set(xlabel="Time[s]", ylabel="Power [V]")
ax[0].grid(True)

ax[1].plot(Frequencies[1:10000],(abs(fft_power[1:10000])))
ax[1].grid(True)
# ax[1].plot(Frequencies[peaks[0:2]],10*np.log10(abs(fft_power[peaks[0:2]])),"x",label='Freq='+str(Frequencies[peaks[0:2]]))
ax[1].set(xlabel='Frequency [Hz]',ylabel='norm db')
for a in range(0,np.size(peaks)):
    ax[1].plot(Frequencies[peaks[a]],(abs(fft_power[peaks[a]])),"x",label='Freq='+str(Frequencies[peaks[a]]))

ax[1].legend()

fig.suptitle("APD data from rigol trap settings 1000Vpp F:220Hz")


#Lets see if we can make sense of the data


# we have then

Omega_trap=220 #Hz
r_o=5 #mm
z_o=3.5 #mm
omega=Frequencies[peaks[0:2]]
V_o=400 #Volts
n=[0,1,2]


QM=(((r_o/1000)**2+2*(z_o/1000)**2)*(Omega_trap**2)/(8*V_o))*(omega[1]/Omega_trap-n[0])*2*math.sqrt(2)

QM_2=(((r_o/1000)**2+2*(z_o/1000)**2)*(Omega_trap**2)/(4*V_o))*(omega[0]/Omega_trap-n[0])*2*math.sqrt(2)



q_silica=QM_2*(V_o*8)/(((r_o/1000)**2+2*(z_o/1000)**2)*(Omega_trap**2))


MQ=V_o/(math.sqrt(2)*omega[1]*Omega_trap*(z_o/1000)**2)


q_trap_corn_starch=QM*(V_o*8)/(((r_o/1000)**2+2*(z_o/1000)**2)*(Omega_trap**2))

# q_trap_corn_starch_MQ=(1/MQ)*(V_o*8)/(((r_o/1000)**2+2*(z_o/1000)**2)*(Omega_trap**2))





