import pyvisa

rm = pyvisa.ResourceManager('@py')
inst = rm.open_resource('USB0::6833::1301::MS5A242205632::0::INSTR')
command = input("Enter command: ")

inst.write(":{}".format(command))
if command[-1] == '?':

    print(inst.read('\n'))